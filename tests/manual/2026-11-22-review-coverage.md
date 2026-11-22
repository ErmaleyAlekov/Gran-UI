# Пофайловый указатель ручного ревью Gran UI

Срез на 2026-11-22, ревизия `2c05506`. Проверены 1457 путей из `git ls-files` и два локальных скрипта проекта. В область ревью вошли **1390 файлов**. Для 199 файлов найдена прежняя задача Claude. Остальные **1191 файлов** распределены по Т-76…Т-247. Каждый файл остатка назначен ровно один раз.

Основание для исключения — область чтения в карточке, раскрытый на дату карточки шаблон имён или однозначно установленный по разбору исходник. Повторное упоминание в старых карточках сохранено. У Т-43 названы целые каталоги Win32, X11 и ввода; исключены лишь установленные файлы интерфейса, контракта и реализации. Другие файлы этих каталогов получили задачи. Для Т-63 учтён текущий `src/layout/data_grid_widths.cpp` с функцией `forgetGridLines`.

Номер Claude в указателе означает, что файл входил в прежнее задание. Глубина личной проверки каждого файла известна лишь в пределах записанного результата. В новых задачах указан полный файл, а функция или диапазон в скобках служит точкой входа. Для примеров в документации проверяются команды и код, для поставляемых массивов шейдеров — происхождение, формат, размер и соответствие точке входа.

### Границы снимка

Включены исходники и заголовки, тесты, инструменты, привязки, сценарии сборки и CI, XML, XSD, SVG, JSON, тестовые конфигурации, шейдеры и документация с исполняемыми примерами. Локально дополнительно учтены `.claude/scripts/local_llama.sh` и `.claude/scripts/manage_model.ps1`. Сборочные деревья, сторонние зависимости, клоны, временные файлы, настройки агента и копии в `tests/manual/review/` находятся за пределами инвентаризации. Для растров сравнение результата относится к тесту или сценарию, который их использует.

### Задачи по темам

