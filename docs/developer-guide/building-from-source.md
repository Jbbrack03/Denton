# Building Sudachi with Multiplayer from Source

## Overview

This guide provides comprehensive instructions for building Sudachi with multiplayer support from source code. The multiplayer system is integrated into the main Sudachi build process and can be enabled/disabled via CMake options.

## Prerequisites

### System Requirements

#### Windows
- **Visual Studio 2019** or **Visual Studio 2022** (Community, Professional, or Enterprise)
- **Windows SDK 10.0.19041.0** or later (for WinRT APIs)
- **Git for Windows** 2.30 or later
- **CMake** 3.15 or later
- **vcpkg** (managed automatically by Sudachi build system)

#### Linux
- **GCC 9** or **Clang 10** or later with C++17 support
- **CMake** 3.15 or later
- **Git** 2.20 or later
- **pkg-config** and build essentials

#### macOS
- **Xcode 12** or later (for macOS 11.0 SDK)
- **Homebrew** package manager
- **CMake** 3.15 or later
- **Git** 2.20 or later

#### Android
- **Android Studio 4.2** or later
- **Android NDK 23** or later
- **Android SDK API 26** or later
- **Gradle 7.0** or later

### Required Dependencies

The Sudachi build system automatically handles most dependencies through vcpkg, but some platform-specific dependencies may need manual installation:

#### Core Dependencies (Auto-managed)
- **Boost** (system, filesystem, thread, asio)
- **OpenSSL** (for TLS/SSL support)
- **spdlog** (logging framework)
- **nlohmann-json** (JSON parsing)
- **websocketpp** (WebSocket client)

#### Platform-Specific Dependencies

**Linux Additional Packages**:
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake git pkg-config \
    libboost-all-dev libssl-dev libspdlog-dev \
    libnl-3-dev libnl-genl-3-dev \
    hostapd wpa-supplicant iw

# Fedora/RHEL
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git pkgconfig boost-devel openssl-devel \
    spdlog-devel nl3-devel hostapd wpa_supplicant iw

# Arch Linux
sudo pacman -S base-devel cmake git boost openssl spdlog \
    libnl hostapd wpa_supplicant iw
```

**macOS Additional Packages**:
```bash
# Install Homebrew if not present
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ninja boost openssl spdlog
```

## Source Code Setup

### 1. Clone the Repository

```bash
# Clone main repository
git clone https://github.com/sudachi-emulator/sudachi.git
cd sudachi

# Initialize submodules
git submodule update --init --recursive
```

### 2. Verify Multiplayer Components

Ensure the multiplayer system files are present:

```bash
# Check for multiplayer directory structure
ls -la src/core/multiplayer/
ls -la src/core/multiplayer/common/
ls -la src/core/multiplayer/model_a/
ls -la src/core/multiplayer/model_b/

# Verify key multiplayer files
ls src/core/multiplayer/common/error_codes.h
ls src/core/multiplayer/ldn_service_bridge.cpp
ls src/core/multiplayer/backend_factory.cpp
```

### 3. Check External Dependencies

```bash
# mDNS library should be present
ls externals/mdns/mdns.h

# vcpkg configuration
cat sudachi/vcpkg.json | grep -A 5 -B 5 "websocketpp\|spdlog\|openssl"
```

## Build Configuration

### CMake Options

The multiplayer system can be configured with the following CMake options:

```cmake
# Enable/disable multiplayer support (default: ON)
-DENABLE_MULTIPLAYER=ON

# Enable/disable specific multiplayer models
-DENABLE_MULTIPLAYER_MODEL_A=ON  # Internet multiplayer
-DENABLE_MULTIPLAYER_MODEL_B=ON  # Offline multiplayer

# Build multiplayer tests (default: OFF unless SUDACHI_TESTS=ON)
-DENABLE_MULTIPLAYER_TESTS=ON

# Use bundled vcpkg (recommended for Windows)
-DSUDACHI_USE_BUNDLED_VCPKG=ON

