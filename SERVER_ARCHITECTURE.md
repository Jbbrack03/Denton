# Sudachi Multiplayer Server Architecture

## Overview

This document provides comprehensive technical specifications for the server infrastructure supporting Sudachi's multiplayer system, including the Room Server, Relay Servers, and supporting services.

## Room Server Specification

### Technology Stack

```
┌─────────────────────────────────────┐
│         Load Balancer               │
│         (NGINX/HAProxy)             │
└─────────────┬───────────────────────┘
              │
    ┌─────────┴─────────┬─────────────┐
    │                   │             │
┌───▼────┐      ┌───────▼──┐   ┌─────▼───┐
│Room    │      │Room      │   │Room     │
│Server 1│      │Server 2  │   │Server N │
└───┬────┘      └────┬─────┘   └────┬────┘
    │                │               │
    └────────────────┴───────────────┘
                     │
           ┌─────────▼─────────┐
           │   Redis Cluster   │
           │  (Session State)  │
           └─────────┬─────────┘
                     │
           ┌─────────▼─────────┐
           │   PostgreSQL      │
           │   (Persistent)    │
           └───────────────────┘
```

### Room Server Implementation

**Package Structure**:
```
room-server/
├── src/
│   ├── index.ts           # Entry point
│   ├── config/
│   │   ├── database.ts
│   │   ├── redis.ts
│   │   └── server.ts
│   ├── middleware/
│   │   ├── auth.ts        # JWT validation
│   │   ├── rateLimit.ts   # Rate limiting
│   │   └── logging.ts
│   ├── services/
│   │   ├── roomService.ts
│   │   ├── p2pService.ts
│   │   └── relayService.ts
│   ├── models/
│   │   ├── Room.ts
│   │   ├── Client.ts
│   │   └── Session.ts
│   └── websocket/
│       ├── handlers/
│       └── messageTypes.ts
├── tests/
├── Dockerfile
└── docker-compose.yml
```

**Core Service Implementation**:
```typescript
// roomService.ts
export class RoomService {
    private rooms: Map<string, Room> = new Map();
    private redis: RedisClient;
    
    async createRoom(config: RoomConfig): Promise<Room> {
        const room = new Room({
            id: generateUUID(),
            gameId: config.gameId,
            maxPlayers: config.maxPlayers,
            hostPeerId: config.hostPeerId,
            createdAt: Date.now(),
            region: await this.detectRegion(config.hostIp)
        });
        
        // Store in Redis for cross-instance access
        await this.redis.setex(
            `room:${room.id}`,
            ROOM_TTL_SECONDS,
            JSON.stringify(room)
        );
        
        // Publish room creation event
        await this.redis.publish('room:created', room.id);
        
        return room;
    }
    
    async findRooms(filter: RoomFilter): Promise<Room[]> {
        const pattern = filter.gameId 
            ? `room:*:game:${filter.gameId}` 
            : 'room:*';
            
        const keys = await this.redis.keys(pattern);
        const rooms = await Promise.all(
            keys.map(key => this.redis.get(key))
        );
        
        return rooms
            .filter(r => r !== null)
            .map(r => JSON.parse(r))
            .filter(r => this.matchesFilter(r, filter))
            .slice(filter.offset || 0, filter.limit || 50);
    }
}
```

### WebSocket Protocol

**Connection Lifecycle**:
```typescript
io.on('connection', async (socket) => {
    const token = socket.handshake.auth.token;
    
    try {
        const client = await authenticateClient(token);
        socket.data.client = client;
        
        // Join client-specific room for targeted messages
        socket.join(`client:${client.id}`);
        
        // Send initial state
        socket.emit('connected', {
            clientId: client.id,
            serverTime: Date.now(),
            serverVersion: SERVER_VERSION
        });
        
        // Register handlers
        registerHandlers(socket);
        
    } catch (error) {
        socket.emit('error', {
            code: 'AUTH_FAILED',
            message: 'Invalid authentication token'
        });
        socket.disconnect();
    }
});
```

