# Sudachi Multiplayer Security Audit Log

## Overview

This document describes the security audit logging system for Sudachi Multiplayer, including log formats, retention policies, analysis procedures, and compliance requirements. The audit log system provides comprehensive visibility into security-relevant events across all system components.

## Audit Logging Architecture

### Centralized Logging System

```
┌─────────────────────────────────────────────────────────────┐
│                    Audit Log Architecture                   │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │ Application │  │   System    │  │    Network          │ │
│  │    Logs     │  │    Logs     │  │     Logs            │ │
│  │             │  │             │  │                     │ │
│  │ - API calls │  │ - Auth      │  │ - Firewall          │ │
│  │ - DB access │  │ - Process   │  │ - IDS/IPS           │ │
│  │ - User acts │  │ - File I/O  │  │ - Traffic analysis  │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                                 │                           │
│            ┌────────────────────▼─────────────────────┐    │
│            │         Log Aggregation Layer           │    │
│            │                                         │    │
│            │  ┌─────────────┐  ┌─────────────────┐   │    │
│            │  │   Logstash  │  │   Fluentd      │   │    │
│            │  │             │  │                │   │    │
│            │  │ - Filtering │  │ - Routing      │   │    │
│            │  │ - Parsing   │  │ - Buffering    │   │    │
│            │  │ - Enrichment│  │ - Formatting   │   │    │
│            │  └─────────────┘  └─────────────────┘   │    │
│            └─────────────────────────────────────────┘    │
│                                 │                           │
│            ┌────────────────────▼─────────────────────┐    │
│            │           Storage Layer                 │    │
│            │                                         │    │
│            │  ┌─────────────┐  ┌─────────────────┐   │    │
│            │  │Elasticsearch│  │     WORM        │   │    │
│            │  │             │  │   Storage       │   │    │
│            │  │ - Searchable│  │                │   │    │
│            │  │ - Real-time │  │ - Immutable    │   │    │
│            │  │ - Analytics │  │ - Compliance   │   │    │
│            │  └─────────────┘  └─────────────────┘   │    │
│            └─────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### Log Sources and Categories

#### Application-Level Audit Events

**Authentication Events**:
```cpp
class AuthenticationAuditor {
public:
    enum class AuthEvent {
        LoginAttempt,
        LoginSuccess,
        LoginFailure,
        Logout,
        SessionExpired,
        PasswordChange,
        AccountLocked,
        MFAEnabled,
        MFADisabled
    };
    
    void LogAuthEvent(AuthEvent event, const AuthContext& context) {
        AuditRecord record;
        record.timestamp = std::chrono::system_clock::now();
        record.event_type = "authentication";
        record.event_subtype = AuthEventToString(event);
        record.user_id = context.user_id;
        record.source_ip = context.source_ip;
        record.user_agent = context.user_agent;
        record.session_id = context.session_id;
        
        // Add event-specific data
        switch (event) {
            case AuthEvent::LoginFailure:
                record.details["failure_reason"] = context.failure_reason;
                record.details["attempt_count"] = std::to_string(context.attempt_count);
                break;
                
            case AuthEvent::AccountLocked:
                record.details["lock_duration"] = std::to_string(context.lock_duration.count());
                record.details["trigger_event"] = context.trigger_event;
                break;
                
            default:
                break;
        }
        
        // Set severity based on event type
        record.severity = GetEventSeverity(event);
        
        // Send to audit log system
        AuditLogger::Instance().WriteRecord(record);
    }
    
private:
    Severity GetEventSeverity(AuthEvent event) {
        switch (event) {
            case AuthEvent::LoginFailure:
            case AuthEvent::AccountLocked:
                return Severity::Warning;
                
            case AuthEvent::LoginSuccess:
            case AuthEvent::Logout:
                return Severity::Info;
                
            case AuthEvent::PasswordChange:
            case AuthEvent::MFAEnabled:
            case AuthEvent::MFADisabled:
                return Severity::Notice;
                
            default:
                return Severity::Info;
        }
    }
};
```

**Authorization Events**:
```cpp
class AuthorizationAuditor {
public:
    enum class AuthzEvent {
        AccessGranted,
        AccessDenied,
        PrivilegeEscalation,
        RoleChange,
        PermissionChange,
        ResourceAccess,
        AdminAction
    };
    
    void LogAuthzEvent(AuthzEvent event, const AuthzContext& context) {
        AuditRecord record;
        record.timestamp = std::chrono::system_clock::now();
        record.event_type = "authorization";
        record.event_subtype = AuthzEventToString(event);
        record.user_id = context.user_id;
        record.resource = context.resource;
        record.action = context.action;
        record.result = context.result;
        
        // Add context-specific information
        record.details["requested_permission"] = context.requested_permission;
        record.details["user_role"] = context.user_role;
        record.details["resource_owner"] = context.resource_owner;
        
        if (event == AuthzEvent::AccessDenied) {
            record.details["denial_reason"] = context.denial_reason;
            record.severity = Severity::Warning;
        } else {
            record.severity = Severity::Info;
        }
        
        AuditLogger::Instance().WriteRecord(record);
    }
};
```

**Data Access Events**:
```cpp
class DataAccessAuditor {
public:
    enum class DataEvent {
        Read,
        Write,
        Delete,
        Export,
        Import,
        Backup,
        Restore,
        Encryption,
        Decryption
    };
    
