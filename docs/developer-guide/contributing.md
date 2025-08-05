# Contributing to Sudachi Multiplayer

## Welcome Contributors!

Thank you for your interest in contributing to the Sudachi Multiplayer system! This guide will help you get started with contributing code, documentation, testing, and other improvements to the project.

## Quick Start for Contributors

### 1. Set Up Your Development Environment

```bash
# Fork the repository on GitHub, then clone your fork
git clone https://github.com/YOUR_USERNAME/sudachi.git
cd sudachi

# Add upstream remote
git remote add upstream https://github.com/sudachi-emulator/sudachi.git

# Install development dependencies (see Building from Source guide)
# Then build with tests enabled
mkdir build && cd build
cmake .. -DENABLE_MULTIPLAYER=ON -DENABLE_MULTIPLAYER_TESTS=ON -DSUDACHI_TESTS=ON
cmake --build . --config Release
```

### 2. Find Something to Work On

- **Check GitHub Issues**: Look for issues labeled `good first issue` or `multiplayer`
- **Review the TODO comments**: Search for `TODO:` in the multiplayer codebase
- **Test games and report bugs**: Try multiplayer with different games
- **Improve documentation**: Help make guides clearer and more comprehensive

### 3. Make Your First Contribution

1. **Create a feature branch**: `git checkout -b feature/my-improvement`
2. **Make your changes** following our coding standards
3. **Write or update tests** for your changes
4. **Test thoroughly** on your platform
5. **Commit with clear messages** describing your changes
6. **Push and create a pull request**

## How to Contribute

### Types of Contributions

#### 🔧 Code Contributions
- **Bug fixes**: Fix issues in existing multiplayer functionality
- **New features**: Implement planned features from the roadmap
- **Performance improvements**: Optimize networking, reduce latency, improve memory usage
- **Platform support**: Improve compatibility with different operating systems
- **Game compatibility**: Add support for more Nintendo Switch games

#### 📚 Documentation Contributions
- **User guides**: Improve installation, setup, and troubleshooting guides
- **Developer documentation**: Enhance API docs, architecture guides
- **Code comments**: Add clear explanations to complex code sections
- **Examples**: Create sample code and usage examples

#### 🧪 Testing Contributions
- **Test coverage**: Write unit tests, integration tests, and benchmarks
- **Game testing**: Test multiplayer functionality with various games
- **Platform testing**: Test on different operating systems and hardware
- **Performance testing**: Create benchmarks and regression tests

#### 🎨 User Experience Contributions
- **UI improvements**: Enhance the multiplayer configuration interface
- **Error messages**: Make error messages clearer and more actionable
- **Accessibility**: Improve support for users with disabilities
- **Internationalization**: Add support for more languages

## Development Workflow

### Branch Strategy

We use a GitHub Flow-based approach:

```
main (stable)
├── feature/new-feature-name
├── bugfix/fix-description
├── docs/documentation-update
└── test/test-improvement
```

#### Branch Naming Conventions
- **Features**: `feature/brief-description`
- **Bug fixes**: `bugfix/issue-description`
- **Documentation**: `docs/what-is-updated`
- **Tests**: `test/what-is-tested`
- **Refactoring**: `refactor/what-is-refactored`

### Commit Message Guidelines

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
type(scope): brief description

Longer description explaining the change in detail.
Include context about why the change was made.

Fixes #123
```

#### Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `test`: Adding or updating tests
- `refactor`: Code refactoring without behavior changes
- `perf`: Performance improvements
- `style`: Code formatting changes
- `chore`: Build system, dependencies, etc.

#### Scopes
- `multiplayer`: General multiplayer system
- `model-a`: Internet multiplayer (Model A)
- `model-b`: Offline multiplayer (Model B)
- `hle`: HLE nn::ldn interface
- `ui`: User interface
- `config`: Configuration system
- `security`: Security-related changes
- `tests`: Test infrastructure

#### Examples

```bash
# Good commit messages
feat(model-a): add automatic relay fallback when P2P fails
fix(model-b): resolve Android Wi-Fi Direct permission handling
docs(api): update multiplayer backend interface documentation
test(integration): add tests for cross-platform session discovery

# Bad commit messages (avoid these)
fix stuff
updated files
wip
asdf
```

### Code Review Process

#### Creating a Pull Request

1. **Ensure your branch is up to date**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Run the full test suite**:
   ```bash
   cd build
   ctest --output-on-failure
   ```

3. **Create the pull request** with:
   - Clear title describing the change
   - Detailed description of what was changed and why
   - Screenshots/videos for UI changes
   - References to related issues

#### Pull Request Template

```markdown
## Description
Brief description of the changes made.

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Manual testing performed
- [ ] Tested on multiple platforms (specify which)

