// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Core::Multiplayer::ModelB {

/**
 * Network interface information for multi-interface mDNS operations
 */
struct NetworkInterface {
    std::string name;           // Interface name (e.g., "eth0", "wlan0")
    std::string ipv4_address;   // IPv4 address
    std::string ipv6_address;   // IPv6 address  
    bool is_active;             // Whether the interface is active
    
    enum class InterfaceType {
        Ethernet,
        WiFi,
        Loopback,
        Unknown
    } type;
    
    NetworkInterface(const std::string& n, const std::string& ipv4, const std::string& ipv6,
                    bool active, InterfaceType t)
        : name(n), ipv4_address(ipv4), ipv6_address(ipv6), is_active(active), type(t) {}
};

using InterfaceType = NetworkInterface::InterfaceType;

/**
 * Mock mDNS socket interface for testing
 * This represents the low-level mDNS socket operations that the discovery service will use
 * Based on the mjansson/mdns library interface
 */
class MockMdnsSocket {
public:
    // Socket creation and management
    MOCK_METHOD(bool, CreateSocket, (int address_family, const std::string& interface_name), ());
    MOCK_METHOD(bool, CloseSocket, (), ());
    MOCK_METHOD(bool, BindToInterface, (const std::string& interface_name), ());
    MOCK_METHOD(bool, SetSocketOptions, (int option, int value), ());
    
    // Multicast group management
    MOCK_METHOD(bool, JoinMulticastGroup, (const std::string& multicast_address), ());
    MOCK_METHOD(bool, LeaveMulticastGroup, (const std::string& multicast_address), ());
    MOCK_METHOD(bool, SetMulticastInterface, (const std::string& interface_name), ());
    MOCK_METHOD(bool, SetMulticastTTL, (int ttl), ());
    
    // Service discovery operations
    MOCK_METHOD(bool, SendQuery, (const std::string& service_type, int query_type, const std::string& interface_name), ());
    MOCK_METHOD(bool, SendResponse, (const std::string& service_name, const std::vector<uint8_t>& response_data, const std::string& interface_name), ());
    
    // Service advertisement operations
    MOCK_METHOD(bool, PublishService, (const std::string& service_type, const std::string& service_name, uint16_t port, const std::string& txt_records), ());
    MOCK_METHOD(bool, UnpublishService, (const std::string& service_type, const std::string& service_name), ());
    MOCK_METHOD(bool, UpdateServiceTxtRecords, (const std::string& service_name, const std::string& txt_records), ());
    
    // Packet I/O operations
    MOCK_METHOD(int, ReceivePacket, (uint8_t* buffer, size_t buffer_size, std::string* source_address, std::string* interface_name), ());
    MOCK_METHOD(bool, SendPacket, (const uint8_t* data, size_t data_size, const std::string& destination_address, const std::string& interface_name), ());
    
    // Socket state and information
    MOCK_METHOD(bool, IsSocketValid, (), (const));
    MOCK_METHOD(int, GetSocketHandle, (), (const));
    MOCK_METHOD(std::string, GetBoundInterface, (), (const));
    MOCK_METHOD(uint16_t, GetBoundPort, (), (const));
    
    // Error handling
    MOCK_METHOD(int, GetLastError, (), (const));
    MOCK_METHOD(std::string, GetErrorMessage, (int error_code), (const));
    
