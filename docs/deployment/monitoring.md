# Sudachi Multiplayer Monitoring and Observability Guide

## Overview

This guide provides comprehensive monitoring, logging, and observability solutions for the Sudachi Multiplayer infrastructure. It covers metrics collection, alerting, log management, performance monitoring, and troubleshooting procedures.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Monitoring Stack                             │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │ Prometheus  │  │   Grafana   │  │      AlertManager       │ │
│  │             │  │             │  │                         │ │
│  │ - Metrics   │──│ - Dashboards│──│ - Alert Routing         │ │
│  │ - Storage   │  │ - Queries   │  │ - Notifications         │ │
│  │ - Rules     │  │ - Users     │  │ - Escalation            │ │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘ │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │   Loki      │  │ Jaeger      │  │       Node Exporter     │ │
│  │             │  │             │  │                         │ │
│  │ - Log Aggr. │  │ - Tracing   │  │ - System Metrics        │ │
│  │ - Search    │  │ - Spans     │  │ - Hardware Stats        │ │
│  │ - Retention │  │ - Analysis  │  │ - OS Performance        │ │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
      ┌─────────────────────────▼─────────────────────────┐
      │              Application Services                  │
      │                                                   │
      │  ┌─────────────┐  ┌─────────────┐  ┌───────────┐ │
      │  │Room Server  │  │Relay Server │  │  NGINX    │ │
      │  │             │  │             │  │           │ │
      │  │ - App       │  │ - Connection│  │ - Access  │ │
      │  │   Metrics   │  │   Metrics   │  │   Logs    │ │
      │  │ - Business  │  │ - Bandwidth │  │ - Error   │ │
      │  │   Logic     │  │ - Latency   │  │   Logs    │ │
      │  └─────────────┘  └─────────────┘  └───────────┘ │
      └───────────────────────────────────────────────────┘
```

## Metrics Collection

### Prometheus Setup

#### Configuration

Create `monitoring/prometheus/prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'sudachi-multiplayer'
    environment: 'production'

rule_files:
  - "rules/*.yml"

scrape_configs:
  # Room Server Metrics
  - job_name: 'room-server'
    static_configs:
      - targets: ['room-server:3000']
    metrics_path: '/metrics'
    scrape_interval: 10s
    scrape_timeout: 5s
    
  # Relay Server Metrics
  - job_name: 'relay-server'
    static_configs:
      - targets: ['relay-server:8080']
    metrics_path: '/metrics'
    scrape_interval: 10s
    scrape_timeout: 5s
    
  # NGINX Metrics
  - job_name: 'nginx'
    static_configs:
      - targets: ['nginx:9113']
    scrape_interval: 30s
    
  # PostgreSQL Metrics
  - job_name: 'postgres'
    static_configs:
      - targets: ['postgres-exporter:9187']
    scrape_interval: 30s
    
  # Redis Metrics
  - job_name: 'redis'
    static_configs:
      - targets: ['redis-exporter:9121']
    scrape_interval: 30s
    
  # System Metrics
  - job_name: 'node-exporter'
    static_configs:
      - targets: ['node-exporter:9100']
    scrape_interval: 30s
    
  # Container Metrics
  - job_name: 'cadvisor'
    static_configs:
      - targets: ['cadvisor:8080']
    scrape_interval: 30s

# Alerting configuration
alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - alertmanager:9093
      scheme: http
      timeout: 10s
      api_version: v1