    void LogDataAccess(DataEvent event, const DataAccessContext& context) {
        AuditRecord record;
        record.timestamp = std::chrono::system_clock::now();
        record.event_type = "data_access";
        record.event_subtype = DataEventToString(event);
        record.user_id = context.user_id;
        record.resource = context.resource;
        
        // Classify data sensitivity
        auto sensitivity = ClassifyDataSensitivity(context.resource);
        record.details["data_classification"] = SensitivityToString(sensitivity);
        record.details["record_count"] = std::to_string(context.record_count);
        record.details["data_size"] = std::to_string(context.data_size);
        
        // Set severity based on operation and sensitivity
        record.severity = GetDataEventSeverity(event, sensitivity);
        
        // Special handling for bulk operations
        if (context.record_count > BULK_OPERATION_THRESHOLD) {
            record.details["bulk_operation"] = "true";
            record.severity = std::max(record.severity, Severity::Notice);
        }
        
        // Add location information for sensitive data
        if (sensitivity >= DataSensitivity::Confidential) {
            record.details["access_location"] = context.location;
            record.details["compliance_context"] = GetComplianceContext(context.resource);
        }
        
        AuditLogger::Instance().WriteRecord(record);
    }
    
private:
    DataSensitivity ClassifyDataSensitivity(const std::string& resource) {
        // Classify based on resource path/type
        if (resource.find("user_data") != std::string::npos ||
            resource.find("personal") != std::string::npos) {
            return DataSensitivity::Personal;
        } else if (resource.find("session") != std::string::npos) {
            return DataSensitivity::Confidential;
        } else if (resource.find("system") != std::string::npos) {
            return DataSensitivity::Internal;
        }
        return DataSensitivity::Public;
    }
};
```

#### System-Level Audit Events

**Process and Service Events**:
```bash
# System audit configuration (auditd rules)
# /etc/audit/rules.d/sudachi-multiplayer.rules

# Monitor service starts/stops
-w /bin/systemctl -p x -k service_control
-w /usr/bin/docker -p x -k container_control

# Monitor configuration changes
-w /etc/sudachi/ -p wa -k config_change
-w /opt/sudachi/ -p wa -k application_change

# Monitor user and group changes
-w /etc/passwd -p wa -k user_management
-w /etc/group -p wa -k group_management
-w /etc/sudoers -p wa -k privilege_escalation

# Monitor network configuration
-w /etc/network/ -p wa -k network_config
-w /etc/iptables/ -p wa -k firewall_config

# Monitor file access to sensitive areas
-w /var/log/sudachi/ -p r -k log_access
-w /opt/sudachi/secure-data/ -p rwa -k sensitive_data_access

# Monitor system calls of interest
-a always,exit -F arch=b64 -S execve -k process_execution
-a always,exit -F arch=b64 -S connect -k network_connection
-a always,exit -F arch=b64 -S bind -k network_bind
```

**Network Security Events**:
```bash
# Firewall logging configuration
# /etc/iptables/rules.v4

# Log dropped connections
-A INPUT -j LOG --log-prefix "SUDACHI-DROP: " --log-level 4
-A INPUT -j DROP

# Log accepted connections to multiplayer ports
-A INPUT -p tcp --dport 7777:7787 -j LOG --log-prefix "SUDACHI-ACCEPT: " --log-level 6
-A INPUT -p tcp --dport 7777:7787 -j ACCEPT