    // Callback registration for asynchronous operations
    MOCK_METHOD(void, SetOnPacketReceivedCallback, (std::function<void(const uint8_t*, size_t, const std::string&, const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnServiceDiscoveredCallback, (std::function<void(const std::string&, const std::string&, uint16_t, const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnServiceRemovedCallback, (std::function<void(const std::string&, const std::string&)> callback), ());
    MOCK_METHOD(void, SetOnErrorCallback, (std::function<void(int, const std::string&)> callback), ());
    
    // Advanced mDNS operations
    MOCK_METHOD(bool, SetServicePriority, (const std::string& service_name, uint16_t priority), ());
    MOCK_METHOD(bool, SetServiceWeight, (const std::string& service_name, uint16_t weight), ());
    MOCK_METHOD(bool, SetServiceTTL, (const std::string& service_name, uint32_t ttl), ());
    
    // Network interface queries
    MOCK_METHOD(std::vector<std::string>, GetAvailableInterfaces, (), (const));
    MOCK_METHOD(bool, IsInterfaceMulticastCapable, (const std::string& interface_name), (const));
    MOCK_METHOD(std::string, GetInterfaceAddress, (const std::string& interface_name, int address_family), (const));
    
    // Batch operations for efficiency
    MOCK_METHOD(bool, SendMultipleQueries, (const std::vector<std::string>& service_types, const std::string& interface_name), ());
    MOCK_METHOD(bool, PublishMultipleServices, (const std::vector<std::tuple<std::string, std::string, uint16_t, std::string>>& services), ());
    
    // Statistics and monitoring
    MOCK_METHOD(size_t, GetPacketsSent, (), (const));
    MOCK_METHOD(size_t, GetPacketsReceived, (), (const));
    MOCK_METHOD(size_t, GetQueriesSent, (), (const));
    MOCK_METHOD(size_t, GetResponsesSent, (), (const));
    MOCK_METHOD(std::chrono::milliseconds, GetAverageResponseTime, (), (const));
    
    // Configuration and tuning
    MOCK_METHOD(bool, SetReceiveBufferSize, (size_t size), ());
    MOCK_METHOD(bool, SetSendBufferSize, (size_t size), ());
    MOCK_METHOD(bool, SetReceiveTimeout, (std::chrono::milliseconds timeout), ());
    MOCK_METHOD(bool, EnableBroadcast, (bool enable), ());
    MOCK_METHOD(bool, EnableReuseAddress, (bool enable), ());
    
    // IPv6 specific operations
    MOCK_METHOD(bool, SetIPv6Only, (bool ipv6_only), ());
    MOCK_METHOD(bool, SetIPv6MulticastHops, (int hops), ());
    MOCK_METHOD(bool, JoinIPv6MulticastGroup, (const std::string& ipv6_address, const std::string& interface_name), ());
    
    // Service resolution operations
    MOCK_METHOD(bool, ResolveService, (const std::string& service_name, const std::string& service_type, std::string* hostname, uint16_t* port, std::string* txt_records), ());
    MOCK_METHOD(bool, ResolveAddress, (const std::string& hostname, std::vector<std::string>* addresses), ());
    
    // Cache management
    MOCK_METHOD(void, ClearCache, (), ());
    MOCK_METHOD(bool, SetCacheSize, (size_t max_entries), ());
    MOCK_METHOD(size_t, GetCacheSize, (), (const));
    MOCK_METHOD(void, InvalidateCacheEntry, (const std::string& service_name), ());
    
    // Platform-specific operations (for testing different platforms)
    MOCK_METHOD(bool, SetPlatformSpecificOption, (const std::string& option_name, const std::string& option_value), ());
    MOCK_METHOD(std::string, GetPlatformSpecificInfo, (const std::string& info_name), (const));
};

/**
 * Constants for mDNS operations used in tests
 */
namespace MdnsConstants {
    constexpr const char* kMulticastIPv4 = "224.0.0.251";
    constexpr const char* kMulticastIPv6 = "ff02::fb";
    constexpr uint16_t kMdnsPort = 5353;
    constexpr const char* kSudachiServiceType = "_sudachi-ldn._tcp.local.";
    constexpr uint32_t kDefaultTTL = 120; // 2 minutes
    constexpr size_t kMaxPacketSize = 9000; // Jumbo frame size
    constexpr size_t kStandardPacketSize = 1500; // Standard MTU
    constexpr std::chrono::milliseconds kDefaultTimeout{5000};
    constexpr int kMaxRetries = 3;
}

/**
 * Helper functions for creating test data
 */
namespace TestHelpers {
    /**
     * Create a mock mDNS query packet for testing
     */
    std::vector<uint8_t> CreateMdnsQuery(const std::string& service_type, uint16_t query_id = 0);
    
    /**
     * Create a mock mDNS response packet for testing
     */
    std::vector<uint8_t> CreateMdnsResponse(const std::string& service_name,
                                          const std::string& service_type,
                                          const std::string& hostname,
                                          uint16_t port,
                                          const std::string& txt_records,
                                          uint32_t ttl = MdnsConstants::kDefaultTTL);
    
    /**
     * Parse a mock mDNS packet for verification in tests
     */
    struct MdnsPacketInfo {
        uint16_t transaction_id;
        bool is_query;
        bool is_response;
        std::vector<std::string> questions;
        std::vector<std::string> answers;
        std::string txt_records;
    };
    
    MdnsPacketInfo ParseMdnsPacket(const uint8_t* data, size_t size);
    
    /**
     * Create test network interface configurations
     */
    std::vector<NetworkInterface> CreateTestInterfaces();
    NetworkInterface CreateEthernetInterface(const std::string& name = "eth0");
    NetworkInterface CreateWiFiInterface(const std::string& name = "wlan0");
    NetworkInterface CreateLoopbackInterface(const std::string& name = "lo");
}

} // namespace Core::Multiplayer::ModelB
