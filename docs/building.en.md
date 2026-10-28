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
| Windows 10 1909+ or Windows 11, Direct3D 12 or D3D11 FL11_0 | Window and rendering (without a GPU — WARP software adapter) |
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
| `make shaders` | HLSL → DXIL; `make shaders-vulkan` generates SPIR-V → `src/rhi/shaders_gen/*.cpp`, requires `dxc` |
| `make beresta` | Build static Beresta in `BERESTA_DIR` |
| `make clean` / `fclean` / `re` | Remove object files / everything built / rebuild |

| Variable | Value |
|---|---|
| `BERESTA_DIR=path` | Beresta directory (default `../Dom lib C++`) |
| `DEBUG=1` | `-O0 -g`, D3D12 debug layer available via `--debug` in examples |
| `VULKAN=1` | Linux Vulkan backend (enabled by default; SDK or system Vulkan headers) |
| `WEBVIEW2_SDK=path` | A browser for `<WebView>` (Windows): `build/native` of the `Microsoft.Web.WebView2` NuGet package; `WebView2Loader.dll` is copied to `bin/`. After a change — `make re` |

Cleaning works in CMD/PowerShell and Unix shell.

By default (`DEBUG=0`), programs are linked with `-s`: debug information is removed from the final
executables, including symbols from statically linked MinGW libraries. `DEBUG=1` preserves symbols.
After changing the mode, run `make re`: the Makefile does not track changes to build flags.

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

With MinGW, `Release` and `MinSizeRel` use `-s` when linking programs. `Debug` and `RelWithDebInfo`
preserve debug information. MSVC flags are unchanged: debug symbols are stored separately in `.pdb`
files. With multi-configuration generators, select the configuration using `--config`.

| Parameter | Value |
|---|---|
| `-DGRAN_BERESTA_DIR=path` | Directory of Beresta sources |
| `-DGRAN_BUILD_TESTS=ON` | Build unit tests (enabled by default) |
| `-DGRAN_WITH_VULKAN=ON` | Linux Vulkan backend (ON by default) |
| `-DGRAN_WEBVIEW2_SDK=path` | A browser for `<WebView>` (Windows): `build/native` of the `Microsoft.Web.WebView2` package; the loader is copied next to the programs |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug build (`GRAN_DEBUG=1`) |

The `gran_shaders` target rebuilds shaders if `dxc` is found.

**The WebView2 SDK** is needed only for the `<WebView>` element. It is a NuGet archive: download it from
`https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2` and unpack it as a zip; the build path is the
`build/native` directory inside (it holds `include/WebView2.h` and `x64/WebView2Loader.dll`). The path must not contain spaces.

Verified combinations: MinGW-w64 + Ninja, Visual Studio 2026 (MSVC 14.5). Makefile and CMake builds produce identical frames, pixel for pixel.

## Shaders

For Vulkan use `make shaders-vulkan DXC=/path/to/dxc` or the CMake `gran_shaders_vulkan` target
(`-DGRAN_DXC=/path/to/dxc`). DXC must include the SPIR-V backend; the Windows SDK version may not.
The same HLSL produces Vulkan 1.0 shaders with fixed descriptors and Y inversion in vertex shaders only.
Normal builds use embedded `*_spv.cpp` without DXC or downloading SDK components.

## Linux

Build requirements are C++17, Beresta, Vulkan SDK/system development headers and system X11 headers.
Runtime uses the Vulkan loader, a GPU driver or Mesa lavapipe, and `libX11.so.6` for windows.
No SDL, GLFW, Qt, GTK, FreeType or Fontconfig is added. X11 headers belong to the system window SDK;
Xlib is the native Linux window service used by this backend. A Vulkan SDK is unnecessary at runtime.
Older Mesa releases may lack lavapipe; CPU rendering requires a package containing that driver.