# Log suspicious activity
-A INPUT -m limit --limit 5/min -j LOG --log-prefix "SUDACHI-LIMIT: " --log-level 4
```

### Audit Log Format

#### Structured JSON Format

```json
{
  "timestamp": "2025-08-04T10:30:45.123Z",
  "log_version": "1.0.0",
  "source": {
    "component": "room-server",
    "hostname": "sudachi-room-01.prod.sudachi.org",
    "instance_id": "i-0123456789abcdef0",
    "process_id": 12345,
    "thread_id": 67890
  },
  "event": {
    "id": "evt_01H7Z6X8N9M2K3L4P5Q6R7S8T9",
    "type": "authentication",
    "subtype": "login_attempt",
    "category": "security",
    "severity": "info",
    "outcome": "success"
  },
  "actor": {
    "user_id": "usr_01H7Z6X8N9M2K3L4P5Q6R7S8T9",
    "username": "player123",
    "role": "player",
    "session_id": "sess_01H7Z6X8N9M2K3L4P5Q6R7S8T9",
    "ip_address": "203.0.113.42",
    "user_agent": "Sudachi/1.0.0 (Windows NT 10.0; Win64; x64)",
    "location": {
      "country": "US",
      "region": "California",
      "city": "San Francisco"
    }
  },
  "target": {
    "resource": "/api/v1/sessions",
    "resource_type": "api_endpoint",
    "resource_id": "session_01H7Z6X8N9M2K3L4P5Q6R7S8T9",
    "owner": "usr_01H7Z6X8N9M2K3L4P5Q6R7S8T9"
  },
  "request": {
    "method": "POST",
    "endpoint": "/api/v1/auth/login",
    "headers": {
      "content-type": "application/json",
      "x-forwarded-for": "203.0.113.42"
    },
    "body_hash": "sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  },
  "response": {
    "status_code": 200,
    "duration_ms": 123,
    "body_hash": "sha256:a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3"
  },
  "context": {
    "game_id": "0x0100000000000001",
    "session_type": "internet_multiplayer",
    "client_version": "1.0.0",
    "platform": "windows"
  },
  "security": {
    "threat_level": "low",
    "indicators": [],
    "correlation_id": "corr_01H7Z6X8N9M2K3L4P5Q6R7S8T9"
  },
  "compliance": {
    "data_classification": "confidential",
    "retention_period": "P90D",
    "jurisdictions": ["US", "EU"],
    "regulations": ["GDPR", "CCPA"]
  },
  "custom": {
    "multiplayer_mode": "internet",
    "connection_type": "websocket",
    "bandwidth_usage": 1024
  }
}
```

#### Legacy Syslog Format Support

```
Aug  4 10:30:45 sudachi-room-01 sudachi-multiplayer[12345]: 
AUDIT: timestamp=2025-08-04T10:30:45.123Z 
event_type=authentication 
event_subtype=login_attempt 
user_id=usr_01H7Z6X8N9M2K3L4P5Q6R7S8T9 
source_ip=203.0.113.42 
outcome=success 
session_id=sess_01H7Z6X8N9M2K3L4P5Q6R7S8T9
```

## Log Storage and Retention

### Storage Tiers

#### Hot Storage (0-30 days)
**Technology**: Elasticsearch cluster
**Purpose**: Real-time search and analysis
**Retention**: 30 days
**Performance**: < 100ms query response
**Backup**: Real-time replication to warm storage

```yaml
# Elasticsearch configuration for hot storage
cluster.name: sudachi-audit-hot
node.name: audit-hot-01
path.data: /var/lib/elasticsearch/hot
path.logs: /var/log/elasticsearch

# Hot tier configuration
node.roles: [ data_hot, ingest, ml ]
xpack.ilm.enabled: true

# Index lifecycle management
PUT _ilm/policy/sudachi-audit-policy
{
  "policy": {
    "phases": {
      "hot": {
        "actions": {
          "rollover": {
            "max_size": "10GB",
            "max_age": "1d"
          }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "allocate": {
            "number_of_replicas": 0
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "allocate": {
            "number_of_replicas": 0
          }
        }
      },
      "delete": {
        "min_age": "90d"
      }
    }
  }
}
```

#### Warm Storage (30-90 days)
**Technology**: Elasticsearch warm tier
**Purpose**: Historical analysis and compliance
**Retention**: 90 days
**Performance**: < 1s query response
**Backup**: Daily snapshots to cold storage

#### Cold Storage (90 days - 7 years)
**Technology**: AWS Glacier / Azure Archive
**Purpose**: Long-term retention and compliance
**Retention**: 7 years (configurable by jurisdiction)
**Performance**: Minutes to hours for retrieval
**Backup**: Geographic replication

#### WORM Storage
**Technology**: AWS S3 Object Lock / Azure Immutable Blob
**Purpose**: Regulatory compliance and forensics
**Retention**: Per compliance requirements
**Features**: Write-once, read-many, tamper-evident

### Retention Policies

#### By Event Type

```cpp
class AuditRetentionManager {
public:
    enum class RetentionClass {
        Security,           // 7 years
        Authentication,     // 2 years
        Authorization,      // 2 years
        DataAccess,        // 3 years (GDPR requirement)
        SystemAccess,      // 1 year
        NetworkAccess,     // 6 months
        Application,       // 90 days
        Debug              // 30 days
    };
    
    struct RetentionPolicy {
        std::chrono::days hot_retention;
        std::chrono::days warm_retention;
        std::chrono::days cold_retention;
        std::chrono::days archive_retention;
        bool immutable_required;
    };
    
private:
    std::map<RetentionClass, RetentionPolicy> policies_ = {
        {RetentionClass::Security, {
            .hot_retention = std::chrono::days(30),
            .warm_retention = std::chrono::days(90),
            .cold_retention = std::chrono::days(730),    // 2 years
            .archive_retention = std::chrono::days(2555), // 7 years
            .immutable_required = true
        }},
        {RetentionClass::DataAccess, {
            .hot_retention = std::chrono::days(30),
            .warm_retention = std::chrono::days(90),
            .cold_retention = std::chrono::days(365),    // 1 year
            .archive_retention = std::chrono::days(1095), // 3 years
            .immutable_required = true
        }},
        {RetentionClass::Application, {
            .hot_retention = std::chrono::days(7),
            .warm_retention = std::chrono::days(30),
            .cold_retention = std::chrono::days(90),
            .archive_retention = std::chrono::days(90),
            .immutable_required = false
        }}
    };
    
public:
    RetentionPolicy GetRetentionPolicy(const AuditRecord& record) {
        auto classification = ClassifyRecord(record);
        return policies_[classification];
    }
    
    void ApplyRetentionPolicy(const AuditRecord& record) {
        auto policy = GetRetentionPolicy(record);
        
        // Schedule transitions between storage tiers
        ScheduleStorageTransition(record.id, StorageTier::Warm, 
                                policy.hot_retention);
        ScheduleStorageTransition(record.id, StorageTier::Cold, 
                                policy.warm_retention);
        ScheduleStorageTransition(record.id, StorageTier::Archive, 
                                policy.cold_retention);
        
        // Schedule deletion
        if (policy.archive_retention > std::chrono::days(0)) {
            ScheduleDeletion(record.id, policy.archive_retention);
        }
        
        // Apply immutability if required
        if (policy.immutable_required) {
            SetImmutableFlag(record.id);
        }
    }
};
```

#### By Jurisdiction

```cpp
class ComplianceRetentionManager {
public:
    struct JurisdictionRequirements {
        std::chrono::days minimum_retention;
        std::chrono::days maximum_retention;
        bool immutability_required;
        std::vector<std::string> required_fields;
        std::string data_residency;
    };
    
private:
    std::map<std::string, JurisdictionRequirements> requirements_ = {
        {"EU", {
            .minimum_retention = std::chrono::days(0),
            .maximum_retention = std::chrono::days(2555), // GDPR Article 5(1)(e)
            .immutability_required = true,
            .required_fields = {"purpose", "legal_basis", "data_subject"},
            .data_residency = "EU"
        }},
        {"US", {
            .minimum_retention = std::chrono::days(365), // SOX requirements
            .maximum_retention = std::chrono::days(2555),
            .immutability_required = true,
            .required_fields = {"user_id", "timestamp", "event_type"},
            .data_residency = "US"
        }},
        {"CA", {
            .minimum_retention = std::chrono::days(365),
            .maximum_retention = std::chrono::days(2190), // PIPEDA
            .immutability_required = false,
            .required_fields = {"user_id", "purpose", "consent"},
            .data_residency = "CA"
        }}
    };
    
public:
    void ValidateRetentionCompliance(const AuditRecord& record) {
        auto jurisdictions = GetApplicableJurisdictions(record);
        
        for (const auto& jurisdiction : jurisdictions) {
            auto requirements = requirements_[jurisdiction];
            
            // Validate required fields
            for (const auto& field : requirements.required_fields) {
                if (record.details.find(field) == record.details.end()) {
                    throw ComplianceException(
                        "Missing required field for " + jurisdiction + ": " + field
                    );
                }
            }
            
            // Validate data residency
            if (!ValidateDataResidency(record, requirements.data_residency)) {
                throw ComplianceException(
                    "Data residency violation for " + jurisdiction
                );
            }
        }
    }
};
```

## Log Analysis and Monitoring

### Real-time Analysis Queries

#### Security Event Detection

```elasticsearch
# Failed authentication attempts from same IP
GET /sudachi-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event.type": "authentication"
          }
        },
        {
          "term": {
            "event.subtype": "login_failure"
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-5m"
            }
          }
        }
      ]
    }
  },
  "aggs": {
    "by_ip": {
      "terms": {
        "field": "actor.ip_address",
        "size": 100
      },
      "aggs": {
        "failure_count": {
          "value_count": {
            "field": "event.id"
          }
        }
      }
    }
  }
}