# Enable additional logging for debugging
-DMULTIPLAYER_DEBUG_LOGGING=ON
```

### Platform-Specific Configuration

#### Windows Configuration
```cmake
# Use static linking for better portability
-DSUDACHI_USE_BUNDLED_VCPKG=ON
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_GENERATOR="Visual Studio 16 2019"
-DCMAKE_GENERATOR_PLATFORM=x64

# Enable Windows-specific features
-DENABLE_MULTIPLAYER_MODEL_B=ON
-DMULTIPLAYER_WINDOWS_WINRT=ON
```

#### Linux Configuration
```cmake
# Use system packages where possible
-DSUDACHI_USE_BUNDLED_VCPKG=OFF
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_CXX_FLAGS="-march=native"

# Enable Linux-specific networking
-DENABLE_MULTIPLAYER_MODEL_B=ON
-DMULTIPLAYER_LINUX_HOSTAPD=ON
```

#### macOS Configuration
```cmake
# Use Homebrew dependencies
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
-DCMAKE_PREFIX_PATH="/opt/homebrew"

# Limited Model B support on macOS
-DENABLE_MULTIPLAYER_MODEL_B=ON
-DMULTIPLAYER_MACOS_LIMITED=ON
```

#### Android Configuration
```cmake
# Android-specific settings
-DANDROID_ABI=arm64-v8a
-DANDROID_PLATFORM=android-26
-DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake

# Enable Android Wi-Fi Direct
-DENABLE_MULTIPLAYER_MODEL_B=ON
-DMULTIPLAYER_ANDROID_WIFIDIRECT=ON
```

## Build Instructions

### Windows Build

#### Using Visual Studio IDE

1. **Open Visual Studio**
2. **Clone Repository**: File → Clone Repository → Enter Sudachi URL
3. **Configure CMake**: Project → CMake Settings
4. **Set CMake Variables**:
   ```
   SUDACHI_USE_BUNDLED_VCPKG = True
   ENABLE_MULTIPLAYER = True
   CMAKE_BUILD_TYPE = Release
   ```
5. **Build**: Build → Build All

#### Using Command Line

```cmd
REM Create build directory
mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 16 2019" -A x64 ^
    -DSUDACHI_USE_BUNDLED_VCPKG=ON ^
    -DENABLE_MULTIPLAYER=ON ^
    -DCMAKE_BUILD_TYPE=Release

REM Build project
cmake --build . --config Release --parallel

REM Optional: Run tests
ctest -C Release --output-on-failure
```

### Linux Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_MULTIPLAYER=ON \
    -DCMAKE_CXX_FLAGS="-march=native" \
    -DSUDACHI_ENABLE_LTO=ON \
    -GNinja

# Build with Ninja (faster than make)
ninja

# Optional: Run tests
ctest --output-on-failure --parallel $(nproc)
```

### macOS Build

```bash
# Install dependencies
brew install cmake ninja boost openssl spdlog

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_MULTIPLAYER=ON \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
    -GNinja

# Build
ninja

# Optional: Run tests
ctest --output-on-failure --parallel $(sysctl -n hw.ncpu)
```

### Android Build

#### Using Android Studio

1. **Open Android Studio**
2. **Open Project**: Select `src/android` directory
3. **Sync Project**: Let Gradle sync dependencies
4. **Configure Build Variants**: Build → Select Build Variant → release
5. **Build APK**: Build → Build Bundle(s) / APK(s) → Build APK(s)

#### Using Command Line

```bash
# Navigate to Android project
cd src/android

# Clean previous builds
./gradlew clean

# Build debug APK
./gradlew assembleDebug

# Build release APK (requires signing configuration)
./gradlew assembleRelease

# Install on connected device
./gradlew installDebug
```

## Troubleshooting Build Issues

### Common CMake Issues

#### "vcpkg not found" Error
```bash
# Windows: Ensure vcpkg is properly initialized
cd sudachi
git submodule update --init --recursive
.\externals\vcpkg\bootstrap-vcpkg.bat

# Linux/macOS: Bootstrap vcpkg
git submodule update --init --recursive
./externals/vcpkg/bootstrap-vcpkg.sh
```