```

#### Alerting Rules

Create `monitoring/prometheus/rules/sudachi-multiplayer.yml`:

```yaml
groups:
  - name: sudachi-multiplayer.rules
    interval: 30s
    rules:
      # Service Health Alerts
      - alert: ServiceDown
        expr: up == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Service {{ $labels.job }} is down"
          description: "Service {{ $labels.job }} on {{ $labels.instance }} has been down for more than 1 minute."
          
      - alert: HighMemoryUsage
        expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High memory usage on {{ $labels.instance }}"
          description: "Memory usage is above 90% on {{ $labels.instance }} for more than 5 minutes."
          
      - alert: HighCPUUsage
        expr: 100 - (avg by (instance) (irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 80
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High CPU usage on {{ $labels.instance }}"
          description: "CPU usage is above 80% on {{ $labels.instance }} for more than 5 minutes."
          
      # Room Server Specific Alerts
      - alert: RoomServerHighConnections
        expr: room_server_active_connections > 8000
        for: 2m
        labels:
          severity: warning
        annotations:
          summary: "Room server has high number of connections"
          description: "Room server has {{ $value }} active connections, approaching limit of 10000."
          
      - alert: RoomServerHighLatency
        expr: histogram_quantile(0.95, rate(room_server_request_duration_seconds_bucket[5m])) > 0.5
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Room server high latency"
          description: "95th percentile latency is {{ $value }}s for room server requests."
          
      - alert: RoomServerErrorRate
        expr: rate(room_server_requests_total{status=~"5.."}[5m]) / rate(room_server_requests_total[5m]) > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Room server high error rate"
          description: "Error rate is {{ $value | humanizePercentage }} for room server."
          
      # Relay Server Specific Alerts
      - alert: RelayServerHighSessions
        expr: relay_server_active_sessions > 800
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Relay server has high number of sessions"
          description: "Relay server has {{ $value }} active sessions, approaching limit of 1000."
          
      - alert: RelayServerHighBandwidth
        expr: rate(relay_server_bytes_transmitted_total[5m]) > 100 * 1024 * 1024  # 100 MB/s
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Relay server high bandwidth usage"
          description: "Relay server bandwidth usage is {{ $value | humanize }}B/s."
          
      # Database Alerts
      - alert: PostgreSQLDown
        expr: pg_up == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "PostgreSQL is down"
          description: "PostgreSQL database is down on {{ $labels.instance }}."
          
      - alert: PostgreSQLHighConnections
        expr: pg_stat_database_numbackends / pg_settings_max_connections > 0.8
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "PostgreSQL high connection usage"
          description: "PostgreSQL connection usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
          
      - alert: PostgreSQLSlowQueries
        expr: pg_stat_statements_mean_time_ms > 1000
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "PostgreSQL slow queries detected"
          description: "Average query time is {{ $value }}ms on {{ $labels.instance }}."
          
      # Redis Alerts
      - alert: RedisDown
        expr: redis_up == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Redis is down"
          description: "Redis instance is down on {{ $labels.instance }}."
          
      - alert: RedisHighMemoryUsage
        expr: redis_memory_used_bytes / redis_memory_max_bytes > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Redis high memory usage"
          description: "Redis memory usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}."
          
      - alert: RedisHighCommandLatency
        expr: redis_commands_duration_seconds_total / redis_commands_processed_total > 0.1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Redis high command latency"
          description: "Average Redis command latency is {{ $value }}s on {{ $labels.instance }}."
```

### Application Metrics

#### Room Server Metrics

Implement metrics in the Room Server Node.js application:

```javascript
const prometheus = require('prom-client');

// Create a Registry
const register = new prometheus.Registry();

// Add default metrics
prometheus.collectDefaultMetrics({
    register,
    prefix: 'room_server_',
});

// Custom metrics
const activeConnections = new prometheus.Gauge({
    name: 'room_server_active_connections',
    help: 'Number of active WebSocket connections',
    registers: [register]
});

const activeSessions = new prometheus.Gauge({
    name: 'room_server_active_sessions',
    help: 'Number of active game sessions',
    registers: [register]
});

const requestDuration = new prometheus.Histogram({
    name: 'room_server_request_duration_seconds',
    help: 'Duration of HTTP requests in seconds',
    labelNames: ['method', 'route', 'status'],
    buckets: [0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1, 5, 10],
    registers: [register]
});

const requestsTotal = new prometheus.Counter({
    name: 'room_server_requests_total',
    help: 'Total number of HTTP requests',
    labelNames: ['method', 'route', 'status'],
    registers: [register]
});

const websocketMessages = new prometheus.Counter({
    name: 'room_server_websocket_messages_total',
    help: 'Total number of WebSocket messages',
    labelNames: ['type', 'direction'],
    registers: [register]
});

// Export metrics endpoint
app.get('/metrics', async (req, res) => {
    res.set('Content-Type', register.contentType);
    res.end(await register.metrics());
});

// Middleware to track request metrics
app.use((req, res, next) => {
    const start = Date.now();
    
    res.on('finish', () => {
        const duration = (Date.now() - start) / 1000;
        requestDuration
            .labels(req.method, req.route?.path || req.path, res.statusCode)
            .observe(duration);
        requestsTotal
            .labels(req.method, req.route?.path || req.path, res.statusCode)
            .inc();
    });
    
    next();
});

// Update connection metrics
io.on('connection', (socket) => {
    activeConnections.inc();
    
    socket.on('disconnect', () => {
        activeConnections.dec();
    });
    
    socket.on('message', (data) => {
        websocketMessages.labels(data.type || 'unknown', 'inbound').inc();
    });
});

// Update session metrics
function updateSessionMetrics() {
    // This would be called when sessions are created/destroyed
    activeSessions.set(getCurrentSessionCount());
}
```

#### Relay Server Metrics

Implement metrics in the C++ Relay Server:

```cpp
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/gateway.h>

class RelayServerMetrics {
private:
    std::shared_ptr<prometheus::Registry> registry_;
    prometheus::Gateway gateway_;
    
    // Metrics
    prometheus::Family<prometheus::Counter>& bytes_transmitted_;
    prometheus::Family<prometheus::Counter>& bytes_received_;
    prometheus::Family<prometheus::Gauge>& active_sessions_;
    prometheus::Family<prometheus::Gauge>& active_connections_;
    prometheus::Family<prometheus::Histogram>& packet_processing_time_;
    
public:
    RelayServerMetrics(const std::string& gateway_url) 
        : registry_(std::make_shared<prometheus::Registry>())
        , gateway_(gateway_url, "relay_server", registry_)
        , bytes_transmitted_(prometheus::BuildCounter()
            .Name("relay_server_bytes_transmitted_total")
            .Help("Total bytes transmitted by relay server")
            .Register(*registry_))
        , bytes_received_(prometheus::BuildCounter()
            .Name("relay_server_bytes_received_total")
            .Help("Total bytes received by relay server")
            .Register(*registry_))
        , active_sessions_(prometheus::BuildGauge()
            .Name("relay_server_active_sessions")
            .Help("Number of active relay sessions")
            .Register(*registry_))
        , active_connections_(prometheus::BuildGauge()
            .Name("relay_server_active_connections")
            .Help("Number of active relay connections")
            .Register(*registry_))
        , packet_processing_time_(prometheus::BuildHistogram()
            .Name("relay_server_packet_processing_seconds")
            .Help("Time spent processing packets")
            .Register(*registry_)) {}
    
    void RecordBytesTransmitted(size_t bytes, const std::string& session_id) {
        bytes_transmitted_.Add({{"session", session_id}}).Increment(bytes);
    }
    
    void RecordBytesReceived(size_t bytes, const std::string& session_id) {
        bytes_received_.Add({{"session", session_id}}).Increment(bytes);
    }
    
    void SetActiveSessions(int count) {
        active_sessions_.Add({}).Set(count);
    }
    
    void SetActiveConnections(int count) {
        active_connections_.Add({}).Set(count);
    }
    
    void RecordPacketProcessingTime(double seconds) {
        packet_processing_time_.Add({}).Observe(seconds);
    }
    
    void Push() {
        gateway_.Push();
    }
};

// Usage in relay server
class RelayServer {
private:
    RelayServerMetrics metrics_;
    
public:
    void ProcessPacket(const Packet& packet) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Process packet logic here
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end - start).count();
        metrics_.RecordPacketProcessingTime(duration);
        metrics_.RecordBytesReceived(packet.size(), packet.session_id());
    }
    
    void SendPacket(const Packet& packet) {
        // Send packet logic here
        metrics_.RecordBytesTransmitted(packet.size(), packet.session_id());
    }
    
    void UpdateMetrics() {
        metrics_.SetActiveSessions(GetActiveSessionCount());
        metrics_.SetActiveConnections(GetActiveConnectionCount());
        metrics_.Push();
    }
};
```

## Grafana Dashboards

### Room Server Dashboard

Create `monitoring/grafana/dashboards/room-server.json`:

```json
{
  "dashboard": {
    "id": null,
    "title": "Sudachi Room Server",
    "tags": ["sudachi", "multiplayer", "room-server"],
    "timezone": "browser",
    "panels": [
      {
        "id": 1,
        "title": "Active Connections",
        "type": "stat",
        "targets": [
          {
            "expr": "room_server_active_connections",
            "legendFormat": "Connections"
          }
        ],
        "fieldConfig": {
          "defaults": {
            "color": {
              "mode": "thresholds"
            },
            "thresholds": {
              "steps": [
                {"color": "green", "value": null},
                {"color": "yellow", "value": 5000},
                {"color": "red", "value": 8000}
              ]
            }
          }
        }
      },
      {
        "id": 2,
        "title": "Active Sessions",
        "type": "stat",
        "targets": [
          {
            "expr": "room_server_active_sessions",
            "legendFormat": "Sessions"
          }
        ]
      },
      {
        "id": 3,
        "title": "Request Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(room_server_requests_total[5m])",
            "legendFormat": "{{method}} {{route}}"
          }
        ]
      },
      {
        "id": 4,
        "title": "Response Time",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.50, rate(room_server_request_duration_seconds_bucket[5m])) * 1000",
            "legendFormat": "50th percentile"
          },
          {
            "expr": "histogram_quantile(0.95, rate(room_server_request_duration_seconds_bucket[5m])) * 1000",
            "legendFormat": "95th percentile"
          },
          {
            "expr": "histogram_quantile(0.99, rate(room_server_request_duration_seconds_bucket[5m])) * 1000",
            "legendFormat": "99th percentile"
          }
        ],
        "yAxes": [
          {
            "unit": "ms"
          }
        ]
      },
      {
        "id": 5,
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(room_server_requests_total{status=~\"5..\"}[5m]) / rate(room_server_requests_total[5m]) * 100",
            "legendFormat": "Error Rate %"
          }
        ],
        "yAxes": [
          {
            "unit": "percent"
          }
        ]
      },
      {
        "id": 6,
        "title": "Memory Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "process_resident_memory_bytes{job=\"room-server\"}",
            "legendFormat": "RSS Memory"
          },
          {
            "expr": "nodejs_heap_size_used_bytes{job=\"room-server\"}",
            "legendFormat": "Heap Used"
          }
        ],
        "yAxes": [
          {
            "unit": "bytes"
          }
        ]
      }
    ],
    "time": {
      "from": "now-1h",
      "to": "now"
    },
    "refresh": "30s"
  }
}
```

### Infrastructure Dashboard

Create `monitoring/grafana/dashboards/infrastructure.json`:

```json
{
  "dashboard": {
    "id": null,
    "title": "Sudachi Infrastructure",
    "tags": ["sudachi", "infrastructure", "system"],
    "panels": [
      {
        "id": 1,
        "title": "CPU Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "100 - (avg by (instance) (irate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "{{instance}}"
          }
        ],
        "yAxes": [
          {
            "max": 100,
            "min": 0,
            "unit": "percent"
          }
        ]
      },
      {
        "id": 2,
        "title": "Memory Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "(node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes * 100",
            "legendFormat": "{{instance}}"
          }
        ],
        "yAxes": [
          {
            "max": 100,
            "min": 0,
            "unit": "percent"
          }
        ]
      },
      {
        "id": 3,
        "title": "Disk Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "(node_filesystem_size_bytes - node_filesystem_free_bytes) / node_filesystem_size_bytes * 100",
            "legendFormat": "{{instance}} {{mountpoint}}"
          }
        ],
        "yAxes": [
          {
            "max": 100,
            "min": 0,
            "unit": "percent"
          }
        ]
      },
      {
        "id": 4,
        "title": "Network I/O",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(node_network_receive_bytes_total[5m])",
            "legendFormat": "{{instance}} {{device}} RX"
          },
          {
            "expr": "rate(node_network_transmit_bytes_total[5m])",
            "legendFormat": "{{instance}} {{device}} TX"
          }
        ],
        "yAxes": [
          {
            "unit": "Bps"
          }
        ]
      }
    ]
  }
}
```

## Log Management

### Structured Logging Configuration

#### Room Server Logging

```javascript
const winston = require('winston');
const { ElasticsearchTransport } = require('winston-elasticsearch');