# Privilege escalation attempts
GET /sudachi-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event.type": "authorization"
          }
        },
        {
          "term": {
            "event.subtype": "privilege_escalation"
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-1h"
            }
          }
        }
      ]
    }
  },
  "sort": [
    {
      "timestamp": {
        "order": "desc"
      }
    }
  ]
}

# Unusual data access patterns
GET /sudachi-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event.type": "data_access"
          }
        },
        {
          "range": {
            "context.record_count": {
              "gte": 1000
            }
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-1h"
            }
          }
        }
      ]
    }
  },
  "aggs": {
    "by_user": {
      "terms": {
        "field": "actor.user_id",
        "size": 50
      },
      "aggs": {
        "total_records": {
          "sum": {
            "field": "context.record_count"
          }
        }
      }
    }
  }
}
```

#### Compliance Reporting Queries

```elasticsearch
# GDPR Article 30 - Records of Processing Activities
GET /sudachi-audit-*/_search
{
  "size": 0,
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event.type": "data_access"
          }
        },
        {
          "exists": {
            "field": "compliance.data_classification"
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-30d"
            }
          }
        }
      ]
    }
  },
  "aggs": {
    "by_purpose": {
      "terms": {
        "field": "context.purpose.keyword",
        "size": 20
      },
      "aggs": {
        "by_classification": {
          "terms": {
            "field": "compliance.data_classification.keyword"
          },
          "aggs": {
            "record_count": {
              "sum": {
                "field": "context.record_count"
              }
            },
            "unique_users": {
              "cardinality": {
                "field": "actor.user_id"
              }
            }
          }
        }
      }
    }
  }
}
```

### Automated Alerting

#### Critical Security Alerts

```yaml
# Elasticsearch Watcher configuration
PUT _watcher/watch/multiple_failed_logins
{
  "trigger": {
    "schedule": {
      "interval": "1m"
    }
  },
  "input": {
    "search": {
      "request": {
        "search_type": "query_then_fetch",
        "indices": ["sudachi-audit-*"],
        "body": {
          "size": 0,
          "query": {
            "bool": {
              "must": [
                {
                  "term": {
                    "event.type": "authentication"
                  }
                },
                {
                  "term": {
                    "event.subtype": "login_failure"
                  }
                },
                {
                  "range": {
                    "timestamp": {
                      "gte": "now-5m"
                    }
                  }
                }
              ]
            }
          },
          "aggs": {
            "by_ip": {
              "terms": {
                "field": "actor.ip_address",
                "size": 100
              },
              "aggs": {
                "failure_count": {
                  "value_count": {
                    "field": "event.id"
                  }
                }
              }
            }
          }
        }
      }
    }
  },
  "condition": {
    "script": {
      "source": """
        def suspiciousIPs = [];
        for (bucket in ctx.payload.aggregations.by_ip.buckets) {
          if (bucket.failure_count.value >= 5) {
            suspiciousIPs.add([
              'ip': bucket.key,
              'failures': bucket.failure_count.value
            ]);
          }
        }
        ctx.vars.suspicious_ips = suspiciousIPs;
        return suspiciousIPs.size() > 0;
      """
    }
  },
  "actions": {
    "send_email": {
      "email": {
        "to": ["security@sudachi.org"],
        "subject": "SECURITY ALERT: Multiple failed login attempts detected",
        "body": """
          Multiple failed login attempts detected from the following IP addresses:
          
          {{#ctx.vars.suspicious_ips}}
          - {{ip}}: {{failures}} failed attempts
          {{/ctx.vars.suspicious_ips}}
          
          Time range: Last 5 minutes
          Investigation required immediately.
        """
      }
    },
    "webhook_notification": {
      "webhook": {
        "scheme": "https",
        "host": "security.sudachi.org",
        "port": 443,
        "method": "post",
        "path": "/api/v1/alerts",
        "params": {},
        "headers": {
          "Content-Type": "application/json",
          "Authorization": "Bearer {{ctx.metadata.webhook_token}}"
        },
        "body": """
        {
          "alert_type": "brute_force_attack",
          "severity": "high",
          "source_ips": {{#toJson}}ctx.vars.suspicious_ips{{/toJson}},
          "timestamp": "{{ctx.execution_time}}"
        }
        """
      }
    }
  }
}
```

### Forensic Analysis Tools

#### Log Correlation Script

```python
#!/usr/bin/env python3
"""
Sudachi Multiplayer Audit Log Forensic Analysis Tool
Correlates events across multiple log sources for incident investigation
"""

