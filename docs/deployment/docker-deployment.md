# Docker Deployment Guide for Sudachi Multiplayer

## Overview

This guide provides comprehensive instructions for deploying the Sudachi Multiplayer server infrastructure using Docker and Docker Compose. This approach offers consistent, reproducible deployments across development, staging, and production environments.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Docker Deployment                        │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   nginx     │  │ room-server │  │   relay-server      │ │
│  │             │  │             │  │                     │ │
│  │ - SSL Term  │──│ - WebSocket │──│ - P2P Fallback      │ │
│  │ - Load Bal  │  │ - Session   │  │ - Bandwidth Ctrl    │ │
│  │ - Rate Lim  │  │   Mgmt      │  │                     │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │ postgresql  │  │    redis    │  │    monitoring       │ │
│  │             │  │             │  │                     │ │
│  │ - Session   │  │ - Cache     │  │ - Prometheus        │ │
│  │   Data      │  │ - Real-time │  │ - Grafana           │ │
│  │ - Analytics │  │   State     │  │ - AlertManager      │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Prerequisites

### System Requirements

#### Development Environment
- **Docker**: 20.10 or later
- **Docker Compose**: 2.0 or later
- **RAM**: 8GB minimum, 16GB recommended
- **Storage**: 50GB available space
- **CPU**: 4 cores minimum

#### Production Environment
- **Docker**: 20.10 or later
- **Docker Compose**: 2.0 or later
- **RAM**: 32GB minimum, 64GB recommended
- **Storage**: 500GB SSD (NVMe preferred)
- **CPU**: 16 cores minimum
- **Network**: 10 Gbps connection

### Installation

#### Ubuntu/Debian
```bash
# Update system
sudo apt update

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Install Docker Compose
sudo apt install docker-compose-plugin

# Add user to docker group
sudo usermod -aG docker $USER
newgrp docker

# Verify installation
docker --version
docker compose version
```

#### CentOS/RHEL
```bash
# Install Docker
sudo dnf config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
sudo dnf install docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Start and enable Docker
sudo systemctl start docker
sudo systemctl enable docker

# Add user to docker group
sudo usermod -aG docker $USER
```

## Project Structure

Create the deployment directory structure:

```
sudachi-multiplayer-docker/
├── docker-compose.yml
├── docker-compose.override.yml     # Development overrides
├── docker-compose.prod.yml         # Production overrides
├── .env                           # Environment variables
├── .env.example                   # Example environment file
├── nginx/
│   ├── Dockerfile
│   ├── nginx.conf
│   ├── conf.d/
│   └── ssl/                       # SSL certificates
├── room-server/
│   ├── Dockerfile
│   ├── package.json
│   ├── server.js
│   └── config/
├── relay-server/
│   ├── Dockerfile
│   ├── CMakeLists.txt
│   └── src/
├── monitoring/
│   ├── prometheus/
│   │   ├── prometheus.yml
│   │   └── rules/
│   ├── grafana/
│   │   ├── dashboards/
│   │   └── provisioning/
│   └── alertmanager/
│       └── alertmanager.yml
└── scripts/
    ├── deploy.sh
    ├── backup.sh
    └── health-check.sh
```

## Environment Configuration

### Environment Variables

Create `.env` file:

