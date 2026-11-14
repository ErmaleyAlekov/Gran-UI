# Шейдеры MSL (сгенерированы) / MSL shaders (generated)

Файлы `*.metal` здесь **сгенерированы** `make shaders-metal` из `shaders/hlsl/ui.hlsl` (ветка `GRAN_METAL`):
HLSL → SPIR-V (`dxc -spirv`) → MSL 2.3 (`spirv-cross --msl --msl-decoration-binding`). Руками не правятся —
меняется HLSL, затем `make shaders-metal` (Linux, WSL или macOS; нужны dxc со SPIR-V и spirv-cross).
Проверка компилятором Apple и раскладка структур против C++ — `make metal-check` (macOS, в CI — `macos.yml`).

Индексы привязок (одинаковы во всех шейдерах): `buffer(0)` — константы кадра, `buffer(1)` — экземпляры,
`buffer(2)` — отсечения; `texture(0)` атлас, `texture(1)` градиенты, `texture(2)` картинка (и источник размытия),
`texture(3)` подложка; `sampler(0)` — линейный. `instance_id` в Metal включает базовый экземпляр, шейдер прибавляет
ещё `frame.instanceBase` — рисовать с базовым экземпляром 0, как в Vulkan.

The `*.metal` files here are **generated** by `make shaders-metal` from `shaders/hlsl/ui.hlsl` (the `GRAN_METAL` branch):
HLSL → SPIR-V (`dxc -spirv`) → MSL 2.3 (`spirv-cross --msl --msl-decoration-binding`). They are not edited by hand —
change the HLSL, then run `make shaders-metal` (Linux, WSL or macOS; needs dxc with SPIR-V and spirv-cross).
The Apple compiler check and the struct layout against C++ — `make metal-check` (macOS, in CI — `macos.yml`).

The binding indices (the same in all the shaders): `buffer(0)` — the frame constants, `buffer(1)` — the instances,
`buffer(2)` — the clips; `texture(0)` the atlas, `texture(1)` the ramps, `texture(2)` the image (and the blur source),
`texture(3)` the backdrop; `sampler(0)` — linear. In Metal `instance_id` includes the base instance, and the shader
adds `frame.instanceBase` on top — draw with base instance 0, as in Vulkan.