import json
import argparse
from datetime import datetime, timedelta
from elasticsearch import Elasticsearch
import pandas as pd
import matplotlib.pyplot as plt

class AuditLogAnalyzer:
    def __init__(self, es_host='localhost:9200'):
        self.es = Elasticsearch([es_host])
        
    def investigate_user_activity(self, user_id, start_time, end_time):
        """Comprehensive user activity analysis"""
        
        # Build query for all user events
        query = {
            "query": {
                "bool": {
                    "must": [
                        {"term": {"actor.user_id": user_id}},
                        {"range": {"timestamp": {"gte": start_time, "lte": end_time}}}
                    ]
                }
            },
            "sort": [{"timestamp": "asc"}],
            "size": 10000
        }
        
        # Execute search
        response = self.es.search(index="sudachi-audit-*", body=query)
        events = [hit['_source'] for hit in response['hits']['hits']]
        
        # Analyze patterns
        analysis = {
            'total_events': len(events),
            'event_types': self._analyze_event_types(events),
            'time_pattern': self._analyze_time_patterns(events),
            'ip_addresses': self._analyze_ip_patterns(events),
            'suspicious_activities': self._detect_suspicious_patterns(events),
            'timeline': self._create_timeline(events)
        }
        
        return analysis
    
    def investigate_security_incident(self, incident_id, correlation_window=3600):
        """Correlate events around a security incident"""
        
        # Find the initial incident event
        incident_query = {
            "query": {"term": {"security.correlation_id": incident_id}},
            "size": 1
        }
        
        incident_response = self.es.search(index="sudachi-audit-*", body=incident_query)
        if not incident_response['hits']['hits']:
            return {"error": "Incident not found"}
        
        incident_event = incident_response['hits']['hits'][0]['_source']
        incident_time = datetime.fromisoformat(incident_event['timestamp'].replace('Z', '+00:00'))
        
        # Define correlation window
        start_time = incident_time - timedelta(seconds=correlation_window)
        end_time = incident_time + timedelta(seconds=correlation_window)
        
        # Correlate by IP address
        ip_correlation = self._correlate_by_ip(
            incident_event['actor']['ip_address'], 
            start_time, 
            end_time
        )
        
        # Correlate by user
        user_correlation = self._correlate_by_user(
            incident_event['actor']['user_id'], 
            start_time, 
            end_time
        )
        
        # Correlate by resource
        resource_correlation = self._correlate_by_resource(
            incident_event['target']['resource'], 
            start_time, 
            end_time
        )
        
        return {
            'incident_event': incident_event,
            'correlation_window': f"{correlation_window} seconds",
            'ip_correlation': ip_correlation,
            'user_correlation': user_correlation,
            'resource_correlation': resource_correlation,
            'attack_chain': self._reconstruct_attack_chain(incident_event, start_time, end_time)
        }
    
    def _detect_suspicious_patterns(self, events):
        """Detect suspicious patterns in user events"""
        suspicious = []
        
        # Check for rapid consecutive failures
        auth_failures = [e for e in events if e['event']['type'] == 'authentication' 
                        and e['event']['subtype'] == 'login_failure']
        
        if len(auth_failures) >= 5:
            suspicious.append({
                'type': 'brute_force_attempt',
                'count': len(auth_failures),
                'time_span': self._calculate_time_span(auth_failures)
            })
        
        # Check for privilege escalation
        privilege_events = [e for e in events if e['event']['subtype'] == 'privilege_escalation']
        if privilege_events:
            suspicious.append({
                'type': 'privilege_escalation',
                'events': len(privilege_events),
                'details': [e['event']['details'] for e in privilege_events]
            })
        
        # Check for unusual data access
        data_events = [e for e in events if e['event']['type'] == 'data_access']
        total_records = sum(e.get('context', {}).get('record_count', 0) for e in data_events)
        
        if total_records > 10000:  # Threshold for bulk access
            suspicious.append({
                'type': 'bulk_data_access',
                'total_records': total_records,
                'events': len(data_events)
            })
        
        return suspicious
    
    def generate_forensic_report(self, investigation_results, output_file):
        """Generate comprehensive forensic report"""
        
        report = {
            'investigation_metadata': {
                'generated_at': datetime.utcnow().isoformat(),
                'analyst': 'Automated Forensic Tool',
                'case_id': f"CASE_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}"
            },
            'executive_summary': self._generate_executive_summary(investigation_results),
            'detailed_findings': investigation_results,
            'recommendations': self._generate_recommendations(investigation_results),
            'evidence_chain': self._document_evidence_chain(investigation_results)
        }
        
        # Save report
        with open(output_file, 'w') as f:
            json.dump(report, f, indent=2, default=str)
        
        # Generate visualizations
        self._generate_charts(investigation_results, output_file.replace('.json', '_charts.png'))
        
        return report

