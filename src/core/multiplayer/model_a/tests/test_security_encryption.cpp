// SPDX-FileCopyrightText: 2025 sudachi Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <random>

#include "core/multiplayer/model_a/p2p_security.h"
#include "core/multiplayer/model_a/noise_protocol.h"
#include "core/multiplayer/common/error_codes.h"

using namespace testing;
using namespace Core::Multiplayer;
using namespace Core::Multiplayer::ModelA;

namespace {

/**
 * Mock Noise protocol implementation for testing
 */
class MockNoiseProtocol {
public:
    MOCK_METHOD(void, initialize, (const std::vector<uint8_t>& static_key), ());
    MOCK_METHOD(std::vector<uint8_t>, getPublicKey, (), (const));
    MOCK_METHOD(std::vector<uint8_t>, getPrivateKey, (), (const));
    MOCK_METHOD(bool, performHandshake, (const std::vector<uint8_t>& remote_public_key), ());
    MOCK_METHOD(std::vector<uint8_t>, encrypt, (const std::vector<uint8_t>& plaintext), ());
    MOCK_METHOD(std::vector<uint8_t>, decrypt, (const std::vector<uint8_t>& ciphertext), ());
    MOCK_METHOD(bool, isHandshakeComplete, (), (const));
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(std::vector<uint8_t>, exportSessionKey, (), (const));
};

/**
 * Mock key manager for testing key generation and storage
 */
class MockKeyManager {
public:
    MOCK_METHOD(std::vector<uint8_t>, generateStaticKey, (), ());
    MOCK_METHOD(std::vector<uint8_t>, generateEphemeralKey, (), ());
    MOCK_METHOD(void, storeStaticKey, (const std::vector<uint8_t>& key), ());
    MOCK_METHOD(std::vector<uint8_t>, loadStaticKey, (), ());
    MOCK_METHOD(bool, hasStoredKey, (), (const));
    MOCK_METHOD(void, clearStoredKey, (), ());
    MOCK_METHOD(std::string, deriveKeyFingerprint, (const std::vector<uint8_t>& key), (const));
};

/**
 * Mock certificate validator for testing peer identity verification
 */
class MockCertificateValidator {
public:
    MOCK_METHOD(bool, validatePeerCertificate, (const std::string& peer_id, const std::vector<uint8_t>& certificate), ());
    MOCK_METHOD(std::vector<uint8_t>, generateSelfSignedCertificate, (const std::vector<uint8_t>& public_key), ());
    MOCK_METHOD(bool, verifyCertificateChain, (const std::vector<std::vector<uint8_t>>& certificate_chain), ());
    MOCK_METHOD(std::string, extractPeerIdFromCertificate, (const std::vector<uint8_t>& certificate), (const));
};

} // anonymous namespace

/**
 * Test fixture for security and encryption tests
 * Tests the Noise protocol implementation and cryptographic security features
 */
class SecurityEncryptionTest : public Test {
protected:
    void SetUp() override {
        mock_noise_ = std::make_shared<MockNoiseProtocol>();
        mock_key_manager_ = std::make_shared<MockKeyManager>();
        mock_cert_validator_ = std::make_shared<MockCertificateValidator>();
        
        // Generate test keys
        test_static_key_ = generateRandomBytes(32); // 256-bit key
        test_public_key_ = generateRandomBytes(32);
        test_remote_public_key_ = generateRandomBytes(32);
        
        // Configure security settings
        security_config_.noise_pattern = "Noise_XX_25519_ChaChaPoly_BLAKE2s";
        security_config_.key_rotation_interval_ms = 3600000; // 1 hour
        security_config_.enable_perfect_forward_secrecy = true;
        security_config_.require_certificate_validation = true;
    }
    
    std::vector<uint8_t> generateRandomBytes(size_t length) {
        std::vector<uint8_t> bytes(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : bytes) {
            byte = dis(gen);
        }
        return bytes;
    }

    std::shared_ptr<MockNoiseProtocol> mock_noise_;
    std::shared_ptr<MockKeyManager> mock_key_manager_;
    std::shared_ptr<MockCertificateValidator> mock_cert_validator_;
    SecurityConfig security_config_;
    std::vector<uint8_t> test_static_key_;
    std::vector<uint8_t> test_public_key_;
    std::vector<uint8_t> test_remote_public_key_;
};