```bash
# Environment
COMPOSE_PROJECT_NAME=sudachi-multiplayer
ENVIRONMENT=production

# Database Configuration
POSTGRES_DB=sudachi_multiplayer
POSTGRES_USER=sudachi
POSTGRES_PASSWORD=your_secure_db_password
POSTGRES_HOST=postgresql
POSTGRES_PORT=5432

# Redis Configuration
REDIS_PASSWORD=your_secure_redis_password
REDIS_HOST=redis
REDIS_PORT=6379

# Room Server Configuration
ROOM_SERVER_PORT=3000
ROOM_SERVER_HOST=0.0.0.0
ROOM_SERVER_MAX_CONNECTIONS=10000
ROOM_SERVER_SESSION_TIMEOUT=1800

# Relay Server Configuration
RELAY_SERVER_PORT=7777
RELAY_SERVER_HOST=0.0.0.0
RELAY_SERVER_MAX_SESSIONS=1000
RELAY_SERVER_BANDWIDTH_LIMIT=1000

# SSL Configuration
SSL_CERT_PATH=/etc/ssl/certs
SSL_KEY_PATH=/etc/ssl/private
DOMAIN_NAME=sudachi.org

# Monitoring
PROMETHEUS_PORT=9090
GRAFANA_PORT=3001
GRAFANA_ADMIN_PASSWORD=your_secure_grafana_password

# Security
JWT_SECRET=your_jwt_secret_key
ENCRYPTION_KEY=your_encryption_key

# Logging
LOG_LEVEL=info
LOG_MAX_SIZE=100m
LOG_MAX_FILES=10
```

Create `.env.example` as a template for new deployments.

## Docker Compose Configuration

