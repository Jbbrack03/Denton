# Sudachi Multiplayer Server Setup Guide

## Overview

This guide covers the deployment and configuration of the server infrastructure required for Sudachi's Model A (Internet-based) multiplayer system. The infrastructure consists of two primary server components:

1. **Room Server**: WebSocket-based discovery and session management
2. **Relay Server**: TCP/UDP relay for P2P connection fallback

## Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Room Server   │    │  Relay Server   │    │  Load Balancer  │
│                 │    │                 │    │                 │
│ - Session Mgmt  │    │ - P2P Fallback  │    │ - NGINX/HAProxy │
│ - Player Disc.  │────│ - Bandwidth Ctrl│────│ - SSL Termina.  │
│ - WebSocket     │    │ - Multi-region  │    │ - Rate Limiting │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
          ┌─────────────────────────────────────────┐
          │            Support Services             │
          │                                         │
          │  ┌──────────┐  ┌──────────┐  ┌────────┐ │
          │  │  Redis   │  │Postgres  │  │Monitor │ │
          │  │ Session  │  │User Data │  │ Grafana│ │
          │  │  Cache   │  │Analytics │  │Prometheus│ │
          │  └──────────┘  └──────────┘  └────────┘ │
          └─────────────────────────────────────────┘
```

## Server Requirements

### Minimum Requirements (Development/Testing)

**Room Server**:
- **CPU**: 2 cores, 2.0GHz
- **RAM**: 2GB
- **Storage**: 20GB SSD
- **Network**: 100 Mbps symmetric
- **OS**: Ubuntu 20.04 LTS or CentOS 8+

**Relay Server**:
- **CPU**: 4 cores, 2.5GHz  
- **RAM**: 4GB
- **Storage**: 10GB SSD
- **Network**: 1 Gbps symmetric
- **OS**: Ubuntu 20.04 LTS or CentOS 8+

### Production Requirements

**Room Server (per 10,000 concurrent users)**:
- **CPU**: 8 cores, 3.0GHz (Intel Xeon or AMD EPYC)
- **RAM**: 16GB DDR4
- **Storage**: 100GB SSD (NVMe preferred)
- **Network**: 10 Gbps
- **OS**: Ubuntu 22.04 LTS

**Relay Server (per 1,000 concurrent sessions)**:
- **CPU**: 16 cores, 3.2GHz
- **RAM**: 32GB DDR4
- **Storage**: 50GB SSD
- **Network**: 10 Gbps with low latency
- **OS**: Ubuntu 22.04 LTS

### Cloud Provider Recommendations

#### AWS
- **Room Server**: c5.2xlarge or c5.4xlarge
- **Relay Server**: c5.4xlarge or c5.9xlarge
- **Database**: RDS PostgreSQL (db.t3.medium+)
- **Cache**: ElastiCache Redis (cache.t3.micro+)

#### Google Cloud Platform
- **Room Server**: n2-standard-4 or n2-standard-8
- **Relay Server**: c2-standard-8 or c2-standard-16
- **Database**: Cloud SQL PostgreSQL (db-standard-2+)
- **Cache**: Memorystore Redis (basic-1gb+)

#### Microsoft Azure
- **Room Server**: Standard_D4s_v3 or Standard_D8s_v3
- **Relay Server**: Standard_F8s_v2 or Standard_F16s_v2
- **Database**: Azure Database for PostgreSQL (Basic B2ms+)
- **Cache**: Azure Cache for Redis (C1 Standard+)

## Room Server Setup

### 1. System Preparation

#### Ubuntu/Debian Setup

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Node.js 18.x
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

# Install additional dependencies
sudo apt install -y nginx redis-server postgresql postgresql-contrib \
    certbot python3-certbot-nginx git build-essential

# Configure firewall
sudo ufw allow 22    # SSH
sudo ufw allow 80    # HTTP
sudo ufw allow 443   # HTTPS
sudo ufw allow 3000  # Room server (internal)
sudo ufw enable
```

#### CentOS/RHEL Setup

```bash
# Update system
sudo dnf update -y

# Install Node.js
sudo dnf module install nodejs:18/common

# Install additional dependencies
sudo dnf install -y nginx redis postgresql postgresql-server \
    certbot python3-certbot-nginx git gcc-c++ make

# Initialize PostgreSQL
sudo postgresql-setup --initdb
sudo systemctl enable postgresql redis nginx
sudo systemctl start postgresql redis nginx

# Configure firewall
sudo firewall-cmd --permanent --add-service=ssh
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --permanent --add-service=https
sudo firewall-cmd --permanent --add-port=3000/tcp
sudo firewall-cmd --reload
```