def main():
    parser = argparse.ArgumentParser(description='Sudachi Audit Log Forensic Analysis')
    parser.add_argument('--user-id', help='Investigate specific user activity')
    parser.add_argument('--incident-id', help='Investigate security incident')
    parser.add_argument('--start-time', help='Investigation start time (ISO format)')
    parser.add_argument('--end-time', help='Investigation end time (ISO format)')
    parser.add_argument('--output', help='Output report file', default='forensic_report.json')
    parser.add_argument('--es-host', help='Elasticsearch host', default='localhost:9200')
    
    args = parser.parse_args()
    
    analyzer = AuditLogAnalyzer(args.es_host)
    
    if args.user_id:
        results = analyzer.investigate_user_activity(
            args.user_id, 
            args.start_time, 
            args.end_time
        )
    elif args.incident_id:
        results = analyzer.investigate_security_incident(args.incident_id)
    else:
        print("Please specify either --user-id or --incident-id")
        return
    
    # Generate forensic report
    report = analyzer.generate_forensic_report(results, args.output)
    
    print(f"Forensic analysis complete. Report saved to: {args.output}")
    print(f"Case ID: {report['investigation_metadata']['case_id']}")

if __name__ == '__main__':
    main()
```

## Compliance and Legal Requirements

### Audit Log Integrity

#### Digital Signatures

```cpp
class AuditLogSigner {
public:
    struct SignedAuditRecord {
        AuditRecord record;
        std::string signature;
        std::string public_key_id;
        std::chrono::system_clock::time_point signed_at;
    };
    
    SignedAuditRecord SignRecord(const AuditRecord& record) {
        // Serialize record for signing
        auto serialized = SerializeRecord(record);
        
        // Create signature
        auto signature = crypto_manager_.SignData(serialized, signing_key_);
        
        SignedAuditRecord signed_record;
        signed_record.record = record;
        signed_record.signature = Base64Encode(signature);
        signed_record.public_key_id = GetPublicKeyId();
        signed_record.signed_at = std::chrono::system_clock::now();
        
        return signed_record;
    }
    