// Create logger
const logger = winston.createLogger({
    level: process.env.LOG_LEVEL || 'info',
    format: winston.format.combine(
        winston.format.timestamp(),
        winston.format.errors({ stack: true }),
        winston.format.json()
    ),
    defaultMeta: {
        service: 'room-server',
        version: process.env.APP_VERSION || '1.0.0',
        environment: process.env.NODE_ENV || 'development'
    },
    transports: [
        // Console output
        new winston.transports.Console({
            format: winston.format.combine(
                winston.format.colorize(),
                winston.format.simple()
            )
        }),
        
        // File output
        new winston.transports.File({
            filename: '/var/log/sudachi/room-server-error.log',
            level: 'error',
            maxsize: 100 * 1024 * 1024, // 100MB
            maxFiles: 5
        }),
        new winston.transports.File({
            filename: '/var/log/sudachi/room-server.log',
            maxsize: 100 * 1024 * 1024, // 100MB
            maxFiles: 10
        })
    ]
});

// Usage examples
logger.info('Server starting', {
    port: 3000,
    environment: process.env.NODE_ENV
});

logger.error('Database connection failed', {
    error: error.message,
    stack: error.stack,
    database: 'postgresql'
});

logger.warn('High memory usage detected', {
    usage: '85%',
    threshold: '80%'
});