### Main Docker Compose File

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  # Reverse Proxy and Load Balancer
  nginx:
    build:
      context: ./nginx
      dockerfile: Dockerfile
    container_name: sudachi-nginx
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx/conf.d:/etc/nginx/conf.d:ro
      - ./nginx/ssl:/etc/ssl:ro
      - nginx_logs:/var/log/nginx
    depends_on:
      - room-server
    networks:
      - sudachi-network
    restart: unless-stopped
    environment:
      - DOMAIN_NAME=${DOMAIN_NAME}
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost/health"]
      interval: 30s
      timeout: 10s
      retries: 3

  # Room Server (WebSocket-based session discovery)
  room-server:
    build:
      context: ./room-server
      dockerfile: Dockerfile
    container_name: sudachi-room-server
    ports:
      - "${ROOM_SERVER_PORT}:${ROOM_SERVER_PORT}"
    volumes:
      - room_logs:/var/log/sudachi
    depends_on:
      - postgresql
      - redis
    networks:
      - sudachi-network
    restart: unless-stopped
    environment:
      - NODE_ENV=${ENVIRONMENT}
      - PORT=${ROOM_SERVER_PORT}
      - HOST=${ROOM_SERVER_HOST}
      - POSTGRES_HOST=${POSTGRES_HOST}
      - POSTGRES_PORT=${POSTGRES_PORT}
      - POSTGRES_DB=${POSTGRES_DB}
      - POSTGRES_USER=${POSTGRES_USER}
      - POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
      - REDIS_HOST=${REDIS_HOST}
      - REDIS_PORT=${REDIS_PORT}
      - REDIS_PASSWORD=${REDIS_PASSWORD}
      - MAX_CONNECTIONS=${ROOM_SERVER_MAX_CONNECTIONS}
      - SESSION_TIMEOUT=${ROOM_SERVER_SESSION_TIMEOUT}
      - LOG_LEVEL=${LOG_LEVEL}
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:${ROOM_SERVER_PORT}/health"]
      interval: 30s
      timeout: 10s
      retries: 3
    deploy:
      resources:
        limits:
          memory: 2G
          cpus: '2.0'
        reservations:
          memory: 1G
          cpus: '1.0'

  # Relay Server (P2P fallback)
  relay-server:
    build:
      context: ./relay-server
      dockerfile: Dockerfile
    container_name: sudachi-relay-server
    ports:
      - "${RELAY_SERVER_PORT}:${RELAY_SERVER_PORT}"
    volumes:
      - relay_logs:/var/log/sudachi
    networks:
      - sudachi-network
    restart: unless-stopped
    environment:
      - BIND_ADDRESS=${RELAY_SERVER_HOST}
      - PORT=${RELAY_SERVER_PORT}
      - MAX_SESSIONS=${RELAY_SERVER_MAX_SESSIONS}
      - BANDWIDTH_LIMIT=${RELAY_SERVER_BANDWIDTH_LIMIT}
      - LOG_LEVEL=${LOG_LEVEL}
    healthcheck:
      test: ["CMD", "nc", "-z", "localhost", "${RELAY_SERVER_PORT}"]
      interval: 30s
      timeout: 10s
      retries: 3
    deploy:
      resources:
        limits:
          memory: 4G
          cpus: '4.0'
        reservations:
          memory: 2G
          cpus: '2.0'

  # PostgreSQL Database
  postgresql:
    image: postgres:15-alpine
    container_name: sudachi-postgresql
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./database/init:/docker-entrypoint-initdb.d:ro
      - postgres_logs:/var/log/postgresql
    networks:
      - sudachi-network
    restart: unless-stopped
    environment:
      - POSTGRES_DB=${POSTGRES_DB}
      - POSTGRES_USER=${POSTGRES_USER}
      - POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
      - POSTGRES_INITDB_ARGS=--encoding=UTF-8 --lc-collate=C --lc-ctype=C
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U ${POSTGRES_USER} -d ${POSTGRES_DB}"]
      interval: 30s
      timeout: 10s
      retries: 3
    deploy:
      resources:
        limits:
          memory: 4G
          cpus: '2.0'
        reservations:
          memory: 2G
          cpus: '1.0'

  # Redis Cache
  redis:
    image: redis:7-alpine
    container_name: sudachi-redis
    ports:
      - "6379:6379"
    volumes:
      - redis_data:/data
      - ./redis/redis.conf:/usr/local/etc/redis/redis.conf:ro
    networks:
      - sudachi-network
    restart: unless-stopped
    command: redis-server /usr/local/etc/redis/redis.conf --requirepass ${REDIS_PASSWORD}
    healthcheck:
      test: ["CMD", "redis-cli", "--raw", "incr", "ping"]
      interval: 30s
      timeout: 10s
      retries: 3
    deploy:
      resources:
        limits:
          memory: 2G
          cpus: '1.0'
        reservations:
          memory: 1G
          cpus: '0.5'

  # Prometheus Monitoring
  prometheus:
    image: prom/prometheus:latest
    container_name: sudachi-prometheus
    ports:
      - "${PROMETHEUS_PORT}:9090"
    volumes:
      - ./monitoring/prometheus/prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - ./monitoring/prometheus/rules:/etc/prometheus/rules:ro
      - prometheus_data:/prometheus
    networks:
      - sudachi-network
    restart: unless-stopped
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
      - '--web.console.libraries=/etc/prometheus/console_libraries'
      - '--web.console.templates=/etc/prometheus/consoles'
      - '--storage.tsdb.retention.time=30d'
      - '--web.enable-lifecycle'
    healthcheck:
      test: ["CMD", "wget", "--quiet", "--tries=1", "--spider", "http://localhost:9090/-/healthy"]
      interval: 30s
      timeout: 10s
      retries: 3

  # Grafana Dashboard
  grafana:
    image: grafana/grafana:latest
    container_name: sudachi-grafana
    ports:
      - "${GRAFANA_PORT}:3000"
    volumes:
      - grafana_data:/var/lib/grafana
      - ./monitoring/grafana/dashboards:/etc/grafana/provisioning/dashboards:ro
      - ./monitoring/grafana/datasources:/etc/grafana/provisioning/datasources:ro
    networks:
      - sudachi-network
    restart: unless-stopped
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=${GRAFANA_ADMIN_PASSWORD}
      - GF_USERS_ALLOW_SIGN_UP=false
      - GF_INSTALL_PLUGINS=grafana-piechart-panel
    depends_on:
      - prometheus
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:3000/api/health"]
      interval: 30s
      timeout: 10s
      retries: 3

# Networks
networks:
  sudachi-network:
    driver: bridge
    ipam:
      config:
        - subnet: 172.20.0.0/16