## Checklist
- [ ] My code follows the project's style guidelines
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works

## Related Issues
Fixes #(issue number)
```

#### Review Process

1. **Automated checks** run on every PR (builds, tests, linting)
2. **Maintainer review** for code quality, design, and correctness
3. **Community review** from other contributors (encouraged)
4. **Testing feedback** from users testing the changes
5. **Approval and merge** by maintainers

## Coding Standards

### C++ Style Guidelines

We follow a consistent style based on the existing Sudachi codebase:

#### Naming Conventions

```cpp
// Classes: PascalCase
class MultiplayerBackend {
public:
    // Public methods: PascalCase
    ResultCode CreateSession(const NetworkConfig& config);
    
    // Private methods: PascalCase
private:
    void HandleInternalError();
    
    // Member variables: snake_case with trailing underscore
    std::unique_ptr<IWebSocketClient> websocket_client_;
    SessionState current_state_;
};

// Free functions: PascalCase
ResultCode InitializeMultiplayerSystem();

// Constants: SCREAMING_SNAKE_CASE
constexpr uint16_t DEFAULT_MULTIPLAYER_PORT = 7777;
constexpr size_t MAX_PLAYERS_PER_SESSION = 8;

// Enums: PascalCase for enum and values
enum class MultiplayerMode {
    Internet,
    Offline,
    Auto
};

// Namespaces: PascalCase
namespace Core::Multiplayer::ModelA {
    // Implementation
}
```

#### Code Formatting

We use clang-format with the project's `.clang-format` configuration:

```bash
# Format a single file
clang-format -i src/core/multiplayer/your_file.cpp

# Format all multiplayer files
find src/core/multiplayer -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

#### Header Guards and Includes

```cpp
// Use #pragma once for header guards
#pragma once

// System includes first
#include <memory>
#include <string>
#include <vector>

// Third-party includes
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

// Project includes
#include "common/common_types.h"
#include "core/multiplayer/error_codes.h"

// Local includes last
#include "multiplayer_backend.h"
```

#### Error Handling

```cpp
// Use Result<T> or ErrorCode for error handling
ResultCode CreateSession(const NetworkConfig& config) {
    if (!ValidateConfig(config)) {
        return ErrorCode::ConfigurationInvalid;
    }
    
    // Implementation
    return ErrorCode::Success;
}

// Use exceptions only for truly exceptional cases
void InitializeCriticalResource() {
    auto resource = CreateResource();
    if (!resource) {
        throw std::runtime_error("Failed to initialize critical resource");
    }
}
```

#### Memory Management

```cpp
// Prefer smart pointers over raw pointers
std::unique_ptr<IMultiplayerBackend> backend;
std::shared_ptr<SessionData> session_data;

// Use RAII for resource management
class NetworkConnection {
public:
    NetworkConnection(const std::string& address, uint16_t port);
    ~NetworkConnection(); // Automatically closes connection
    
    // No copy, allow move
    NetworkConnection(const NetworkConnection&) = delete;
    NetworkConnection& operator=(const NetworkConnection&) = delete;
    NetworkConnection(NetworkConnection&&) = default;
    NetworkConnection& operator=(NetworkConnection&&) = default;
};
```

#### Threading and Concurrency

```cpp
// Use std::mutex for synchronization
class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::queue<Item> queue_;
    std::condition_variable condition_;
    
public:
    void Push(Item item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        condition_.notify_one();
    }
    
    bool TryPop(Item& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};
```

### Testing Standards

#### Unit Test Guidelines

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/multiplayer/ldn_service_bridge.h"

class LdnServiceBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
        bridge = std::make_unique<LdnServiceBridge>();
    }
    
    void TearDown() override {
        // Clean up resources
        bridge.reset();
    }
    
    std::unique_ptr<LdnServiceBridge> bridge;
};

TEST_F(LdnServiceBridgeTest, InitializeReturnsSuccessWhenConfigured) {
    // Arrange
    // (Setup is done in SetUp())
    
    // Act
    auto result = bridge->Initialize();
    
    // Assert
    EXPECT_EQ(result, Core::Multiplayer::ErrorCode::Success);
}

