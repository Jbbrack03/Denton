# HLE Integration Test Suite

This suite validates the components that connect the Nintendo Switch LDN HLE service with the emulator's multiplayer system. The tests exercise both real and mocked interactions to ensure that:

- backends can be created through the factory,
- error codes map correctly between subsystems,
- type translations between internal and LDN representations are accurate,
- the service bridge performs expected state transitions and backend switching.

Tests rely on stub implementations where full platform support is unavailable. Individual cases are skipped when a required platform feature is missing.

## Building

```bash
cd sudachi/build
cmake .. -DSUDACHI_TESTS=ON
make hle_integration_tests
```

## Running

```bash
./hle_integration_tests
```

Custom targets such as `make test_backend_interface` or `make test_ldn_bridge` are also available for running specific groups of tests.