**Message Handlers**:
```typescript
const handlers = {
    'room:create': async (socket, data) => {
        try {
            const room = await roomService.createRoom({
                ...data,
                hostId: socket.data.client.id
            });
            
            socket.emit('room:created', {
                success: true,
                room: room.toJSON()
            });
            
            // Notify other clients about new room
            socket.broadcast.emit('room:list:update', {
                type: 'add',
                room: room.toPublicJSON()
            });
            
        } catch (error) {
            handleError(socket, error);
        }
    },
    
    'p2p:failure': async (socket, data) => {
        const { roomId, targetClientId } = data;
        
        // Allocate relay server
        const relay = await relayService.allocateRelay(
            socket.data.client.id,
            targetClientId
        );
        
        // Notify both clients
        io.to(`client:${socket.data.client.id}`).emit('use:relay', {
            server: relay.server,
            port: relay.port,
            token: relay.token
        });
        
        io.to(`client:${targetClientId}`).emit('use:relay', {
            server: relay.server,
            port: relay.port,
            token: relay.token
        });
    }
};
```

### Database Schema

**PostgreSQL Tables**:
```sql
-- Clients table
CREATE TABLE clients (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    platform VARCHAR(16) NOT NULL,
    sudachi_version VARCHAR(16) NOT NULL,
    stats JSONB DEFAULT '{}',
    INDEX idx_username (username),
    INDEX idx_last_seen (last_seen)
);

-- Room history for analytics
CREATE TABLE room_history (
    id UUID PRIMARY KEY,
    game_id BIGINT NOT NULL,
    host_client_id UUID REFERENCES clients(id),
    max_players SMALLINT NOT NULL,
    actual_players SMALLINT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    closed_at TIMESTAMP,
    duration_seconds INTEGER,
    region VARCHAR(16),
    connection_types JSONB,
    INDEX idx_game_id (game_id),
    INDEX idx_created_at (created_at)
);

-- Connection metrics
CREATE TABLE connection_metrics (
    id BIGSERIAL PRIMARY KEY,
    client_id UUID REFERENCES clients(id),
    room_id UUID,
    connection_type VARCHAR(16),
    latency_ms INTEGER,
    packet_loss_rate FLOAT,
    bandwidth_kbps INTEGER,
    recorded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_client_time (client_id, recorded_at)
);
```

### Security Implementation

**JWT Token Generation**:
```typescript
export function generateToken(clientId: string): string {
    return jwt.sign(
        {
            sub: clientId,
            iat: Math.floor(Date.now() / 1000),
            exp: Math.floor(Date.now() / 1000) + TOKEN_LIFETIME_SECONDS,
            iss: 'sudachi-rooms',
            aud: 'sudachi-multiplayer'
        },
        JWT_SECRET,
        { algorithm: 'HS256' }
    );
}
```

**Rate Limiting**:
```typescript
const rateLimiter = new RateLimiterRedis({
    storeClient: redisClient,
    keyPrefix: 'ratelimit',
    points: 100, // requests
    duration: 60, // per minute
    blockDuration: 60 * 5 // block for 5 minutes
});

export async function rateLimit(
    socket: Socket, 
    next: Function
): Promise<void> {
    try {
        await rateLimiter.consume(socket.data.client.id);
        next();
    } catch (rejRes) {
        socket.emit('error', {
            code: 'RATE_LIMITED',
            message: 'Too many requests',
            retryAfter: Math.round(rejRes.msBeforeNext / 1000)
        });
    }
}
```

## Relay Server Specification

### Architecture

**High-Performance C++ Implementation**:
```cpp
// relay_server.h
class RelayServer {
    using SessionMap = std::unordered_map<uint32_t, SessionPair>;
    
public:
    RelayServer(uint16_t port, size_t thread_count = 0);
    void Start();
    void Stop();
    
private:
    void HandleTcpAccept(tcp::socket socket);
    void HandleUdpReceive(const boost::system::error_code& error, 
                         size_t bytes_transferred);
    
    boost::asio::io_context io_context_;
    tcp::acceptor tcp_acceptor_;
    udp::socket udp_socket_;
    
    SessionMap sessions_;
    std::mutex sessions_mutex_;
    
    // Statistics
    std::atomic<uint64_t> total_bytes_relayed_{0};
    std::atomic<uint32_t> active_sessions_{0};
};

struct SessionPair {
    std::shared_ptr<RelaySession> client_a;
    std::shared_ptr<RelaySession> client_b;
    std::chrono::steady_clock::time_point created_at;
    uint32_t token;
    std::atomic<uint64_t> bytes_relayed{0};
};
```