TEST_F(LdnServiceBridgeTest, CreateNetworkFailsWithInvalidConfig) {
    // Arrange
    NetworkConfig invalid_config{};
    invalid_config.max_players = 0; // Invalid
    
    // Act
    auto result = bridge->CreateNetwork(invalid_config);
    
    // Assert
    EXPECT_EQ(result, Core::Multiplayer::ErrorCode::ConfigurationInvalid);
}
```

#### Integration Test Guidelines

```cpp
class MultiplayerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start test servers if needed
        StartTestRoomServer();
        StartTestRelayServer();
    }
    
    void TearDown() override {
        // Clean up test environment
        StopTestServers();
    }
    
private:
    void StartTestRoomServer();
    void StopTestServers();
};

TEST_F(MultiplayerIntegrationTest, FullSessionCreationAndJoinFlow) {
    // This test verifies the complete flow from session creation to joining
    
    // Create host
    auto host_bridge = CreateLdnServiceBridge();
    NetworkConfig config = CreateValidNetworkConfig();
    
    ASSERT_EQ(host_bridge->Initialize(), ErrorCode::Success);
    ASSERT_EQ(host_bridge->CreateNetwork(config), ErrorCode::Success);
    
    // Wait for session to be advertised
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Create client
    auto client_bridge = CreateLdnServiceBridge();
    ASSERT_EQ(client_bridge->Initialize(), ErrorCode::Success);
    ASSERT_EQ(client_bridge->Scan(), ErrorCode::Success);
    
    // Wait for discovery
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Verify session was discovered
    auto discovered = client_bridge->GetNetworkInfo();
    ASSERT_FALSE(discovered.empty());
    
    // Join session
    EXPECT_EQ(client_bridge->Connect(discovered[0]), ErrorCode::Success);
    
    // Verify connection
    EXPECT_EQ(client_bridge->GetState(), SessionState::Connected);
}
```

### Documentation Standards

#### Code Documentation

```cpp
/**
 * Brief description of the class.
 * 
 * Longer description explaining the purpose, usage patterns,
 * and any important implementation details.
 * 
 * @example
 * ```cpp
 * auto backend = BackendFactory::CreateBackend(MultiplayerMode::Internet);
 * auto result = backend->Initialize(config);
 * if (result == ErrorCode::Success) {
 *     // Use backend
 * }
 * ```
 */
class IMultiplayerBackend {
public:
    /**
     * Initialize the multiplayer backend with the given configuration.
     * 
     * This method must be called before any other backend operations.
     * It sets up internal state, validates configuration, and prepares
     * the backend for session management.
     * 
     * @param config Configuration parameters for the backend
     * @return Success on successful initialization, or appropriate error code
     * 
     * @note This method is not thread-safe and should only be called once
     * @see BackendConfig for configuration options
     */
    virtual ResultCode Initialize(const BackendConfig& config) = 0;
    
    /**
     * Create a new multiplayer session.
     * 
     * @param session_config Configuration for the new session
     * @return Success if session was created, or error code on failure
     * 
     * @pre Backend must be initialized
     * @post On success, backend enters hosting state
     * 
     * @throws std::invalid_argument if session_config is invalid
     */
    virtual ResultCode CreateSession(const NetworkConfig& session_config) = 0;
};
```

#### API Documentation

- **Always document public APIs** with detailed descriptions
- **Include parameter descriptions** and return value meanings
- **Provide usage examples** for complex APIs
- **Document preconditions and postconditions**
- **Explain thread safety guarantees**
- **Note any exceptions that may be thrown**

## Specialized Contribution Areas

### Multiplayer Game Compatibility

#### Testing New Games

1. **Choose a game** that supports local wireless multiplayer
2. **Test both multiplayer modes** (Internet and Offline)
3. **Document your findings**:
   - Game title and version
   - Multiplayer features tested
   - Performance characteristics
   - Any issues encountered
4. **Update the supported games list**
5. **Create game-specific tests** if needed

#### Game Compatibility Test Template

```cpp
// tests/integration/game_specific/test_your_game.cpp
#include "tests/integration/game_specific_test_framework.h"

class YourGameMultiplayerTest : public GameSpecificTestFramework {
protected:
    void SetUp() override {
        GameSpecificTestFramework::SetUp();
        LoadGameROM("path/to/your_game.nsp");
    }
    
    GameId GetGameId() const override {
        return 0x0100000000000001; // Your game's ID
    }
    
    std::string GetGameName() const override {
        return "Your Game Title";
    }
};