/**
 * Test: Noise protocol initialization with static key
 * Verifies that the Noise protocol can be properly initialized with a static key
 */
TEST_F(SecurityEncryptionTest, NoiseProtocolInitializationWithStaticKey) {
    // ARRANGE
    EXPECT_CALL(*mock_key_manager_, hasStoredKey())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_key_manager_, loadStaticKey())
        .WillOnce(Return(test_static_key_));
    EXPECT_CALL(*mock_noise_, initialize(test_static_key_));
    EXPECT_CALL(*mock_noise_, getPublicKey())
        .WillOnce(Return(test_public_key_));
    
    // ACT
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    auto result = security_manager->initialize();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(security_manager->isInitialized());
    auto public_key = security_manager->getPublicKey();
    EXPECT_EQ(public_key, test_public_key_);
}

/**
 * Test: Noise protocol initialization with generated key
 * Verifies that a new static key is generated when none exists
 */
TEST_F(SecurityEncryptionTest, NoiseProtocolInitializationWithGeneratedKey) {
    // ARRANGE
    EXPECT_CALL(*mock_key_manager_, hasStoredKey())
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_key_manager_, generateStaticKey())
        .WillOnce(Return(test_static_key_));
    EXPECT_CALL(*mock_key_manager_, storeStaticKey(test_static_key_));
    EXPECT_CALL(*mock_noise_, initialize(test_static_key_));
    EXPECT_CALL(*mock_noise_, getPublicKey())
        .WillOnce(Return(test_public_key_));
    
    // ACT
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    auto result = security_manager->initialize();
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(security_manager->isInitialized());
}

/**
 * Test: Successful Noise handshake between peers
 * Verifies that the Noise handshake completes successfully
 */
TEST_F(SecurityEncryptionTest, SuccessfulNoiseHandshakeBetweenPeers) {
    // ARRANGE
    const std::string peer_id = "12D3KooWSecurePeer";
    
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, exportSessionKey())
        .WillOnce(Return(generateRandomBytes(32)));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT
    auto result = security_manager->performHandshake(peer_id, test_remote_public_key_);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(security_manager->isHandshakeComplete(peer_id));
    EXPECT_TRUE(security_manager->hasSecureSession(peer_id));
}

/**
 * Test: Handshake failure with invalid remote key
 * Verifies proper error handling when handshake fails
 */
TEST_F(SecurityEncryptionTest, HandshakeFailureWithInvalidRemoteKey) {
    // ARRANGE
    const std::string peer_id = "12D3KooWInvalidPeer";
    std::vector<uint8_t> invalid_key(10, 0x00); // Invalid key size
    
    EXPECT_CALL(*mock_noise_, performHandshake(invalid_key))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillOnce(Return(false));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT
    auto result = security_manager->performHandshake(peer_id, invalid_key);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::AuthenticationFailed);
    EXPECT_FALSE(security_manager->isHandshakeComplete(peer_id));
    EXPECT_FALSE(security_manager->hasSecureSession(peer_id));
}

/**
 * Test: Data encryption and decryption
 * Verifies that data can be encrypted and decrypted correctly
 */
TEST_F(SecurityEncryptionTest, DataEncryptionAndDecryption) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCryptoPeer";
    const std::vector<uint8_t> plaintext = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    const std::vector<uint8_t> ciphertext = generateRandomBytes(plaintext.size() + 16); // Include auth tag
    
    // Set up successful handshake first
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillRepeatedly(Return(true));
    
    // Set up encryption/decryption
    EXPECT_CALL(*mock_noise_, encrypt(plaintext))
        .WillOnce(Return(ciphertext));
    EXPECT_CALL(*mock_noise_, decrypt(ciphertext))
        .WillOnce(Return(plaintext));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    security_manager->performHandshake(peer_id, test_remote_public_key_);
    
    // ACT
    auto encrypted_data = security_manager->encryptData(peer_id, plaintext);
    auto decrypted_data = security_manager->decryptData(peer_id, encrypted_data);
    
    // ASSERT
    EXPECT_EQ(encrypted_data, ciphertext);
    EXPECT_EQ(decrypted_data, plaintext);
}

/**
 * Test: Encryption without completed handshake fails
 * Verifies that encryption is not allowed before handshake completion
 */
