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
8. [Linux](#linux)
9. [macOS](#macos)

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
| `make test-crash` | Check the crash reports (the program crashes itself: SEGV, terminate, abort) |
| `bin/gran_rcc` | Embedding resources into the exe by a manifest (built together with the examples) |
| `make shaders` | HLSL → DXIL; `make shaders-vulkan` generates SPIR-V → `src/rhi/shaders_gen/*.cpp`, requires `dxc` |
| `make beresta` | Build static Beresta in `BERESTA_DIR` |
| `make clean` / `fclean` / `re` | Remove object files / everything built / rebuild |

| Variable | Value |
|---|---|
| `BERESTA_DIR=path` | Beresta directory (default `../Dom lib C++`) |
| `DEBUG=1` | `-O0 -g`, D3D12 debug layer available via `--debug` in examples |
| `VULKAN=1` | Linux Vulkan backend (enabled by default; SDK or system Vulkan headers) |
| `NATIVE=0` | The core without an OS window and GPU: the windowless platform and `NullBackend` (Linux without X11, macOS without Xcode, see [macOS](#macos)) |
| `WEBVIEW2_SDK=path` | A browser for `<WebView>` (Windows): `build/native` of the `Microsoft.Web.WebView2` NuGet package; `WebView2Loader.dll` is copied to `bin/`. After a change — `make re` |

Cleaning works in CMD/PowerShell and Unix shell.

By default (`DEBUG=0`), programs are linked with `-s`: debug information is removed from the final
executables, including symbols from statically linked MinGW libraries. `DEBUG=1` preserves symbols.
The Makefile notices a mode change (`DEBUG`, `SANITIZE`, `VULKAN` and other flags) by itself: the flags line is
kept in `obj/.flags`, and when it changes all object files are rebuilt — `make re` is not needed.

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
| `-DGRAN_NATIVE=OFF` | The core without an OS window and GPU (Linux without X11, macOS without Xcode) |
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
The engine does not link `libvulkan`: the system loader `libvulkan.so.1` is opened through `dlopen` when the graphics
are created. So the program starts without the Vulkan package too and shows a clear error with the package name
(`libvulkan1` / `vulkan-loader`, `mesa-vulkan-drivers` to work without a GPU) instead of an OS loader refusal.
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
For direct linking use `-lX11 -lX11-xcb -lxcb -ldl -pthread` (the engine does not link `libvulkan`); CMake propagates these through the `gran` target.

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
| Astra Linux, Debian | `g++ make libx11-dev libx11-xcb-dev libxcb1-dev libvulkan-dev` (+ `mesa-vulkan-drivers` for CPU) |
| ALT | `gcc-c++ make libX11-devel libvulkan-devel` |
| RED OS | `gcc-c++ make libX11-devel vulkan-loader-devel` (+ `mesa-vulkan-drivers`) |

On Linux file dialogs are shown by the system `zenity` or `kdialog` program (if neither exists — an error
with a hint); dragging files into a window and AT-SPI are not implemented yet.

X11 provides UTF-8/XIM input, mouse/wheel, cursors, CLIPBOARD, `Xft.dpi` scaling (`GRAN_UI_SCALE=1.5`
overrides it), native/custom frames, EWMH window management and sleeping with a thread-safe pipe wakeup.
Native Wayland, AT-SPI and the browser (`<WebView>`) on Linux and macOS are currently unsupported. Large clipboard INCR
transfers are currently unsupported; outgoing clipboard text is limited to 200,000 bytes.

Source: `shaders/hlsl/*.hlsl`. The DXIL bytecode is stored in the repository as generated arrays `src/rhi/shaders_gen/*.cpp`, so `dxc` is not needed for regular builds. After editing a shader:

```bash
make shaders
```

Generated files should not be edited manually.

## Integration with Your Program

Public headers are in `include/gran/`, the main header for the application is `gran/app/application.hpp`; the version number is in `gran/gran_version.h` (`GRAN_VERSION_AT_LEAST`), and what is guaranteed between versions is in [versioning.en.md](versioning.en.md). The program is linked with `libgran.a`, `libberesta.a`, and Windows system libraries:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d11 -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

The `-mwindows` flag removes the console window from a GUI program.

With CMake, it is easiest to add your program in the `examples/<name>/` directory: each such directory is automatically built into `gran_<name>` (the `windows_gui.txt` label indicates a program without a console, and `system_libs.txt` specifies additional system libraries, such as `ws2_32` for chat).

### Prebuilt Release Archives

Every release on GitHub and Gitverse has archives attached — the engine can be used without building from source:

| Archive | Built with | Runs on |
|---|---|---|
| `gran-ui-X.Y.Z-linux-x86_64-gcc10.tar.gz` | GCC 10.3 in the ALT p10 image | programs on GCC 10 and newer, glibc 2.30+: ALT p10 and p11, Astra 1.8, RED OS 8, Ubuntu 22.04+ |
| `gran-ui-X.Y.Z-linux-x86_64-gcc8.tar.gz` | GCC 8.3 in the Astra Linux SE 1.7 image | programs on GCC 8, glibc 2.27+: Astra 1.7, RED OS 7.3 |
| `gran-ui-X.Y.Z-windows-x86_64-mingw.zip` | MinGW-w64 GCC (the runtime inside, `-static`) | Windows 10 and 11 x64 |
| `gran-ui-X.Y.Z-windows-x86_64-msvc.zip` | MSVC, Release, the static `/MT` runtime | Windows 10 and 11 x64, programs with `/MT` |
| `gran-ui-X.Y.Z-windows-x86_64-msvc-md.zip` | MSVC, Release, the dynamic `/MD` runtime | Windows 10 and 11 x64, programs with `/MD` (the default of a new Visual Studio project) |

Inside: `include/` (the Gran and Beresta headers), `lib/` (`libgran` and `libberesta` are static, `gran_c` is
the C interface shared library for the wrappers), `bin/` (`gran_rcc`, `gran_snapshot`), `schemas/`, `samples/`, `docs/`,
the licenses and `BUILD_INFO.txt` (the commit, the Beresta version, the compiler). The MinGW, MSVC `/MT` and MSVC `/MD`
static libraries are incompatible with each other — take the archive for your compiler and runtime: otherwise the MSVC
linker answers `LNK2038` ("RuntimeLibrary" or "_ITERATOR_DEBUG_LEVEL"). The libraries in the archives are Release only;
build the Debug configuration of your program with them using the same runtime and `_ITERATOR_DEBUG_LEVEL=0` (for example,
`/MD /Od /Zi /D_ITERATOR_DEBUG_LEVEL=0` with the `msvc-md` archive) or build the engine from source (CMake,
`-DGRAN_MSVC_DYNAMIC_RUNTIME=ON` for `/MD`). Integrity: `sha256sum -c SHA256SUMS.txt` next to the archives. Linking is as above, only with the archive paths: `-Igran-ui/include gran-ui/lib/libgran.a gran-ui/lib/libberesta.a`
(MSVC: `gran.lib beresta_static.lib`). The archives have no embedded browser (`<WebView>`): for it the engine is
built from source with the WebView2 SDK.

**Why there are two Linux archives.** Static C++ libraries are bound to the compiler: `std::filesystem` (Beresta uses
it) is experimental in GCC 8, and GCC 9 changed its binary layout — a GCC 13 program built with the GCC 8 static
libraries crashes on the first path operation (checked on ALT p11). So a GCC 8 program needs the `gcc8` archive (link
with `-lstdc++fs`), and a program on GCC 10 and newer needs the `gcc10` archive (checked with GCC 12 on Astra 1.8 and
GCC 13 on ALT p11). The `bin/` tools and `libgran_c.so` (the C interface for the wrappers) do not depend on the program
compiler: take them from any archive that fits by glibc. To run they need the system X11 and Vulkan loader: Astra and
Debian — `libx11-6 libvulkan1`, ALT — `libX11 libvulkan1`, RED OS — `libX11 vulkan-loader`; without a video card also
`mesa-vulkan-drivers` (software Vulkan).

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

### Continuous Integration (CI)

Every push is checked in two places. GitHub Actions (`.github/workflows`): `linux.yml` — the Makefile with GCC on
x86_64 and aarch64, CMake with Clang, the `NATIVE=0` core, the GPU checks on lavapipe, ASan + UBSan and TSan;
`windows.yml` — MinGW-w64 and MSVC with WARP; `macos.yml` — see [macOS](#macos). Gitverse
(`.gitverse/workflows/ci.yml`, the repository CI/CD settings select the configuration from `.gitverse/workflows`):
the same Linux x86_64 matrix — the Gitverse cloud runner is Linux x86_64 only with a 30-minute limit per job, so the
checks are split into short jobs, and each one prepares the packages and Beresta itself (`tools/ci/gitverse_prepare.sh`).

A release by the `vX.Y.Z` tag: `.github/workflows/release.yml` checks that the tag matches `gran_version.h` and that
CHANGELOG has the version section, builds the [release archives](#prebuilt-release-archives) (Linux in the Astra 1.7 and ALT p10
container, `tools/release/build_linux.sh`; Windows with `tools/release/build_windows.sh mingw|msvc`) after the unit tests,
computes SHA-256 and creates the GitHub release with the notes from CHANGELOG. A manual run of this workflow from a branch
builds the archives without a release — a check before the tag. `.gitverse/workflows/release.yml` waits for the GitHub
release, checks the sums and repeats it on Gitverse (the `GV_API_TOKEN` secret).

### Static Analysis (clang-tidy)

The rules are in [`.clang-tidy`](../.clang-tidy): the `bugprone`, `cert`, `clang-analyzer` groups (including
`optin.core.EnumCastOutOfRange` — casting a number outside an enumeration), `concurrency`, `performance`; what is disabled
and why is in a comment there. A compile database `compile_commands.json` is needed: CMake writes it with
`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

```bash
cmake -S . -B build-tidy -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-tidy
run-clang-tidy -p build-tidy -quiet '/(src|examples)/'
```

clang-tidy is not part of the compiler; without administrator rights it can be installed from PyPI: `pip install clang-tidy`.
The argument checks in the code are visible to the analyzer: in the C interface `require` is a template in a header over a `[[noreturn]]`
function, otherwise after `require(p != nullptr)` it would consider `p` null.

## macOS

On macOS the **Cocoa window and Metal graphics** are built (`NATIVE=1` is the default since step M6, as on Windows and
Linux): the library, all the examples except `chat` (it uses Winsock networking) and the C interface for the bindings.
You need **Xcode** (the Metal compiler `xcrun metal`; the Command Line Tools alone do not have it — then the build says so
at once) and GNU make 4+: the stock macOS `make` is version 3.81, the engine Makefile does not fit it and says so itself.

```bash
brew install make
```

```bash
gmake beresta BERESTA_DIR=../beresta
```

```bash
gmake BERESTA_DIR=../beresta all test test-render
```

```bash
bin/gran_gallery
```

Without Xcode the **core without an OS window** is built (`NATIVE=0` / `-DGRAN_NATIVE=OFF`, the Command Line Tools are
enough: `xcode-select --install`): the markup, styles, layout, text, input, bindings, models, the program and all the unit
tests — on the windowless platform (`AppOptions::headless`, `AppDriver`) and `NullBackend`. Calling an OS window or a GPU
backend in such a build is an exception with a clear text, the examples and the C interface are not built.

```bash
gmake NATIVE=0 BERESTA_DIR=../beresta all test test-crash
```

Why `NATIVE=1` became the default: so `make` gives the same on all three OSes — windowed programs; Xcode is needed for
macOS programs anyway (signing, the `.app` bundle), and without it the error comes at once and suggests `NATIVE=0`.

The code is built for macOS 11.0 and newer (`MACOS_MIN` in the Makefile, `CMAKE_OSX_DEPLOYMENT_TARGET` in CMake) — as
Beresta. A universal Intel + Apple Silicon build — through CMake:

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

```bash
cmake --build build -j
```

```bash
ctest --test-dir build --output-on-failure
```

### The Cocoa window

The `src/platform/cocoa/` platform (Objective-C++ with ARC) fulfils the `platform::` contract
([architecture](architecture.en.md)):

- **The window** is an own `NSWindow` subclass, the content is a view with a `CAMetalLayer` layer (`nativeHandle`). The
  event loop is own (`nextEventMatchingMask:untilDate:`, `[NSApp run]` is not called), `wake()` from any thread puts an
  event into the AppKit queue. A program run from a terminal without an `.app` bundle still gets the Dock, the menu bar
  and the focus; "Quit" (Cmd+Q, the Dock) and the close button give the usual `CLOSE` — the program decides.
- **Retina:** a logical pixel is a point, `scale()` is `backingScaleFactor`; moving the window to a monitor with another
  scale gives `DPI_CHANGED` and `RESIZE`.
- **The mouse** is in points from the top left corner; the OS `clickCount`; Control+click is the right button (the macOS
  convention). **The wheel and the trackpad:** precise scrolling (`hasPreciseScrollingDeltas`) is divided by 48 points per
  "notch", the OS gives the inertia.
- **The keyboard:** keys go by the physical code (Cmd+C works in the Russian layout), Cmd is `Mod` (`MODIFIER_CONTROL`),
  the physical Control is `META`, Control+Tab is Mod+Tab. Text goes through the input method (`interpretKeyEvents` →
  `NSTextInputClient`): dead keys and Option type symbols, Option with a symbol comes without `ALT`. IME is the contract
  minimum: the committed string comes as symbols, the string being composed is not visible to the engine yet
  (`COMPOSITION` is item 2.5), the candidate window is at the top left corner of the window.
- **The clipboard** is `NSPasteboard` (UTF-8), **the cursors** are `NSCursor`, **the file dialogs** are `NSOpenPanel` /
  `NSSavePanel`.
- **A custom frame** (`titleBar="custom"`) is the content under a transparent title, the macOS "traffic lights" stay; an
  area with the drag role moves the window, a double click acts by the system setting.
- **Not yet:** dragging files into the window (`NSDraggingDestination`) and the `NSAccessibility` accessibility bridge
  (VoiceOver sees only the window) — next steps.

The Metal backend draws the frame into its own texture, at the end of the frame copies it into the layer `drawable` and
shows it; the window surface is BGRA8 (CAMetalLayer cannot do RGBA8), the colors are sRGB, `vsync` is
`displaySyncEnabled`. The window frame is read the same way as offscreen (`SnapshotSource`), without the "Screen
Recording" rights.

### Offscreen Metal

Without a window the **Metal** backend (`src/rhi/metal/`, Objective-C++ with ARC) draws the frame into its own texture
and reads it back — as `gran_snapshot` on the other OSes (the `--metal` key). The `src/rhi/shaders_gen/msl/*.metal`
shaders are built into `gran.metallib` at build time and embedded into the library, shaders are not compiled in the
engine. The MSL texts come from the same HLSL (`make shaders-metal` on Linux, WSL or macOS: dxc with SPIR-V and
spirv-cross), `make metal-check` checks them with the Apple compiler and compares the GPU struct layout with C++.

`test-render` (`bin/gran_macos_tests`) checks the backend (frames in flight, the blur, resizing, texture slots,
rejecting invalid data), then **the in-process Cocoa window** — a real window with `CAMetalLayer`: frames through
`nextDrawable`, the frame pixels, input through the AppKit queue (`[NSApp postEvent:]`, without the "Accessibility" rights:
the mouse, Shift+A with a symbol, Cmd+C, Control+Tab, precise scrolling), resizing, the clipboard, `wake` from a thread,
closing — and compares the snapshots of the `tests/render/scenes` scenes with the D3D12 WARP baselines: the whole frame
within a tolerance of 2 per channel, the outliers on the very shape edges at most 0.1 % of the pixels and not farther than
48 (Metal rasterizes the edges slightly differently from WARP). The Metal debug layer is enabled only by environment
variables: `MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`.

The crash report on macOS is as on Linux (signals on an own stack, backtrace, the stack bytes); instead of
`/proc/self/maps` the report gets the dyld images with their load addresses (for `atos -l`). Fonts are looked up in
`/System/Library/Fonts`, `/Library/Fonts` and `~/Library/Fonts`.

The developers have no own Mac yet: every push is checked on a GitHub Actions macOS 15 runner
(`.github/workflows/macos.yml`): the `NATIVE=0` core on arm64 (the tests, the crash report, the Metal shaders), the Cocoa
window and Metal (the Makefile and CMake: the tests, `test-render` with the in-process window, the snapshots) and the
universal CMake build with the tests of both architectures (x86_64 — through Rosetta). A live window on a real Mac (the
trackpad, IME, Retina, VoiceOver) is checked by a person.

`NATIVE=0` works on Linux too: the core without X11 and Vulkan — for example, for server-side markup checking or a CI
container without graphics packages.