# Volumes
volumes:
  postgres_data:
    driver: local
  redis_data:
    driver: local
  prometheus_data:
    driver: local
  grafana_data:
    driver: local
  nginx_logs:
    driver: local
  room_logs:
    driver: local
  relay_logs:
    driver: local
  postgres_logs:
    driver: local
```

### Development Override

Create `docker-compose.override.yml` for development:

```yaml
version: '3.8'

services:
  room-server:
    build:
      target: development
    volumes:
      - ./room-server:/app
      - /app/node_modules
    environment:
      - NODE_ENV=development
      - DEBUG=sudachi:*
    ports:
      - "3000:3000"
      - "9229:9229"  # Node.js debugging port

  relay-server:
    build:
      target: development
    volumes:
      - ./relay-server:/app
    environment:
      - LOG_LEVEL=debug

  nginx:
    ports:
      - "8080:80"  # Use different port for development

  postgresql:
    environment:
      - POSTGRES_LOG_STATEMENT=all
    volumes:
      - ./database/dev-data:/docker-entrypoint-initdb.d

  redis:
    command: redis-server --requirepass ${REDIS_PASSWORD} --loglevel debug
```

### Production Override

Create `docker-compose.prod.yml` for production:

```yaml
version: '3.8'

services:
  nginx:
    deploy:
      replicas: 2
      update_config:
        parallelism: 1
        delay: 10s
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3

  room-server:
    deploy:
      replicas: 3
      update_config:
        parallelism: 1
        delay: 10s
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3
    logging:
      driver: "json-file"
      options:
        max-size: "100m"
        max-file: "5"

  relay-server:
    deploy:
      replicas: 2
      update_config:
        parallelism: 1
        delay: 10s
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3
    logging:
      driver: "json-file"
      options:
        max-size: "100m"
        max-file: "5"

  postgresql:
    volumes:
      - /opt/sudachi/postgres-data:/var/lib/postgresql/data
    deploy:
      resources:
        limits:
          memory: 8G
          cpus: '4.0'

  redis:
    volumes:
      - /opt/sudachi/redis-data:/data
    deploy:
      resources:
        limits:
          memory: 4G
          cpus: '2.0'
```

## Dockerfile Configurations

### NGINX Dockerfile

Create `nginx/Dockerfile`:

```dockerfile
FROM nginx:1.24-alpine

# Install additional packages
RUN apk add --no-cache \
    curl \
    openssl \
    certbot \
    certbot-nginx

# Copy configuration files
COPY nginx.conf /etc/nginx/nginx.conf
COPY conf.d/ /etc/nginx/conf.d/

# Create directories for logs and certificates
RUN mkdir -p /var/log/nginx /etc/ssl/certs /etc/ssl/private

# Set permissions
RUN chown -R nginx:nginx /var/log/nginx

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
    CMD curl -f http://localhost/health || exit 1

EXPOSE 80 443

CMD ["nginx", "-g", "daemon off;"]
```

### Room Server Dockerfile

Create `room-server/Dockerfile`:

```dockerfile
# Multi-stage build for production optimization
FROM node:18-alpine AS base

WORKDIR /app

# Install system dependencies
RUN apk add --no-cache \
    dumb-init \
    curl

# Copy package files
COPY package*.json ./

# Development stage
FROM base AS development
RUN npm ci --include=dev
COPY . .
EXPOSE 3000 9229
CMD ["dumb-init", "npm", "run", "dev"]

# Production dependencies stage
FROM base AS production-deps
RUN npm ci --only=production && npm cache clean --force

# Production stage
FROM base AS production
COPY --from=production-deps /app/node_modules ./node_modules
COPY . .

# Create non-root user
RUN addgroup -g 1001 -S nodejs && \
    adduser -S sudachi -u 1001

# Set ownership
RUN chown -R sudachi:nodejs /app
USER sudachi

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
    CMD curl -f http://localhost:3000/health || exit 1

EXPOSE 3000