#### "Boost not found" Error
```cmake
# Add to CMake configuration
-DBoost_USE_STATIC_LIBS=ON
-DBOOST_ROOT=/path/to/boost
```

#### "OpenSSL not found" Error (Windows)
```cmake
# Use vcpkg OpenSSL
-DOPENSSL_ROOT_DIR=externals/vcpkg/installed/x64-windows
```

### Platform-Specific Issues

#### Windows Issues

**"Windows SDK not found"**:
```cmd
# Install Windows SDK via Visual Studio Installer
# Or download directly from Microsoft
winget install Microsoft.WindowsSDK.10.0.19041
```

**"WinRT headers not found"**:
```cmake
# Ensure proper Windows SDK version
-DCMAKE_SYSTEM_VERSION=10.0.19041.0
```

**"MSBuild errors"**:
```cmd
# Clean and rebuild
msbuild sudachi.sln /t:Clean
msbuild sudachi.sln /t:Rebuild /p:Configuration=Release
```

#### Linux Issues

**"Boost version too old"**:
```bash
# Install newer Boost from source
wget https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz
tar xzf boost_1_78_0.tar.gz
cd boost_1_78_0
./bootstrap.sh --prefix=/usr/local
sudo ./b2 install
```

**"Permission denied" network operations**:
```bash
# Add user to netdev group
sudo usermod -a -G netdev $USER
# Log out and back in
```

**"hostapd not found"**:
```bash
# Install hostapd and related tools
sudo apt install hostapd wpa-supplicant iw
# Or equivalent for your distribution
```

#### macOS Issues

**"Xcode command line tools not found"**:
```bash
xcode-select --install
```

**"Framework not found"**:
```cmake
# Add framework search paths
-DCMAKE_FRAMEWORK_PATH="/System/Library/Frameworks;/Library/Frameworks"
```

**"Homebrew libraries not found"**:
```bash
# Set proper paths
export CMAKE_PREFIX_PATH="$(brew --prefix)"
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig"
```

#### Android Issues

**"NDK not found"**:
```bash
# Set NDK path in local.properties
echo "ndk.dir=/path/to/android-ndk" >> src/android/local.properties
```

**"SDK path not found"**:
```bash
# Set SDK path in local.properties
echo "sdk.dir=/path/to/android-sdk" >> src/android/local.properties
```

**"JNI build failures"**:
```gradle
// In app/build.gradle, ensure proper NDK configuration
android {
    ndkVersion "23.1.7779620"
    
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

## Development Builds

### Debug Configuration

For development and debugging, use debug builds with additional logging:

```cmake
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_MULTIPLAYER=ON \
    -DMULTIPLAYER_DEBUG_LOGGING=ON \
    -DENABLE_MULTIPLAYER_TESTS=ON \
    -DSUDACHI_TESTS=ON
```

### Sanitizer Builds

For finding memory issues and undefined behavior:

```cmake
# Address Sanitizer
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"

# Thread Sanitizer
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
```

### Profile-Guided Optimization

For maximum performance builds:

```cmake
# Step 1: Build with profiling
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fprofile-generate" \
    -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"

# Build and run with typical workload
ninja && ./bin/sudachi

# Step 2: Rebuild with profile data
cmake .. \
    -DCMAKE_CXX_FLAGS="-fprofile-use" \
    -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"
ninja
```

## Testing the Build

### Unit Tests

```bash
# Run all tests
ctest --output-on-failure

# Run only multiplayer tests
ctest -R "multiplayer" --output-on-failure

# Run specific test categories
ctest -R "test_.*_multiplayer" --output-on-failure
```

### Integration Tests

```bash
# Start Docker test environment (if available)
cd tests/docker
docker-compose up -d

# Run integration tests
ctest -R "integration" --output-on-failure

# Clean up
docker-compose down
```

### Manual Testing

1. **Launch Sudachi**: Run the built executable
2. **Check Multiplayer Settings**: Settings → Multiplayer
3. **Verify Network Test**: Use built-in network connectivity test
4. **Test with Game**: Load a supported multiplayer game
5. **Test Both Modes**: Try Internet and Offline multiplayer

## Performance Optimization

### Compiler Optimizations

```cmake
# GCC/Clang optimizations
-DCMAKE_CXX_FLAGS="-O3 -march=native -flto"