**Session Management**:
```cpp
void RelayServer::HandleNewSession(
    tcp::socket socket, 
    const RelayHandshake& handshake
) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(handshake.session_token);
    if (it == sessions_.end()) {
        // First client of the pair
        auto session = std::make_shared<RelaySession>(
            std::move(socket), 
            handshake.session_token
        );
        
        sessions_[handshake.session_token] = {
            .client_a = session,
            .client_b = nullptr,
            .created_at = std::chrono::steady_clock::now(),
            .token = handshake.session_token
        };
        
        session->Start();
        
    } else if (it->second.client_b == nullptr) {
        // Second client of the pair
        auto session = std::make_shared<RelaySession>(
            std::move(socket), 
            handshake.session_token
        );
        
        it->second.client_b = session;
        
        // Link the sessions
        it->second.client_a->SetPeer(session);
        session->SetPeer(it->second.client_a);
        
        // Start relaying
        session->Start();
        it->second.client_a->StartRelaying();
        
        active_sessions_++;
        
    } else {
        // Session already full
        boost::asio::write(socket, boost::asio::buffer(
            "SESSION_FULL\n"
        ));
        socket.close();
    }
}
```

### Performance Optimizations

**Zero-Copy Relay**:
```cpp
void RelaySession::HandleRead(
    const boost::system::error_code& error,
    size_t bytes_transferred
) {
    if (!error && peer_) {
        // Direct buffer transfer without copying
        boost::asio::async_write(
            peer_->socket_,
            boost::asio::buffer(read_buffer_, bytes_transferred),
            [this, self = shared_from_this()](
                const boost::system::error_code& write_error,
                size_t bytes_written
            ) {
                if (!write_error) {
                    bytes_relayed_ += bytes_written;
                    StartRead(); // Continue reading
                } else {
                    HandleError(write_error);
                }
            }
        );
    } else {
        HandleError(error);
    }
}
```

**io_uring Support (Linux)**:
```cpp
#ifdef __linux__
class IOUringRelayServer : public RelayServer {
    struct io_uring ring_;
    
    void InitializeIOUring() {
        struct io_uring_params params = {};
        params.flags = IORING_SETUP_SQPOLL;
        params.sq_thread_idle = 1000; // 1 second
        
        int ret = io_uring_queue_init_params(4096, &ring_, &params);
        if (ret < 0) {
            throw std::runtime_error("io_uring init failed");
        }
    }
    
    void SubmitRead(int fd, void* buf, size_t len, uint64_t user_data) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        io_uring_prep_read(sqe, fd, buf, len, 0);
        io_uring_sqe_set_data(sqe, (void*)user_data);
        io_uring_submit(&ring_);
    }
};
#endif
```

## Deployment Architecture

### Kubernetes Configuration

**Room Server Deployment**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: room-server
  namespace: sudachi-multiplayer
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: room-server
  template:
    metadata:
      labels:
        app: room-server
    spec:
      containers:
      - name: room-server
        image: sudachi/room-server:latest
        ports:
        - containerPort: 8080
          name: websocket
        env:
        - name: REDIS_URL
          valueFrom:
            secretKeyRef:
              name: redis-credentials
              key: url
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: postgres-credentials
              key: url
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

**Horizontal Pod Autoscaler**:
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: room-server-hpa
  namespace: sudachi-multiplayer
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: room-server
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: websocket_connections
      target:
        type: AverageValue
        averageValue: "1000"
```

### Geographic Distribution

**Multi-Region Deployment**:
```
┌────────────────────────────────────────┐
│            Global DNS                  │
│         (Route 53/Cloudflare)          │
└─────────────┬──────────────────────────┘
              │
    ┌─────────┴─────────┬─────────────┐
    │                   │             │
