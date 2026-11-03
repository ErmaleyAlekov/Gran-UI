# Грань UI — сборка и подключение

**Русский** · [English](building.en.md)

← [К описанию](../README.md)

## Содержание

1. [Что нужно](#что-нужно)
2. [Получение исходников](#получение-исходников)
3. [Сборка через Makefile](#сборка-через-makefile)
4. [Сборка через CMake](#сборка-через-cmake)
5. [Шейдеры](#шейдеры)
6. [Подключение к своей программе](#подключение-к-своей-программе)
7. [Проверка](#проверка)

## Что нужно

| Что | Зачем |
|---|---|
| Windows 10 1909+ или Windows 11, Direct3D 12 или D3D11 FL11_0 | Окно и отрисовка (без видеокарты — программный адаптер WARP) |
| MinGW-w64 с GCC 11+ и `make` **или** Visual Studio 2022+ (MSVC) и CMake 3.20+ | Компилятор C++17 |
| [«Берёста»](https://github.com/ErmaleyAlekov/beresta) | Разбор XML и проверка схемы XSD 1.1 |
| `dxc` из Windows SDK (необязательно) | Только для пересборки шейдеров: готовый байт-код уже лежит в `src/rhi/shaders_gen/` |

Заголовки Direct3D 12, DXGI и UI Automation есть и в MinGW-w64, и в Windows SDK. Других зависимостей нет.

## Получение исходников

Движок собирается вместе со статической «Берёстой», поэтому оба репозитория кладутся рядом:

```bash
git clone https://github.com/ErmaleyAlekov/beresta.git
```

```bash
git clone https://github.com/ErmaleyAlekov/Gran-UI.git
```

Путь к «Берёсте» задаётся переменной `BERESTA_DIR` (Makefile) или `GRAN_BERESTA_DIR` (CMake). Путь может
содержать пробелы.

## Сборка через Makefile

Из каталога `Gran-UI`:

```bash
make beresta BERESTA_DIR=../beresta
```

```bash
make BERESTA_DIR=../beresta
```

Первая команда собирает `libberesta.a` собственным Makefile «Берёсты», вторая — движок и примеры.
Зависимости заголовков отслеживаются автоматически (`dep/*.d`).

| Цель | Что делает |
|---|---|
| `make` | `bin/libgran.a`, `bin/gran_gallery.exe`, `bin/gran_snapshot.exe`, `bin/gran_chat.exe`, `bin/gran_tests.exe` |
| `make test` | Собрать и запустить модульные тесты |
| `make test-crash` | Проверить отчёты о сбое (программа падает сама: SEGV, terminate, abort) |
| `bin/gran_rcc` | Встраивание ресурсов в exe по манифесту (собирается вместе с примерами) |
| `make shaders` | HLSL → DXIL; `make shaders-vulkan` — SPIR-V → `src/rhi/shaders_gen/*.cpp`, нужен `dxc` |
| `make beresta` | Собрать статическую «Берёсту» в `BERESTA_DIR` |
| `make clean` / `fclean` / `re` | Удалить объектные файлы / всё собранное / пересобрать |

| Переменная | Значение |
|---|---|
| `BERESTA_DIR=путь` | Каталог «Берёсты» (по умолчанию `../Dom lib C++`) |
| `DEBUG=1` | `-O0 -g`, отладочный слой D3D12 доступен через `--debug` у примеров |
| `VULKAN=1` | Vulkan на Linux (включён по умолчанию; заголовки Vulkan SDK или системные) |
| `WEBVIEW2_SDK=путь` | Браузер для `<WebView>` (Windows): `build/native` пакета NuGet `Microsoft.Web.WebView2`; `WebView2Loader.dll` копируется в `bin/`. После смены — `make re` |

Очистка работает и в CMD/PowerShell, и в Unix-оболочке.

По умолчанию (`DEBUG=0`) программы компонуются с `-s`: отладочная информация удаляется из готовых
исполняемых файлов, включая отладку статически подключённых библиотек MinGW. При `DEBUG=1` символы
сохраняются. После смены режима выполните `make re`: Makefile не отслеживает изменения флагов сборки.

Встроенные в библиотеку схема `schemas/gran-ui.xsd` и тема `schemas/default_theme.xml` превращаются в
исходники `gen/markup/*.cpp` при сборке — утилитой `gran_embed` (`tools/embed/`), которая собирается первой.

## Сборка через CMake

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --config Release
```

```bash
ctest --test-dir build -C Release
```

«Берёста» подключается подпроектом и собирается тем же компилятором. С MSVC движок и «Берёста» собираются со
статическим runtime (`/MT`), программы кладутся в `build/bin`.

В MinGW конфигурации `Release` и `MinSizeRel` используют `-s` при компоновке программ. `Debug` и
`RelWithDebInfo` сохраняют отладочную информацию. Для MSVC флаги не меняются: отладочные символы
хранятся отдельно в `.pdb`. У многоконфигурационных генераторов режим выбирается через `--config`.

| Параметр | Значение |
|---|---|
| `-DGRAN_BERESTA_DIR=путь` | Каталог исходников «Берёсты» |
| `-DGRAN_BUILD_TESTS=ON` | Собирать модульные тесты (по умолчанию включено) |
| `-DGRAN_WITH_VULKAN=ON` | Vulkan на Linux (по умолчанию ON) |
| `-DGRAN_WEBVIEW2_SDK=путь` | Браузер для `<WebView>` (Windows): `build/native` пакета `Microsoft.Web.WebView2`; загрузчик копируется к программам |
| `-DCMAKE_BUILD_TYPE=Debug` | Отладочная сборка (`GRAN_DEBUG=1`) |

Цель `gran_shaders` пересобирает шейдеры, если найден `dxc`.

**SDK WebView2** нужен только для элемента `<WebView>`. Это архив NuGet: его можно скачать с
`https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2` и распаковать как zip; путь для сборки —
каталог `build/native` внутри (в нём `include/WebView2.h` и `x64/WebView2Loader.dll`). Путь без пробелов.

Проверенные сочетания: MinGW-w64 + Ninja, Visual Studio 2026 (MSVC 14.5). Makefile и CMake дают одинаковые
кадры пиксель в пиксель.

## Шейдеры

Для Vulkan: `make shaders-vulkan DXC=/путь/к/dxc` или цель CMake `gran_shaders_vulkan`
(`-DGRAN_DXC=/путь/к/dxc`). Нужен DXC с поддержкой SPIR-V; Windows SDK-вариант может её не содержать.
Используется общий HLSL, Vulkan 1.0, фиксированные дескрипторы и инверсия Y только для вершинных шейдеров.
Обычная сборка читает встроенные `*_spv.cpp`, без DXC и без загрузки SDK из сети.

Исходник один — `shaders/hlsl/*.hlsl`. Байт-код DXIL лежит в репозитории как сгенерированные массивы
`src/rhi/shaders_gen/*.cpp`, поэтому для обычной сборки `dxc` не нужен. После правки шейдера:

```bash
make shaders
```

Сгенерированные файлы руками не правятся.

## Подключение к своей программе

На Linux CMake передаёт системные Vulkan/X11 автоматически через `target_link_libraries(app PRIVATE gran)`.
Для ручной компоновки нужны `-lX11 -ldl -pthread` вместо библиотек Windows (с `libvulkan` движок не компонуется).

Публичные заголовки — `include/gran/`, главный для приложения — `gran/app/application.hpp`. Программа линкуется
с `libgran.a`, `libberesta.a` и системными библиотеками Windows:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d11 -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

Ключ `-mwindows` убирает консольное окно у программы с интерфейсом.

С CMake проще всего добавить свою программу каталогом `examples/<имя>/`: каждый такой каталог собирается в
`gran_<имя>` автоматически (метка `windows_gui.txt` — программа без консоли, `system_libs.txt` — дополнительные
системные библиотеки, как `ws2_32` у чата).

## Проверка

Выбор API: `AppOptions::backend` / `BackendOptions::backend` (`AUTO`, `D3D11`, `D3D12`, `VULKAN`).
Галерея и снимки принимают `--d3d11`, `--d3d12`, `--warp`; галерея с `--watch` перезагружает XML.
`make test-render` запускает снимки обоих API на WARP, `make update-render-baselines` обновляет эталоны явно.

## Linux

Сборка использует C++17, «Берёсту», заголовки/loader Vulkan и системный Xlib. Нет SDL, GLFW, Qt, GTK,
Fontconfig, FreeType, менеджеров пакетов C++ или автоматически скачиваемых зависимостей.
Заголовки X11 относятся к SDK оконной системы, `libX11.so.6` — к системным библиотекам Linux;
это необходимая часть выбранного нативного оконного слоя.

Движок не компонуется с `libvulkan`: системный загрузчик `libvulkan.so.1` открывается через `dlopen`
при создании графики. Поэтому программа запускается и без пакета Vulkan, а вместо отказа загрузчика ОС
показывает понятную ошибку с названием пакета (`libvulkan1` / `vulkan-loader`, для работы без видеокарты —
`mesa-vulkan-drivers`). Для сборки нужны только заголовки Vulkan.

Vulkan SDK не устанавливается на пользовательские машины. Для запуска нужны системные Vulkan loader,
драйвер GPU либо Mesa lavapipe и X11/XWayland для окон. Снимки работают с тем же бэкендом без `DISPLAY`.
Mesa — программная реализация Vulkan, а не замена заголовкам SDK при компиляции.
Устаревшие выпуски Mesa могут не содержать lavapipe: для CPU нужен пакет с этим драйвером.

На Linux CMake по умолчанию включает `GRAN_WITH_VULKAN=ON`, Makefile — `VULKAN=1`.
Оба собирают gallery/snapshot и библиотеку; чат пока Windows-only. `AUTO` перебирает discrete/integrated/
virtual/other GPU и лишь затем CPU; `--software` (совместимый псевдоним `--warp`) требует CPU-устройство.
`--vulkan` явно выбирает Vulkan. `nativeDisplay()` передаёт Display отдельно от XID окна.
Vulkan 1.0 не требует descriptor indexing, timeline semaphores, dynamic rendering или synchronization2.

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DGRAN_BUILD_RENDER_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/bin/gran_gallery samples/showcase.xml --debug --software --exit-after 5
env -u DISPLAY ./build/bin/gran_snapshot tests/render/scenes/glass.xml glass.png --software
```

`gran_vulkan_cpu` проверяет настоящий CPU Vulkan вне экрана. `-DGRAN_BUILD_WINDOW_TESTS=ON` добавляет
`gran_x11_vulkan`, которому нужен запущенный X-сервер. Для строгой проверки установите системный
`VK_LAYER_KHRONOS_validation`; если его нет, runtime не требует SDK и продолжает без слоя.
Отладочные warnings/errors доступны через `takeDiagnostics()` и считаются ошибкой тестов.

Для Astra Linux, Альт, РЕД ОС и других дистрибутивов нужны те же возможности ОС, без проверки имени
дистрибутива в коде. Пакеты SDK, loader, Mesa и X11 выбираются из репозитория конкретной редакции.
Сборку следует выполнять в самом старом целевом окружении: glibc/libstdc++ от новой Ubuntu не обещают
совместимость готового бинарника со старой Astra. Фактические проверки перечислены в `project_context.md`.

Проверено в контейнерах (сборка из исходников пакетами самой редакции, `make test`, C ABI, Vulkan на
Mesa lavapipe вне экрана): Astra Linux SE 1.7 (GCC 8.3, glibc 2.28) и 1.8, Альт p10 и p11, РЕД ОС 7.3 и 8.
Сборка на Astra 1.7 требует только glibc 2.27 — такой бинарник подходит всем перечисленным системам.
На GCC 8 `std::filesystem` подключается из `libstdc++fs` автоматически (Makefile и CMake). Пакеты:

| ОС | Пакеты |
|---|---|
| Astra Linux, Debian | `g++ make libx11-dev libvulkan-dev` (+ `mesa-vulkan-drivers` для CPU) |
| Альт | `gcc-c++ make libX11-devel libvulkan-devel` |
| РЕД ОС | `gcc-c++ make libX11-devel vulkan-loader-devel` (+ `mesa-vulkan-drivers`) |

Диалоги файлов на Linux показывает системная программа `zenity` или `kdialog` (если ни одной нет — ошибка
с подсказкой); перетаскивание файлов в окно и AT-SPI пока не реализованы.

X11: клавиатура/XIM с UTF-8, мышь, колесо, курсоры, CLIPBOARD, DPI из `Xft.dpi` (переопределение
`GRAN_UI_SCALE=1.5`), системная и собственная рамка, EWMH resize/maximize/minimize, ожидание событий
и thread-safe wake через pipe. Нативный Wayland, AT-SPI, браузер Linux и macOS пока не реализованы.
Большие clipboard-передачи INCR пока не поддерживаются; отдача ограничена 200 000 байтами.
CMake: `-DGRAN_BUILD_RENDER_TESTS=ON` добавляет GPU-проверки в CTest.
Подробнее: [API](api.md), [проверки отрисовки](render_testing.md).

Шейдеры D3D11: `make shaders-dx11` (`FXC=путь`) или цель CMake `gran_shaders_dx11`
(`-DGRAN_FXC=путь`). Обычная сборка использует встроенный DXBC без SDK-компилятора.

```bash
make test
```

```bash
bin/gran_snapshot samples/showcase.xml showcase.png
```

```bash
bin/gran_gallery samples/popups.xml --debug
```

`gran_snapshot` рисует кадр вне экрана с включённым отладочным слоем D3D12 и печатает время этапов.
`gran_gallery --debug` открывает живое окно; сообщения отладочного слоя уходят в консоль.