TEST_F(SecurityEncryptionTest, EncryptionWithoutHandshakeFails) {
    // ARRANGE
    const std::string peer_id = "12D3KooWNoHandshake";
    const std::vector<uint8_t> plaintext = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT & ASSERT
    EXPECT_THROW({
        security_manager->encryptData(peer_id, plaintext);
    }, std::runtime_error);
}

/**
 * Test: Key rotation functionality
 * Verifies that keys can be rotated for perfect forward secrecy
 */
TEST_F(SecurityEncryptionTest, KeyRotationFunctionality) {
    // ARRANGE
    const std::string peer_id = "12D3KooWRotationPeer";
    const std::vector<uint8_t> new_ephemeral_key = generateRandomBytes(32);
    
    EXPECT_CALL(*mock_key_manager_, generateEphemeralKey())
        .WillOnce(Return(new_ephemeral_key));
    EXPECT_CALL(*mock_noise_, reset());
    EXPECT_CALL(*mock_noise_, initialize(_)); // Re-initialize with new key
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT
    auto result = security_manager->rotateKeys(peer_id, test_remote_public_key_);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
}

/**
 * Test: Certificate-based peer identity verification
 * Verifies that peer certificates are properly validated
 */
TEST_F(SecurityEncryptionTest, PeerIdentityVerificationWithCertificates) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCertPeer";
    const std::vector<uint8_t> peer_certificate = generateRandomBytes(256);
    
    EXPECT_CALL(*mock_cert_validator_, validatePeerCertificate(peer_id, peer_certificate))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_cert_validator_, extractPeerIdFromCertificate(peer_certificate))
        .WillOnce(Return(peer_id));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_, mock_cert_validator_);
    
    // ACT
    auto result = security_manager->validatePeerIdentity(peer_id, peer_certificate);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(security_manager->isPeerVerified(peer_id));
}

/**
 * Test: Invalid certificate rejection
 * Verifies that invalid certificates are properly rejected
 */
TEST_F(SecurityEncryptionTest, InvalidCertificateRejection) {
    // ARRANGE
    const std::string peer_id = "12D3KooWInvalidCert";
    const std::vector<uint8_t> invalid_certificate = generateRandomBytes(64); // Too small
    
    EXPECT_CALL(*mock_cert_validator_, validatePeerCertificate(peer_id, invalid_certificate))
        .WillOnce(Return(false));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_, mock_cert_validator_);
    
    // ACT
    auto result = security_manager->validatePeerIdentity(peer_id, invalid_certificate);
    
    // ASSERT
    EXPECT_EQ(result, ErrorCode::AuthenticationFailed);
    EXPECT_FALSE(security_manager->isPeerVerified(peer_id));
}

/**
 * Test: Self-signed certificate generation
 * Verifies that self-signed certificates can be generated for the local peer
 */
TEST_F(SecurityEncryptionTest, SelfSignedCertificateGeneration) {
    // ARRANGE
    const std::vector<uint8_t> self_signed_cert = generateRandomBytes(256);
    
    EXPECT_CALL(*mock_noise_, getPublicKey())
        .WillOnce(Return(test_public_key_));
    EXPECT_CALL(*mock_cert_validator_, generateSelfSignedCertificate(test_public_key_))
        .WillOnce(Return(self_signed_cert));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_, mock_cert_validator_);
    
    // ACT
    auto certificate = security_manager->generateSelfSignedCertificate();
    
    // ASSERT
    EXPECT_EQ(certificate, self_signed_cert);
    EXPECT_GT(certificate.size(), 0);
}

/**
 * Test: Multiple concurrent secure sessions
 * Verifies that multiple secure sessions can be maintained simultaneously
 */
TEST_F(SecurityEncryptionTest, MultipleConcurrentSecureSessions) {
    // ARRANGE
    const std::vector<std::string> peer_ids = {
        "12D3KooWPeer1", "12D3KooWPeer2", "12D3KooWPeer3"
    };
    
    for (const auto& peer_id : peer_ids) {
        EXPECT_CALL(*mock_noise_, performHandshake(_))
            .WillOnce(Return(true));
    }
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillRepeatedly(Return(true));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT
    std::vector<ErrorCode> results;
    for (const auto& peer_id : peer_ids) {
        results.push_back(security_manager->performHandshake(peer_id, test_remote_public_key_));
    }
    
    // ASSERT
    for (const auto& result : results) {
        EXPECT_EQ(result, ErrorCode::Success);
    }
    
    for (const auto& peer_id : peer_ids) {
        EXPECT_TRUE(security_manager->hasSecureSession(peer_id));
    }
}