# MSVC optimizations
-DCMAKE_CXX_FLAGS="/O2 /GL /arch:AVX2"
```

### Link-Time Optimization

```cmake
# Enable LTO for smaller, faster binaries
-DSUDACHI_ENABLE_LTO=ON
-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Memory Optimization

```cmake
# Reduce memory usage during compilation
-DCMAKE_CXX_FLAGS="-fno-keep-inline-dllexport"  # Windows
-DCMAKE_CXX_FLAGS="-ffunction-sections -fdata-sections"  # Linux
```

## Packaging and Distribution

### Windows Packaging

```cmd
REM Install to staging directory
cmake --build . --target install --config Release

REM Create installer package (if NSIS configured)
cpack -G NSIS

REM Create ZIP package
cpack -G ZIP
```

### Linux Packaging

```bash
# Install to staging directory
DESTDIR=./staging ninja install

# Create DEB package (Ubuntu/Debian)
cpack -G DEB

# Create RPM package (Fedora/RHEL)
cpack -G RPM

# Create AppImage (universal)
# Requires additional AppImage tools
```

### macOS Packaging

```bash
# Create app bundle
ninja install

# Create DMG
cpack -G DragNDrop

# Code signing (requires developer certificate)
codesign --deep --force --verify --verbose --sign "Developer ID" Sudachi.app
```

### Android Packaging

```bash
# Create signed APK
cd src/android
./gradlew assembleRelease

# Create AAB for Play Store
./gradlew bundleRelease
```

## Continuous Integration

### GitHub Actions

Example workflow for automated builds:

```yaml
name: Build Sudachi Multiplayer

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [windows-latest, ubuntu-latest, macos-latest]
        
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
        
    - name: Setup CMake
      uses: jwlawson/actions-setup-cmake@v1.13
      
    - name: Configure
      run: |
        cmake -B build \
          -DENABLE_MULTIPLAYER=ON \
          -DCMAKE_BUILD_TYPE=Release
          
    - name: Build
      run: cmake --build build --config Release
      
    - name: Test
      run: |
        cd build
        ctest --output-on-failure
```

## Contributing to Multiplayer Development

### Development Environment Setup

1. **Fork the repository** on GitHub
2. **Clone your fork** locally
3. **Create a feature branch** for your changes
4. **Set up pre-commit hooks** (if available)
5. **Build with tests enabled**
6. **Run full test suite** before committing

### Code Style Guidelines

- **Follow existing code style** in the multiplayer modules
- **Use clang-format** for consistent formatting
- **Add documentation** for public APIs
- **Write unit tests** for new functionality
- **Update integration tests** for significant changes

### Debugging Multiplayer Issues

#### Enable Debug Logging

```cpp
// In your development build
#define MULTIPLAYER_DEBUG_LOGGING 1

// Or via CMake
-DMULTIPLAYER_DEBUG_LOGGING=ON
```

#### Use Network Analysis Tools

```bash
# Wireshark for packet analysis
wireshark

# netstat for connection monitoring
netstat -an | grep 7777

# tcpdump for command-line packet capture
sudo tcpdump -i any port 7777-7787
```

## Support and Resources

### Documentation
- [Architecture Guide](architecture.md) - System design and components
- [API Reference](api-reference.md) - Complete API documentation
- [Contributing Guide](contributing.md) - How to contribute to development

### Community Support
- **Discord**: [Sudachi Community](https://discord.gg/sudachi)
- **GitHub Issues**: [Report Problems](https://github.com/sudachi-emulator/sudachi/issues)
- **Developer Forums**: [Technical Discussions](https://community.sudachi-emulator.org)

### Professional Support
- **Paid Support**: Available for enterprise/commercial use
- **Custom Development**: Contract development services
- **Security Audits**: Professional security reviews available

---

*This build guide is maintained by the Sudachi development team. For questions or issues not covered here, please join our [Discord community](https://discord.gg/sudachi) or file an issue on [GitHub](https://github.com/sudachi-emulator/sudachi/issues).*