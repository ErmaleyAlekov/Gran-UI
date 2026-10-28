# Render Testing

On Linux `make test-render` runs CPU Vulkan/Mesa checks in `tests/linux/`; `make test-window` runs
a real X11/Vulkan window. CMake uses `GRAN_BUILD_RENDER_TESTS=ON` and, for windows,
`GRAN_BUILD_WINDOW_TESTS=ON`. Tests cover RGBA readback/orientation, resize/Kawase, bounds,
texture slot replacement with two frames in flight, validation diagnostics, XIM/shortcuts,
cross-process UTF-8 clipboard and cross-thread wake. WARP baselines remain Windows checks;
pixel-for-pixel equivalence across drivers is not claimed.

[Русский](render_testing.md) · **English**

GPU tests are run separately from `make test`: unit tests work without a GPU device.
`tests/render/scenes/` contains scenes, `tests/render/baselines/` — verified baseline PNGs.
Baselines are created by D3D12 WARP, size and scale are fixed. A regular run does not change them.

The unit suite also checks window frames without a GPU: XML/roles, command input and accessibility,
Maximized state, clipping/transforms/modality, and on Windows real HWNDs in system/custom modes,
all eight resize regions, size constraints, WM_CLOSE and a synthetic 96→144 DPI message. The live example
is `bin/gran_gallery samples/custom_window.xml --debug`. Snap Layouts depends on Windows 11 and OS settings;
automated checks confirm HTMAXBUTTON, while the menu itself is checked in a live window.

## Make

```sh
make test
make test-render
```

The second command builds `bin/gran_render_tests.exe` and runs D3D12 and D3D11 on WARP.
Current frames are located in `scratchpad/render-tests/` (not included in Git).

Separate API or hardware adapter:

```sh
bin/gran_render_tests --d3d11
bin/gran_render_tests --d3d12 --hardware --tolerance 3 --output scratchpad/render-hardware
```

Each channel of RGBA8 is compared; the default tolerance is 2. Any pixel outside the tolerance, size mismatch,
missing baseline, GPU error, or debug layer error results in a non-zero return code.
D3D12 checks ERROR/CORRUPTION severity; D3D11 additionally fails on warnings.
The debug layer is enabled if Windows Graphics Tools are present; its absence is allowed.
A failed frame is saved; the output shows the number of mismatched pixels and the maximum channel difference.
`max` can be non-zero on success if the difference is within the tolerance.

On AMD RX 9070, the maximum difference with WARP in blurring was 3 in one pixel for both implementations;
for hardware checks on this machine, use `--tolerance 3`. The WARP test threshold remains 2.
Identical fonts are required between machines: the text scene uses the Windows system Consolas.
A separate set of baselines is needed for a different font.

## CMake

```sh
cmake -S . -B build -DGRAN_BUILD_RENDER_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

On Windows, `gran_render_d3d12` and `gran_render_d3d11` are added. Frames are located in the build folder,
baselines are read from the sources. For multi-configuration builds, add `--config Release` and `ctest -C Release`.
The set is designed for Windows 10/11; Linux/macOS do not yet have a ready-made graphics backend.

## Updating Baselines

After intentional rendering changes:

```sh
make update-render-baselines
```

Or `bin/gran_render_tests --update`. Updates are allowed only with D3D12 WARP to avoid random GPU selection
changing the baseline origin. Review the new PNGs and differences in Git, then run `make test-render`.
Do not update the baseline only to fix an unexplained failure.

## What is Covered

- Rounded corners, different borders, linear/radial/conic gradients, outer shadow, rotation, transparency,
  clipping inside Scroll; scale 1 and 1.5.
- Russian/English text, styled sections, underlining, PNG, transparent image, and Input.
- Two overlapping backgrounds with different blur radii.
- A/B/A images, resize targets, slot reuse, and pixel reading with row stride consideration.
- Live window: loading from the timer is deferred to the cycle boundary; the watcher saves the interface on XML
  error and accepts corrections.
- Unit tests for comparison: tolerance, alpha, size mismatch, and array length mismatch.

Tests do not replace checks on NVIDIA/Intel and different drivers. WebView in offscreen is not included.
Animations in snapshots show the initial state.

## Snapshot of Arbitrary Markup

```sh
bin/gran_snapshot samples/glass.xml scratchpad/glass-dx11.png --d3d11 --warp
bin/gran_snapshot samples/showcase.xml scratchpad/showcase.png 1000 760 1.5 --d3d12
```

Keys can be mixed with positional sizes after the two paths.
A debug layer error terminates the tool with a non-zero code.
Compilers are needed only when changing HLSL: `make shaders` (DXIL/dxc), `make shaders-dx11` (DXBC/fxc);
CMake — `gran_shaders` and `gran_shaders_dx11`.