    bool VerifyRecord(const SignedAuditRecord& signed_record) {
        auto serialized = SerializeRecord(signed_record.record);
        auto signature = Base64Decode(signed_record.signature);
        auto public_key = GetPublicKey(signed_record.public_key_id);
        
        return crypto_manager_.VerifySignature(serialized, signature, public_key);
    }
    
private:
    CryptographyManager crypto_manager_;
    PrivateKey signing_key_;
};
```

#### Chain of Custody

```cpp
class ChainOfCustodyManager {
public:
    struct CustodyRecord {
        std::string record_id;
        std::string handler_id;
        std::string action;
        std::chrono::system_clock::time_point timestamp;
        std::string justification;
        std::string hash_before;
        std::string hash_after;
        std::string signature;
    };
    
    void RecordCustodyTransfer(const std::string& record_id,
                              const std::string& from_handler,
                              const std::string& to_handler,
                              const std::string& justification) {
        CustodyRecord custody;
        custody.record_id = record_id;
        custody.handler_id = to_handler;
        custody.action = "transfer";
        custody.timestamp = std::chrono::system_clock::now();
        custody.justification = justification;
        
        // Get record hash before and after transfer
        custody.hash_before = GetRecordHash(record_id);
        // Transfer logic here
        custody.hash_after = GetRecordHash(record_id);
        
        // Sign the custody record
        custody.signature = SignCustodyRecord(custody);
        
        // Store custody record
        StoreCustodyRecord(custody);
        
        // Notify relevant parties
        NotifyCustodyTransfer(from_handler, to_handler, custody);
    }
    
    std::vector<CustodyRecord> GetCustodyChain(const std::string& record_id) {
        return custody_store_.GetCustodyHistory(record_id);
    }
    
    bool ValidateCustodyChain(const std::string& record_id) {
        auto chain = GetCustodyChain(record_id);
        
        for (const auto& custody : chain) {
            // Verify signature
            if (!VerifyCustodySignature(custody)) {
                return false;
            }
            
            // Verify hash integrity
            if (!ValidateHashIntegrity(custody)) {
                return false;
            }
        }
        
        return true;
    }
};
```

### Privacy and Data Protection

#### Log Anonymization

```cpp
class LogAnonymizer {
public:
    enum class AnonymizationLevel {
        None,           // No anonymization
        Pseudonymous,   // Replace with consistent pseudonyms
        Anonymous,      // Remove all identifying information
        Aggregate       // Only aggregate statistics
    };
    
    AuditRecord AnonymizeRecord(const AuditRecord& record, 
                               AnonymizationLevel level) {
        AuditRecord anonymized = record;
        
        switch (level) {
            case AnonymizationLevel::Pseudonymous:
                anonymized.actor.user_id = GetPseudonym(record.actor.user_id);
                anonymized.actor.username = "";
                anonymized.actor.ip_address = AnonymizeIP(record.actor.ip_address);
                break;
                
            case AnonymizationLevel::Anonymous:
                anonymized.actor.user_id = "";
                anonymized.actor.username = "";
                anonymized.actor.ip_address = "";
                anonymized.actor.location = {};
                anonymized.actor.user_agent = "";
                break;
                
            case AnonymizationLevel::Aggregate:
                // Only keep statistical information
                anonymized = CreateAggregateRecord(record);
                break;
                
            default:
                break;
        }
        
        // Mark as anonymized
        anonymized.compliance.anonymization_level = AnonymizationLevelToString(level);
        anonymized.compliance.anonymized_at = std::chrono::system_clock::now();
        
        return anonymized;
    }
    
private:
    std::string GetPseudonym(const std::string& original_id) {
        // Use cryptographic hash for consistent pseudonymization
        return "pseudo_" + SHA256Hash(original_id + pseudonym_salt_);
    }
    
    std::string AnonymizeIP(const std::string& ip_address) {
        // Remove last octet for IPv4, last 80 bits for IPv6
        if (ip_address.find(':') != std::string::npos) {
            // IPv6 - keep only first 48 bits
            return ip_address.substr(0, ip_address.find(":", ip_address.find(":", ip_address.find(":") + 1) + 1)) + "::";
        } else {
            // IPv4 - keep only first 3 octets
            auto last_dot = ip_address.find_last_of('.');
            return ip_address.substr(0, last_dot) + ".0";
        }
    }
};
```

## Operational Procedures

### Daily Operations

#### Log Health Monitoring

```bash
#!/bin/bash
# Daily audit log health check

LOG_HEALTH_REPORT="/var/log/sudachi/audit-health-$(date +%Y%m%d).txt"

echo "Sudachi Audit Log Health Check - $(date)" > $LOG_HEALTH_REPORT
echo "=================================================" >> $LOG_HEALTH_REPORT

# Check log ingestion rate
echo "Log Ingestion Statistics:" >> $LOG_HEALTH_REPORT
curl -s "http://elasticsearch:9200/sudachi-audit-*/_stats" | \
    jq '.indices | to_entries[] | {index: .key, docs: .value.total.docs.count}' >> $LOG_HEALTH_REPORT

# Check for missing logs
echo -e "\nMissing Log Detection:" >> $LOG_HEALTH_REPORT
EXPECTED_SOURCES=("room-server" "relay-server" "nginx" "postgresql" "redis")