// Request logging middleware
app.use((req, res, next) => {
    const start = Date.now();
    
    res.on('finish', () => {
        const duration = Date.now() - start;
        logger.info('HTTP request', {
            method: req.method,
            url: req.url,
            status: res.statusCode,
            duration: duration,
            userAgent: req.get('User-Agent'),
            ip: req.ip
        });
    });
    
    next();
});
```

#### Relay Server Logging

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class RelayServerLogger {
private:
    std::shared_ptr<spdlog::logger> logger_;
    
public:
    RelayServerLogger() {
        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "/var/log/sudachi/relay-server.log", 
            100 * 1024 * 1024,  // 100MB
            10                   // 10 files
        );
        
        // Create logger
        logger_ = std::make_shared<spdlog::logger>("relay-server", 
            spdlog::sinks_init_list{console_sink, file_sink});
        
        // Set format
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
        logger_->set_level(spdlog::level::info);
        
        // Register globally
        spdlog::register_logger(logger_);
    }
    
    template<typename... Args>
    void Info(const std::string& format, Args&&... args) {
        logger_->info(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Error(const std::string& format, Args&&... args) {
        logger_->error(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void Warn(const std::string& format, Args&&... args) {
        logger_->warn(format, std::forward<Args>(args)...);
    }
};

// Usage
RelayServerLogger logger;

void ProcessSession(const SessionInfo& session) {
    logger.Info("Processing session: id={}, players={}", 
                session.id, session.player_count);
    
    try {
        // Process session logic
    } catch (const std::exception& e) {
        logger.Error("Session processing failed: id={}, error={}", 
                     session.id, e.what());
    }
}
```

### Log Aggregation with Loki

#### Loki Configuration

Create `monitoring/loki/loki.yml`:

