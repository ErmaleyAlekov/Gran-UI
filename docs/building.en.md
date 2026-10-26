# Gran UI — Building and Integration

[Русский](building.md) · **English**

← [Back to Description](../README.en.md)

## Table of Contents

1. [What You Need](#what-you-need)
2. [Getting the Sources](#getting-the-sources)
3. [Building with Makefile](#building-with-makefile)
4. [Building with CMake](#building-with-cmake)
5. [Shaders](#shaders)
6. [Integration with Your Program](#integration-with-your-program)
7. [Testing](#testing)

## What You Need

| What | Purpose |
|---|---|
| Windows 10 1909+ or Windows 11, a GPU with Direct3D 12 | Window and rendering (without a GPU — WARP software adapter) |
| MinGW-w64 with GCC 11+ and `make` **or** Visual Studio 2022+ (MSVC) and CMake 3.20+ | C++17 compiler |
| [Beresta](https://github.com/ErmaleyAlekov/beresta) | XML parsing and XSD 1.1 schema validation |
| `dxc` from Windows SDK (optional) | Only for rebuilding shaders: precompiled bytecode is already in `src/rhi/shaders_gen/` |

Headers for Direct3D 12, DXGI, and UI Automation are available in both MinGW-w64 and Windows SDK. No other dependencies are required.

## Getting the Sources

The engine is built together with static Beresta, so both repositories are placed side by side:

```bash
git clone https://github.com/ErmaleyAlekov/beresta.git
```

```bash
git clone https://github.com/ErmaleyAlekov/Gran-UI.git
```

The path to Beresta is specified by the `BERESTA_DIR` variable (for Makefile) or `GRAN_BERESTA_DIR` (for CMake). The path can contain spaces.

## Building with Makefile

From the `Gran-UI` directory:

```bash
make beresta BERESTA_DIR=../beresta
```

```bash
make BERESTA_DIR=../beresta
```

The first command builds `libberesta.a` using Beresta's own Makefile, the second — the engine and examples. Header dependencies are tracked automatically (`dep/*.d`).

| Target | What It Does |
|---|---|
| `make` | `bin/libgran.a`, `bin/gran_gallery.exe`, `bin/gran_snapshot.exe`, `bin/gran_chat.exe`, `bin/gran_tests.exe` |
| `make test` | Build and run unit tests |
| `make shaders` | HLSL → DXIL (and SPIR-V with `VULKAN=1`) → `src/rhi/shaders_gen/*.cpp`, requires `dxc` |
| `make beresta` | Build static Beresta in `BERESTA_DIR` |
| `make clean` / `fclean` / `re` | Remove object files / everything built / rebuild |

| Variable | Value |
|---|---|
| `BERESTA_DIR=path` | Beresta directory (default `../Dom lib C++`) |
| `DEBUG=1` | `-O0 -g`, D3D12 debug layer available via `--debug` in examples |
| `VULKAN=1` | Vulkan backend (under development, requires Vulkan SDK and `VULKAN_SDK` variable) |

Cleaning works in CMD/PowerShell and Unix shell.

The built-in schema `schemas/gran-ui.xsd` and theme `schemas/default_theme.xml` are turned into source files `gen/markup/*.cpp` during the build — using the `gran_embed` utility (`tools/embed/`), which is built first.

## Building with CMake

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --config Release
```

```bash
ctest --test-dir build -C Release
```

Beresta is included as a subproject and is built with the same compiler. With MSVC, both the engine and Beresta are built with the static runtime (`/MT`), and the programs are placed in `build/bin`.

| Parameter | Value |
|---|---|
| `-DGRAN_BERESTA_DIR=path` | Directory of Beresta sources |
| `-DGRAN_BUILD_TESTS=ON` | Build unit tests (enabled by default) |
| `-DGRAN_WITH_VULKAN=ON` | Vulkan backend (under development) |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug build (`GRAN_DEBUG=1`) |

The `gran_shaders` target rebuilds shaders if `dxc` is found.

Verified combinations: MinGW-w64 + Ninja, Visual Studio 2026 (MSVC 14.5). Makefile and CMake builds produce identical frames, pixel for pixel.

## Shaders

Source: `shaders/hlsl/*.hlsl`. The DXIL bytecode is stored in the repository as generated arrays `src/rhi/shaders_gen/*.cpp`, so `dxc` is not needed for regular builds. After editing a shader:

```bash
make shaders
```

Generated files should not be edited manually.

## Integration with Your Program

Public headers are in `include/gran/`, the main header for the application is `gran/app/application.hpp`. The program is linked with `libgran.a`, `libberesta.a`, and Windows system libraries:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

The `-mwindows` flag removes the console window from a GUI program.

With CMake, it is easiest to add your program in the `examples/<name>/` directory: each such directory is automatically built into `gran_<name>` (the `windows_gui.txt` label indicates a program without a console, and `system_libs.txt` specifies additional system libraries, such as `ws2_32` for chat).

## Testing

```bash
make test
```

```bash
bin/gran_snapshot samples/showcase.xml showcase.png
```

```bash
bin/gran_gallery samples/popups.xml --debug
```

`gran_snapshot` renders a frame off-screen with the D3D12 debug layer enabled and prints the time of each stage.
`gran_gallery --debug` opens a live window; debug layer messages go to the console.