- [Т-76](tasks/2026-11-22-early-code-review.md#t-76)…[Т-90](tasks/2026-11-22-early-code-review.md#t-90) — Первые карточки — события, графика, раскладка, модели и разбор данных.
- [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91)…[Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) — Языковые привязки и приложения-примеры.
- [Т-105](tasks/2026-11-22-review-T99-T106.md#t-105)…[Т-130](tasks/2026-11-22-review-T123-T130.md#t-130) — Схемы и образцы, шейдеры, сборка, инструменты и тестовые стенды.
- [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131)…[Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) — Доступность, анимация и приложение.
- [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148)…[Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) — Привязки, Си-интерфейс, модели данных и изображения.
- [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170)…[Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) — Ввод, компоновка, журнал и XML-разметка.
- [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189)…[Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) — Безоконная платформа, браузер, Cocoa, Win32 и X11.
- [Т-203](tasks/2026-11-22-review-T203-T210.md#t-203)…[Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) — Рендеринг, ресурсы, стили, текст, элементы интерфейса и графические бэкенды.

В карточках О-1 означает проверку входных значений, О-2 — границ и арифметики, О-3 — владения и срока жизни, О-4 — лимитов затрат, О-5 — обработки ошибок, О-6 — сообщений, О-7 — работы между потоками.

### Прежние карточки

| Задача Claude | Файлов из текущего дерева |
|---|---:|
| [Т-41](tasks/2026-11-12-code-review.md) | 11 |
| [Т-43](tasks/2026-11-13-platform-contract.md) | 8 |
| [Т-44](tasks/2026-11-13-headless-driver.md) | 12 |
| [Т-46](tasks/2026-11-14-review-fixes.md) | 6 |
| [Т-48](tasks/2026-11-14-macos-core.md) | 8 |
| [Т-49](tasks/2026-11-15-metal.md) | 8 |
| [Т-51](tasks/2026-11-15-cocoa.md) | 17 |
| [Т-57](tasks/2026-11-15-release-archives.md) | 14 |
| [Т-60](tasks/2026-11-16-variable-rows.md) | 12 |
| [Т-63](tasks/2026-11-17-data-grid.md) | 13 |
| [Т-66](tasks/2026-11-18-grid-columns.md) | 21 |
| [Т-69](tasks/2026-11-18-grid-edit.md) | 24 |
| [Т-72](tasks/2026-11-19-tree-view.md) | 19 |
| [Т-75](tasks/2026-11-20-grid-access.md) | 40 |

### Где читать файл

Пути внутри каталога упорядочены по имени. Ссылки ведут к карточкам как открытых, так и закрытых задач.

<details><summary>.claude · 2</summary>

| Файл | Задача |
|---|---|
| `.claude/scripts/local_llama.sh` | [Т-118](tasks/2026-11-22-review-T115-T122.md#t-118) |
| `.claude/scripts/manage_model.ps1` | [Т-118](tasks/2026-11-22-review-T115-T122.md#t-118) |

</details>

<details><summary>.github · 4</summary>

| Файл | Задача |
|---|---|
| `.github/workflows/linux.yml` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `.github/workflows/macos.yml` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `.github/workflows/release.yml` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `.github/workflows/windows.yml` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |

</details>

<details><summary>.gitverse · 2</summary>

| Файл | Задача |
|---|---|
| `.gitverse/workflows/ci.yml` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `.gitverse/workflows/release.yml` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |

</details>

<details><summary>bindings · 15</summary>

| Файл | Задача |
|---|---|
| `bindings/csharp/GranModels.cs` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/csharp/GranProgram.cs` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/csharp/GranUI.cs` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/csharp/GranUI.csproj` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/examples/counter.xml` | [Т-93](tasks/2026-11-22-review-T91-T98.md#t-93) |
| `bindings/examples/csharp/Counter.csproj` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/examples/csharp/Program.cs` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/examples/java/Counter.java` | [Т-92](tasks/2026-11-22-review-T91-T98.md#t-92) |
| `bindings/examples/python/counter.py` | [Т-93](tasks/2026-11-22-review-T91-T98.md#t-93) |
| `bindings/java/src/granui/Application.java` | [Т-92](tasks/2026-11-22-review-T91-T98.md#t-92) |
| `bindings/python/granui.py` | [Т-93](tasks/2026-11-22-review-T91-T98.md#t-93) |
| `bindings/tests/csharp/Program.cs` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/tests/csharp/Smoke.csproj` | [Т-91](tasks/2026-11-22-review-T91-T98.md#t-91) |
| `bindings/tests/java/Smoke.java` | [Т-92](tasks/2026-11-22-review-T91-T98.md#t-92) |
| `bindings/tests/test_python.py` | [Т-93](tasks/2026-11-22-review-T91-T98.md#t-93) |

</details>

<details><summary>cmake · 6</summary>

| Файл | Задача |
|---|---|
| `cmake/GranMetal.cmake` | [Т-113](tasks/2026-11-22-review-T107-T114.md#t-113) |
| `cmake/GranOptions.cmake` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `cmake/GranShaders.cmake` | [Т-113](tasks/2026-11-22-review-T107-T114.md#t-113) |
| `cmake/GranSources.cmake` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `cmake/GranVersion.cmake` | [Т-113](tasks/2026-11-22-review-T107-T114.md#t-113) |
| `cmake/GranVulkanShaders.cmake` | [Т-113](tasks/2026-11-22-review-T107-T114.md#t-113) |

</details>

<details><summary>docs · 15</summary>

| Файл | Задача |
|---|---|
| `docs/api.en.md` | [Т-119](tasks/2026-11-22-review-T115-T122.md#t-119) |
| `docs/api.md` | [Т-119](tasks/2026-11-22-review-T115-T122.md#t-119) |
| `docs/architecture.md` | [Т-43](tasks/2026-11-13-platform-contract.md), [Т-46](tasks/2026-11-14-review-fixes.md) |
| `docs/bindings.en.md` | [Т-119](tasks/2026-11-22-review-T115-T122.md#t-119) |
| `docs/bindings.md` | [Т-119](tasks/2026-11-22-review-T115-T122.md#t-119) |
| `docs/building.en.md` | [Т-120](tasks/2026-11-22-review-T115-T122.md#t-120) |
| `docs/building.md` | [Т-120](tasks/2026-11-22-review-T115-T122.md#t-120) |
| `docs/development.en.md` | [Т-120](tasks/2026-11-22-review-T115-T122.md#t-120) |
| `docs/development.md` | [Т-120](tasks/2026-11-22-review-T115-T122.md#t-120) |
| `docs/guide.en.md` | [Т-121](tasks/2026-11-22-review-T115-T122.md#t-121) |
| `docs/guide.md` | [Т-121](tasks/2026-11-22-review-T115-T122.md#t-121) |
| `docs/render_testing.en.md` | [Т-121](tasks/2026-11-22-review-T115-T122.md#t-121) |
| `docs/render_testing.md` | [Т-121](tasks/2026-11-22-review-T115-T122.md#t-121) |
| `docs/versioning.en.md` | [Т-122](tasks/2026-11-22-review-T115-T122.md#t-122) |
| `docs/versioning.md` | [Т-122](tasks/2026-11-22-review-T115-T122.md#t-122) |

</details>

<details><summary>examples · 106</summary>

| Файл | Задача |
|---|---|
| `examples/chat/chat.xml` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/chat/chat_events.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/chat/chat_job.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/chat/chat_job.hpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/chat_main.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_errors.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_errors.hpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_texts.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_texts.hpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_texts_en.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chat_texts_ru.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/common/chunked_decoder.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/chunked_decoder.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/http_head.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/http_head.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/json_cursor.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_internal.hpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_numbers.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_parser.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_strings.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_value.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_value.hpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/json_writer.cpp` | [Т-94](tasks/2026-11-22-review-T91-T98.md#t-94) |
| `examples/chat/common/line_splitter.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/line_splitter.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/common/markdown.hpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/markdown_blocks.cpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/markdown_inline.cpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/markdown_internal.hpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/markdown_lines.cpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/ollama_protocol.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/common/ollama_protocol.hpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/common/text_utils.cpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/common/text_utils.hpp` | [Т-95](tasks/2026-11-22-review-T91-T98.md#t-95) |
| `examples/chat/net/http_body.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/http_client.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/http_client.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/http_internal.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/tcp_connection.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/tcp_connection.hpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/net/winsock_session.cpp` | [Т-96](tasks/2026-11-22-review-T91-T98.md#t-96) |
| `examples/chat/ollama/chat_receiver.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/ollama/chat_receiver.hpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/ollama/ollama_client.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/ollama/ollama_client.hpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/ollama/ollama_probe.cpp` | [Т-97](tasks/2026-11-22-review-T91-T98.md#t-97) |
| `examples/chat/platform/fatal_error.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/platform/fatal_error.hpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/platform/utf8_arguments.cpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/platform/utf8_arguments.hpp` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/system_libs.txt` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/chat/ui/chat_actions.hpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_controller.cpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_controller.hpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_replies.cpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_view.cpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_view.hpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_view_feed.cpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/ui/chat_view_markdown.cpp` | [Т-98](tasks/2026-11-22-review-T91-T98.md#t-98) |
| `examples/chat/windows_gui.txt` | [Т-99](tasks/2026-11-22-review-T99-T106.md#t-99) |
| `examples/editor/core/document.hpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/files.cpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/messages.cpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/messages.hpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/save.cpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/text.cpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/core/text.hpp` | [Т-100](tasks/2026-11-22-review-T99-T106.md#t-100) |
| `examples/editor/editor_main.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/actions.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/close.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/closed_tabs.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/controller.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/controller.hpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/documents.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/explorer.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/files.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/nodes.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/refresh.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/ui/views.cpp` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/editor/windows_gui.txt` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `examples/gallery/gallery_actions.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_actions.hpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_board.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_commands.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_events.hpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_grid.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_keys.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_log.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_main.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_notes.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_platform.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_pointer.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_program.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_reset.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_save.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_settings.cpp` | [Т-102](tasks/2026-11-22-review-T99-T106.md#t-102) |
| `examples/gallery/gallery_tabs.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_tasks.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_timers.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/gallery_web.cpp` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/gallery/resources.xml` | [Т-103](tasks/2026-11-22-review-T99-T106.md#t-103) |
| `examples/snapshot/snapshot.hpp` | [Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) |
| `examples/snapshot/snapshot_arguments.cpp` | [Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) |
| `examples/snapshot/snapshot_lists.cpp` | [Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) |
| `examples/snapshot/snapshot_main.cpp` | [Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) |
| `examples/snapshot/snapshot_render.cpp` | [Т-104](tasks/2026-11-22-review-T99-T106.md#t-104) |

</details>

<details><summary>include/gran/a11y · 1</summary>

| Файл | Задача |
|---|---|
| `include/gran/a11y/access_tree.hpp` | [Т-75](tasks/2026-11-20-grid-access.md) |

</details>

<details><summary>include/gran/animation · 5</summary>

| Файл | Задача |
|---|---|
| `include/gran/animation/animator.hpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `include/gran/animation/internal/interpolation.hpp` | [Т-133](tasks/2026-11-22-review-T131-T138.md#t-133) |
| `include/gran/animation/internal/keyframe_runs.hpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `include/gran/animation/keyframes.hpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `include/gran/animation/visual_state.hpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |

</details>

<details><summary>include/gran/app · 11</summary>

| Файл | Задача |
|---|---|
| `include/gran/app/app_driver.hpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `include/gran/app/application.hpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `include/gran/app/background_task.hpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `include/gran/app/commands.hpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `include/gran/app/internal/access_items.hpp` | [Т-139](tasks/2026-11-22-review-T139-T146.md#t-139) |
| `include/gran/app/internal/app_state.hpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `include/gran/app/internal/app_window.hpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `include/gran/app/internal/grid_edit.hpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `include/gran/app/internal/task_pool.hpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `include/gran/app/internal/web_slots.hpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `include/gran/app/timer_queue.hpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |

</details>

<details><summary>include/gran/binding · 5</summary>

| Файл | Задача |
|---|---|
| `include/gran/binding/expression_text.hpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `include/gran/binding/internal/expression_binding.hpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `include/gran/binding/internal/extent_index.hpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `include/gran/binding/internal/list_binding.hpp` | [Т-150](tasks/2026-11-22-review-T147-T154.md#t-150) |
| `include/gran/binding/list_bindings.hpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |

</details>

<details><summary>include/gran/capi · 2</summary>

| Файл | Задача |
|---|---|
| `include/gran/capi/gran.h` | [Т-87](tasks/2026-11-22-early-code-review.md#t-87) |
| `include/gran/capi/internal/bridge.hpp` | [Т-153](tasks/2026-11-22-review-T147-T154.md#t-153) |

</details>

<details><summary>include/gran/core · 4</summary>

| Файл | Задача |
|---|---|
| `include/gran/core/error_code.hpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `include/gran/core/gran_exception.hpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `include/gran/core/i18n.hpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `include/gran/core/message.hpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |

</details>

<details><summary>include/gran/data · 17</summary>

| Файл | Задача |
|---|---|
| `include/gran/data/cell_value.hpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `include/gran/data/column_filter.hpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `include/gran/data/data_source.hpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `include/gran/data/internal/cell_formats.hpp` | [Т-158](tasks/2026-11-22-review-T155-T162.md#t-158) |
| `include/gran/data/internal/list_notify.hpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `include/gran/data/internal/source_readers.hpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `include/gran/data/json_reader.hpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `include/gran/data/list_model.hpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `include/gran/data/list_source.hpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `include/gran/data/record_order.hpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `include/gran/data/record_tracker.hpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `include/gran/data/selection_model.hpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `include/gran/data/sort_filter_proxy.hpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `include/gran/data/template_text.hpp` | [Т-158](tasks/2026-11-22-review-T155-T162.md#t-158) |
| `include/gran/data/tree_list.hpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `include/gran/data/tree_model.hpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `include/gran/data/variable.hpp` | [Т-158](tasks/2026-11-22-review-T155-T162.md#t-158) |

</details>

<details><summary>include/gran/gran_export.h · 1</summary>

| Файл | Задача |
|---|---|
| `include/gran/gran_export.h` | [Т-246](tasks/2026-11-22-review-T243-T247.md#t-246) |

</details>

<details><summary>include/gran/gran_version.h · 1</summary>

| Файл | Задача |
|---|---|
| `include/gran/gran_version.h` | [Т-246](tasks/2026-11-22-review-T243-T247.md#t-246) |

</details>

<details><summary>include/gran/image · 5</summary>

| Файл | Задача |
|---|---|
| `include/gran/image/image.hpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `include/gran/image/internal/image_error.hpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `include/gran/image/internal/inflate_internal.hpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `include/gran/image/internal/jpeg_internal.hpp` | [Т-168](tasks/2026-11-22-review-T163-T170.md#t-168) |
| `include/gran/image/internal/png_internal.hpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |

</details>

<details><summary>include/gran/input · 4</summary>

| Файл | Задача |
|---|---|
| `include/gran/input/input_router.hpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `include/gran/input/internal/text_cursor.hpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `include/gran/input/internal/text_history.hpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `include/gran/input/internal/text_keys.hpp` | [Т-46](tasks/2026-11-14-review-fixes.md) |

</details>

<details><summary>include/gran/layout · 2</summary>

| Файл | Задача |
|---|---|
| `include/gran/layout/internal/layout_internal.hpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `include/gran/layout/layout.hpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |

</details>

<details><summary>include/gran/log · 5</summary>

| Файл | Задача |
|---|---|
| `include/gran/log/crash_report.hpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `include/gran/log/internal/crash_posix.hpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `include/gran/log/internal/crash_windows.hpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `include/gran/log/internal/log_state.hpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `include/gran/log/log.hpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |

</details>

<details><summary>include/gran/markup · 6</summary>

| Файл | Задача |
|---|---|
| `include/gran/markup/components.hpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `include/gran/markup/internal/build_context.hpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `include/gran/markup/internal/components.hpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `include/gran/markup/live_document.hpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `include/gran/markup/markup_loader.hpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `include/gran/markup/theme.hpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |

</details>

<details><summary>include/gran/platform · 16</summary>

| Файл | Задача |
|---|---|
| `include/gran/platform/cocoa/internal/cocoa_keys.hpp` | [Т-193](tasks/2026-11-22-review-T187-T194.md#t-193) |
| `include/gran/platform/cocoa/internal/cocoa_window_impl.hpp` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `include/gran/platform/file_dialog.hpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `include/gran/platform/headless/headless_window.hpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `include/gran/platform/host.hpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `include/gran/platform/web_view.hpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `include/gran/platform/webview2/internal/com_callback.hpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `include/gran/platform/webview2/internal/webview2_host.hpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `include/gran/platform/win32/internal/drop_target.hpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `include/gran/platform/win32/internal/file_dialog_win32.hpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `include/gran/platform/win32/internal/uia_element.hpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `include/gran/platform/win32/win32_window.hpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `include/gran/platform/window.hpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `include/gran/platform/window_frame.hpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `include/gran/platform/x11/x11_file_dialog.hpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `include/gran/platform/x11/x11_window.hpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |

</details>

<details><summary>include/gran/render · 9</summary>

| Файл | Задача |
|---|---|
| `include/gran/render/content_sizer.hpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |
| `include/gran/render/gpu_types.hpp` | [Т-203](tasks/2026-11-22-review-T203-T210.md#t-203) |
| `include/gran/render/gradient_ramps.hpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `include/gran/render/image_store.hpp` | [Т-203](tasks/2026-11-22-review-T203-T210.md#t-203) |
| `include/gran/render/internal/scene_internal.hpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `include/gran/render/pixel_compare.hpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `include/gran/render/renderer.hpp` | [Т-203](tasks/2026-11-22-review-T203-T210.md#t-203) |
| `include/gran/render/scene_builder.hpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `include/gran/render/text_engine.hpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |

</details>

<details><summary>include/gran/res · 3</summary>

| Файл | Задача |
|---|---|
| `include/gran/res/internal/registry.hpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `include/gran/res/manifest.hpp` | [Т-209](tasks/2026-11-22-review-T203-T210.md#t-209) |
| `include/gran/res/resources.hpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |

</details>

<details><summary>include/gran/rhi · 20</summary>

| Файл | Задача |
|---|---|
| `include/gran/rhi/adapter_choice.hpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `include/gran/rhi/backend_factory.hpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `include/gran/rhi/dx11/dx11_backend.hpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `include/gran/rhi/dx11/internal/dx11_backend_impl.hpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `include/gran/rhi/dx12/dx12_backend.hpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `include/gran/rhi/dx12/internal/com_ref.hpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `include/gran/rhi/dx12/internal/dx12_backend_impl.hpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `include/gran/rhi/dx12/internal/dx12_internal.hpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `include/gran/rhi/dx12/internal/offscreen_surface.hpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `include/gran/rhi/dx12/internal/swapchain_surface.hpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `include/gran/rhi/metal/internal/metal_backend_impl.hpp` | [Т-49](tasks/2026-11-15-metal.md) |
| `include/gran/rhi/metal/metal_backend.hpp` | [Т-242](tasks/2026-11-22-review-T235-T242.md#t-242) |
| `include/gran/rhi/null/null_backend.hpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `include/gran/rhi/render_backend.hpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `include/gran/rhi/shader_blobs.hpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `include/gran/rhi/vulkan/internal/owned.hpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `include/gran/rhi/vulkan/internal/pipeline_state.hpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `include/gran/rhi/vulkan/internal/vulkan_api.hpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `include/gran/rhi/vulkan/internal/vulkan_backend_impl.hpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `include/gran/rhi/vulkan/vulkan_backend.hpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |

</details>

<details><summary>include/gran/settings · 2</summary>

| Файл | Задача |
|---|---|
| `include/gran/settings/internal/settings_xml.hpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `include/gran/settings/settings.hpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |

</details>

<details><summary>include/gran/style · 11</summary>

| Файл | Задача |
|---|---|
| `include/gran/style/computed_style.hpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `include/gran/style/internal/gradient_support.hpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `include/gran/style/internal/keyword_tables.hpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `include/gran/style/internal/named_colors.hpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `include/gran/style/internal/value_support.hpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `include/gran/style/property.hpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `include/gran/style/style_compare.hpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `include/gran/style/style_sheet.hpp` | [Т-79](tasks/2026-11-22-early-code-review.md#t-79) |
| `include/gran/style/value_parsers.hpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `include/gran/style/value_scanner.hpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `include/gran/style/values.hpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |

</details>

<details><summary>include/gran/text · 7</summary>

| Файл | Задача |
|---|---|
| `include/gran/text/font_face.hpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `include/gran/text/font_library.hpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `include/gran/text/glyph_atlas.hpp` | [Т-220](tasks/2026-11-22-review-T219-T226.md#t-220) |
| `include/gran/text/glyph_rasterizer.hpp` | [Т-220](tasks/2026-11-22-review-T219-T226.md#t-220) |
| `include/gran/text/internal/font_parts.hpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `include/gran/text/internal/text_internal.hpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `include/gran/text/text_layout.hpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |

</details>

<details><summary>include/gran/ui · 7</summary>

| Файл | Задача |
|---|---|
| `include/gran/ui/command.hpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `include/gran/ui/data_grid.hpp` | [Т-222](tasks/2026-11-22-review-T219-T226.md#t-222) |
| `include/gran/ui/node.hpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `include/gran/ui/node_state.hpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `include/gran/ui/style_pass.hpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `include/gran/ui/tree_view.hpp` | [Т-223](tasks/2026-11-22-review-T219-T226.md#t-223) |
| `include/gran/ui/ui_event.hpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |

</details>

<details><summary>include/gran/undo · 1</summary>

| Файл | Задача |
|---|---|
| `include/gran/undo/undo_stack.hpp` | [Т-227](tasks/2026-11-22-review-T227-T234.md#t-227) |

</details>

<details><summary>include/gran/utils · 9</summary>

| Файл | Задача |
|---|---|
| `include/gran/utils/color.hpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `include/gran/utils/file_system.hpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `include/gran/utils/geometry.hpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `include/gran/utils/internal/file_system_detail.hpp` | [Т-228](tasks/2026-11-22-review-T227-T234.md#t-228) |
| `include/gran/utils/mapped_file.hpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `include/gran/utils/png_writer.hpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `include/gran/utils/program_arguments.hpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `include/gran/utils/uri_list.hpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `include/gran/utils/utf8.hpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |

</details>

<details><summary>include/gran/vector · 2</summary>

| Файл | Задача |
|---|---|
| `include/gran/vector/internal/vector_internal.hpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `include/gran/vector/shape.hpp` | [Т-232](tasks/2026-11-22-review-T227-T234.md#t-232) |

</details>

<details><summary>samples · 30</summary>

| Файл | Задача |
|---|---|
| `samples/animations.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/assets/icons/folder.svg` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/assets/icons/star.svg` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/bindings.xml` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/commands.xml` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/components.xml` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/components/widgets.xml` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/custom_window.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/data/departments.xml` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/data/notes.json` | [Т-106](tasks/2026-11-22-review-T99-T106.md#t-106) |
| `samples/data/orders.json` | [Т-107](tasks/2026-11-22-review-T107-T114.md#t-107) |
| `samples/dialogs.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/editor.xml` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `samples/editor_frame.xml` | [Т-101](tasks/2026-11-22-review-T99-T106.md#t-101) |
| `samples/events.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/glass.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/grid.xml` | [Т-107](tasks/2026-11-22-review-T107-T114.md#t-107) |
| `samples/lists.xml` | [Т-107](tasks/2026-11-22-review-T107-T114.md#t-107) |
| `samples/models.xml` | [Т-107](tasks/2026-11-22-review-T107-T114.md#t-107) |
| `samples/platform.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/popups.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/program.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/program_dialog.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/rich_text.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/showcase.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/text_area.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |
| `samples/tool_window.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/variable_rows.xml` | [Т-107](tasks/2026-11-22-review-T107-T114.md#t-107) |
| `samples/web_view.xml` | [Т-108](tasks/2026-11-22-review-T107-T114.md#t-108) |
| `samples/widgets.xml` | [Т-109](tasks/2026-11-22-review-T107-T114.md#t-109) |

</details>

<details><summary>schemas · 4</summary>

| Файл | Задача |
|---|---|
| `schemas/default_theme.xml` | [Т-105](tasks/2026-11-22-review-T99-T106.md#t-105) |
| `schemas/gran-resources.xsd` | [Т-105](tasks/2026-11-22-review-T99-T106.md#t-105) |
| `schemas/gran-settings.xsd` | [Т-105](tasks/2026-11-22-review-T99-T106.md#t-105) |
| `schemas/gran-ui.xsd` | [Т-105](tasks/2026-11-22-review-T99-T106.md#t-105) |

</details>

<details><summary>shaders · 3</summary>

| Файл | Задача |
|---|---|
| `shaders/hlsl/ui.hlsl` | [Т-110](tasks/2026-11-22-review-T107-T114.md#t-110) |
| `shaders/hlsl/ui_common.hlsli` | [Т-110](tasks/2026-11-22-review-T107-T114.md#t-110) |
| `shaders/hlsl/ui_shadow.hlsli` | [Т-110](tasks/2026-11-22-review-T107-T114.md#t-110) |

</details>

<details><summary>src/a11y · 7</summary>

| Файл | Задача |
|---|---|
| `src/a11y/a11y_collection.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/a11y/a11y_describe.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/a11y/a11y_items.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/a11y/a11y_name.cpp` | [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131) |
| `src/a11y/a11y_navigation.cpp` | [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131) |
| `src/a11y/a11y_patterns.cpp` | [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131) |
| `src/a11y/a11y_tree.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |

</details>

<details><summary>src/animation · 13</summary>

| Файл | Задача |
|---|---|
| `src/animation/animator.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/animator_keyframes.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/animator_tick.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/easing.cpp` | [Т-133](tasks/2026-11-22-review-T131-T138.md#t-133) |
| `src/animation/interpolate.cpp` | [Т-133](tasks/2026-11-22-review-T131-T138.md#t-133) |
| `src/animation/interpolate_shadows.cpp` | [Т-133](tasks/2026-11-22-review-T131-T138.md#t-133) |
| `src/animation/interpolate_values.cpp` | [Т-133](tasks/2026-11-22-review-T131-T138.md#t-133) |
| `src/animation/keyframe_runs.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/keyframe_sample.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/keyframe_timing.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/keyframe_track.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/visual_apply.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `src/animation/visual_state.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |

</details>

<details><summary>src/app · 101</summary>

| Файл | Задача |
|---|---|
| `src/app/access_focus.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/access_items.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/access_items_select.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/app_actions.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `src/app/app_commands.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/app_driver.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/app_driver_frame.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/app_driver_keys.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/app_driver_life.cpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `src/app/app_driver_pointer.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/app_driver_windows.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/app_event_dispatch.cpp` | [Т-80](tasks/2026-11-22-early-code-review.md#t-80) |
| `src/app/app_event_handlers.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `src/app/app_grid_input.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md), [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/app/app_list_input.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `src/app/app_window_events.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/app_window_list.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/application.cpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `src/app/application_access.cpp` | [Т-139](tasks/2026-11-22-review-T139-T146.md#t-139) |
| `src/app/application_background.cpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `src/app/application_caret.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `src/app/application_commands.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/application_data.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_dialogs.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/application_dock.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/application_events.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/application_files.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/application_frame.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/application_graphics.cpp` | [Т-135](tasks/2026-11-22-review-T131-T138.md#t-135) |
| `src/app/application_layout.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/application_live_resize.cpp` | [Т-135](tasks/2026-11-22-review-T131-T138.md#t-135) |
| `src/app/application_load.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/application_logging.cpp` | [Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) |
| `src/app/application_loop.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/application_markup.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/application_markup_save.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/application_models.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_popups.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `src/app/application_reload.cpp` | [Т-82](tasks/2026-11-22-early-code-review.md#t-82) |
| `src/app/application_run.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/app/application_settings.cpp` | [Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) |
| `src/app/application_sources.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_tasks.cpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `src/app/application_timers.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `src/app/application_tooltip.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `src/app/application_tree.cpp` | [Т-135](tasks/2026-11-22-review-T131-T138.md#t-135) |
| `src/app/application_undo.cpp` | [Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) |
| `src/app/application_values.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_variables.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_views.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `src/app/application_web.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/application_window.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/application_window_frame.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/application_windows.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/command_binding.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/command_builtin.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/command_execute.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/command_read.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/command_values.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `src/app/dock_float.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/drop_zones.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/file_drop.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `src/app/graphics_diagnostics.cpp` | [Т-135](tasks/2026-11-22-review-T131-T138.md#t-135) |
| `src/app/grid_edit_begin.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/grid_edit_editor.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/grid_edit_event.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/grid_edit_finish.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md), [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/grid_edit_keys.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/grid_edit_settle.cpp` | [Т-140](tasks/2026-11-22-review-T139-T146.md#t-140) |
| `src/app/grid_edit_step.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/grid_edit_value.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/grid_edit_watch.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/node_rebuild.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/app/node_removal.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md), [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/app/reload_document.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/app/reload_fonts.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/reload_libraries.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/reload_patch.cpp` | [Т-82](tasks/2026-11-22-early-code-review.md#t-82) |
| `src/app/reload_source.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `src/app/status_announce.cpp` | [Т-139](tasks/2026-11-22-review-T139-T146.md#t-139) |
| `src/app/task_context.cpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `src/app/task_pool.cpp` | [Т-83](tasks/2026-11-22-early-code-review.md#t-83) |
| `src/app/task_pool_run.cpp` | [Т-83](tasks/2026-11-22-early-code-review.md#t-83) |
| `src/app/task_progress.cpp` | [Т-83](tasks/2026-11-22-early-code-review.md#t-83) |
| `src/app/task_worker.cpp` | [Т-83](tasks/2026-11-22-early-code-review.md#t-83) |
| `src/app/timer_queue.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `src/app/timer_run.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `src/app/undo_commands.cpp` | [Т-81](tasks/2026-11-22-early-code-review.md#t-81) |
| `src/app/undo_link.cpp` | [Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) |
| `src/app/web_content.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/web_place.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/web_slot.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/web_source.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/web_sync.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/web_views.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `src/app/window_closing.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/app/window_layout.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/window_lookup.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/window_modal.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/window_owner.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `src/app/window_restore.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |

</details>

<details><summary>src/binding · 51</summary>

| Файл | Задача |
|---|---|
| `src/binding/column_filter_apply.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/binding/data_mirror.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/data_mirror_versions.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/data_mirror_watch.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/expression_bindings.cpp` | [Т-89](tasks/2026-11-22-early-code-review.md#t-89) |
| `src/binding/expression_check.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_compile.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_convert.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_evaluate.cpp` | [Т-89](tasks/2026-11-22-early-code-review.md#t-89) |
| `src/binding/expression_filter.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_filter_watch.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_filters.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_rows.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_scan.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_target_read.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_targets.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/expression_text.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_update.cpp` | [Т-89](tasks/2026-11-22-early-code-review.md#t-89) |
| `src/binding/expression_values.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `src/binding/expression_variables.cpp` | [Т-149](tasks/2026-11-22-review-T147-T154.md#t-149) |
| `src/binding/extent_index.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/extent_index_offsets.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/extent_index_sizes.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/extent_index_tree.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/list_binding.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/list_binding_fields.cpp` | [Т-150](tasks/2026-11-22-review-T147-T154.md#t-150) |
| `src/binding/list_binding_items.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/binding/list_binding_measure.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/list_binding_order.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md), [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/binding/list_binding_query.cpp` | [Т-150](tasks/2026-11-22-review-T147-T154.md#t-150) |
| `src/binding/list_binding_reveal.cpp` | [Т-150](tasks/2026-11-22-review-T147-T154.md#t-150) |
| `src/binding/list_binding_spacers.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/list_binding_tree.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/binding/list_binding_window.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `src/binding/list_bindings.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/binding/list_bindings_adopt.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/list_bindings_grid.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/binding/list_bindings_grid_filter.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/binding/list_bindings_grid_keys.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/binding/list_bindings_items.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/binding/list_bindings_lookup.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/binding/list_bindings_models.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/list_bindings_select.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/binding/list_bindings_tree.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/list_bindings_variables.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/list_bindings_views.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/list_bindings_window.cpp` | [Т-151](tasks/2026-11-22-review-T147-T154.md#t-151) |
| `src/binding/mirror_entry.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/selection_mirror.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/tree_mirror.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |
| `src/binding/tree_mirror_events.cpp` | [Т-152](tasks/2026-11-22-review-T147-T154.md#t-152) |

</details>

<details><summary>src/capi · 32</summary>

| Файл | Задача |
|---|---|
| `src/capi/callbacks.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/capi_fetchers.cpp` | [Т-84](tasks/2026-11-22-early-code-review.md#t-84) |
| `src/capi/converter_api.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/errors.cpp` | [Т-87](tasks/2026-11-22-early-code-review.md#t-87) |
| `src/capi/event_snapshot.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/capi/events.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/capi/fetch_api.cpp` | [Т-84](tasks/2026-11-22-early-code-review.md#t-84) |
| `src/capi/handlers.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/lifecycle.cpp` | [Т-87](tasks/2026-11-22-early-code-review.md#t-87) |
| `src/capi/log_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/markup_api.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/model_read.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/model_write.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/models_api.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/properties.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/resources_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/run.cpp` | [Т-87](tasks/2026-11-22-early-code-review.md#t-87) |
| `src/capi/selection_api.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/settings_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/settings_list_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/task_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/task_context_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/tree_api.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/tree_nodes.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/tree_query_api.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |
| `src/capi/undo_api.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/undo_step.cpp` | [Т-156](tasks/2026-11-22-review-T155-T162.md#t-156) |
| `src/capi/validation.cpp` | [Т-87](tasks/2026-11-22-early-code-review.md#t-87) |
| `src/capi/variables_api.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/version_api.cpp` | [Т-153](tasks/2026-11-22-review-T147-T154.md#t-153) |
| `src/capi/view_api.cpp` | [Т-155](tasks/2026-11-22-review-T155-T162.md#t-155) |
| `src/capi/window_api.cpp` | [Т-154](tasks/2026-11-22-review-T147-T154.md#t-154) |

</details>

<details><summary>src/core · 8</summary>

| Файл | Задача |
|---|---|
| `src/core/data_exception.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/derived_exceptions.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/error_code_names.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/gran_exception.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/i18n/catalogs/messages_en.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/i18n/catalogs/messages_ru.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/i18n/i18n_format.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `src/core/i18n/i18n_language.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |

</details>

<details><summary>src/data · 65</summary>

| Файл | Задача |
|---|---|
| `src/data/cell_date.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/data/cell_number.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/data/cell_value.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/data/column_filter.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/data/column_filter_accept.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/data/column_filter_parse.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/data/data_source.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/json_reader.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_reader_scan.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_reader_text.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_reader_values.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_reader_word.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_source_fetcher.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_source_record.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/json_source_select.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `src/data/list_model.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_model_columns.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_model_edit.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_model_fetch.cpp` | [Т-84](tasks/2026-11-22-early-code-review.md#t-84) |
| `src/data/list_notify.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_source.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_source_defaults.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_source_keys.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/list_source_notify.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `src/data/record_compare.cpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `src/data/record_natural.cpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `src/data/record_tracker.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/data/record_tracker_events.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/data/selection_click.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_edit.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_events.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_keys.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_model.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_notify.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/selection_query.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `src/data/sort_filter_events.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_filter_order.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_filter_place.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_filter_proxy.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_filter_query.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_filter_rows.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `src/data/sort_keys.cpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `src/data/template_text.cpp` | [Т-158](tasks/2026-11-22-review-T155-T162.md#t-158) |
| `src/data/text_fold.cpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `src/data/tree_list.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_build.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_expand.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_keys.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_levels.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_observe.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_order.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_publish.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_list_query.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `src/data/tree_model.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/tree_model_clear.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/tree_model_edit.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/tree_model_move.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/tree_model_observers.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/tree_model_query.cpp` | [Т-165](tasks/2026-11-22-review-T163-T170.md#t-165) |
| `src/data/xml_source_convert.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/xml_source_fetcher.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/xml_source_handler.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/xml_source_parse.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/xml_source_path.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `src/data/xml_source_worker.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |

</details>

<details><summary>src/image · 22</summary>

| Файл | Задача |
|---|---|
| `src/image/image_decoder.cpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `src/image/inflate.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/image/inflate_bits.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/image/inflate_blocks.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/image/inflate_tables.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/image/jpeg_ac.cpp` | [Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) |
| `src/image/jpeg_bits.cpp` | [Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) |
| `src/image/jpeg_blocks.cpp` | [Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) |
| `src/image/jpeg_color.cpp` | [Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) |
| `src/image/jpeg_decoder.cpp` | [Т-168](tasks/2026-11-22-review-T163-T170.md#t-168) |
| `src/image/jpeg_frame.cpp` | [Т-88](tasks/2026-11-22-early-code-review.md#t-88) |
| `src/image/jpeg_huffman.cpp` | [Т-168](tasks/2026-11-22-review-T163-T170.md#t-168) |
| `src/image/jpeg_idct.cpp` | [Т-169](tasks/2026-11-22-review-T163-T170.md#t-169) |
| `src/image/jpeg_markers.cpp` | [Т-88](tasks/2026-11-22-early-code-review.md#t-88) |
| `src/image/jpeg_scan.cpp` | [Т-88](tasks/2026-11-22-early-code-review.md#t-88) |
| `src/image/jpeg_scan_header.cpp` | [Т-88](tasks/2026-11-22-early-code-review.md#t-88) |
| `src/image/jpeg_segments.cpp` | [Т-88](tasks/2026-11-22-early-code-review.md#t-88) |
| `src/image/png_chunks.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/image/png_convert.cpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `src/image/png_decoder.cpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `src/image/png_samples.cpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `src/image/png_unfilter.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |

</details>

<details><summary>src/input · 64</summary>

| Файл | Задача |
|---|---|
| `src/input/access_actions.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/input/access_items.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/input/action_sink.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/action_sink_dock.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/action_sink_lists.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/action_sink_reveal.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/input/character_input.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/input/choice_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/command_keys.cpp` | [Т-46](tasks/2026-11-14-review-fixes.md) |
| `src/input/command_match.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/context_menu.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/dock_handle_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/dock_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/drag_zone.cpp` | [Т-172](tasks/2026-11-22-review-T171-T178.md#t-172) |
| `src/input/event_dispatch.cpp` | [Т-80](tasks/2026-11-22-early-code-review.md#t-80) |
| `src/input/event_queue.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/field_history.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/focus_order.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/grid_header_input.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/input/grid_header_keys.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/input/hit_test.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/input/input_router.cpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `src/input/input_router_events.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/input_router_popups.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/input_router_tree.cpp` | [Т-80](tasks/2026-11-22-early-code-review.md#t-80) |
| `src/input/interaction.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/key_events.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/keyboard_input.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `src/input/layer_stack.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/menu_bar_cues.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/menu_bar_input.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/menu_bar_keys.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/menu_open.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/node_drag.cpp` | [Т-172](tasks/2026-11-22-review-T171-T178.md#t-172) |
| `src/input/node_states.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/notify_events.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `src/input/pointer_events.cpp` | [Т-172](tasks/2026-11-22-review-T171-T178.md#t-172) |
| `src/input/pointer_input.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/input/popup_keys.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/popup_layer.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `src/input/press_input.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/input/scroll_bar_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/slider_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/span_links.cpp` | [Т-172](tasks/2026-11-22-review-T171-T178.md#t-172) |
| `src/input/splitter_events.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/splitter_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/tab_drag.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/tab_input.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `src/input/text_change.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_clipboard.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_cursor.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `src/input/text_editing.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_insert.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_keys.cpp` | [Т-46](tasks/2026-11-14-review-fixes.md) |
| `src/input/text_keys_mac.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `src/input/text_navigation.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `src/input/text_pointer.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `src/input/text_selection.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `src/input/text_step.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_step_merge.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_undo.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `src/input/text_word_steps.cpp` | [Т-46](tasks/2026-11-14-review-fixes.md) |
| `src/input/text_words.cpp` | [Т-46](tasks/2026-11-14-review-fixes.md) |
| `src/input/window_frame.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |

</details>

<details><summary>src/layout · 28</summary>

| Файл | Задача |
|---|---|
| `src/layout/axis.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/box_model.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/canvas_layout.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/data_grid_layout.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/layout/data_grid_view_layout.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/layout/data_grid_widths.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/layout/dock_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/dock_titles.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/flex_items.cpp` | [Т-77](tasks/2026-11-22-early-code-review.md#t-77) |
| `src/layout/flex_lines.cpp` | [Т-77](tasks/2026-11-22-early-code-review.md#t-77) |
| `src/layout/flex_resolve.cpp` | [Т-77](tasks/2026-11-22-early-code-review.md#t-77) |
| `src/layout/grid_fractions.cpp` | [Т-180](tasks/2026-11-22-review-T179-T186.md#t-180) |
| `src/layout/grid_items.cpp` | [Т-180](tasks/2026-11-22-review-T179-T186.md#t-180) |
| `src/layout/grid_layout.cpp` | [Т-180](tasks/2026-11-22-review-T179-T186.md#t-180) |
| `src/layout/grid_row_layout.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/layout/grid_tracks.cpp` | [Т-180](tasks/2026-11-22-review-T179-T186.md#t-180) |
| `src/layout/layout_content.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/layout/layout_engine.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/layout_reuse.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/leaf_layout.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `src/layout/menu_item_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/popup_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/scroll_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/splitter_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/stack_layout.cpp` | [Т-179](tasks/2026-11-22-review-T179-T186.md#t-179) |
| `src/layout/stack_place.cpp` | [Т-179](tasks/2026-11-22-review-T179-T186.md#t-179) |
| `src/layout/text_area_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |
| `src/layout/tool_bar_layout.cpp` | [Т-181](tasks/2026-11-22-review-T179-T186.md#t-181) |

</details>

<details><summary>src/log · 15</summary>

| Файл | Задача |
|---|---|
| `src/log/crash_posix.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_posix_context.cpp` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `src/log/crash_posix_format.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_posix_report.cpp` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `src/log/crash_previous.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_windows.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_windows_hooks.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_windows_report.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/crash_windows_stack.cpp` | [Т-183](tasks/2026-11-22-review-T179-T186.md#t-183) |
| `src/log/log_console.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `src/log/log_open.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `src/log/log_paths.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `src/log/log_ring.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `src/log/log_rotate.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/log/log_write.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |

</details>

<details><summary>src/markup · 49</summary>

| Файл | Задача |
|---|---|
| `src/markup/command_resources.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/component_expand.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/component_library.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_params.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_public.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_read.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_schema.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_scope.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/component_shadow.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/component_slots.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/component_text.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/component_validate.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `src/markup/component_values.cpp` | [Т-90](tasks/2026-11-22-early-code-review.md#t-90) |
| `src/markup/data_grid_options.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/markup/data_grid_read.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/markup/data_grid_types.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/data_items.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/data_resources.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/data_sources.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/declarations.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/dock_attributes.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/document_builder.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/dom_helpers.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/icon_shape.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/keyframes_resources.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/live_attributes.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/live_compare.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/live_dom.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/live_patch.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/live_relink.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/live_serialize.cpp` | [Т-188](tasks/2026-11-22-review-T187-T194.md#t-188) |
| `src/markup/markup_fragment.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/markup_loader.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/markup_parse.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/markup_schema.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/node_bindings.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_builder.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_children.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_content.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_events.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_finish.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/node_text.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/splitter_attributes.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/markup/style_resources.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/tabs_attributes.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/text_runs_builder.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `src/markup/theme.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `src/markup/tree_view_read.cpp` | [Т-187](tasks/2026-11-22-review-T187-T194.md#t-187) |
| `src/markup/window_spec.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |

</details>

<details><summary>src/platform · 115</summary>

| Файл | Задача |
|---|---|
| `src/platform/cocoa/cocoa_app.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_clipboard.mm` | [Т-194](tasks/2026-11-22-review-T187-T194.md#t-194) |
| `src/platform/cocoa/cocoa_commands.mm` | [Т-194](tasks/2026-11-22-review-T187-T194.md#t-194) |
| `src/platform/cocoa/cocoa_create.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_cursor.mm` | [Т-194](tasks/2026-11-22-review-T187-T194.md#t-194) |
| `src/platform/cocoa/cocoa_delegate.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_delegate_state.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_events.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_file_dialog.mm` | [Т-194](tasks/2026-11-22-review-T187-T194.md#t-194) |
| `src/platform/cocoa/cocoa_keyboard.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_keys.cpp` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_mouse.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_native_window.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_placement.mm` | [Т-192](tasks/2026-11-22-review-T187-T194.md#t-192) |
| `src/platform/cocoa/cocoa_resize.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_system.mm` | [Т-192](tasks/2026-11-22-review-T187-T194.md#t-192) |
| `src/platform/cocoa/cocoa_text_input.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_text_marked.mm` | [Т-193](tasks/2026-11-22-review-T187-T194.md#t-193) |
| `src/platform/cocoa/cocoa_text_query.mm` | [Т-193](tasks/2026-11-22-review-T187-T194.md#t-193) |
| `src/platform/cocoa/cocoa_view.mm` | [Т-192](tasks/2026-11-22-review-T187-T194.md#t-192) |
| `src/platform/cocoa/cocoa_view_input.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_wheel.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/cocoa/cocoa_window.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/platform/headless/headless_access.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `src/platform/headless/headless_commands.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `src/platform/headless/headless_events.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/platform/headless/headless_frame.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `src/platform/headless/headless_state.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `src/platform/headless/headless_system.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/platform/headless/headless_test_input.cpp` | [Т-44](tasks/2026-11-13-headless-driver.md) |
| `src/platform/headless/headless_window.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `src/platform/host_os.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/key_names.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/none/none_platform.cpp` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `src/platform/web_view_fallback.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/webview2/webview2_apply.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_callbacks.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_core.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_frame.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_host.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_host_layout.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_loader.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/webview2/webview2_requests.cpp` | [Т-191](tasks/2026-11-22-review-T187-T194.md#t-191) |
| `src/platform/win32/uia_element.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_events.cpp` | [Т-198](tasks/2026-11-22-review-T195-T202.md#t-198) |
| `src/platform/win32/uia_events_items.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_expand.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_fragment.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_grid.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_grid_item.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_invoke.cpp` | [Т-199](tasks/2026-11-22-review-T195-T202.md#t-199) |
| `src/platform/win32/uia_item_key.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_item_properties.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_pattern_table.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_patterns.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_properties.cpp` | [Т-198](tasks/2026-11-22-review-T195-T202.md#t-198) |
| `src/platform/win32/uia_range.cpp` | [Т-199](tasks/2026-11-22-review-T195-T202.md#t-199) |
| `src/platform/win32/uia_root.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_selection.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_selection_list.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_simple.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_table.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_table_item.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/uia_value.cpp` | [Т-199](tasks/2026-11-22-review-T195-T202.md#t-199) |
| `src/platform/win32/uia_virtual.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/platform/win32/win32_access.cpp` | [Т-198](tasks/2026-11-22-review-T195-T202.md#t-198) |
| `src/platform/win32/win32_commands.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `src/platform/win32/win32_cursor.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `src/platform/win32/win32_drop.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_drop_data.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_drop_events.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_drop_target.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_events.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `src/platform/win32/win32_file_dialog.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_file_dialog_setup.cpp` | [Т-197](tasks/2026-11-22-review-T195-T202.md#t-197) |
| `src/platform/win32/win32_frame.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_frame_hit.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_frame_messages.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_frame_mouse.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_input.cpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `src/platform/win32/win32_keyboard.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `src/platform/win32/win32_owner.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_paint.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_placement.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_procedure.cpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `src/platform/win32/win32_system.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_text.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `src/platform/win32/win32_wake.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/win32/win32_window.cpp` | [Т-195](tasks/2026-11-22-review-T195-T202.md#t-195) |
| `src/platform/window_commands.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/window_defaults.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/window_frame.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/window_owner_defaults.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `src/platform/x11/x11_access.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_clipboard.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_commands.cpp` | [Т-201](tasks/2026-11-22-review-T195-T202.md#t-201) |
| `src/platform/x11/x11_create.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_cursor.cpp` | [Т-201](tasks/2026-11-22-review-T195-T202.md#t-201) |
| `src/platform/x11/x11_dispatch.cpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `src/platform/x11/x11_drop.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_drop_data.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_drop_events.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_drop_log.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_events.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_file_dialog.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_file_dialog_run.cpp` | [Т-202](tasks/2026-11-22-review-T195-T202.md#t-202) |
| `src/platform/x11/x11_frame.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_keyboard.cpp` | [Т-201](tasks/2026-11-22-review-T195-T202.md#t-201) |
| `src/platform/x11/x11_mouse.cpp` | [Т-43](tasks/2026-11-13-platform-contract.md) |
| `src/platform/x11/x11_owner.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_sync.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_system.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_text.cpp` | [Т-201](tasks/2026-11-22-review-T195-T202.md#t-201) |
| `src/platform/x11/x11_wait.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |
| `src/platform/x11/x11_window.cpp` | [Т-200](tasks/2026-11-22-review-T195-T202.md#t-200) |

</details>

<details><summary>src/render · 36</summary>

| Файл | Задача |
|---|---|
| `src/render/content_sizer.cpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |
| `src/render/gradient_ramps.cpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `src/render/image_store.cpp` | [Т-203](tasks/2026-11-22-review-T203-T210.md#t-203) |
| `src/render/pixel_compare.cpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `src/render/ramp_fill.cpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `src/render/renderer.cpp` | [Т-78](tasks/2026-11-22-early-code-review.md#t-78) |
| `src/render/renderer_textures.cpp` | [Т-78](tasks/2026-11-22-early-code-review.md#t-78) |
| `src/render/scene/instance_place.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_backdrop.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_box.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_caret.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/scene/paint_controls.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_dock.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_expander.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/render/scene/paint_fill.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_grid.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/render/scene/paint_grip.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_icon.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_image.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_layers.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_range.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_runs.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/scene/paint_scrollbar.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_shapes.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/paint_shortcut.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/scene/paint_state.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/render/scene/paint_tab_close.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/paint_text.cpp` | [Т-78](tasks/2026-11-22-early-code-review.md#t-78) |
| `src/render/scene/paint_toggles.cpp` | [Т-247](tasks/2026-11-22-review-T243-T247.md#t-247) |
| `src/render/scene/scene_builder.cpp` | [Т-206](tasks/2026-11-22-review-T203-T210.md#t-206) |
| `src/render/scene/text_lines.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/scene/text_place.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/scene/text_reveal.cpp` | [Т-207](tasks/2026-11-22-review-T203-T210.md#t-207) |
| `src/render/text_engine.cpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |
| `src/render/text_mask.cpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |
| `src/render/text_style.cpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |

</details>

<details><summary>src/res · 9</summary>

| Файл | Задача |
|---|---|
| `src/res/blob.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `src/res/files.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `src/res/generate_blob.cpp` | [Т-209](tasks/2026-11-22-review-T203-T210.md#t-209) |
| `src/res/generate_source.cpp` | [Т-209](tasks/2026-11-22-review-T203-T210.md#t-209) |
| `src/res/manifest_folder.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/res/manifest_read.cpp` | [Т-209](tasks/2026-11-22-review-T203-T210.md#t-209) |
| `src/res/normalize.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `src/res/registry.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `src/res/streams.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |

</details>

<details><summary>src/rhi · 100</summary>

| Файл | Задача |
|---|---|
| `src/rhi/adapter_choice.cpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `src/rhi/dx11/dx11_adapter.cpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `src/rhi/dx11/dx11_backdrop.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_backend.cpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `src/rhi/dx11/dx11_buffers.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_check.cpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `src/rhi/dx11/dx11_device.cpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `src/rhi/dx11/dx11_draw.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_frame.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_pipeline.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_readback.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx11/dx11_surface.cpp` | [Т-234](tasks/2026-11-22-review-T227-T234.md#t-234) |
| `src/rhi/dx11/dx11_textures.cpp` | [Т-235](tasks/2026-11-22-review-T235-T242.md#t-235) |
| `src/rhi/dx12/deferred_releases.cpp` | [Т-76](tasks/2026-11-22-early-code-review.md#t-76) |
| `src/rhi/dx12/dx12_adapter.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dx12_backdrop.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/dx12_backdrop_chain.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/dx12_backend.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dx12_check.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dx12_debug.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dx12_device.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dx12_frame.cpp` | [Т-76](tasks/2026-11-22-early-code-review.md#t-76) |
| `src/rhi/dx12/dx12_pipeline.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/dx12_pipeline_blur.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/dx12_record.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/dx12_textures.cpp` | [Т-76](tasks/2026-11-22-early-code-review.md#t-76) |
| `src/rhi/dx12/dxgi_adapter.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/dxgi_lost.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/fence_sync.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/dx12/offscreen_readback.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/dx12/offscreen_surface.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/dx12/platform_backend.cpp` | [Т-236](tasks/2026-11-22-review-T235-T242.md#t-236) |
| `src/rhi/dx12/swapchain_present.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/dx12/swapchain_surface.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/dx12/texture_table.cpp` | [Т-76](tasks/2026-11-22-early-code-review.md#t-76) |
| `src/rhi/dx12/texture_update.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/texture_upload.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/texture_views.cpp` | [Т-238](tasks/2026-11-22-review-T235-T242.md#t-238) |
| `src/rhi/dx12/upload_ring.cpp` | [Т-237](tasks/2026-11-22-review-T235-T242.md#t-237) |
| `src/rhi/metal/metal_backdrop.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/metal_backend.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/metal_check.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/metal_device.mm` | [Т-242](tasks/2026-11-22-review-T235-T242.md#t-242) |
| `src/rhi/metal/metal_draw.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/metal_encoder.mm` | [Т-242](tasks/2026-11-22-review-T235-T242.md#t-242) |
| `src/rhi/metal/metal_frame.mm` | [Т-49](tasks/2026-11-15-metal.md), [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/rhi/metal/metal_present.mm` | [Т-51](tasks/2026-11-15-cocoa.md) |
| `src/rhi/metal/metal_readback.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/metal_textures.mm` | [Т-49](tasks/2026-11-15-metal.md) |
| `src/rhi/metal/platform_backend.cpp` | [Т-242](tasks/2026-11-22-review-T235-T242.md#t-242) |
| `src/rhi/none/platform_backend.cpp` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `src/rhi/null/null_backend.cpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `src/rhi/null/null_textures.cpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `src/rhi/render_backend.cpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `src/rhi/shaders_gen/blur_down_ps_dxbc.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/blur_down_ps_dxil.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/blur_down_ps_spv.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/blur_up_ps_dxbc.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/blur_up_ps_dxil.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/blur_up_ps_spv.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/fullscreen_vs_dxbc.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/fullscreen_vs_dxil.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/fullscreen_vs_spv.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/msl/blur_down_ps.metal` | [Т-112](tasks/2026-11-22-review-T107-T114.md#t-112) |
| `src/rhi/shaders_gen/msl/blur_up_ps.metal` | [Т-112](tasks/2026-11-22-review-T107-T114.md#t-112) |
| `src/rhi/shaders_gen/msl/fullscreen_vs.metal` | [Т-112](tasks/2026-11-22-review-T107-T114.md#t-112) |
| `src/rhi/shaders_gen/msl/ui_ps.metal` | [Т-112](tasks/2026-11-22-review-T107-T114.md#t-112) |
| `src/rhi/shaders_gen/msl/ui_vs.metal` | [Т-112](tasks/2026-11-22-review-T107-T114.md#t-112) |
| `src/rhi/shaders_gen/ui_ps_dxbc.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/ui_ps_dxil.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/ui_ps_spv.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/ui_vs_dxbc.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/ui_vs_dxil.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/shaders_gen/ui_vs_spv.cpp` | [Т-111](tasks/2026-11-22-review-T107-T114.md#t-111) |
| `src/rhi/vulkan/pipeline_state.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/platform_backend.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_adapter.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_backend.cpp` | [Т-85](tasks/2026-11-22-early-code-review.md#t-85) |
| `src/rhi/vulkan/vulkan_blur.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_buffers.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_check.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_descriptors.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_device.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_draw.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_frame.cpp` | [Т-85](tasks/2026-11-22-early-code-review.md#t-85) |
| `src/rhi/vulkan/vulkan_frame_resources.cpp` | [Т-85](tasks/2026-11-22-early-code-review.md#t-85) |
| `src/rhi/vulkan/vulkan_images.cpp` | [Т-240](tasks/2026-11-22-review-T235-T242.md#t-240) |
| `src/rhi/vulkan/vulkan_instance.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_layout.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_loader.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_names.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_pipeline.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_present.cpp` | [Т-240](tasks/2026-11-22-review-T235-T242.md#t-240) |
| `src/rhi/vulkan/vulkan_present_wait.cpp` | [Т-240](tasks/2026-11-22-review-T235-T242.md#t-240) |
| `src/rhi/vulkan/vulkan_readback.cpp` | [Т-240](tasks/2026-11-22-review-T235-T242.md#t-240) |
| `src/rhi/vulkan/vulkan_surface.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |
| `src/rhi/vulkan/vulkan_swapchain.cpp` | [Т-85](tasks/2026-11-22-early-code-review.md#t-85) |
| `src/rhi/vulkan/vulkan_textures.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_upload.cpp` | [Т-241](tasks/2026-11-22-review-T235-T242.md#t-241) |
| `src/rhi/vulkan/vulkan_validation.cpp` | [Т-239](tasks/2026-11-22-review-T235-T242.md#t-239) |

</details>

<details><summary>src/settings · 7</summary>

| Файл | Задача |
|---|---|
| `src/settings/settings_edit.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `src/settings/settings_file.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/settings/settings_layout.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `src/settings/settings_read.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `src/settings/settings_values.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `src/settings/settings_variables.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `src/settings/settings_write.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |

</details>

<details><summary>src/style · 27</summary>

| Файл | Задача |
|---|---|
| `src/style/apply_declaration.cpp` | [Т-79](tasks/2026-11-22-early-code-review.md#t-79) |
| `src/style/cascade.cpp` | [Т-79](tasks/2026-11-22-early-code-review.md#t-79) |
| `src/style/keyword_tables.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/gradient_stops.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `src/style/parse/named_colors.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `src/style/parse/parse_animation.cpp` | [Т-215](tasks/2026-11-22-review-T211-T218.md#t-215) |
| `src/style/parse/parse_box.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/parse_color.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `src/style/parse/parse_gradient.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `src/style/parse/parse_integer.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/parse_keyword.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/parse_motion.cpp` | [Т-215](tasks/2026-11-22-review-T211-T218.md#t-215) |
| `src/style/parse/parse_numbers.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/parse_paint.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `src/style/parse/parse_tracks.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/parse_transform.cpp` | [Т-215](tasks/2026-11-22-review-T211-T218.md#t-215) |
| `src/style/parse/value_scanner.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/value_scanner_numbers.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/value_scanner_split.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/value_scanner_tokens.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/parse/value_support.cpp` | [Т-212](tasks/2026-11-22-review-T211-T218.md#t-212) |
| `src/style/property_parse.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `src/style/property_set.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `src/style/property_table.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `src/style/style_compare.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `src/style/style_compare_values.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `src/style/style_sheet.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |

</details>

<details><summary>src/text · 33</summary>

| Файл | Задача |
|---|---|
| `src/text/atlas/atlas_insert.cpp` | [Т-220](tasks/2026-11-22-review-T219-T226.md#t-220) |
| `src/text/atlas/atlas_key.cpp` | [Т-220](tasks/2026-11-22-review-T219-T226.md#t-220) |
| `src/text/atlas/atlas_packing.cpp` | [Т-78](tasks/2026-11-22-early-code-review.md#t-78) |
| `src/text/atlas/glyph_atlas.cpp` | [Т-78](tasks/2026-11-22-early-code-review.md#t-78) |
| `src/text/font/font_cmap.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_component.cpp` | [Т-217](tasks/2026-11-22-review-T211-T218.md#t-217) |
| `src/text/font/font_composite.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/text/font/font_coverage.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_glyf.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/text/font/font_gpos.cpp` | [Т-217](tasks/2026-11-22-review-T211-T218.md#t-217) |
| `src/text/font/font_gpos_setup.cpp` | [Т-217](tasks/2026-11-22-review-T211-T218.md#t-217) |
| `src/text/font/font_kern.cpp` | [Т-217](tasks/2026-11-22-review-T211-T218.md#t-217) |
| `src/text/font/font_load.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_loca.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/text/font/font_metrics.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_names.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_os2.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_reader.cpp` | [Т-41](tasks/2026-11-12-code-review.md) |
| `src/text/font/font_reader_wide.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/font/font_tables.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `src/text/layout/break_rules.cpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |
| `src/text/layout/line_breaker.cpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |
| `src/text/layout/text_layout.cpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |
| `src/text/layout/text_runs.cpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |
| `src/text/layout/text_shaper.cpp` | [Т-219](tasks/2026-11-22-review-T219-T226.md#t-219) |
| `src/text/library/font_files.cpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `src/text/library/font_known.cpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `src/text/library/font_library.cpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `src/text/library/font_matching.cpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `src/text/library/font_scan.cpp` | [Т-218](tasks/2026-11-22-review-T211-T218.md#t-218) |
| `src/text/raster/glyph_rasterizer.cpp` | [Т-86](tasks/2026-11-22-early-code-review.md#t-86) |
| `src/text/raster/raster_accumulator.cpp` | [Т-86](tasks/2026-11-22-early-code-review.md#t-86) |
| `src/text/raster/raster_cells.cpp` | [Т-86](tasks/2026-11-22-early-code-review.md#t-86) |

</details>

<details><summary>src/ui · 45</summary>

| Файл | Задача |
|---|---|
| `src/ui/choice_group.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |
| `src/ui/combo_box.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |
| `src/ui/command_bind.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/data_grid_build.cpp` | [Т-63](tasks/2026-11-17-data-grid.md), [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/ui/data_grid_columns.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/data_grid_edit.cpp` | [Т-69](tasks/2026-11-18-grid-edit.md) |
| `src/ui/data_grid_layout_text.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/data_grid_lines.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/data_grid_marks.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/ui/data_grid_pinned.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md), [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/ui/data_grid_query.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/ui/data_grid_resize.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/data_grid_reveal.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/data_grid_tree.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/ui/data_grid_view.cpp` | [Т-66](tasks/2026-11-18-grid-columns.md) |
| `src/ui/dock_build.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/dock_query.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/layer_tree.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/menu_bar.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/menu_shortcut.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/node.cpp` | [Т-79](tasks/2026-11-22-early-code-review.md#t-79) |
| `src/ui/node_kinds.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/node_serial.cpp` | [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/ui/node_state.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/node_state_restore.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/node_transform.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/node_tree.cpp` | [Т-63](tasks/2026-11-17-data-grid.md) |
| `src/ui/scroll_bar.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |
| `src/ui/shortcut_label.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/shortcut_parse.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/shortcut_symbols.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/splitter.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/status_bar.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |
| `src/ui/style_pass.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/ui/tabs.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/tabs_build.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/tabs_edit.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/tabs_query.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `src/ui/tool_bar.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/tool_bar_sync.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `src/ui/tree_view_build.cpp` | [Т-72](tasks/2026-11-19-tree-view.md) |
| `src/ui/tree_view_query.cpp` | [Т-72](tasks/2026-11-19-tree-view.md), [Т-75](tasks/2026-11-20-grid-access.md) |
| `src/ui/ui_event.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/ui_event_make.cpp` | [Т-221](tasks/2026-11-22-review-T219-T226.md#t-221) |
| `src/ui/window_frame.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |

</details>

<details><summary>src/undo · 6</summary>

| Файл | Задача |
|---|---|
| `src/undo/action_step.cpp` | [Т-227](tasks/2026-11-22-review-T227-T234.md#t-227) |
| `src/undo/undo_info.cpp` | [Т-81](tasks/2026-11-22-early-code-review.md#t-81) |
| `src/undo/undo_push.cpp` | [Т-81](tasks/2026-11-22-early-code-review.md#t-81) |
| `src/undo/undo_stack.cpp` | [Т-81](tasks/2026-11-22-early-code-review.md#t-81) |
| `src/undo/undo_state.cpp` | [Т-81](tasks/2026-11-22-early-code-review.md#t-81) |
| `src/undo/undo_step.cpp` | [Т-227](tasks/2026-11-22-review-T227-T234.md#t-227) |

</details>

<details><summary>src/utils · 21</summary>

| Файл | Задача |
|---|---|
| `src/utils/color.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `src/utils/fs/file_list.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/file_places_posix.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/file_places_windows.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/file_query.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/file_system_posix.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/file_system_windows.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/path_join.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/path_parts.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/path_root.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/fs/system_path_windows.cpp` | [Т-228](tasks/2026-11-22-review-T227-T234.md#t-228) |
| `src/utils/fs/wide_path_windows.cpp` | [Т-57](tasks/2026-11-15-release-archives.md) |
| `src/utils/geometry_affine.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `src/utils/geometry_rect.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `src/utils/mapped_file.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `src/utils/mapped_file_os.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `src/utils/png_checksums.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `src/utils/png_writer.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `src/utils/program_arguments.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `src/utils/uri_list.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `src/utils/utf8.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |

</details>

<details><summary>src/vector · 11</summary>

| Файл | Задача |
|---|---|
| `src/vector/path_arc.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_commands.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_curves.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_parser.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_points.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_scanner.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/path_tokens.cpp` | [Т-231](tasks/2026-11-22-review-T227-T234.md#t-231) |
| `src/vector/shape_raster.cpp` | [Т-86](tasks/2026-11-22-early-code-review.md#t-86) |
| `src/vector/svg_elements.cpp` | [Т-232](tasks/2026-11-22-review-T227-T234.md#t-232) |
| `src/vector/svg_loader.cpp` | [Т-232](tasks/2026-11-22-review-T227-T234.md#t-232) |
| `src/vector/view_box.cpp` | [Т-232](tasks/2026-11-22-review-T227-T234.md#t-232) |

</details>

<details><summary>tests/crash · 1</summary>

| Файл | Задача |
|---|---|
| `tests/crash/crash_probe.cpp` | [Т-130](tasks/2026-11-22-review-T123-T130.md#t-130) |

</details>

<details><summary>tests/editor · 2</summary>

| Файл | Задача |
|---|---|
| `tests/editor/editor_tests.cpp` | [Т-129](tasks/2026-11-22-review-T123-T130.md#t-129) |
| `tests/editor/run.ps1` | [Т-129](tasks/2026-11-22-review-T123-T130.md#t-129) |

</details>

<details><summary>tests/include · 5</summary>

| Файл | Задача |
|---|---|
| `tests/include/framework/test_framework.hpp` | [Т-243](tasks/2026-11-22-review-T243-T247.md#t-243) |
| `tests/include/suites/test_suites.hpp` | [Т-243](tasks/2026-11-22-review-T243-T247.md#t-243) |
| `tests/include/support/font_samples.hpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |
| `tests/include/support/jpeg_samples.hpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |
| `tests/include/support/layout_helpers.hpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |

</details>

<details><summary>tests/linux · 11</summary>

| Файл | Задача |
|---|---|
| `tests/linux/linux_backend.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_clipboard.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_drop.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_drop_peer.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_input.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_main.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_scene.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_tests.hpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_texture.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/linux_window.cpp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |
| `tests/linux/lsan_mesa.supp` | [Т-125](tasks/2026-11-22-review-T123-T130.md#t-125) |

</details>

<details><summary>tests/macos · 9</summary>

| Файл | Задача |
|---|---|
| `tests/macos/macos_backend.cpp` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_cocoa.mm` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_input.mm` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_main.cpp` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_review.mm` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_scene.cpp` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_snapshots.cpp` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_tests.hpp` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |
| `tests/macos/macos_window.mm` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |

</details>

<details><summary>tests/manual · 2</summary>

| Файл | Задача |
|---|---|
| `tests/manual/files/horizontal_scroll.xml` | [Т-117](tasks/2026-11-22-review-T115-T122.md#t-117) |
| `tests/manual/tools/memory_watch.ps1` | [Т-117](tasks/2026-11-22-review-T115-T122.md#t-117) |

</details>

<details><summary>tests/metal · 1</summary>

| Файл | Задача |
|---|---|
| `tests/metal/layout_check.metal` | [Т-126](tasks/2026-11-22-review-T123-T130.md#t-126) |

</details>

<details><summary>tests/render · 19</summary>

| Файл | Задача |
|---|---|
| `tests/render/backend_live.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/headless_live.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/reload_live.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/render_compare.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/render_main.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/render_scene.cpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/render_tests.hpp` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/resize_artifacts.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_capture.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_capture.hpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_drag.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_drag.hpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_duplication.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_duplication.hpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_live.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/resize_watcher.cpp` | [Т-127](tasks/2026-11-22-review-T123-T130.md#t-127) |
| `tests/render/scenes/glass.xml` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/scenes/primitives.xml` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |
| `tests/render/scenes/text_image.xml` | [Т-128](tasks/2026-11-22-review-T123-T130.md#t-128) |

</details>

<details><summary>tests/src · 111</summary>

| Файл | Задача |
|---|---|
| `tests/src/a11y/access_action_tests.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `tests/src/a11y/accessibility_tests.cpp` | [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131) |
| `tests/src/a11y/grid_access_tests.cpp` | [Т-139](tasks/2026-11-22-review-T139-T146.md#t-139) |
| `tests/src/a11y/menu_access_tests.cpp` | [Т-131](tasks/2026-11-22-review-T131-T138.md#t-131) |
| `tests/src/a11y/uia_grid_tests.cpp` | [Т-199](tasks/2026-11-22-review-T195-T202.md#t-199) |
| `tests/src/app/app_driver_life_tests.cpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `tests/src/app/app_driver_tests.cpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `tests/src/app/app_undo_tests.cpp` | [Т-147](tasks/2026-11-22-review-T147-T154.md#t-147) |
| `tests/src/app/builtin_command_tests.cpp` | [Т-138](tasks/2026-11-22-review-T131-T138.md#t-138) |
| `tests/src/app/dock_float_tests.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `tests/src/app/driver_field_tests.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `tests/src/app/driver_key_tests.cpp` | [Т-137](tasks/2026-11-22-review-T131-T138.md#t-137) |
| `tests/src/app/expression_tests.cpp` | [Т-148](tasks/2026-11-22-review-T147-T154.md#t-148) |
| `tests/src/app/frame_rebuild_tests.cpp` | [Т-135](tasks/2026-11-22-review-T131-T138.md#t-135) |
| `tests/src/app/keyframe_animator_tests.cpp` | [Т-132](tasks/2026-11-22-review-T131-T138.md#t-132) |
| `tests/src/app/list_binding_tests.cpp` | [Т-150](tasks/2026-11-22-review-T147-T154.md#t-150) |
| `tests/src/app/live_app_tests.cpp` | [Т-134](tasks/2026-11-22-review-T131-T138.md#t-134) |
| `tests/src/app/model_markup_tests.cpp` | [Т-142](tasks/2026-11-22-review-T139-T146.md#t-142) |
| `tests/src/app/reload_tests.cpp` | [Т-141](tasks/2026-11-22-review-T139-T146.md#t-141) |
| `tests/src/app/task_pool_tests.cpp` | [Т-143](tasks/2026-11-22-review-T139-T146.md#t-143) |
| `tests/src/app/timer_queue_tests.cpp` | [Т-144](tasks/2026-11-22-review-T139-T146.md#t-144) |
| `tests/src/app/web_view_tests.cpp` | [Т-146](tasks/2026-11-22-review-T139-T146.md#t-146) |
| `tests/src/app/window_owner_tests.cpp` | [Т-136](tasks/2026-11-22-review-T131-T138.md#t-136) |
| `tests/src/binding/extent_index_tests.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `tests/src/capi/capi_tests.cpp` | [Т-153](tasks/2026-11-22-review-T147-T154.md#t-153) |
| `tests/src/capi/version_tests.cpp` | [Т-153](tasks/2026-11-22-review-T147-T154.md#t-153) |
| `tests/src/chat/chat_json_tests.cpp` | [Т-160](tasks/2026-11-22-review-T155-T162.md#t-160) |
| `tests/src/chat/chat_markdown_tests.cpp` | [Т-245](tasks/2026-11-22-review-T243-T247.md#t-245) |
| `tests/src/chat/chat_stream_tests.cpp` | [Т-245](tasks/2026-11-22-review-T243-T247.md#t-245) |
| `tests/src/core/i18n_tests.cpp` | [Т-157](tasks/2026-11-22-review-T155-T162.md#t-157) |
| `tests/src/data/cell_value_tests.cpp` | [Т-158](tasks/2026-11-22-review-T155-T162.md#t-158) |
| `tests/src/data/column_filter_tests.cpp` | [Т-159](tasks/2026-11-22-review-T155-T162.md#t-159) |
| `tests/src/data/data_list_tests.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `tests/src/data/data_source_tests.cpp` | [Т-161](tasks/2026-11-22-review-T155-T162.md#t-161) |
| `tests/src/data/model_signal_tests.cpp` | [Т-162](tasks/2026-11-22-review-T155-T162.md#t-162) |
| `tests/src/data/selection_model_tests.cpp` | [Т-163](tasks/2026-11-22-review-T163-T170.md#t-163) |
| `tests/src/data/sort_filter_tests.cpp` | [Т-164](tasks/2026-11-22-review-T163-T170.md#t-164) |
| `tests/src/data/tree_selection_tests.cpp` | [Т-166](tasks/2026-11-22-review-T163-T170.md#t-166) |
| `tests/src/framework/assertions.cpp` | [Т-243](tasks/2026-11-22-review-T243-T247.md#t-243) |
| `tests/src/framework/test_runner.cpp` | [Т-243](tasks/2026-11-22-review-T243-T247.md#t-243) |
| `tests/src/image/inflate_limit_tests.cpp` | [Т-167](tasks/2026-11-22-review-T163-T170.md#t-167) |
| `tests/src/image/jpeg_tests.cpp` | [Т-168](tasks/2026-11-22-review-T163-T170.md#t-168) |
| `tests/src/input/command_tests.cpp` | [Т-173](tasks/2026-11-22-review-T171-T178.md#t-173) |
| `tests/src/input/dialog_tests.cpp` | [Т-145](tasks/2026-11-22-review-T139-T146.md#t-145) |
| `tests/src/input/event_input_tests.cpp` | [Т-170](tasks/2026-11-22-review-T163-T170.md#t-170) |
| `tests/src/input/event_routing_tests.cpp` | [Т-171](tasks/2026-11-22-review-T171-T178.md#t-171) |
| `tests/src/input/menu_bar_tests.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `tests/src/input/popup_tests.cpp` | [Т-174](tasks/2026-11-22-review-T171-T178.md#t-174) |
| `tests/src/input/scroll_bar_tests.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `tests/src/input/text_area_tests.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `tests/src/input/text_key_tests.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `tests/src/input/text_selection_tests.cpp` | [Т-176](tasks/2026-11-22-review-T171-T178.md#t-176) |
| `tests/src/input/text_step_tests.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `tests/src/input/text_undo_tests.cpp` | [Т-177](tasks/2026-11-22-review-T171-T178.md#t-177) |
| `tests/src/input/widget_tests.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `tests/src/input/window_frame_tests.cpp` | [Т-175](tasks/2026-11-22-review-T171-T178.md#t-175) |
| `tests/src/layout/flex_auto_min_tests.cpp` | [Т-179](tasks/2026-11-22-review-T179-T186.md#t-179) |
| `tests/src/layout/incremental_layout_tests.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `tests/src/layout/layout_cache_tests.cpp` | [Т-178](tasks/2026-11-22-review-T171-T178.md#t-178) |
| `tests/src/layout/layout_grid_tests.cpp` | [Т-180](tasks/2026-11-22-review-T179-T186.md#t-180) |
| `tests/src/layout/layout_stack_tests.cpp` | [Т-179](tasks/2026-11-22-review-T179-T186.md#t-179) |
| `tests/src/layout/variable_list_tests.cpp` | [Т-60](tasks/2026-11-16-variable-rows.md) |
| `tests/src/log/log_tests.cpp` | [Т-182](tasks/2026-11-22-review-T179-T186.md#t-182) |
| `tests/src/main.cpp` | [Т-243](tasks/2026-11-22-review-T243-T247.md#t-243) |
| `tests/src/markup/component_tests.cpp` | [Т-185](tasks/2026-11-22-review-T179-T186.md#t-185) |
| `tests/src/markup/markup_tests.cpp` | [Т-184](tasks/2026-11-22-review-T179-T186.md#t-184) |
| `tests/src/markup/rich_text_tests.cpp` | [Т-186](tasks/2026-11-22-review-T179-T186.md#t-186) |
| `tests/src/platform/headless_window_tests.cpp` | [Т-190](tasks/2026-11-22-review-T187-T194.md#t-190) |
| `tests/src/platform/win32_keymenu_tests.cpp` | [Т-196](tasks/2026-11-22-review-T195-T202.md#t-196) |
| `tests/src/platform/window_frame_tests.cpp` | [Т-189](tasks/2026-11-22-review-T187-T194.md#t-189) |
| `tests/src/render/adapter_choice_tests.cpp` | [Т-233](tasks/2026-11-22-review-T227-T234.md#t-233) |
| `tests/src/render/password_mask_tests.cpp` | [Т-205](tasks/2026-11-22-review-T203-T210.md#t-205) |
| `tests/src/render/pixel_compare_tests.cpp` | [Т-204](tasks/2026-11-22-review-T203-T210.md#t-204) |
| `tests/src/res/resources_tests.cpp` | [Т-208](tasks/2026-11-22-review-T203-T210.md#t-208) |
| `tests/src/settings/settings_tests.cpp` | [Т-210](tasks/2026-11-22-review-T203-T210.md#t-210) |
| `tests/src/style/input_bounds_tests.cpp` | [Т-211](tasks/2026-11-22-review-T211-T218.md#t-211) |
| `tests/src/style/style_color_tests.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `tests/src/style/style_keyframes_tests.cpp` | [Т-215](tasks/2026-11-22-review-T211-T218.md#t-215) |
| `tests/src/style/style_motion_tests.cpp` | [Т-215](tasks/2026-11-22-review-T211-T218.md#t-215) |
| `tests/src/style/style_number_tests.cpp` | [Т-213](tasks/2026-11-22-review-T211-T218.md#t-213) |
| `tests/src/style/style_paint_tests.cpp` | [Т-214](tasks/2026-11-22-review-T211-T218.md#t-214) |
| `tests/src/style/style_track_tests.cpp` | [Т-213](tasks/2026-11-22-review-T211-T218.md#t-213) |
| `tests/src/support/font_samples.cpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |
| `tests/src/support/jpeg_samples.cpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |
| `tests/src/support/layout_helpers.cpp` | [Т-244](tasks/2026-11-22-review-T243-T247.md#t-244) |
| `tests/src/text/font_damage_tests.cpp` | [Т-216](tasks/2026-11-22-review-T211-T218.md#t-216) |
| `tests/src/ui/cell_edit_event_tests.cpp` | [Т-140](tasks/2026-11-22-review-T139-T146.md#t-140) |
| `tests/src/ui/data_grid_tests.cpp` | [Т-222](tasks/2026-11-22-review-T219-T226.md#t-222) |
| `tests/src/ui/dock_tests.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `tests/src/ui/document_tabs_tests.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `tests/src/ui/grid_column_tests.cpp` | [Т-222](tasks/2026-11-22-review-T219-T226.md#t-222) |
| `tests/src/ui/grid_edit_more_tests.cpp` | [Т-140](tasks/2026-11-22-review-T139-T146.md#t-140) |
| `tests/src/ui/grid_edit_tests.cpp` | [Т-140](tasks/2026-11-22-review-T139-T146.md#t-140) |
| `tests/src/ui/grid_layout_text_tests.cpp` | [Т-222](tasks/2026-11-22-review-T219-T226.md#t-222) |
| `tests/src/ui/grid_review_tests.cpp` | [Т-223](tasks/2026-11-22-review-T219-T226.md#t-223) |
| `tests/src/ui/shortcut_symbols_tests.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `tests/src/ui/shortcut_tests.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `tests/src/ui/splitter_tests.cpp` | [Т-224](tasks/2026-11-22-review-T219-T226.md#t-224) |
| `tests/src/ui/status_bar_tests.cpp` | [Т-226](tasks/2026-11-22-review-T219-T226.md#t-226) |
| `tests/src/ui/tool_bar_tests.cpp` | [Т-225](tasks/2026-11-22-review-T219-T226.md#t-225) |
| `tests/src/ui/tree_grid_tests.cpp` | [Т-223](tasks/2026-11-22-review-T219-T226.md#t-223) |
| `tests/src/ui/tree_view_tests.cpp` | [Т-223](tasks/2026-11-22-review-T219-T226.md#t-223) |
| `tests/src/undo/undo_stack_tests.cpp` | [Т-227](tasks/2026-11-22-review-T227-T234.md#t-227) |
| `tests/src/utils/file_system_edge_tests.cpp` | [Т-228](tasks/2026-11-22-review-T227-T234.md#t-228) |
| `tests/src/utils/file_system_tests.cpp` | [Т-228](tasks/2026-11-22-review-T227-T234.md#t-228) |
| `tests/src/utils/geometry_tests.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `tests/src/utils/mapped_file_tests.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `tests/src/utils/path_text_tests.cpp` | [Т-228](tasks/2026-11-22-review-T227-T234.md#t-228) |
| `tests/src/utils/uri_list_tests.cpp` | [Т-229](tasks/2026-11-22-review-T227-T234.md#t-229) |
| `tests/src/utils/utf8_png_tests.cpp` | [Т-230](tasks/2026-11-22-review-T227-T234.md#t-230) |
| `tests/src/vector/vector_shape_tests.cpp` | [Т-232](tasks/2026-11-22-review-T227-T234.md#t-232) |

</details>

<details><summary>tools · 14</summary>

| Файл | Задача |
|---|---|
| `tools/ci/annotate_log.sh` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `tools/ci/gitverse_prepare.sh` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `tools/embed/embed_main.cpp` | [Т-116](tasks/2026-11-22-review-T115-T122.md#t-116) |
| `tools/rcc/rcc_main.cpp` | [Т-116](tasks/2026-11-22-review-T115-T122.md#t-116) |
| `tools/release/build_linux.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/build_windows.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/github_assets.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/gitverse_release.ps1` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/gitverse_release.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/lib/release_common.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/release/release_notes.sh` | [Т-115](tasks/2026-11-22-review-T115-T122.md#t-115) |
| `tools/samples/make_notes.py` | [Т-116](tasks/2026-11-22-review-T115-T122.md#t-116) |
| `tools/uia_scroll_check.ps1` | [Т-117](tasks/2026-11-22-review-T115-T122.md#t-117) |
| `tools/uia_walk.ps1` | [Т-117](tasks/2026-11-22-review-T115-T122.md#t-117) |

</details>

<details><summary>Корень · 7</summary>

| Файл | Задача |
|---|---|
| `.clang-tidy` | [Т-114](tasks/2026-11-22-review-T107-T114.md#t-114) |
| `CMakeLists.txt` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `CONTRIBUTING.en.md` | [Т-124](tasks/2026-11-22-review-T123-T130.md#t-124) |
| `CONTRIBUTING.md` | [Т-124](tasks/2026-11-22-review-T123-T130.md#t-124) |
| `Makefile` | [Т-48](tasks/2026-11-14-macos-core.md) |
| `README.en.md` | [Т-123](tasks/2026-11-22-review-T123-T130.md#t-123) |
| `README.md` | [Т-123](tasks/2026-11-22-review-T123-T130.md#t-123) |

</details>

### Остальные отслеживаемые файлы

Вне области чтения кода осталось 69 файлов среза.

<details><summary>Журнал и сценарии прежних ручных проверок · 28</summary>

- `tests/manual/README.md`
- `tests/manual/tasks/2026-11-04-release-0.1.0.md`
- `tests/manual/tasks/2026-11-06-retest-fixes.md`
- `tests/manual/tasks/2026-11-08-test-stand.md`
- `tests/manual/tasks/2026-11-09-linux-platform.md`
- `tests/manual/tasks/2026-11-09-linux-resize.md`
- `tests/manual/tasks/2026-11-09-linux-xdnd.md`
- `tests/manual/tasks/2026-11-09-menu-bar.md`
- `tests/manual/tasks/2026-11-09-toolbar.md`
- `tests/manual/tasks/2026-11-10-dock-panels.md`
- `tests/manual/tasks/2026-11-10-document-tabs.md`
- `tests/manual/tasks/2026-11-10-splitter.md`
- `tests/manual/tasks/2026-11-10-status-bar.md`
- `tests/manual/tasks/2026-11-12-code-review.md`
- `tests/manual/tasks/2026-11-12-frame-fixes.md`
- `tests/manual/tasks/2026-11-13-headless-driver.md`
- `tests/manual/tasks/2026-11-13-platform-contract.md`
- `tests/manual/tasks/2026-11-14-macos-core.md`
- `tests/manual/tasks/2026-11-14-review-fixes.md`
- `tests/manual/tasks/2026-11-15-cocoa.md`
- `tests/manual/tasks/2026-11-15-metal.md`
- `tests/manual/tasks/2026-11-15-release-archives.md`
- `tests/manual/tasks/2026-11-16-variable-rows.md`
- `tests/manual/tasks/2026-11-17-data-grid.md`
- `tests/manual/tasks/2026-11-18-grid-columns.md`
- `tests/manual/tasks/2026-11-18-grid-edit.md`
- `tests/manual/tasks/2026-11-19-tree-view.md`
- `tests/manual/tasks/2026-11-20-grid-access.md`

</details>

<details><summary>Лицензия и метаданные Git · 4</summary>

- `.gitattributes`
- `.gitignore`
- `LICENSE`
- `bindings/.gitignore`

</details>

<details><summary>Растровые материалы и тестовые данные · 24</summary>

- `docs/images/components.png`
- `docs/images/editor_test.png`
- `docs/images/glass.png`
- `docs/images/grid.png`
- `docs/images/models.png`
- `docs/images/showcase.png`
- `samples/assets/orb.png`
- `samples/assets/photo.jpg`
- `tests/manual/results/2026-11-20-T-73/1-table-without-name.webp`
- `tests/manual/results/2026-11-20-T-73/2a-row-elementnotavailable.jpg`
- `tests/manual/results/2026-11-20-T-73/2b-row-elementnotavailable.jpg`
- `tests/manual/results/2026-11-20-T-73/2c-row-elementnotavailable.jpg`
- `tests/manual/results/2026-11-20-T-73/3-after-structure-event-still-unavailable.jpg`
- `tests/manual/results/2026-11-20-T-73/4-virtual-row-runtimeid-legacy-errors.jpg`
- `tests/manual/results/2026-11-20-T-73/5-rows-without-children.jpg`
- `tests/manual/results/2026-11-20-T-73/6-visible-row-without-children-in-tree.webp`
- `tests/render/baselines/glass-100.png`
- `tests/render/baselines/primitives-100.png`
- `tests/render/baselines/primitives-150.png`
- `tests/render/baselines/text_image-100.png`
- `tests/render/macos-metal-snapshots/glass-100.png`
- `tests/render/macos-metal-snapshots/primitives-100.png`
- `tests/render/macos-metal-snapshots/primitives-150.png`
- `tests/render/macos-metal-snapshots/text_image-100.png`

</details>

<details><summary>Текст документации без исполняемого примера · 13</summary>

- `CHANGELOG.en.md`
- `CHANGELOG.md`
- `CODE_OF_CONDUCT.en.md`
- `CODE_OF_CONDUCT.md`
- `SECURITY.en.md`
- `SECURITY.md`
- `docs/architecture.en.md`
- `docs/roadmap.en.md`
- `docs/roadmap.md`
- `docs/test_stand.en.md`
- `docs/test_stand.md`
- `src/rhi/shaders_gen/msl/README.md`
- `tools/release/notes-v0.1.0.md`

</details>