```yaml
auth_enabled: false

server:
  http_listen_port: 3100
  grpc_listen_port: 9096

ingester:
  wal:
    enabled: true
    dir: /loki/wal
  lifecycler:
    address: 127.0.0.1
    ring:
      kvstore:
        store: inmemory
      replication_factor: 1
    final_sleep: 0s
  chunk_idle_period: 1h
  max_chunk_age: 1h
  chunk_target_size: 1048576
  chunk_retain_period: 30s
  max_transfer_retries: 0

schema_config:
  configs:
    - from: 2020-10-24
      store: boltdb-shipper
      object_store: filesystem
      schema: v11
      index:
        prefix: index_
        period: 24h

storage_config:
  boltdb_shipper:
    active_index_directory: /loki/boltdb-shipper-active
    cache_location: /loki/boltdb-shipper-cache
    cache_ttl: 24h
    shared_store: filesystem
  filesystem:
    directory: /loki/chunks

compactor:
  working_directory: /loki/boltdb-shipper-compactor
  shared_store: filesystem

limits_config:
  reject_old_samples: true
  reject_old_samples_max_age: 168h

chunk_store_config:
  max_look_back_period: 0s

table_manager:
  retention_deletes_enabled: false
  retention_period: 0s

ruler:
  storage:
    type: local
    local:
      directory: /loki/rules
  rule_path: /loki/rules
  alertmanager_url: http://alertmanager:9093
  ring:
    kvstore:
      store: inmemory
  enable_api: true
```

#### Promtail Configuration

Create `monitoring/promtail/promtail.yml`:

```yaml
server:
  http_listen_port: 9080
  grpc_listen_port: 0

positions:
  filename: /tmp/positions.yaml

clients:
  - url: http://loki:3100/loki/api/v1/push

scrape_configs:
  # Room Server logs
  - job_name: room-server
    static_configs:
      - targets:
          - localhost
        labels:
          job: room-server
          service: sudachi-multiplayer
          __path__: /var/log/sudachi/room-server*.log
    pipeline_stages:
      - json:
          expressions:
            timestamp: timestamp
            level: level
            message: message
            service: service
      - timestamp:
          source: timestamp
          format: RFC3339
      - labels:
          level:
          service:
          
  # Relay Server logs
  - job_name: relay-server
    static_configs:
      - targets:
          - localhost
        labels:
          job: relay-server
          service: sudachi-multiplayer
          __path__: /var/log/sudachi/relay-server*.log
    pipeline_stages:
      - regex:
          expression: '^\[(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\] \[(?P<level>\w+)\] \[(?P<logger>\w+)\] (?P<message>.*)'
      - timestamp:
          source: timestamp
          format: '2006-01-02 15:04:05.000'
      - labels:
          level:
          logger:
          
  # NGINX access logs
  - job_name: nginx-access
    static_configs:
      - targets:
          - localhost
        labels:
          job: nginx-access
          service: sudachi-multiplayer
          __path__: /var/log/nginx/access.log
    pipeline_stages:
      - regex:
          expression: '^(?P<remote_addr>[\d\.]+) - (?P<remote_user>[^ ]*) \[(?P<time_local>[^\]]*)\] "(?P<method>\S+)(?: +(?P<path>[^\"]*?)(?: +\S*)?)?" (?P<status>\d+) (?P<body_bytes_sent>\d+) "(?P<http_referer>[^\"]*)" "(?P<http_user_agent>[^\"]*)"'
      - labels:
          method:
          status:
      - timestamp:
          source: time_local
          format: '02/Jan/2006:15:04:05 -0700'
          
  # NGINX error logs
  - job_name: nginx-error
    static_configs:
      - targets:
          - localhost
        labels:
          job: nginx-error
          service: sudachi-multiplayer
          __path__: /var/log/nginx/error.log
```

## Alerting Configuration

### AlertManager Setup

Create `monitoring/alertmanager/alertmanager.yml`:

```yaml
global:
  smtp_smarthost: 'smtp.gmail.com:587'
  smtp_from: 'alerts@sudachi.org'
  smtp_auth_username: 'alerts@sudachi.org'
  smtp_auth_password: 'your-email-password'

route:
  group_by: ['alertname', 'cluster', 'service']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 1h
  receiver: 'web.hook'
  routes:
    - match:
        severity: critical
      receiver: 'critical-alerts'
      group_wait: 10s
      repeat_interval: 5m
    - match:
        severity: warning
      receiver: 'warning-alerts'
      repeat_interval: 30m

receivers:
  - name: 'web.hook'
    webhook_configs:
      - url: 'http://webhook-receiver:8080/webhook'
        
  - name: 'critical-alerts'
    email_configs:
      - to: 'ops-team@sudachi.org'
        subject: 'CRITICAL: {{ .GroupLabels.alertname }} in {{ .GroupLabels.cluster }}'
        body: |
          {{ range .Alerts }}
          Alert: {{ .Annotations.summary }}
          Description: {{ .Annotations.description }}
          Labels: {{ range .Labels.SortedPairs }}{{ .Name }}: {{ .Value }}{{ end }}
          {{ end }}
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#alerts-critical'
        title: 'Critical Alert in {{ .GroupLabels.cluster }}'
        text: '{{ range .Alerts }}{{ .Annotations.summary }}{{ end }}'
        
  - name: 'warning-alerts'
    email_configs:
      - to: 'monitoring@sudachi.org'
        subject: 'WARNING: {{ .GroupLabels.alertname }} in {{ .GroupLabels.cluster }}'
        body: |
          {{ range .Alerts }}
          Alert: {{ .Annotations.summary }}
          Description: {{ .Annotations.description }}
          {{ end }}

inhibit_rules:
  - source_match:
      severity: 'critical'
    target_match:
      severity: 'warning'
    equal: ['alertname', 'cluster', 'service']
```

