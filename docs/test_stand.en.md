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

The Linux code changes of this run (`--hardware`, no replacement of the chosen card by lavapipe with `GRAN_ADAPTER`)
were checked first in WSL Ubuntu, then on the stand (below).

6. **The Linux backend factory replaced the chosen card by lavapipe** on any GPU error — with `GRAN_ADAPTER` the
   replacement is disabled (found while checking `--hardware` in WSL).
7. The GCC `-Wformat-truncation` warning in `json_reader_scan.cpp` (the JSON error byte) is removed.

**Linux (ALT p11)**

The desktop is GNOME in a Wayland session; the X11 window tests go through XWayland. The GPU tests use the debug build
(`make DEBUG=1`), which enables `VK_LAYER_KHRONOS_validation` (confirmed by the Vulkan loader log).

| Check | Result |
|---|---|
| Release and debug builds (GCC) | no warnings; release — 2 min 9 s on 12 threads |
| Unit tests, crash reports | 338/338, passed |
| `gran_linux_tests --hardware` + snapshots of 21 markups: NVIDIA (proprietary 595), Intel ANV, lavapipe | passed; no validation layer messages; the frames of different drivers differ — the card chosen is really that one |
| The same on NVK (GRUB entry `gran-nvk`) | backend tests passed; snapshots — see finding 8 |
| X11 window (`--window`): NVIDIA, NVK, lavapipe | passed |
| X11 window on Intel | `vkCreateSwapchainKHR` → `VK_ERROR_INITIALIZATION_FAILED` — a platform limitation, see finding 9 |
| ASan + UBSan (`make DEBUG=1 SANITIZE=address,undefined test`) | 338/338, clean |
| TSan (`SANITIZE=thread`) | 338/338, no warnings |
| Valgrind memcheck (`gran_tests`) | 338/338, 0 errors |

**Found on Linux (not engine bugs)**

8. **NVK (Mesa 26.1.8, kernel 6.12.94, RTX 3060): every second process hangs on its first command submission.** After
   any process that used the card exits, the first submission of the next one never completes: the kernel logs
   `job timeout, channel N killed`, the program gets `VK_ERROR_DEVICE_LOST` after ~15 s; the process after that works
   again. It does not depend on the markup, the validation layer or the build kind; the reference `vkcube` fails the
   same way, and so it does with the NVIDIA Vulkan driver removed from the loader (`VK_DRIVER_FILES`). It is a
   nouveau/NVK bug. The workaround on the stand is to repeat the failed run; with a repeat all 21 markups rendered
   without layer messages.
9. **Intel cannot present to a window while the desktop runs on NVIDIA.** The GNOME compositor (on the proprietary NVIDIA
   driver) does not accept Intel buffers: `vkcube` on Intel fails the same way both through Wayland and XWayland.
   Without a window (snapshots, backend tests) Intel works. To check the window on Intel, the monitor is connected to
   the motherboard output, so that the desktop runs on Intel.
10. **The proprietary NVIDIA driver is seen by Vulkan twice** (two ICD files in ALT). Harmless: `GRAN_ADAPTER=nvidia`
    takes the first device.
11. **Booting ALT after Windows stood for 2 h 25 min.** GRUB and the kernel did their part at once (the kernel started
    1.5 min after the reboot command). The `plymouth-read-write` service (the boot splash) stopped, and with it the whole
    system up to `sysinit.target` and SSH. It went on when a keyboard was connected to the stand. All that time the
    screen showed the last GRUB frame, so the stall looked like waiting in the menu. The cure for the stand is booting
    without the splash (T-13).
12. **The clock.** Windows keeps local time in the hardware clock, and its time zone there is Pacific (UTC−8). So ALT
    starts with the clock 8 h behind until NTP corrects it. This is dangerous for `make`: files "from the future" break
    rebuilds. The cure is T-13.
13. **The stand suspends after 15 minutes** without keyboard and mouse input: GNOME in the autologin session does not
    count SSH work as activity. Long runs are started under `systemd-inhibit --what=sleep:idle`; turning suspend off
    entirely is T-13.

**Manual check T-14 (Windows, Intel and NVIDIA): found and fixed**