TEST_F(YourGameMultiplayerTest, LocalWirelessSessionCreation) {
    // Test session creation through game interface
    ASSERT_TRUE(StartGame());
    ASSERT_TRUE(NavigateToMultiplayerMenu());
    ASSERT_TRUE(CreateLocalSession());
    
    // Verify session appears in discovery
    auto sessions = DiscoverSessions();
    EXPECT_FALSE(sessions.empty());
    EXPECT_EQ(sessions[0].game_id, GetGameId());
}
```

### Platform-Specific Development

#### Android Development

Areas that need contribution:
- **JNI wrapper improvements**: Better error handling, more robust lifecycle management
- **Permission handling**: Streamlined permission request flow
- **UI integration**: Better Android-style UI components
- **Performance optimization**: Reduce battery usage, optimize for mobile hardware

```cpp
// Example Android contribution
// src/core/multiplayer/model_b/platform/android/wifi_direct_wrapper.cpp

ResultCode WiFiDirectWrapper::RequestPermissions() {
    // Check if all required permissions are granted
    std::vector<std::string> missing_permissions;
    
    if (!HasPermission("android.permission.ACCESS_FINE_LOCATION")) {
        missing_permissions.push_back("android.permission.ACCESS_FINE_LOCATION");
    }
    
    if (android_api_level >= 33 && !HasPermission("android.permission.NEARBY_WIFI_DEVICES")) {
        missing_permissions.push_back("android.permission.NEARBY_WIFI_DEVICES");
    }
    
    if (!missing_permissions.empty()) {
        return RequestPermissionsFromSystem(missing_permissions);
    }
    
    return ErrorCode::Success;
}
```

#### Windows Development

Areas that need contribution:
- **WinRT API improvements**: Better Mobile Hotspot integration
- **Firewall integration**: Automatic firewall rule management
- **UPnP improvements**: More robust port forwarding
- **Registry integration**: Store configuration in Windows registry

#### Linux Development

Areas that need contribution:
- **Distribution support**: Better support for various Linux distributions
- **NetworkManager integration**: Work better with NetworkManager
- **systemd integration**: Proper service integration
- **AppImage/Flatpak support**: Distribution-agnostic packaging

### Security Contributions

#### Security Testing

```cpp
// Example security test
TEST(NetworkSecurityTest, RejectsOversizedPackets) {
    NetworkInputValidator validator;
    
    // Create oversized packet
    std::vector<uint8_t> oversized_packet(1024 * 1024, 0xFF); // 1MB packet
    
    // Should reject packet that's too large
    EXPECT_FALSE(validator.ValidatePacket(oversized_packet));
}

TEST(NetworkSecurityTest, RateLimitingWorks) {
    ClientRateManager rate_manager;
    std::string client_id = "test_client";
    
    // Should allow initial requests
    EXPECT_TRUE(rate_manager.AllowRequest(client_id, RateLimitType::SessionCreation));
    
    // Should block excessive requests
    for (int i = 0; i < 100; ++i) {
        rate_manager.RecordRequest(client_id, RateLimitType::SessionCreation);
    }
    
    EXPECT_FALSE(rate_manager.AllowRequest(client_id, RateLimitType::SessionCreation));
}
```

#### Security Review Areas

- **Input validation**: Ensure all network input is properly validated
- **Rate limiting**: Prevent abuse and DoS attacks
- **Encryption**: Verify all communications are properly encrypted
- **Authentication**: Secure session joining and player authentication
- **Memory safety**: Prevent buffer overflows and use-after-free bugs

### Performance Optimization

#### Profiling and Benchmarking

```cpp
// Example benchmark contribution
#include <benchmark/benchmark.h>
#include "core/multiplayer/common/packet_protocol.h"

