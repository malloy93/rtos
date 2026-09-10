# Repository structure

The repository separates portable kernel code, STM32F429I-DISC1 platform code,
the demo application, tests, and development tooling.

- `kernel/include/rtos/` contains the public RTOS headers.
- `kernel/src/` contains the RTOS implementation and context-switch assembly.
- `platform/stm32f429i_disc1/` contains board headers, STM32 HAL glue, startup
  code, linker scripts, and the OpenOCD configuration.
- `apps/demo/` contains the current firmware application.
- `tests/unit/` contains host-side unit tests; `tests/stubs/` contains their
  hardware substitutes.
- `cmake/` contains shared firmware build options.
- `tools/` is reserved for development tooling.

Production code includes kernel headers as `<rtos/...>` and board headers as
`<board/...>`. The `firmware-debug` preset builds the demo, kernel, and platform
objects into `rrtos_f429_core.elf` and generates matching HEX, BIN, and MAP
artifacts.

Only STM32F429I-DISC1 platform support is implemented. Additional MCU records
in `kernel/include/rtos/Configuration.hpp` are retained configuration data, not supported
platform directories.