14. **After quickly dragging the edge the window froze for seconds, with black and white strips at the edges.** Every size
    change event accumulated during the Windows modal loop recreated the swap chain for a stale size. Measured on 300
    steps: Intel D3D12 — 2.8 s, Intel D3D11 — 1.0 s, NVIDIA — 0.4–0.7 s (AMD — up to 0.9 s); after the fix — about
    1 ms. The test is "live resize storm" in `gran_render_tests`; the visual recheck is T-15.

**Manual check T-15 (Windows, Intel and NVIDIA): found and fixed**

15. **While the edge is quickly dragged back and forth, white and black strips show at the growing edge.** After T-14
    there is no freeze, but DWM shows the window of the new size earlier than the GPU frame of that size arrives, and
    the grown strip shows the window redirection surface. The window did not paint it (`WM_PAINT` only called
    `ValidateRect`), hence white and black. Now it is painted with the solid window background, and the strip blends
    with the background.
    The measurement is `gran_render_tests --d3d12|--d3d11 --hardware --resize-artifacts` in a session with a desktop.
    It takes the showcase corner with a real mouse (`SendInput`): 40 steps of 20 pixels 4 ms apart, three times right
    and left. The backdrop is gray and the monitor frames are captured through Desktop Duplication, so white and black
    in the strip are only window garbage.
    Bad frames before: Intel D3D12 — 120 of 376, Intel D3D11 — 88 of 386, NVIDIA D3D12 — 34 of 391, NVIDIA D3D11 —
    28 of 405. After the fix — 0 everywhere.
    Two false paths of the measurement, so as not to repeat them:
    - a GDI screen capture (`BitBlt`) makes DWM build a fresh composition and does not see the intermediate frames;
    - `SetWindowPos` from code instead of the mouse, and growing the window one way, give almost no garbage.
    The test moves the cursor, so it is not part of the regular run — the stand only.

**Manual checks T-16 and T-17 (Windows, NVIDIA): found and fixed**

16. **While dragging, the window sometimes completely black for 1–5 s — on the monitor only.** A screen recording,
    screenshots and Desktop Duplication show a normal frame; it was caught with a phone camera. Both stand monitors are
    connected to NVIDIA (MPO3, 4 planes).
    - With the hardware planes off (`HKLM\SOFTWARE\Microsoft\Windows\Dwm`, `OverlayTestMode=5`) there is no black.
    - Edge, Windows Terminal and Explorer do not go dark with the same dragging.
    - Did not help: `FLIP_SEQUENTIAL` instead of `FLIP_DISCARD`, D3D11 instead of D3D12, presenting through
      DirectComposition.
    - The cause: for every `WM_SIZE` (several per screen refresh) the window recreated its buffers and sent a frame;
      1 frame of 7 reached the monitor (PresentMon: 271 of 1888). Without buffer recreation there is less black, but
      not zero.
    - The fix: after every drag frame — `DwmFlush` (`src/platform/win32/win32_procedure.cpp`), one frame per
      composition. Now 502 of 909 are shown, and there is no black.
    - The black cannot be caught automatically: the monitor shows not what DWM gives to programs. An indirect check is
      the share of shown frames while dragging in PresentMon 2.6.0 (`C:\gran\tools\presentmon`; manual recording —
      `C:\gran\pm_manual.cmd --d3d12`, the mouse driver for the gallery — `C:\gran\drag.ps1`).
    - Along the way: the first run of a freshly built exe waits up to 10 s for the Microsoft Defender cloud check; a
      repeated run is instant. For the stand — a Defender exclusion for `C:\gran`.

**Manual check 2026-11-09 (Alt, GNOME Wayland, the window through XWayland): found and fixed**