┌───▼────────┐ ┌────────▼──────┐ ┌───▼────────┐
│US-West     │ │EU-Central     │ │Asia-Pacific│
│Oregon      │ │Frankfurt      │ │Tokyo       │
├────────────┤ ├───────────────┤ ├────────────┤
│Room Servers│ │Room Servers   │ │Room Servers│
│Relay Pool  │ │Relay Pool     │ │Relay Pool  │
│Redis       │ │Redis          │ │Redis       │
└────────────┘ └───────────────┘ └────────────┘
       │                │               │
       └────────────────┴───────────────┘
                        │
              ┌─────────▼─────────┐
              │  Global Postgres  │
              │   (Primary)       │
              └───────────────────┘
```

### Monitoring and Observability

**Metrics Collection**:
```yaml
# Prometheus configuration
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'room-servers'
    kubernetes_sd_configs:
    - role: pod
      namespaces:
        names:
        - sudachi-multiplayer
    relabel_configs:
    - source_labels: [__meta_kubernetes_pod_label_app]
      regex: room-server
      action: keep
    - source_labels: [__meta_kubernetes_pod_name]
      target_label: instance
```

**Key Metrics**:
```typescript
// Custom metrics exported
export const metrics = {
    websocketConnections: new promClient.Gauge({
        name: 'websocket_connections_total',
        help: 'Total number of active WebSocket connections'
    }),
    
    roomsActive: new promClient.Gauge({
        name: 'rooms_active_total',
        help: 'Total number of active game rooms',
        labelNames: ['game_id']
    }),
    
    p2pSuccessRate: new promClient.Histogram({
        name: 'p2p_connection_success_rate',
        help: 'Success rate of P2P connections',
        buckets: [0.5, 0.7, 0.8, 0.9, 0.95, 0.99, 1]
    }),
    
    relayBandwidth: new promClient.Counter({
        name: 'relay_bandwidth_bytes_total',
        help: 'Total bytes relayed',
        labelNames: ['server_region']
    })
};
```

## Security Hardening

### DDoS Protection
- CloudFlare or AWS Shield in front of services
- Rate limiting at multiple levels
- Connection limits per IP
- Automatic blacklisting of abusive IPs

### TLS Configuration
```nginx
# NGINX TLS configuration
ssl_protocols TLSv1.2 TLSv1.3;
ssl_ciphers ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384;
ssl_prefer_server_ciphers on;
ssl_session_cache shared:SSL:10m;
ssl_session_timeout 10m;
ssl_stapling on;
ssl_stapling_verify on;
```

### Secrets Management
- HashiCorp Vault or AWS Secrets Manager
- Automatic key rotation
- Encrypted environment variables
- No hardcoded credentials

## Capacity Planning

### Resource Requirements
- **Room Server**: 1 CPU core per 5000 WebSocket connections
- **Memory**: 1GB per 10000 connections
- **Redis**: 100MB for 10000 active rooms
- **PostgreSQL**: 10GB initial, grows ~1GB/month
- **Relay Server**: 10Gbps network per instance
- **Bandwidth**: 1Mbps per active game session

### Auto-Scaling Triggers
- CPU > 70%: Scale up
- Memory > 80%: Scale up
- Connections > 1000/instance: Scale up
- Response time > 100ms: Scale up
- Scale down after 10 minutes below thresholds

## Disaster Recovery

### Backup Strategy
- PostgreSQL: Daily snapshots, 30-day retention
- Redis: AOF persistence, replicated
- Configuration: Version controlled
- Secrets: Backed up to separate region

### Failover Procedures
1. Automatic health checks every 5 seconds
2. Remove unhealthy instances from load balancer
3. Spin up replacement instances
4. Restore state from Redis/PostgreSQL
5. Resume operations < 30 seconds

### Regional Failover
- DNS-based failover with health checks
- < 60 second TTL for quick switches
- Clients automatically reconnect to healthy region
- Room state synchronized cross-region