### 2. Database Setup

#### PostgreSQL Configuration

```bash
# Switch to postgres user
sudo -u postgres psql

-- Create database and user
CREATE DATABASE sudachi_multiplayer;
CREATE USER sudachi WITH ENCRYPTED PASSWORD 'your_secure_password';
GRANT ALL PRIVILEGES ON DATABASE sudachi_multiplayer TO sudachi;

-- Create required tables
\c sudachi_multiplayer;

CREATE TABLE game_sessions (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) UNIQUE NOT NULL,
    game_id BIGINT NOT NULL,
    game_version VARCHAR(32) NOT NULL,
    host_name VARCHAR(64) NOT NULL,
    session_name VARCHAR(128) NOT NULL,
    current_players INTEGER DEFAULT 1,
    max_players INTEGER NOT NULL,
    has_password BOOLEAN DEFAULT FALSE,
    is_public BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    host_endpoint VARCHAR(128),
    region VARCHAR(16) DEFAULT 'global'
);

CREATE TABLE session_players (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) REFERENCES game_sessions(session_id),
    player_id INTEGER NOT NULL,
    player_name VARCHAR(64) NOT NULL,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_ready BOOLEAN DEFAULT FALSE
);

CREATE INDEX idx_sessions_game_id ON game_sessions(game_id);
CREATE INDEX idx_sessions_region ON game_sessions(region);
CREATE INDEX idx_sessions_created ON game_sessions(created_at);

-- Enable automatic timestamp updates
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_sessions_updated_at 
    BEFORE UPDATE ON game_sessions 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
```

#### Redis Configuration

Edit `/etc/redis/redis.conf`:

```bash
# Basic configuration
port 6379
bind 127.0.0.1
protected-mode yes
requirepass your_redis_password

# Memory configuration
maxmemory 1gb
maxmemory-policy allkeys-lru

# Persistence configuration
save 900 1
save 300 10
save 60 10000

# Security
rename-command FLUSHDB ""
rename-command FLUSHALL ""
rename-command DEBUG ""
```

Restart Redis:
```bash
sudo systemctl restart redis
```

### 3. Room Server Installation

#### Download and Setup

```bash
# Create application directory
sudo mkdir -p /opt/sudachi-room-server
cd /opt/sudachi-room-server

# Clone room server repository
# Note: Replace with actual repository URL when available
git clone https://github.com/sudachi-emulator/sudachi-room-server.git .

# Install dependencies
npm install --production

# Create configuration file
sudo mkdir -p /etc/sudachi
sudo cp config/production.example.json /etc/sudachi/room-server.json
```

#### Configuration

Edit `/etc/sudachi/room-server.json`:

```json
{
  "server": {
    "port": 3000,
    "host": "0.0.0.0",
    "environment": "production"
  },
  "database": {
    "host": "localhost",
    "port": 5432,
    "name": "sudachi_multiplayer",
    "username": "sudachi",
    "password": "your_secure_password",
    "ssl": false,
    "pool": {
      "min": 2,
      "max": 10
    }
  },
  "redis": {
    "host": "localhost",
    "port": 6379,
    "password": "your_redis_password",
    "db": 0,
    "keyPrefix": "sudachi:room:"
  },
  "websocket": {
    "pingTimeout": 30000,
    "pingInterval": 25000,
    "maxConnections": 10000,
    "cors": {
      "origin": "*",
      "methods": ["GET", "POST"]
    }
  },
  "session": {
    "maxSessions": 5000,
    "sessionTimeout": 1800,
    "cleanupInterval": 300,
    "maxPlayersPerSession": 8
  },
  "logging": {
    "level": "info",
    "file": "/var/log/sudachi/room-server.log",
    "maxFiles": 10,
    "maxSize": "10m"
  },
  "security": {
    "rateLimit": {
      "windowMs": 60000,
      "max": 100
    },
    "maxRequestSize": "1mb",
    "helmet": {
      "contentSecurityPolicy": false
    }
  }
}
```

#### Systemd Service

Create `/etc/systemd/system/sudachi-room-server.service`:

```ini
[Unit]
Description=Sudachi Room Server
After=network.target postgresql.service redis.service
Wants=postgresql.service redis.service

[Service]
Type=simple
User=sudachi
Group=sudachi
WorkingDirectory=/opt/sudachi-room-server
ExecStart=/usr/bin/node server.js
ExecReload=/bin/kill -USR2 $MAINPID
Restart=always
RestartSec=5
Environment=NODE_ENV=production
Environment=CONFIG_PATH=/etc/sudachi/room-server.json

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/log/sudachi

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
```