/**
 * Test: Session cleanup and resource management
 * Verifies that secure sessions are properly cleaned up when peers disconnect
 */
TEST_F(SecurityEncryptionTest, SessionCleanupAndResourceManagement) {
    // ARRANGE
    const std::string peer_id = "12D3KooWCleanupPeer";
    
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, reset()); // Session cleanup
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // Establish secure session
    auto result = security_manager->performHandshake(peer_id, test_remote_public_key_);
    EXPECT_EQ(result, ErrorCode::Success);
    EXPECT_TRUE(security_manager->hasSecureSession(peer_id));
    
    // ACT
    security_manager->cleanupSession(peer_id);
    
    // ASSERT
    EXPECT_FALSE(security_manager->hasSecureSession(peer_id));
    EXPECT_FALSE(security_manager->isHandshakeComplete(peer_id));
}

/**
 * Test: Security configuration validation
 * Verifies that security configuration parameters are properly validated
 */
TEST_F(SecurityEncryptionTest, SecurityConfigurationValidation) {
    // Test invalid Noise pattern
    SecurityConfig invalid_config = security_config_;
    invalid_config.noise_pattern = "InvalidPattern";
    
    EXPECT_THROW({
        auto security_manager = std::make_unique<P2PSecurityManager>(invalid_config, mock_noise_, mock_key_manager_);
    }, std::invalid_argument);
    
    // Test valid configuration
    EXPECT_NO_THROW({
        auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    });
}

/**
 * Test: Perfect forward secrecy key rotation
 * Verifies that key rotation maintains perfect forward secrecy
 */
TEST_F(SecurityEncryptionTest, PerfectForwardSecrecyKeyRotation) {
    // ARRANGE
    const std::string peer_id = "12D3KooWPFSPeer";
    const std::vector<uint8_t> old_session_key = generateRandomBytes(32);
    const std::vector<uint8_t> new_session_key = generateRandomBytes(32);
    
    // First handshake
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, exportSessionKey())
        .WillOnce(Return(old_session_key));
    
    // Key rotation
    EXPECT_CALL(*mock_key_manager_, generateEphemeralKey())
        .WillOnce(Return(generateRandomBytes(32)));
    EXPECT_CALL(*mock_noise_, reset());
    EXPECT_CALL(*mock_noise_, initialize(_));
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, exportSessionKey())
        .WillOnce(Return(new_session_key));
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    
    // ACT
    security_manager->performHandshake(peer_id, test_remote_public_key_);
    auto old_key = security_manager->getCurrentSessionKey(peer_id);
    
    security_manager->rotateKeys(peer_id, test_remote_public_key_);
    auto new_key = security_manager->getCurrentSessionKey(peer_id);
    
    // ASSERT
    EXPECT_EQ(old_key, old_session_key);
    EXPECT_EQ(new_key, new_session_key);
    EXPECT_NE(old_key, new_key); // Keys should be different
}

/**
 * Test: Security metrics and monitoring
 * Verifies that security metrics are properly tracked and reported
 */
TEST_F(SecurityEncryptionTest, SecurityMetricsAndMonitoring) {
    // ARRANGE
    const std::string peer_id = "12D3KooWMetricsPeer";
    const std::vector<uint8_t> test_data = generateRandomBytes(1024);
    
    EXPECT_CALL(*mock_noise_, performHandshake(test_remote_public_key_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_noise_, isHandshakeComplete())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_noise_, encrypt(_))
        .Times(5); // Multiple encryption operations
    
    auto security_manager = std::make_unique<P2PSecurityManager>(security_config_, mock_noise_, mock_key_manager_);
    security_manager->performHandshake(peer_id, test_remote_public_key_);
    
    // ACT
    for (int i = 0; i < 5; i++) {
        security_manager->encryptData(peer_id, test_data);
    }
    
    // ASSERT
    auto metrics = security_manager->getSecurityMetrics(peer_id);
    EXPECT_EQ(metrics.handshakes_completed, 1);
    EXPECT_EQ(metrics.messages_encrypted, 5);
    EXPECT_EQ(metrics.messages_decrypted, 0);
    EXPECT_GT(metrics.bytes_encrypted, 0);
    EXPECT_EQ(metrics.encryption_errors, 0);
}