for source in "${EXPECTED_SOURCES[@]}"; do
    RECENT_LOGS=$(curl -s "http://elasticsearch:9200/sudachi-audit-*/_count" \
        -H "Content-Type: application/json" \
        -d "{\"query\":{\"bool\":{\"must\":[{\"term\":{\"source.component\":\"$source\"}},{\"range\":{\"timestamp\":{\"gte\":\"now-1h\"}}}]}}}" | \
        jq '.count')
    
    if [ "$RECENT_LOGS" -eq 0 ]; then
        echo "WARNING: No recent logs from $source" >> $LOG_HEALTH_REPORT
    else
        echo "OK: $source - $RECENT_LOGS logs in last hour" >> $LOG_HEALTH_REPORT
    fi
done

# Check storage usage
echo -e "\nStorage Usage:" >> $LOG_HEALTH_REPORT
df -h /var/lib/elasticsearch >> $LOG_HEALTH_REPORT

# Check for critical security events
echo -e "\nCritical Security Events (Last 24h):" >> $LOG_HEALTH_REPORT
CRITICAL_EVENTS=$(curl -s "http://elasticsearch:9200/sudachi-audit-*/_count" \
    -H "Content-Type: application/json" \
    -d '{"query":{"bool":{"must":[{"term":{"event.severity":"critical"}},{"range":{"timestamp":{"gte":"now-24h"}}}]}}}' | \
    jq '.count')

echo "Critical events found: $CRITICAL_EVENTS" >> $LOG_HEALTH_REPORT

if [ "$CRITICAL_EVENTS" -gt 0 ]; then
    echo "ALERT: Critical security events detected!" >> $LOG_HEALTH_REPORT
    mail -s "URGENT: Critical Security Events Detected" security@sudachi.org < $LOG_HEALTH_REPORT
fi

# Archive health report
cp $LOG_HEALTH_REPORT /archive/audit-health-reports/
```

### Incident Response Procedures

#### Log Preservation for Legal Hold

```bash
#!/bin/bash
# Legal hold script for audit logs

CASE_ID="$1"
START_DATE="$2"
END_DATE="$3"
PRESERVATION_DIR="/legal-hold/$CASE_ID"

if [ $# -ne 3 ]; then
    echo "Usage: $0 <case-id> <start-date> <end-date>"
    echo "Example: $0 CASE-2025-001 2025-08-01 2025-08-04"
    exit 1
fi

echo "Initiating legal hold for case: $CASE_ID"
echo "Date range: $START_DATE to $END_DATE"

# Create preservation directory
mkdir -p "$PRESERVATION_DIR"

# Export relevant logs
echo "Exporting audit logs..."
curl -X GET "elasticsearch:9200/sudachi-audit-*/_search" \
    -H "Content-Type: application/json" \
    -d "{
        \"query\": {
            \"range\": {
                \"timestamp\": {
                    \"gte\": \"$START_DATE\",
                    \"lte\": \"$END_DATE\"
                }
            }
        },
        \"size\": 10000,
        \"sort\": [{\"timestamp\": \"asc\"}]
    }" > "$PRESERVATION_DIR/raw_logs.json"

# Create integrity hash
sha256sum "$PRESERVATION_DIR/raw_logs.json" > "$PRESERVATION_DIR/integrity.sha256"

# Create preservation metadata
cat > "$PRESERVATION_DIR/metadata.json" << EOF
{
    "case_id": "$CASE_ID",
    "preservation_date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "date_range": {
        "start": "$START_DATE",
        "end": "$END_DATE"
    },
    "preserved_by": "$(whoami)",
    "system": "sudachi-multiplayer-audit",
    "record_count": $(jq '.hits.total.value' "$PRESERVATION_DIR/raw_logs.json"),
    "integrity_hash": "$(cat "$PRESERVATION_DIR/integrity.sha256" | cut -d' ' -f1)"
}
EOF

# Apply legal hold flag in database
echo "Applying legal hold flag to preserved records..."
# Implementation would mark records as under legal hold

# Create chain of custody record
echo "Creating chain of custody record..."
# Implementation would create initial custody record

echo "Legal hold preservation complete for case: $CASE_ID"
echo "Preserved data location: $PRESERVATION_DIR"
echo "Record count: $(jq '.hits.total.value' "$PRESERVATION_DIR/raw_logs.json")"
echo "Integrity hash: $(cat "$PRESERVATION_DIR/integrity.sha256" | cut -d' ' -f1)"

# Notify legal team
mail -s "Legal Hold Applied - Case $CASE_ID" legal@sudachi.org << EOF
Legal hold has been applied to audit logs for case: $CASE_ID

Date range: $START_DATE to $END_DATE
Records preserved: $(jq '.hits.total.value' "$PRESERVATION_DIR/raw_logs.json")
Preservation location: $PRESERVATION_DIR
Integrity hash: $(cat "$PRESERVATION_DIR/integrity.sha256" | cut -d' ' -f1)

Chain of custody has been initiated.
EOF
```

---

*This audit log documentation is maintained by the Sudachi security and compliance team. For questions about audit logs or compliance requirements, contact audit@sudachi.org or security@sudachi.org.*