Create the service user:
```bash
sudo useradd -r -s /bin/false sudachi
sudo mkdir -p /var/log/sudachi
sudo chown sudachi:sudachi /var/log/sudachi
sudo chown -R sudachi:sudachi /opt/sudachi-room-server
```

Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable sudachi-room-server
sudo systemctl start sudachi-room-server
```

### 4. NGINX Reverse Proxy

Create `/etc/nginx/sites-available/sudachi-room`:

```nginx
upstream room_server {
    server 127.0.0.1:3000;
    keepalive 32;
}

# Redirect HTTP to HTTPS
server {
    listen 80;
    server_name room.sudachi.org;
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name room.sudachi.org;

    # SSL Configuration
    ssl_certificate /etc/letsencrypt/live/room.sudachi.org/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/room.sudachi.org/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-SHA384;
    ssl_prefer_server_ciphers on;
    ssl_session_cache shared:SSL:10m;
    ssl_session_timeout 10m;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000; includeSubdomains; preload";
    add_header X-Frame-Options DENY;
    add_header X-Content-Type-Options nosniff;

    # Rate limiting
    limit_req_zone $binary_remote_addr zone=room_limit:10m rate=10r/s;
    limit_req zone=room_limit burst=20 nodelay;

    # WebSocket and HTTP proxy
    location / {
        proxy_pass http://room_server;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # WebSocket specific settings
        proxy_read_timeout 300s;
        proxy_send_timeout 300s;
        proxy_connect_timeout 75s;
        
        # Disable buffering for real-time communication
        proxy_buffering off;
        proxy_cache off;
    }

    # Health check endpoint
    location /health {
        proxy_pass http://room_server;
        access_log off;
    }

    # Security and monitoring
    location /nginx_status {
        stub_status on;
        allow 127.0.0.1;
        deny all;
    }
}
```

Enable the site:
```bash
sudo ln -s /etc/nginx/sites-available/sudachi-room /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### 5. SSL Certificate Setup

```bash
# Obtain SSL certificate
sudo certbot --nginx -d room.sudachi.org

# Set up automatic renewal
sudo crontab -e
# Add: 0 12 * * * /usr/bin/certbot renew --quiet
```

## Relay Server Setup

### 1. System Preparation

Follow the same system preparation steps as for the Room Server, but with these additional considerations:

```bash
# Install additional packages for high-performance networking
sudo apt install -y iperf3 netperf tcpdump

# Optimize network settings
sudo tee -a /etc/sysctl.conf << EOF

# Network optimizations for relay server
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 65536 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.core.netdev_max_backlog = 5000
net.ipv4.tcp_congestion_control = bbr
net.core.default_qdisc = fq
EOF

sudo sysctl -p
```

### 2. Relay Server Installation

#### Download and Build

```bash
# Create application directory
sudo mkdir -p /opt/sudachi-relay-server
cd /opt/sudachi-relay-server

# Clone relay server repository (C++)
# Note: Replace with actual repository URL when available
git clone https://github.com/sudachi-emulator/sudachi-relay-server.git .

# Install build dependencies
sudo apt install -y build-essential cmake libboost-all-dev libssl-dev

# Build the relay server
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install binary
sudo cp sudachi-relay-server /usr/local/bin/
```

#### Configuration

Create `/etc/sudachi/relay-server.json`:

```json
{
  "server": {
    "bind_address": "0.0.0.0",
    "port": 7777,
    "max_sessions": 1000,
    "max_connections_per_session": 8,
    "session_timeout": 300
  },
  "network": {
    "io_threads": 4,
    "buffer_size": 65536,
    "keepalive_interval": 30,
    "keepalive_timeout": 90,
    "tcp_nodelay": true
  },
  "bandwidth": {
    "max_bandwidth_per_session_mbps": 10,
    "global_bandwidth_limit_mbps": 1000,
    "bandwidth_window_seconds": 1,
    "bandwidth_burst_allowance": 2.0
  },
  "logging": {
    "level": "info",
    "file": "/var/log/sudachi/relay-server.log",
    "max_size": "100MB",
    "max_files": 5
  },
  "security": {
    "max_packet_size": 32768,
    "rate_limit_packets_per_second": 1000,
    "blacklist_threshold": 100,
    "blacklist_duration": 300
  },
  "monitoring": {
    "stats_port": 8080,
    "stats_bind": "127.0.0.1",
    "metrics_enabled": true
  }
}
```

#### Systemd Service

Create `/etc/systemd/system/sudachi-relay-server.service`:

