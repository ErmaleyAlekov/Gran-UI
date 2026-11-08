# Test Stand: Intel + NVIDIA, Windows + Linux

[Русский](test_stand.md) · **English**

The main development computer is AMD Radeon RX 9070 (Windows). To ensure the engine is tested on all three families of video cards, a separate stand was set up: integrated Intel graphics (mass-market graphics for office machines) and NVIDIA RTX 3060. The stand runs two operating systems on one disk and is accessible via SSH. Builds, tests, frame captures, and GPU debug layers can be run on it without human intervention.

## Hardware and Software

| | |
|---|---|
| Processor | Intel Core i5-10400 (6 cores / 12 threads), integrated Intel UHD Graphics 630 |
| Graphics Card | NVIDIA GeForce RTX 3060 (GA106) |
| Memory, Disk | 16 GB DDR4, SSD 512 GB (both OS) |
| Windows | Windows 11 Pro; Intel drivers 31.0.101.2145, NVIDIA drivers 32.0.16.2114 |
| Windows: Build | MinGW-w64 GCC 15.2 + make; Visual Studio 2022 Build Tools (MSVC 14.44); CMake; Vulkan SDK 1.4.363; Graphics Tools (D3D12 Debug Layer) |
| Linux | ALT p11 (11.2), kernel 6.12, Mesa 26.1.8, proprietary NVIDIA driver 595 |
| Linux: Vulkan | NVIDIA (proprietary), NVK (Mesa, RTX 3060), ANV (Mesa, Intel), lavapipe (CPU) |
| Linux: Build | GCC, clang 19, make, CMake, Valgrind 3.24, Vulkan Validation Layers 1.4.335 |

Automatic login is enabled in both Windows and Linux: live window tests run in the actual graphical session.

## Graphics Card Selection

`GRAN_ADAPTER=<part of name>` (case-insensitive) selects the card for D3D12, D3D11, and Vulkan:
`GRAN_ADAPTER=intel`, `GRAN_ADAPTER=nvidia`. If no card with that name is found, the engine throws an error
`GPU_ADAPTER_NOT_FOUND` with a list of found cards, rather than silently switching to WARP or lavapipe: otherwise, the test
"would pass" on the wrong card. Without the variable D3D12 and Vulkan take the fastest card (on the stand — NVIDIA),
D3D11 — the first in the DXGI list (on the stand — Intel).

On Linux, the NVIDIA card works either with the proprietary driver or with NVK. For NVK, there is a separate boot option
in GRUB (`gran-nvk`): the proprietary module is not connected to the card, nouveau loads before the X11 driver selection.
Normal boot — with the proprietary driver.

## What is Tested

| Check | Windows | Linux |
|---|---|---|
| Unit tests | `make test`; CMake + MSVC: `ctest` | `make test` |
| Crash reports | `make test-crash` | `make test-crash` |
| Rendering on WARP / lavapipe, comparison with baselines | `make test-render` | `bin/gran_linux_tests` |
| Rendering on the graphics card | `GRAN_ADAPTER=<card> bin/gran_render_tests --d3d12 --hardware` (and `--d3d11`) | `GRAN_ADAPTER=<card> bin/gran_linux_tests --hardware` |
| Live window | the same tests in the user session (markup reload, resize); `gran_gallery --exit-after N` | `bin/gran_linux_tests --window [--hardware]` |
| Snapshots of all `samples/*.xml` with debug layers | `GRAN_GPU_VALIDATION=1 bin/gran_snapshot … --d3d12` and `--d3d11`; any layer message is an error | `make DEBUG=1`, `bin/gran_snapshot … --vulkan` with `VK_LAYER_KHRONOS_validation` |
| Sanitizers | CMake + MSVC `-DGRAN_SANITIZE=address`, `ctest` | `make DEBUG=1 SANITIZE=address,undefined test`, `SANITIZE=thread`, Valgrind memcheck |

Live window tests in Windows cannot be run directly from the SSH session: it has no desktop, and creating a swap chain returns `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` (0x887A0022). They are run as a one-time task in the user session. Windows installers (components, drivers, Build Tools) from the SSH session receive "Access is denied"; they are run as a task under SYSTEM.

## Results

### 2026-11-08 — first run

**Windows 11**

| Check | Result |
|---|---|
| MinGW: unit tests | 343/343 (via SSH and in the user session) |
| MinGW: crash reports | passed |
| WARP D3D12 and D3D11: 4 frames + markup reload + backend | passed, pixel for pixel |
| RTX 3060 D3D12 and D3D11 | passed, difference from WARP ≤ 2 units |
| Intel UHD 630 D3D12 and D3D11 | passed, difference ≤ 2 units; card confirmed by the program log |
| Snapshots of 21 markups × D3D12 (with GPU-Based Validation) and D3D11, on Intel and on NVIDIA | 84 frames, no debug layer messages |
| `gran_gallery`, 5 minutes in live window (NVIDIA) | passed |
| MSVC 14.44, release: `ctest` | 4/4 after fixing "Beresta" (before that — 144 out of 342 unit tests failed) |
| MSVC 14.44 ASan: `ctest` | unit tests and rendering are clean; the crash probe is incompatible with ASan and is not built with a sanitizer |

**Found and Fixed**

1. **MSVC 14.44 /O2 incorrectly compiled `a|b|c` choices in XSD "Beresta" regular expressions**: middle choices from three or more branches were unreachable. In Gran UI, this broke color validation like `#172033`.
   GCC, MSVC 14.51, and debug builds are not affected, so the error did not manifest on the main computer.
   Fixed in "Beresta" (cycle `compileAlternation` rewritten), a regression test added to its suite.
2. **CMake + MSVC with `GRAN_SANITIZE=address` did not link** (LNK2038 `annotate_string`): "Beresta" was built without ASan. The sanitizer is now applied to `beresta_objects` / `beresta_static`.
3. **Crash test `gran_crash_reports` hung under ASan**: ASan intercepts the intentional crash before the engine's handler.
   With the sanitizer, the test does not build.
4. **There was no way to select the graphics card**: D3D12 always took NVIDIA, D3D11 — Intel. Added `GRAN_ADAPTER`.
5. **Linux GPU tests only checked lavapipe**: added the `--hardware` key.

**Linux (ALT p11)** — not run on the stand: after the reboot from Windows ALT did not answer over SSH (the boot
stalled, task T-13 in [`tests/manual/tasks/2026-11-08-test-stand.md`](../tests/manual/tasks/2026-11-08-test-stand.md)).
The Linux code changes of this run (`--hardware`, no replacement of the chosen card by lavapipe with `GRAN_ADAPTER`) were
checked in WSL Ubuntu: a build without warnings, unit tests 338/338, `gran_linux_tests` on lavapipe passed,
`GRAN_ADAPTER=zzz --hardware` gives an error listing the devices.

6. **The Linux backend factory replaced the chosen card by lavapipe** on any GPU error — with `GRAN_ADAPTER` the
   replacement is disabled (found while checking `--hardware` in WSL).
7. The GCC `-Wformat-truncation` warning in `json_reader_scan.cpp` (the JSON error byte) is removed.

**Observation (the main computer, AMD RX 9070):** `gran_render_tests --d3d12 --hardware` — in the `glass` frame one
pixel differs from the WARP baseline by 3 with a tolerance of 2 (the blur); it was so before this run as well. The baselines
are WARP, pixel equality between drivers is not claimed; comparing on AMD needs `--tolerance 3`.