CMD ["dumb-init", "node", "server.js"]
```

### Relay Server Dockerfile

Create `relay-server/Dockerfile`:

```dockerfile
# Multi-stage build for C++ application
FROM alpine:3.18 AS builder

# Install build dependencies
RUN apk add --no-cache \
    build-base \
    cmake \
    boost-dev \
    openssl-dev \
    git

WORKDIR /app

# Copy source code
COPY . .

# Build application
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Production stage
FROM alpine:3.18 AS production

# Install runtime dependencies
RUN apk add --no-cache \
    boost-system \
    boost-filesystem \
    boost-thread \
    openssl \
    libstdc++

# Create non-root user
RUN addgroup -g 1001 -S sudachi && \
    adduser -S sudachi -u 1001 -G sudachi

# Copy binary from builder stage
COPY --from=builder /app/build/sudachi-relay-server /usr/local/bin/
COPY --from=builder /app/config/ /etc/sudachi/

# Set ownership
RUN chown -R sudachi:sudachi /etc/sudachi
USER sudachi

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
    CMD nc -z localhost 7777 || exit 1

EXPOSE 7777 8080

CMD ["/usr/local/bin/sudachi-relay-server", "--config", "/etc/sudachi/relay-server.json"]
```

## Configuration Files

### NGINX Configuration

Create `nginx/nginx.conf`:

```nginx
user nginx;
worker_processes auto;
error_log /var/log/nginx/error.log warn;
pid /var/run/nginx.pid;