```ini
[Unit]
Description=Sudachi Relay Server
After=network.target
Wants=network.target

[Service]
Type=simple
User=sudachi
Group=sudachi
ExecStart=/usr/local/bin/sudachi-relay-server --config /etc/sudachi/relay-server.json
Restart=always
RestartSec=5

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/log/sudachi

# Performance settings
LimitNOFILE=65536
LimitNPROC=4096
LimitMEMLOCK=infinity
IOSchedulingClass=1
IOSchedulingPriority=4

# Network optimizations
TasksMax=8192

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable sudachi-relay-server
sudo systemctl start sudachi-relay-server
```

### 3. Load Balancer for Relay Servers

For production deployments with multiple relay servers, use HAProxy:

Install HAProxy:
```bash
sudo apt install -y haproxy
```

Configure `/etc/haproxy/haproxy.cfg`:

```bash
global
    daemon
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    
    # SSL configuration
    ca-base /etc/ssl/certs
    crt-base /etc/ssl/private
    ssl-default-bind-ciphers ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS
    ssl-default-bind-options no-sslv3

defaults
    mode tcp
    timeout connect 5000ms
    timeout client 50000ms
    timeout server 50000ms
    option tcplog

# Stats interface
frontend stats
    bind *:8404
    mode http
    stats enable
    stats uri /stats
    stats refresh 30s
    stats admin if TRUE

# Relay server frontend
frontend relay_frontend
    bind *:7777
    mode tcp
    option tcplog
    default_backend relay_servers

# Relay server backend
backend relay_servers
    mode tcp
    balance leastconn
    option tcp-check
    
    server relay1 10.0.1.10:7777 check
    server relay2 10.0.1.11:7777 check
    server relay3 10.0.1.12:7777 check
```

## Monitoring and Logging

### 1. Prometheus Monitoring

Install Prometheus:
```bash
# Download and install Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.40.0/prometheus-2.40.0.linux-amd64.tar.gz
tar xzf prometheus-2.40.0.linux-amd64.tar.gz
sudo mv prometheus-2.40.0.linux-amd64 /opt/prometheus
sudo ln -s /opt/prometheus/prometheus /usr/local/bin/
```

Configure `/etc/prometheus/prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

rule_files:
  - "sudachi_multiplayer.rules.yml"

scrape_configs:
  - job_name: 'room-server'
    static_configs:
      - targets: ['localhost:3000']
    metrics_path: '/metrics'
    
  - job_name: 'relay-server'
    static_configs:
      - targets: ['localhost:8080']
    metrics_path: '/metrics'
    
  - job_name: 'nginx'
    static_configs:
      - targets: ['localhost:9113']
    
  - job_name: 'node-exporter'
    static_configs:
      - targets: ['localhost:9100']

alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - localhost:9093
```

### 2. Grafana Dashboard

Install Grafana:
```bash
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
sudo apt update && sudo apt install grafana
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
```

Import the Sudachi Multiplayer dashboard (JSON configuration would be provided separately).

### 3. Log Management

Configure log rotation for all services:

Create `/etc/logrotate.d/sudachi-multiplayer`:

```bash
/var/log/sudachi/*.log {
    daily
    missingok
    rotate 14
    compress
    delaycompress
    notifempty
    create 0644 sudachi sudachi
    postrotate
        systemctl reload sudachi-room-server
        systemctl reload sudachi-relay-server
    endscript
}
```

## Security Hardening

### 1. Firewall Configuration

```bash
# Configure iptables for production
sudo iptables -A INPUT -i lo -j ACCEPT
sudo iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 7777 -j ACCEPT
sudo iptables -A INPUT -j DROP
sudo iptables -A FORWARD -j DROP

# Save rules
sudo iptables-save > /etc/iptables/rules.v4
```

### 2. Fail2Ban Configuration

Install and configure Fail2Ban:

```bash
sudo apt install -y fail2ban

# Create custom filter for room server
sudo tee /etc/fail2ban/filter.d/sudachi-room.conf << EOF
[Definition]
failregex = ^.*sudachi-room.*Authentication failed for <HOST>.*$
            ^.*sudachi-room.*Rate limit exceeded for <HOST>.*$
ignoreregex =
EOF

# Configure jail
sudo tee /etc/fail2ban/jail.d/sudachi.conf << EOF
[sudachi-room]
enabled = true
port = http,https
filter = sudachi-room
logpath = /var/log/sudachi/room-server.log
maxretry = 5
bantime = 3600
findtime = 600
EOF
```

### 3. SSL/TLS Hardening