static void BM_PacketSerialization(benchmark::State& state) {
    NetworkPacket packet;
    packet.header.packet_type = PacketType::GAME_DATA;
    packet.payload = std::vector<uint8_t>(state.range(0), 0xAB);
    
    for (auto _ : state) {
        auto serialized = SerializePacket(packet);
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_PacketSerialization)->Range(64, 8192);
```

#### Areas for Optimization

- **Packet processing**: Optimize serialization/deserialization
- **Memory allocation**: Reduce allocations in hot paths
- **Network stack**: Optimize socket handling and buffering
- **Threading**: Improve thread pool usage and synchronization
- **Caching**: Add intelligent caching for frequently accessed data

## Release Process

### Version Numbering

We follow [Semantic Versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH** (e.g., 1.2.3)
- **MAJOR**: Breaking changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, backward compatible

### Release Checklist

For maintainers preparing releases:

1. **Code freeze** and final testing
2. **Update version numbers** in all relevant files
3. **Update CHANGELOG.md** with release notes
4. **Run full test suite** on all platforms
5. **Create release branch** and tag
6. **Build release packages** for all platforms
7. **Update documentation** to reflect new features
8. **Announce release** to community

## Community Guidelines

### Code of Conduct

We follow the [Contributor Covenant](https://www.contributor-covenant.org/) code of conduct. In summary:

- **Be respectful** and inclusive to all contributors
- **Focus on constructive feedback** in code reviews
- **Help newcomers** get started with contributing
- **Report inappropriate behavior** to maintainers
- **Assume good intentions** in communications

### Communication Channels

- **GitHub Issues**: Bug reports, feature requests, technical discussions
- **GitHub Discussions**: General questions, ideas, community discussions
- **Discord**: Real-time chat, getting help, coordination
- **Email**: Security issues, private matters (security@sudachi-emulator.org)

### Getting Help

#### For New Contributors

1. **Join our Discord** and introduce yourself in #contributors
2. **Read this contributing guide** thoroughly
3. **Start with a small issue** labeled `good first issue`
4. **Ask questions** - we're here to help!
5. **Don't be afraid to make mistakes** - everyone learns

#### For Experienced Contributors

1. **Help review pull requests** from other contributors
2. **Mentor new contributors** by answering questions
3. **Propose architectural improvements** via GitHub Discussions
4. **Lead feature development** for larger initiatives

### Recognition

We value all contributions and recognize contributors in several ways:

- **Contributors file**: All contributors are listed in CONTRIBUTORS.md
- **Release notes**: Significant contributions are highlighted in releases
- **Special roles**: Active contributors may be invited to become maintainers
- **Conference talks**: We promote contributor work at conferences and events

## Troubleshooting Development Issues

### Common Development Problems

#### Build Issues

```bash
# Clean build directory
rm -rf build
mkdir build && cd build

# Reconfigure from scratch
cmake .. -DENABLE_MULTIPLAYER=ON -DENABLE_MULTIPLAYER_TESTS=ON

# Force vcpkg rebuild
cmake .. -DSUDACHI_USE_BUNDLED_VCPKG=ON -DVCPKG_FORCE_REBUILD=ON
```

#### Test Failures

```bash
# Run tests with verbose output
ctest --output-on-failure --verbose

# Run specific failing test
ctest -R "failing_test_name" --output-on-failure

# Debug test in GDB/LLDB
gdb ./bin/test_multiplayer_system
```

#### Git Issues

```bash
# Fix merge conflicts
git status
git add resolved_files
git commit

# Rebase your branch on latest main
git fetch upstream
git rebase upstream/main

# Force push after rebase (use with caution)
git push --force-with-lease origin your-branch
```

### Getting Help with Development

1. **Check existing issues** and discussions first
2. **Search the codebase** for similar patterns or solutions
3. **Ask in Discord #contributors** channel
4. **Create a draft pull request** for early feedback
5. **Reach out to maintainers** for architectural questions

## Resources for Contributors

### Learning Resources

- **C++ Best Practices**: [Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- **Network Programming**: [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- **Testing**: [Google Test Documentation](https://google.github.io/googletest/)
- **Git Workflow**: [Atlassian Git Tutorials](https://www.atlassian.com/git/tutorials)

### Tools and Setup

#### Recommended Development Tools

- **IDE**: Visual Studio Code, CLion, Visual Studio, Qt Creator
- **Debugger**: GDB, LLDB, Visual Studio Debugger
- **Profiler**: Valgrind, Intel VTune, Visual Studio Profiler
- **Network Analysis**: Wireshark, tcpdump, netstat
- **Code Quality**: clang-tidy, cppcheck, sanitizers

#### Editor Configuration

**VS Code settings.json**:
```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_fallbackStyle": "file"
}
```

## Thank You!

Your contributions make Sudachi Multiplayer better for everyone. Whether you're fixing a typo in documentation, adding support for a new game, or implementing a major new feature, every contribution is valuable and appreciated.

**Happy contributing!** 🎮

---

*For questions about contributing, join our [Discord community](https://discord.gg/sudachi) or check out our [GitHub Discussions](https://github.com/sudachi-emulator/sudachi/discussions).*