events {
    worker_connections 4096;
    use epoll;
    multi_accept on;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    # Logging
    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent" "$http_x_forwarded_for"';
    access_log /var/log/nginx/access.log main;

    # Performance settings
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;
    types_hash_max_size 2048;
    client_max_body_size 1m;

    # Gzip compression
    gzip on;
    gzip_vary on;
    gzip_proxied any;
    gzip_comp_level 6;
    gzip_types
        text/plain
        text/css
        text/xml
        text/javascript
        application/json
        application/javascript
        application/xml+rss
        application/atom+xml
        image/svg+xml;

    # Rate limiting
    limit_req_zone $binary_remote_addr zone=api:10m rate=10r/s;
    limit_req_zone $binary_remote_addr zone=websocket:10m rate=5r/s;

    # Include server configurations
    include /etc/nginx/conf.d/*.conf;
}
```

Create `nginx/conf.d/sudachi.conf`:

```nginx
upstream room_servers {
    least_conn;
    server room-server:3000;
    keepalive 32;
}

upstream relay_servers {
    least_conn;
    server relay-server:7777;
}

# HTTP to HTTPS redirect
server {
    listen 80;
    server_name _;
    return 301 https://$host$request_uri;
}

# Main HTTPS server
server {
    listen 443 ssl http2;
    server_name room.sudachi.org;

    # SSL configuration
    ssl_certificate /etc/ssl/certs/sudachi.org.crt;
    ssl_certificate_key /etc/ssl/private/sudachi.org.key;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384;
    ssl_prefer_server_ciphers off;
    ssl_session_cache shared:SSL:10m;
    ssl_session_timeout 10m;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000; includeSubdomains; preload" always;
    add_header X-Frame-Options "DENY" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header Referrer-Policy "strict-origin-when-cross-origin" always;

    # WebSocket and API routes
    location / {
        limit_req zone=websocket burst=10 nodelay;
        
        proxy_pass http://room_servers;
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

    # Health check
    location /health {
        proxy_pass http://room_servers;
        access_log off;
    }

    # Metrics endpoint (restricted)
    location /metrics {
        allow 172.20.0.0/16;  # Docker network
        deny all;
        proxy_pass http://room_servers;
    }
}

# Relay server (TCP proxy)
stream {
    upstream relay_backend {
        server relay-server:7777;
    }
    
    server {
        listen 7777;
        proxy_pass relay_backend;
        proxy_timeout 300s;
        proxy_responses 1;
    }
}
```

### Database Initialization

Create `database/init/01-init.sql`:

```sql
-- Create database extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pg_stat_statements";

-- Create tables
CREATE TABLE IF NOT EXISTS game_sessions (
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

CREATE TABLE IF NOT EXISTS session_players (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) REFERENCES game_sessions(session_id),
    player_id INTEGER NOT NULL,
    player_name VARCHAR(64) NOT NULL,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_ready BOOLEAN DEFAULT FALSE
);

-- Create indexes
CREATE INDEX IF NOT EXISTS idx_sessions_game_id ON game_sessions(game_id);
CREATE INDEX IF NOT EXISTS idx_sessions_region ON game_sessions(region);
CREATE INDEX IF NOT EXISTS idx_sessions_created ON game_sessions(created_at);
CREATE INDEX IF NOT EXISTS idx_players_session ON session_players(session_id);

-- Create update trigger
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER IF NOT EXISTS update_sessions_updated_at 
    BEFORE UPDATE ON game_sessions 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
```

### Redis Configuration

Create `redis/redis.conf`:

```bash
# Network
bind 0.0.0.0
port 6379
protected-mode yes
tcp-backlog 511
timeout 0
tcp-keepalive 300

# General
daemonize no
supervised no
pidfile /var/run/redis_6379.pid
loglevel notice
logfile ""
databases 16

# Memory management
maxmemory 1gb
maxmemory-policy allkeys-lru
maxmemory-samples 5

# Persistence
save 900 1
save 300 10
save 60 10000
stop-writes-on-bgsave-error yes
rdbcompression yes
rdbchecksum yes
dbfilename dump.rdb
dir /data

# Replication
replica-serve-stale-data yes
replica-read-only yes
repl-diskless-sync no
repl-diskless-sync-delay 5

# Security
rename-command FLUSHDB ""
rename-command FLUSHALL ""
rename-command DEBUG ""
rename-command CONFIG "CONFIG_9a2f8b3e1d4c6f7a8b9c0d1e2f3a4b5c"

# Clients
maxclients 10000

# Slow log
slowlog-log-slower-than 10000
slowlog-max-len 128

# Latency monitoring
latency-monitor-threshold 100
```

## Deployment Scripts

### Deployment Script

Create `scripts/deploy.sh`:

```bash
#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
ENVIRONMENT=${1:-production}
BACKUP_BEFORE_DEPLOY=${BACKUP_BEFORE_DEPLOY:-true}
MAX_DEPLOY_TIME=600  # 10 minutes

echo -e "${GREEN}Starting Sudachi Multiplayer deployment...${NC}"
echo -e "Environment: ${YELLOW}$ENVIRONMENT${NC}"

# Function to print colored output
print_status() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] WARNING: $1${NC}"
}

print_error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $1${NC}"
}

# Check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed"
        exit 1
    fi
    
    if ! command -v docker compose &> /dev/null; then
        print_error "Docker Compose is not installed"
        exit 1
    fi
    
    if [ ! -f ".env" ]; then
        print_error ".env file not found"
        exit 1
    fi
    
    print_status "Prerequisites check passed"
}

# Backup function
backup_data() {
    if [ "$BACKUP_BEFORE_DEPLOY" = "true" ]; then
        print_status "Creating backup before deployment..."
        ./scripts/backup.sh
    fi
}

# Health check function
health_check() {
    local service=$1
    local max_attempts=${2:-30}
    local attempt=1
    
    print_status "Checking health of $service..."
    
    while [ $attempt -le $max_attempts ]; do
        if docker compose ps $service | grep -q "healthy\|Up"; then
            print_status "$service is healthy"
            return 0
        fi
        
        print_warning "$service health check attempt $attempt/$max_attempts failed"
        sleep 10
        ((attempt++))
    done
    
    print_error "$service failed health check"
    return 1
}

# Deployment function
deploy() {
    local compose_files="-f docker-compose.yml"
    
    if [ "$ENVIRONMENT" = "development" ]; then
        compose_files="$compose_files -f docker-compose.override.yml"
    elif [ "$ENVIRONMENT" = "production" ]; then
        compose_files="$compose_files -f docker-compose.prod.yml"
    fi
    
    print_status "Building images..."
    docker compose $compose_files build --parallel
    
    print_status "Starting services..."
    docker compose $compose_files up -d
    
    # Wait for services to be healthy
    print_status "Waiting for services to be healthy..."
    
    services=("postgresql" "redis" "room-server" "relay-server" "nginx")
    for service in "${services[@]}"; do
        if ! health_check $service; then
            print_error "Deployment failed - $service is not healthy"
            exit 1
        fi
    done
    
    print_status "All services are healthy"
}

# Cleanup function
cleanup_old_images() {
    print_status "Cleaning up old Docker images..."
    docker image prune -f
    docker volume prune -f
}

# Main deployment flow
main() {
    check_prerequisites
    backup_data
    deploy
    cleanup_old_images
    
    print_status "Deployment completed successfully!"
    echo -e "${GREEN}Sudachi Multiplayer is now running on $ENVIRONMENT environment${NC}"
    
    # Show service status
    docker compose ps
}

# Error handling
trap 'print_error "Deployment failed!"; exit 1' ERR

# Run main function
main "$@"
```

### Backup Script

Create `scripts/backup.sh`:

```bash
#!/bin/bash

set -e

BACKUP_DIR="${BACKUP_DIR:-/opt/sudachi/backups}"
DATE=$(date +%Y%m%d_%H%M%S)
RETENTION_DAYS=${RETENTION_DAYS:-7}

# Create backup directory
mkdir -p "$BACKUP_DIR"

echo "Starting backup process..."

# Backup PostgreSQL
echo "Backing up PostgreSQL..."
docker compose exec -T postgresql pg_dump -U sudachi sudachi_multiplayer | gzip > "$BACKUP_DIR/postgres_$DATE.sql.gz"

# Backup Redis
echo "Backing up Redis..."
docker compose exec -T redis redis-cli --rdb /tmp/dump.rdb
docker cp "$(docker compose ps -q redis):/tmp/dump.rdb" "$BACKUP_DIR/redis_$DATE.rdb"

# Backup configuration
echo "Backing up configuration..."
tar -czf "$BACKUP_DIR/config_$DATE.tar.gz" \
    .env \
    docker-compose*.yml \
    nginx/ \
    monitoring/ \
    --exclude='*.log'

# Cleanup old backups
echo "Cleaning up old backups..."
find "$BACKUP_DIR" -name "*.gz" -o -name "*.rdb" -o -name "*.tar.gz" | \
    while read file; do
        if [[ $(find "$file" -mtime +$RETENTION_DAYS) ]]; then
            rm -f "$file"
            echo "Removed old backup: $file"
        fi
    done

echo "Backup completed successfully!"
ls -la "$BACKUP_DIR"
```

### Health Check Script

Create `scripts/health-check.sh`:

```bash
#!/bin/bash

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check service health
check_service() {
    local service=$1
    local url=$2
    
    echo -n "Checking $service... "
    
    if curl -f -s "$url" > /dev/null 2>&1; then
        echo -e "${GREEN}OK${NC}"
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        return 1
    fi
}

# Check WebSocket connection
check_websocket() {
    echo -n "Checking WebSocket... "
    
    if command -v wscat &> /dev/null; then
        if timeout 10 wscat -c "wss://room.sudachi.org" -x '{"type":"ping"}' 2>/dev/null; then
            echo -e "${GREEN}OK${NC}"
            return 0
        fi
    fi
    
    echo -e "${YELLOW}SKIP (wscat not available)${NC}"
    return 0
}

# Check relay server
check_relay() {
    echo -n "Checking Relay Server... "
    
    if nc -z relay.sudachi.org 7777 2>/dev/null; then
        echo -e "${GREEN}OK${NC}"
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        return 1
    fi
}

# Main health check
main() {
    echo "Sudachi Multiplayer Health Check"
    echo "================================="
    
    local failed=0
    
    # Check HTTP endpoints
    check_service "Room Server" "https://room.sudachi.org/health" || ((failed++))
    check_service "Prometheus" "http://localhost:9090/-/healthy" || ((failed++))
    check_service "Grafana" "http://localhost:3001/api/health" || ((failed++))
    
    # Check WebSocket
    check_websocket || ((failed++))
    
    # Check Relay Server
    check_relay || ((failed++))
    
    echo "================================="
    
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}All services are healthy!${NC}"
        exit 0
    else
        echo -e "${RED}$failed service(s) failed health check${NC}"
        exit 1
    fi
}

main "$@"
```

## Monitoring Configuration

### Prometheus Configuration

Create `monitoring/prometheus/prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

rule_files:
  - "rules/*.yml"

scrape_configs:
  - job_name: 'room-server'
    static_configs:
      - targets: ['room-server:3000']
    metrics_path: '/metrics'
    
  - job_name: 'relay-server'
    static_configs:
      - targets: ['relay-server:8080']
    metrics_path: '/metrics'
    
  - job_name: 'nginx'
    static_configs:
      - targets: ['nginx:9113']
    
  - job_name: 'postgres'
    static_configs:
      - targets: ['postgresql:9187']
      
  - job_name: 'redis'
    static_configs:
      - targets: ['redis:9121']

alerting:
  alertmanagers:
    - static_configs:
        - targets:
          - alertmanager:9093
```

### Grafana Provisioning

Create `monitoring/grafana/datasources/prometheus.yml`:

```yaml
apiVersion: 1

datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: true
    editable: true
```

## Operations Guide

### Starting the Stack

```bash
# Development environment
docker compose up -d

# Production environment
docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d

# View logs
docker compose logs -f

# Check service status
docker compose ps
```

### Scaling Services

```bash
# Scale room servers
docker compose up -d --scale room-server=3

# Scale relay servers
docker compose up -d --scale relay-server=2
```

### Updating Services

```bash
# Update specific service
docker compose pull room-server
docker compose up -d --no-deps room-server

# Update all services
docker compose pull
docker compose up -d
```

### Troubleshooting

```bash
# Check service logs
docker compose logs room-server
docker compose logs relay-server

# Execute commands in containers
docker compose exec room-server bash
docker compose exec postgresql psql -U sudachi -d sudachi_multiplayer

# Monitor resource usage
docker stats

# Check container health
docker compose ps
```

### Maintenance Tasks

```bash
# Backup data
./scripts/backup.sh

# Clean up unused resources
docker system prune -a

# Update SSL certificates
docker compose exec nginx certbot renew

# Database maintenance
docker compose exec postgresql vacuumdb -U sudachi -d sudachi_multiplayer
```

## Security Considerations

### Container Security

1. **Use non-root users** in all containers
2. **Limit container capabilities** and resources
3. **Scan images** for vulnerabilities regularly
4. **Keep base images updated**
5. **Use secrets management** for sensitive data

### Network Security

1. **Use custom networks** to isolate services
2. **Limit exposed ports** to only what's necessary
3. **Implement proper firewall rules**
4. **Use SSL/TLS** for all external communications
5. **Regular security audits**

### Data Security

1. **Encrypt data at rest** (database encryption)
2. **Secure backup storage**
3. **Regular backup testing**
4. **Access control** for sensitive endpoints
5. **Audit logging**

---

*This Docker deployment guide is maintained by the Sudachi infrastructure team. For support with containerized deployments, join our [Discord community](https://discord.gg/sudachi) or check our [GitHub repository](https://github.com/sudachi-emulator/sudachi).*