### Custom Alert Rules

Create `monitoring/prometheus/rules/custom-alerts.yml`:

```yaml
groups:
  - name: sudachi-business-logic.rules
    rules:
      # Business logic alerts
      - alert: LowSessionCreationRate
        expr: rate(room_server_sessions_created_total[1h]) < 10
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Low session creation rate detected"
          description: "Only {{ $value }} sessions created per hour, below normal threshold of 10."
          
      - alert: HighSessionFailureRate
        expr: rate(room_server_sessions_failed_total[5m]) / rate(room_server_sessions_created_total[5m]) > 0.1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High session failure rate"
          description: "{{ $value | humanizePercentage }} of sessions are failing to create."
          
      - alert: UnusualTrafficPattern
        expr: rate(room_server_requests_total[5m]) > 2 * avg_over_time(rate(room_server_requests_total[5m])[1h:5m])
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Unusual traffic spike detected"
          description: "Current request rate is {{ $value }} req/s, significantly higher than normal."
          
      # Player behavior alerts
      - alert: HighPlayerDisconnectionRate
        expr: rate(room_server_player_disconnections_total[5m]) > 50
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "High player disconnection rate"
          description: "{{ $value }} players disconnecting per second, indicating potential network issues."
```

## Performance Monitoring

### Application Performance Monitoring (APM)

#### Jaeger Tracing Setup

Create `monitoring/jaeger/jaeger.yml`:

```yaml
version: '3.8'

services:
  jaeger:
    image: jaegertracing/all-in-one:latest
    container_name: sudachi-jaeger
    ports:
      - "16686:16686"  # Jaeger UI
      - "14268:14268"  # HTTP collector
      - "6831:6831/udp"  # UDP agent
    environment:
      - COLLECTOR_ZIPKIN_HTTP_PORT=9411
      - SPAN_STORAGE_TYPE=badger
      - BADGER_EPHEMERAL=false
      - BADGER_DIRECTORY_VALUE=/badger/data
      - BADGER_DIRECTORY_KEY=/badger/key
    volumes:
      - jaeger_badger:/badger
    networks:
      - sudachi-network

volumes:
  jaeger_badger:
```

Implement tracing in Room Server:

```javascript
const opentracing = require('opentracing');
const { initTracer } = require('jaeger-client');

// Initialize tracer
const config = {
    serviceName: 'room-server',
    sampler: {
        type: 'const',
        param: 1,
    },
    reporter: {
        logSpans: true,
        agentHost: process.env.JAEGER_AGENT_HOST || 'localhost',
        agentPort: process.env.JAEGER_AGENT_PORT || 6832,
    },
};

const tracer = initTracer(config);
opentracing.initGlobalTracer(tracer);

// Tracing middleware
app.use((req, res, next) => {
    const span = tracer.startSpan(`${req.method} ${req.path}`);
    span.setTag('http.method', req.method);
    span.setTag('http.url', req.url);
    span.setTag('user.ip', req.ip);
    
    req.span = span;
    
    res.on('finish', () => {
        span.setTag('http.status_code', res.statusCode);
        if (res.statusCode >= 400) {
            span.setTag('error', true);
        }
        span.finish();
    });
    
    next();
});

// WebSocket tracing
io.on('connection', (socket) => {
    const span = tracer.startSpan('websocket_connection');
    span.setTag('socket.id', socket.id);
    span.setTag('client.ip', socket.handshake.address);
    
    socket.on('message', (data) => {
        const messageSpan = tracer.startSpan('websocket_message', { childOf: span });
        messageSpan.setTag('message.type', data.type);
        
        // Process message with tracing
        processMessage(data, messageSpan)
            .then(() => messageSpan.finish())
            .catch(err => {
                messageSpan.setTag('error', true);
                messageSpan.log({ error: err.message });
                messageSpan.finish();
            });
    });
    
    socket.on('disconnect', () => {
        span.finish();
    });
});
```

### Database Performance Monitoring

#### PostgreSQL Monitoring

Add PostgreSQL exporter to Docker Compose:

```yaml
postgres-exporter:
  image: prometheuscommunity/postgres-exporter
  container_name: sudachi-postgres-exporter
  environment:
    DATA_SOURCE_NAME: "postgresql://sudachi:${POSTGRES_PASSWORD}@postgresql:5432/sudachi_multiplayer?sslmode=disable"
  ports:
    - "9187:9187"
  depends_on:
    - postgresql
  networks:
    - sudachi-network
```

Create custom PostgreSQL queries in `monitoring/postgres-exporter/queries.yml`:

