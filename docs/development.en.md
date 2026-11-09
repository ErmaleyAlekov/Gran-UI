# Gran UI — Project Structure, Tests, and Coding Rules

[Русский](development.md) · **English**

← [Back to Description](../README.en.md)

## Table of Contents

1. [Directories](#directories)
2. [Tests and Frame Validation](#tests-and-frame-validation)
3. [Coding Rules](#coding-rules)
4. [GPU Usage Rules](#gpu-usage-rules)
5. [Adding an Element](#how-to-add-an-element)
6. [Messages and Languages](#messages-and-languages)
7. [Debugging](#debugging)

## Directories

| Directory | What's Inside |
|---|---|
| `include/gran/<module>/` | Public headers; `internal/` — headers for module's own files |
| `src/<module>/` | Implementation, mirrored `include/gran/` |
| `schemas/` | Layout schema `gran-ui.xsd` and default theme `default_theme.xml` (embedded into the library) |
| `shaders/hlsl/` | Shader sources; bytecode — `src/rhi/shaders_gen/` (generated) |
| `samples/` | Layouts for experiments and frame validation |
| `examples/` | Programs: `gallery`, `snapshot`, `chat`; each folder — program `gran_<name>` |
| `tests/` | Unit tests (`tests/src/<module>/`) and a small test framework |
| `tools/embed/` | `gran_embed` — converts a file into a C++ array |
| `cmake/` | CMake build parts: parameters, source lists, shaders, reading the version from `gran_version.h` |
| `docs/` | Documentation |

Modules and the pipeline are described in [architecture](architecture.en.md). The `gran/` prefix in header paths is needed to
avoid conflicts with Beresta headers (`core/`, `utils/`).

## Tests and Frame Validation

```bash
make test
```

`gran_tests` includes all suites from `tests/src/` (styles, markup, Stack and Grid layout, text selection,
fields, pop-up layers, accessibility, UTF-8, PNG, message catalog, as well as the portable part of the chat example:
JSON, chunked, NDJSON, Markdown). In CMake, the same tests are run by `ctest`.

Test sets are declared in `tests/include/suites/test_suites.hpp` with the function `register<Set>Tests(test::TestRunner&)` and added to the list in `tests/src/main.cpp`; checks —
macros `TEST_ASSERT`, `TEST_ASSERT_EQ`, `TEST_ASSERT_NEAR`, `TEST_ASSERT_THROWS`.

Frame validation is done by an off-screen frame: `gran_snapshot <markup.xml> <frame.png>` renders the markup through
Direct3D 12 without a window, with the debug layer enabled, and prints the time of stages. Scene and shader changes
are compared pixel by pixel with the previous frame.

### Sanitizers and Debug Layers

| What | How to run |
|---|---|
| ASan + UBSan (Linux, GCC/Clang) | `make DEBUG=1 SANITIZE=address,undefined test` |
| TSan (Linux) — for threaded code | `make DEBUG=1 SANITIZE=thread test` (cannot be combined with `address`) |
| ASan (Windows, MSVC) | `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DGRAN_SANITIZE=address`, then `ctest` |
| Valgrind memcheck (Linux) | `valgrind --error-exitcode=1 bin/gran_tests` (a build without sanitizers) |
| D3D12 Debug Layer + GPU-Based Validation | `GRAN_GPU_VALIDATION=1 bin/gran_snapshot <markup> <frame.png>` (the layer is always on, GBV — by the variable) |
| Vulkan Validation + synchronization validation | `make DEBUG=1`, the `VK_LAYER_KHRONOS_validation` layer is enabled automatically when installed |

MinGW does not support sanitizers — on Windows only MSVC. Any sanitizer or debug layer message is a bug, not noise.
The exception is leaks of the Vulkan driver itself (Mesa lavapipe) in the Linux window test: they are listed in
`tests/linux/lsan_mesa.supp` (`LSAN_OPTIONS=suppressions=tests/linux/lsan_mesa.supp bin/gran_linux_tests --window`),
and the stacks of the remaining ones must not contain a single Gran frame.

### Manual Checks

Some checks are done by a person: a live window, how the frame looks, frame captures in RenderDoc or PIX, the debugger,
long sessions, real GPUs. Tasks with exact steps are prepared after every big change — [tests/manual](../tests/manual/README.md).

## Coding Rules

Engine code is written according to unified rules — both the person and the models follow them:

- **C++17**, standard library, and system SDKs; no third-party libraries. Parsers (TrueType, PNG,
  inflate, style values, JSON in the example) are our own.
- Function body — no longer than **25 lines** of code, in an implementation file — no more than **5 functions**, a function — no more than
  **5 parameters** (more — a parameter structure).
- In headers — only declarations; all logic — in `.cpp`.
- **Comments in two languages:** first Russian, then the same meaning in English. Comments stand before
  each class, function, and non-obvious branch.
- Texts that the user sees are not written as literals — only message identifiers from the catalog.
- Ternary operators and lambdas — only when without them the code becomes more complex.
- Own exception classes (`GranException` and descendants) with an error code.
- Build — simultaneously Makefile and CMake, without changes on Windows, Linux, and macOS (where there is a platform).

## GPU Usage Rules

A memory error on the GPU does not crash immediately; it corrupts a frame or "loses" the device after a few minutes.
Therefore:

- **GPU Object Ownership** — only through wrappers (`ComRef<T>` for D3D12/DXGI); wrappers are not copied,
  only moved. A resource that has been in a command list is deleted through a deferred deletion queue —
  when the fence of the frame where it was last used has been passed.
- **Two Frames in Flight:** each has its own command allocator, a section of the ring upload buffer, and a fence value. No "just in case" idle waits in a regular frame.
- **Every `HRESULT` is checked** by the `GRAN_CHECK_HR` macro, which throws a `GpuException`. Device loss —
  a normal event: the entire scene is stored on the CPU, the GPU is only a cache.
- **Memory Boundaries:** any write to a GPU buffer is checked against capacity before writing. Structures common to C++
  and HLSL (`GpuInstance`, `FrameConstants`) — from 4-byte fields, size is a multiple of 16, `static_assert` on size
  and offset; in HLSL nearby — a comment with offsets. If the structure changes — both files change.
- **Shaders** — one HLSL source, compiled with `dxc` at build time; shader compilation is not in the engine code.
- Each GPU object is given a name (`SetName`) for PIX and RenderDoc.
- GPU objects are created and destroyed in the render thread; the window procedure only puts events in the queue
  and does not throw exceptions.

## How to Add an Element

1. Schema: element type and its attributes in `schemas/gran-ui.xsd` (constraints — through `xs:assert`).
2. Kind: a value in `ui::ElementKind` (`include/gran/ui/node.hpp`), element name — table `NAMES`
   in `src/ui/node.cpp`, content attribute parsing — `src/markup/node_content.cpp`.
3. Default styling — `Style` with the element name in `schemas/default_theme.xml`.
4. Layout — `src/layout/`, rendering — `src/render/scene/`, input — `src/input/`, accessibility role —
   `src/a11y/`.
5. Test in `tests/src/<module>/` and markup in `samples/` for frame checking.

## Messages and Languages

Message catalog — `src/core/i18n/catalogs/messages_ru.cpp` and `messages_en.cpp`; identifiers —
`include/gran/core/message.hpp`. A new language — another catalog file. The language is changed at run time by
`gran::i18n::setLanguage("en")`. Errors from Beresta (XML parsing and schema) come in its language, with a line,
column, and path to the element.

## Debugging

Hot reload: `gran_gallery --watch`; API selection: `--d3d11` / `--d3d12`.
The [API reference](api.en.md) describes node lifetimes; [GPU checks](render_testing.en.md) cover
both backends and the `make test-render` / CTest commands; the [test stand](test_stand.en.md) — Intel + NVIDIA,
Windows + Linux and the run results.

- Debug build: `make DEBUG=1` or `-DCMAKE_BUILD_TYPE=Debug` (`GRAN_DEBUG=1`).
- D3D12 debug layer: the `--debug` key for `gran_gallery`, always enabled for `gran_snapshot`; the environment variable `GRAN_GPU_VALIDATION=1` adds GPU-Based Validation. Layer messages go to the engine log.
- Choosing the video card: `GRAN_ADAPTER=<part of the name>` (case-insensitive: `intel`, `nvidia`, `RTX 3060`) — for D3D12, D3D11 and Vulkan. Without the variable D3D12 and Vulkan take the fastest card, D3D11 — the first in the DXGI list. No suitable card — an error listing the ones found, without a switch to WARP / lavapipe; with `--warp` the variable has no effect.
- `gran_gallery --exit-after N` closes the window after N seconds — convenient for checks without human involvement.
- A GPU frame is analyzed in PIX or RenderDoc: all objects have names.