CMake defaults to `GRAN_WITH_VULKAN=ON` on Linux; Make defaults to `VULKAN=1`.
Both build gallery/snapshot and the engine; chat remains Windows-only.
AUTO tries discrete, integrated, virtual and other GPUs before CPU. `--software` (alias `--warp`)
requires a CPU device; `--vulkan` explicitly selects Vulkan. Vulkan 1.0 requires no descriptor indexing,
timeline semaphores, dynamic rendering or synchronization2. Offscreen snapshots need no DISPLAY.

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DGRAN_BUILD_RENDER_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/bin/gran_gallery samples/showcase.xml --debug --software --exit-after 5
env -u DISPLAY ./build/bin/gran_snapshot tests/render/scenes/glass.xml glass.png --software
```

`gran_vulkan_cpu` tests real CPU Vulkan offscreen. `-DGRAN_BUILD_WINDOW_TESTS=ON` also registers
`gran_x11_vulkan`, which needs a running X11/XWayland server. Install the system Khronos validation
layer for strict checks; runtime continues without it when unavailable. Tests reject validation diagnostics.
For direct linking use `-lvulkan -lX11 -ldl -pthread`; CMake propagates these through the `gran` target.

Astra Linux, ALT and RED OS use the same build path and OS capabilities, without distro-name checks.
Use packages from the particular edition's repositories. Build binaries in the oldest target environment;
new Ubuntu glibc/libstdc++ binaries are not promised to run on older Astra installations.
Actual validation environments are recorded in `project_context.md`.

Checked in containers (a build from sources with the edition's own packages, `make test`, the C ABI, Vulkan on
Mesa lavapipe offscreen): Astra Linux SE 1.7 (GCC 8.3, glibc 2.28) and 1.8, ALT p10 and p11, RED OS 7.3 and 8.
A build on Astra 1.7 requires only glibc 2.27 — such a binary suits all the listed systems.
On GCC 8 `std::filesystem` is linked from `libstdc++fs` automatically (Makefile and CMake). Packages:

| OS | Packages |
|---|---|
| Astra Linux, Debian | `g++ make libx11-dev libvulkan-dev` (+ `mesa-vulkan-drivers` for CPU) |
| ALT | `gcc-c++ make libX11-devel libvulkan-devel` |
| RED OS | `gcc-c++ make libX11-devel vulkan-loader-devel` (+ `mesa-vulkan-drivers`) |

On Linux file dialogs are shown by the system `zenity` or `kdialog` program (if neither exists — an error
with a hint); dragging files into a window and AT-SPI are not implemented yet.

X11 provides UTF-8/XIM input, mouse/wheel, cursors, CLIPBOARD, `Xft.dpi` scaling (`GRAN_UI_SCALE=1.5`
overrides it), native/custom frames, EWMH window management and sleeping with a thread-safe pipe wakeup.
Native Wayland, AT-SPI, Linux WebView and macOS are currently unsupported. Large clipboard INCR
transfers are currently unsupported; outgoing clipboard text is limited to 200,000 bytes.

Source: `shaders/hlsl/*.hlsl`. The DXIL bytecode is stored in the repository as generated arrays `src/rhi/shaders_gen/*.cpp`, so `dxc` is not needed for regular builds. After editing a shader:

```bash
make shaders
```

Generated files should not be edited manually.

## Integration with Your Program

Public headers are in `include/gran/`, the main header for the application is `gran/app/application.hpp`. The program is linked with `libgran.a`, `libberesta.a`, and Windows system libraries:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d11 -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

The `-mwindows` flag removes the console window from a GUI program.

With CMake, it is easiest to add your program in the `examples/<name>/` directory: each such directory is automatically built into `gran_<name>` (the `windows_gui.txt` label indicates a program without a console, and `system_libs.txt` specifies additional system libraries, such as `ws2_32` for chat).

## Testing

Select the API through `AppOptions::backend` / `BackendOptions::backend` (`AUTO`, `D3D11`, `D3D12`, `VULKAN`).
Gallery and snapshot accept `--d3d11`, `--d3d12`, `--warp`; gallery with `--watch` reloads XML.
`make test-render` checks snapshots of both APIs on WARP; `make update-render-baselines` explicitly updates baselines.
CMake: `-DGRAN_BUILD_RENDER_TESTS=ON` adds GPU checks to CTest.
See [API](api.en.md) and [rendering tests](render_testing.en.md).

D3D11 shaders: `make shaders-dx11` (`FXC=path`) or the CMake `gran_shaders_dx11` target
(`-DGRAN_FXC=path`). Normal builds use embedded DXBC without an SDK compiler.

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