17. **While the window edge is dragged, the window flickers solid black — on Intel and on NVIDIA.** Caught by a phone camera.
    - Causes: the X11 window was created with a black background and `ForgetGravity`, so on every resize the X server
      filled the whole window with black until the next GPU frame. A frame for which `vkAcquireNextImageKHR` returned
      `OUT_OF_DATE` (the window had changed meanwhile) was lost, and without a new event there was no second attempt.
      Every ConfigureNotify recreated the swap chain.
    - Fix: the window background is the `<Window background>` color with `NorthWestGravity`
      (`src/platform/x11/x11_create.cpp`), ConfigureNotify merging (`x11_events.cpp`), a frame retry after the chain is
      recreated (`src/rhi/vulkan/vulkan_frame.cpp`).
    - Measurement: an X client changes the gallery window width 3 times, 40 steps of 20 pixels 4 ms apart, and after
      each step reads 5 window points with `XGetImage`; a black frame is when all 5 are almost black. It was 238 of 240
      (NVIDIA; the window stayed black after stopping) and 179 of 240 (Intel), now 0 and 0.
    - T-20: the black was gone, but a background strip flickered at the right edge while dragging — Mutter showed
      the window of the new size before the frame. Fix — the `_NET_WM_SYNC_REQUEST` synchronization
      (`src/platform/x11/x11_sync.cpp`). Mutter sends the requests only while dragging with the mouse; the probe sends
      them itself (240 requests → the counter is 240). By eye — T-21.
    - T-21: the flicker stayed — the window answered right after `vkQueuePresentKHR`, while a FIFO frame reaches the
      window at the next screen refresh. Now the answer goes after `vkWaitForPresentKHR`
      (`src/rhi/vulkan/vulkan_present_wait.cpp`). The probe measures the answer delay (`answer delay`). Two builds
      compared, three alternating runs: with waiting — NVIDIA 42 ms on average (worst 87–97), Intel 87 (144–145);
      without it — NVIDIA 36.5 (84–85), Intel 90 (over 200). Waiting is kept for the Intel worst case. The first
      measurements (21 and 31 ms) did not repeat. By eye — T-22.
    - T-22…T-24: the right edge flicker stayed. The GNOME screen recording: the strip is pure black, not the window
      background color. The Gran window is nested in a `mutter-x11-frames` frame (a 32-bit window with the shadow and
      the title bar); `vkcube` shows the same strip, a window with its own title bar (`samples/custom_window.xml`)
      does not. The `sync_watch` watcher: 289 resizes — 289 window answers. Conclusion: the rest is the behavior of the
      GNOME frame for X11 windows, not a Gran bug.
    - For the window to open on Intel, Intel was made the primary GPU of the GNOME desktop (the udev tag
      `mutter-device-preferred-primary`); before that `GRAN_ADAPTER=intel` in a window gave `vkCreateSwapchainKHR` = −3.

**Observation (the main computer, AMD RX 9070):** `gran_render_tests --d3d12 --hardware` — in the `glass` frame one
pixel differs from the WARP baseline by 3 with a tolerance of 2 (the blur); it was so before this run as well. The baselines
are WARP, pixel equality between drivers is not claimed; comparing on AMD needs `--tolerance 3`.

### 2026-11-10 — Linux, 0.2.0 development (the menu bar, toolbars, XDND)

ALT p11, the main tree 8c7e873: the release and debug builds without warnings, the unit tests 356/356, the crash reports;
the X11 window test (`gran_linux_tests --window`, including file dragging by XDND with a source process) on NVIDIA and
lavapipe; the backend tests and snapshots of all markups with `VK_LAYER_KHRONOS_validation` on NVIDIA, Intel and lavapipe —
no layer messages; ASan+UBSan 356/356. Live dragging from Nautilus and the menu bar in GNOME are manual tasks T-26 and T-28.

### 2026-11-10 — Linux, the whole 0.2.0 window frame (plan 2.1 complete)

ALT p11, the main tree 22d714d (the splitter, document tabs, dockable panels and tearing them off): the release and debug
builds without warnings, the unit tests 377/377; the X11 window test with XDND on NVIDIA and lavapipe; the backend tests and
snapshots with the validation layer on NVIDIA, Intel and lavapipe — 0 differences and layer messages; ASan+UBSan 377/377.
**Found:** the crash report test sometimes fails on the SEGV case (about 2 of 15): the `.stack` file is empty when the stack
pointer is closer than 4 KB to the stack top (depends on ASLR) — the write goes in 4096-byte blocks from sp, not by page
boundaries. The bug is from 0.1.0 and does not belong to the window frame.