```yaml
pg_stat_statements:
  query: |
    SELECT 
      query,
      calls,
      total_time,
      mean_time,
      rows,
      100.0 * shared_blks_hit / nullif(shared_blks_hit + shared_blks_read, 0) AS hit_percent
    FROM pg_stat_statements 
    WHERE calls > 100
    ORDER BY total_time DESC 
    LIMIT 50
  metrics:
    - query:
        usage: "LABEL"
        description: "Query text"
    - calls:
        usage: "COUNTER"
        description: "Number of times executed"
    - total_time:
        usage: "COUNTER"
        description: "Total time spent in the statement, in milliseconds"
    - mean_time:
        usage: "GAUGE"
        description: "Mean time spent in the statement, in milliseconds"
    - rows:
        usage: "COUNTER"
        description: "Total number of rows retrieved or affected by the statement"
    - hit_percent:
        usage: "GAUGE"
        description: "Percentage of shared block hits"

pg_slow_queries:
  query: |
    SELECT 
      query,
      state,
      now() - query_start as duration,
      waiting,
      client_addr
    FROM pg_stat_activity 
    WHERE state = 'active' 
    AND now() - query_start > interval '1 minute'
  metrics:
    - query:
        usage: "LABEL"
        description: "Query text"
    - state:
        usage: "LABEL"
        description: "Query state"
    - duration:
        usage: "GAUGE"
        description: "Query duration in seconds"
    - waiting:
        usage: "GAUGE"
        description: "Whether query is waiting"
```

## Log Analysis and Troubleshooting

### Common Log Patterns

#### Error Detection Queries (Loki)

```logql
# High error rate detection
count_over_time({job="room-server"} |= "ERROR" [5m])

# Database connection issues
{job="room-server"} |= "database" |= "connection" |= "failed"

# WebSocket connection failures
{job="room-server"} |= "websocket" |= "error" | json | level="error"

# Session creation failures
{job="room-server"} |= "session" |= "creation" |= "failed" | json

# High latency requests
{job="nginx-access"} | logfmt | duration > 1000

# Memory-related issues
{job="room-server"} |~ "memory|heap|oom"

# Security-related events
{job="nginx-access"} | logfmt | status >= 400 | client_ip != "127.0.0.1"
```

#### Performance Analysis Queries

```logql
# Request duration percentiles
quantile_over_time(0.95, 
  {job="nginx-access"} 
  | logfmt 
  | unwrap duration [5m]
)

# Error rate by endpoint
sum by (path) (
  count_over_time({job="nginx-access"} | logfmt | status >= 400 [5m])
) / 
sum by (path) (
  count_over_time({job="nginx-access"} | logfmt [5m])
)

# Top error messages
topk(10, 
  count by (message) (
    count_over_time({job="room-server"} | json | level="error" [1h])
  )
)
```

### Automated Log Analysis

Create `scripts/log-analysis.sh`:

```bash
#!/bin/bash

# Log analysis script for Sudachi Multiplayer
LOGDIR="/var/log/sudachi"
REPORT_FILE="/tmp/sudachi-log-analysis-$(date +%Y%m%d_%H%M%S).txt"

echo "Sudachi Multiplayer Log Analysis Report" > $REPORT_FILE
echo "Generated: $(date)" >> $REPORT_FILE
echo "=======================================" >> $REPORT_FILE

# Function to analyze error patterns
analyze_errors() {
    echo "" >> $REPORT_FILE
    echo "ERROR ANALYSIS" >> $REPORT_FILE
    echo "=============" >> $REPORT_FILE
    
    # Count errors by service
    echo "Error counts by service:" >> $REPORT_FILE
    for service in room-server relay-server; do
        if [ -f "$LOGDIR/$service.log" ]; then
            error_count=$(grep -c "ERROR\|error" "$LOGDIR/$service.log" || echo "0")
            echo "  $service: $error_count errors" >> $REPORT_FILE
        fi
    done
    
    # Top error messages
    echo "" >> $REPORT_FILE
    echo "Top 10 error messages (last 24h):" >> $REPORT_FILE
    find $LOGDIR -name "*.log" -mtime -1 -exec grep -h "ERROR\|error" {} \; | \
        sed 's/.*ERROR.*: //' | sed 's/.*error.*: //' | \
        sort | uniq -c | sort -nr | head -10 >> $REPORT_FILE
}

# Function to analyze performance issues
analyze_performance() {
    echo "" >> $REPORT_FILE
    echo "PERFORMANCE ANALYSIS" >> $REPORT_FILE
    echo "==================" >> $REPORT_FILE
    
    # High latency requests from NGINX logs
    if [ -f "/var/log/nginx/access.log" ]; then
        echo "High latency requests (>1s):" >> $REPORT_FILE
        awk '$NF > 1000 {print $0}' /var/log/nginx/access.log | tail -10 >> $REPORT_FILE
    fi
    
    # Memory warnings
    echo "" >> $REPORT_FILE
    echo "Memory-related warnings:" >> $REPORT_FILE
    find $LOGDIR -name "*.log" -mtime -1 -exec grep -h "memory\|heap\|Memory" {} \; | \
        tail -10 >> $REPORT_FILE
}

# Function to analyze security events
analyze_security() {
    echo "" >> $REPORT_FILE
    echo "SECURITY ANALYSIS" >> $REPORT_FILE
    echo "================" >> $REPORT_FILE
    
    # Failed authentication attempts
    echo "Failed authentication attempts:" >> $REPORT_FILE
    find $LOGDIR -name "*.log" -mtime -1 -exec grep -h "authentication.*failed\|auth.*fail" {} \; | \
        wc -l >> $REPORT_FILE
    
    # Suspicious IP addresses
    if [ -f "/var/log/nginx/access.log" ]; then
        echo "" >> $REPORT_FILE
        echo "Top suspicious IPs (4xx/5xx responses):" >> $REPORT_FILE
        awk '$9 >= 400 {print $1}' /var/log/nginx/access.log | \
            sort | uniq -c | sort -nr | head -10 >> $REPORT_FILE
    fi
}

# Function to generate recommendations
generate_recommendations() {
    echo "" >> $REPORT_FILE
    echo "RECOMMENDATIONS" >> $REPORT_FILE
    echo "==============" >> $REPORT_FILE
    
    # Check error rates
    total_errors=$(find $LOGDIR -name "*.log" -mtime -1 -exec grep -c "ERROR\|error" {} \; | \
                   awk '{sum += $1} END {print sum}')
    
    if [ "$total_errors" -gt 100 ]; then
        echo "- High error rate detected ($total_errors errors). Investigate error patterns." >> $REPORT_FILE
    fi
    
    # Check disk space
    disk_usage=$(df $LOGDIR | tail -1 | awk '{print $5}' | sed 's/%//')
    if [ "$disk_usage" -gt 80 ]; then
        echo "- Log directory is ${disk_usage}% full. Consider log rotation or cleanup." >> $REPORT_FILE
    fi
    
    # Check log file sizes
    large_logs=$(find $LOGDIR -name "*.log" -size +100M)
    if [ -n "$large_logs" ]; then
        echo "- Large log files detected. Consider increasing rotation frequency." >> $REPORT_FILE
    fi
}

# Run analysis
analyze_errors
analyze_performance
analyze_security
generate_recommendations

echo "" >> $REPORT_FILE
echo "Analysis complete. Report saved to: $REPORT_FILE"

# Display summary
echo "Sudachi Multiplayer Log Analysis Summary"
echo "======================================="
echo "Total errors found: $(find $LOGDIR -name "*.log" -mtime -1 -exec grep -c "ERROR\|error" {} \; | awk '{sum += $1} END {print sum}')"
echo "Report saved to: $REPORT_FILE"

# Send report via email if configured
if [ -n "$ALERT_EMAIL" ]; then
    mail -s "Sudachi Multiplayer Log Analysis Report" "$ALERT_EMAIL" < $REPORT_FILE
fi
```