Test SSL configuration:
```bash
# Install SSL testing tools
sudo apt install -y sslscan testssl.sh

# Test SSL configuration
sslscan room.sudachi.org
testssl.sh https://room.sudachi.org
```

## Backup and Disaster Recovery

### 1. Database Backup

Create automated backup script `/opt/sudachi/backup-db.sh`:

```bash
#!/bin/bash
BACKUP_DIR="/backup/postgresql"
DATE=$(date +%Y%m%d_%H%M%S)
DB_NAME="sudachi_multiplayer"

mkdir -p $BACKUP_DIR

# Create database backup
pg_dump -h localhost -U sudachi $DB_NAME | gzip > $BACKUP_DIR/sudachi_db_$DATE.sql.gz

# Remove backups older than 7 days
find $BACKUP_DIR -name "sudachi_db_*.sql.gz" -mtime +7 -delete

# Upload to cloud storage (example for AWS S3)
aws s3 cp $BACKUP_DIR/sudachi_db_$DATE.sql.gz s3://sudachi-backups/database/
```

### 2. Configuration Backup

```bash
#!/bin/bash
BACKUP_DIR="/backup/config"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Backup configuration files
tar -czf $BACKUP_DIR/sudachi_config_$DATE.tar.gz \
    /etc/sudachi/ \
    /etc/nginx/sites-available/sudachi-* \
    /etc/systemd/system/sudachi-*

# Upload to cloud storage
aws s3 cp $BACKUP_DIR/sudachi_config_$DATE.tar.gz s3://sudachi-backups/config/
```

Add to crontab:
```bash
sudo crontab -e
# Add these lines:
0 2 * * * /opt/sudachi/backup-db.sh
0 3 * * * /opt/sudachi/backup-config.sh
```

## Performance Tuning

### 1. Database Optimization

PostgreSQL configuration (`/etc/postgresql/14/main/postgresql.conf`):

```bash
# Memory settings
shared_buffers = 2GB
effective_cache_size = 6GB
maintenance_work_mem = 512MB
work_mem = 16MB

# Connection settings
max_connections = 200
shared_preload_libraries = 'pg_stat_statements'

# Write-ahead logging
wal_buffers = 16MB
checkpoint_completion_target = 0.9
checkpoint_timeout = 15min

# Query planner
random_page_cost = 1.1
effective_io_concurrency = 200

# Monitoring
log_statement = 'mod'
log_duration = on
log_min_duration_statement = 1000
```

### 2. Redis Optimization

Redis configuration additions:

```bash
# Additional Redis optimizations
tcp-keepalive 60
tcp-backlog 511
timeout 0
databases 16

# Memory efficiency
hash-max-ziplist-entries 512
hash-max-ziplist-value 64
list-max-ziplist-size -2
set-max-intset-entries 512
zset-max-ziplist-entries 128
zset-max-ziplist-value 64
```

## Troubleshooting

### Common Issues

#### High Memory Usage

```bash
# Monitor memory usage
htop
free -h
sudo dmesg | grep -i memory

# Check for memory leaks in applications
valgrind --tool=memcheck --leak-check=full /usr/local/bin/sudachi-relay-server
```

#### Network Connectivity Issues

```bash
# Check port availability
netstat -tlnp | grep :3000
netstat -tlnp | grep :7777

# Test WebSocket connectivity
wscat -c wss://room.sudachi.org

# Check relay server connectivity
telnet relay.sudachi.org 7777
```

#### Database Performance Issues

```bash
# Monitor database performance
sudo -u postgres psql -c "SELECT * FROM pg_stat_activity;"
sudo -u postgres psql -c "SELECT * FROM pg_stat_statements ORDER BY total_time DESC LIMIT 10;"

# Check slow queries
sudo tail -f /var/log/postgresql/postgresql-14-main.log | grep "duration:"
```

## Scaling Considerations

### Horizontal Scaling

For high-traffic deployments:

1. **Multiple Room Servers**: Deploy room servers in multiple regions
2. **Database Clustering**: Use PostgreSQL streaming replication
3. **Redis Clustering**: Set up Redis Cluster for session data
4. **CDN Integration**: Use CloudFlare or AWS CloudFront
5. **Container Orchestration**: Consider Kubernetes deployment

### Monitoring Scaling Metrics

Key metrics to monitor:
- **WebSocket connections per room server**
- **Active sessions per relay server**
- **Database connection pool usage**
- **Redis memory usage**
- **Network bandwidth utilization**
- **Response times and error rates**

---

*This server setup guide is maintained by the Sudachi infrastructure team. For support with server deployment, join our [Discord community](https://discord.gg/sudachi) or contact the infrastructure team.*
