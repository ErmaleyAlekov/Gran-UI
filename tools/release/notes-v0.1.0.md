# Грань UI 0.1.0

Первый выпуск с номером версии: этап 1 плана «Фундамент программ» (события, команды, отмена, живой документ,
выражения привязки, компоненты, модели данных, окна и программа), семантические версии и гарантии веток выпуска,
сборка с санитайзерами и первые задачи ручной проверки. Полный список изменений — [CHANGELOG.md](../../CHANGELOG.md#010--2026-11-09).

**Требования:** «Берёста» не ниже коммита `2e8412c` (после v2.2.0: `$value` в `xs:assert` сложного типа и обход ошибки
оптимизатора MSVC 14.44). Windows 10 1909+, MinGW-w64 GCC 11+ или MSVC 2022; Linux — GCC 8+, glibc 2.27+, Vulkan 1.0.

## Проверено перед выпуском (2026-11-09, коммит выпуска)

| Где | Что | Итог |
|---|---|---|
| Windows 11, AMD RX 9070, MinGW GCC 15.2 | `make all test` | 343/343 |
| Windows, WARP | `make test-render` (D3D12, D3D11) | прошло |
| Windows | обёртки Python (25 тестов), C# и Java (smoke) | прошло |
| Windows, MSVC 14.51 | CMake: сборка выпуска, `gran_tests` | 343/343 |
| Альт p11, GCC | `make all`, `make DEBUG=1 all` | 0 предупреждений |
| Альт p11 | `make test`, `make test-crash` | 338/338, отчёты о сбое верны |
| Альт p11, Vulkan + `VK_LAYER_KHRONOS_validation` | тесты бэкенда и снимки всех разметок: lavapipe, Intel UHD 630, NVIDIA RTX 3060 | 0 ошибок, 0 сообщений слоя |
| Альт p11 | `make DEBUG=1 SANITIZE=address,undefined test` | 338/338 |

Ручные проверки: все задачи с пометкой «блокирует выпуск» закрыты ([tests/manual](../../tests/manual/README.md)).
Открыты (не блокируют): Т-3 шаг 5 (своя рамка окна на втором мониторе с другим DPI — на стенде), Т-8 (Linux: живое окно —
частично проверено на стенде в Т-20…Т-24, захват кадра Vulkan в RenderDoc — ещё нет), Т-13 (настройка загрузки Альт на стенде).

**Известное ограничение:** на GNOME (X11/XWayland) при перетаскивании правого края окна со стандартной рамкой мерцает
полоса — это рамка `mutter-x11-frames`, у всех программ X11 так же; окна со своим заголовком (`titleBar="custom"`)
её не имеют. Собственный бэкенд Wayland — в плане.

---

# Gran UI 0.1.0

The first release with a version number: roadmap stage 1 "Program Foundation" (events, commands, undo, live document,
binding expressions, components, data models, windows and the program), semantic versions and release branch
guarantees, sanitizer builds and the first manual check tasks. The full list of changes is in
[CHANGELOG.en.md](../../CHANGELOG.en.md#010--2026-11-09).

**Requirements:** Beresta at commit `2e8412c` or later (after v2.2.0: `$value` in a complex type `xs:assert` and a
workaround for an MSVC 14.44 optimizer bug). Windows 10 1909+, MinGW-w64 GCC 11+ or MSVC 2022; Linux — GCC 8+,
glibc 2.27+, Vulkan 1.0.

**Checked before the release** (2026-11-09): Windows `make test` 343/343, `make test-render` on WARP (D3D12, D3D11),
Python/C#/Java bindings, CMake + MSVC; Alt p11: build without warnings, `make test` 338/338, crash reports, Vulkan backend
tests and snapshots of every markup with the validation layer on lavapipe, Intel UHD 630 and NVIDIA RTX 3060 — no errors,
ASan + UBSan 338/338.

Manual checks: every task marked "blocks the release" is closed. Open (not blocking): T-3 step 5 (custom window frame
on a second monitor with a different DPI), T-8 (Linux: the live window is partly
checked on the stand in T-20…T-24, a Vulkan frame capture in RenderDoc is not yet), T-13 (Alt boot setup on the stand).

**Known limitation:** on GNOME (X11/XWayland) a strip flickers while the right edge of a window with the standard frame
is dragged — it is the `mutter-x11-frames` frame, the same for every X11 program; windows with their own title bar
(`titleBar="custom"`) do not have it. A native Wayland backend is planned.