## Maintenance and Operations

### Regular Maintenance Tasks

Create `scripts/maintenance.sh`:

```bash
#!/bin/bash

# Maintenance script for Sudachi Multiplayer monitoring
set -e

echo "Starting maintenance tasks..."

# Clean up old metrics data
echo "Cleaning up old Prometheus data..."
docker exec sudachi-prometheus sh -c "
    find /prometheus -name '*.db' -mtime +30 -delete
    find /prometheus -name 'wal' -type d -mtime +7 -exec rm -rf {} +
"

# Clean up old log files
echo "Cleaning up old log files..."
find /var/log/sudachi -name "*.log.gz" -mtime +14 -delete
find /var/log/sudachi -name "*.log.[0-9]*" -mtime +7 -delete

# Optimize database
echo "Optimizing PostgreSQL database..."
docker exec sudachi-postgresql psql -U sudachi -d sudachi_multiplayer -c "
    VACUUM ANALYZE;
    REINDEX DATABASE sudachi_multiplayer;
"

# Clean up Redis memory
echo "Cleaning up Redis memory..."
docker exec sudachi-redis redis-cli MEMORY PURGE

# Check disk space
echo "Checking disk space..."
df -h /var/log/sudachi
df -h /var/lib/docker

# Update Grafana dashboards
echo "Updating Grafana dashboards..."
curl -X POST http://admin:${GRAFANA_ADMIN_PASSWORD}@localhost:3001/api/admin/provisioning/dashboards/reload

echo "Maintenance tasks completed."
```

### Backup Monitoring Data

```bash
#!/bin/bash

# Backup monitoring data
BACKUP_DIR="/backup/monitoring"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Backup Prometheus data
echo "Backing up Prometheus data..."
docker exec sudachi-prometheus sh -c "
    tar -czf /tmp/prometheus-data-$DATE.tar.gz /prometheus
"
docker cp sudachi-prometheus:/tmp/prometheus-data-$DATE.tar.gz $BACKUP_DIR/

# Backup Grafana data
echo "Backing up Grafana data..."
docker exec sudachi-grafana sh -c "
    tar -czf /tmp/grafana-data-$DATE.tar.gz /var/lib/grafana
"
docker cp sudachi-grafana:/tmp/grafana-data-$DATE.tar.gz $BACKUP_DIR/

# Clean up old backups
find $BACKUP_DIR -name "*.tar.gz" -mtime +7 -delete

echo "Monitoring backup completed."
```

---

*This monitoring guide is maintained by the Sudachi infrastructure team. For support with monitoring setup and troubleshooting, join our [Discord community](https://discord.gg/sudachi) or check our [GitHub repository](https://github.com/sudachi-emulator/sudachi).*