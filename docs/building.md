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
8. [Linux](#linux)
9. [macOS](#macos)

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
| `NATIVE=0` | Ядро без окна ОС и GPU: безоконная платформа и `NullBackend` (Linux без X11, macOS без Xcode, см. [macOS](#macos)) |
| `WEBVIEW2_SDK=путь` | Браузер для `<WebView>` (Windows): `build/native` пакета NuGet `Microsoft.Web.WebView2`; `WebView2Loader.dll` копируется в `bin/`. После смены — `make re` |

Очистка работает и в CMD/PowerShell, и в Unix-оболочке.

По умолчанию (`DEBUG=0`) программы компонуются с `-s`: отладочная информация удаляется из готовых
исполняемых файлов, включая отладку статически подключённых библиотек MinGW. При `DEBUG=1` символы
сохраняются. Смену режима (`DEBUG`, `SANITIZE`, `VULKAN` и других флагов) Makefile замечает сам: строка флагов
хранится в `obj/.flags`, и при её изменении все объектные файлы пересобираются — `make re` не нужен.

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
| `-DGRAN_NATIVE=OFF` | Ядро без окна ОС и GPU (Linux без X11, macOS без Xcode) |
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
Для ручной компоновки нужны `-lX11 -lX11-xcb -lxcb -ldl -pthread` вместо библиотек Windows (с `libvulkan` движок не компонуется).

Публичные заголовки — `include/gran/`, главный для приложения — `gran/app/application.hpp`; номер версии —
`gran/gran_version.h` (`GRAN_VERSION_AT_LEAST`), что гарантируется между версиями — [versioning.md](versioning.md). Программа линкуется
с `libgran.a`, `libberesta.a` и системными библиотеками Windows:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d11 -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

Ключ `-mwindows` убирает консольное окно у программы с интерфейсом.

С CMake проще всего добавить свою программу каталогом `examples/<имя>/`: каждый такой каталог собирается в
`gran_<имя>` автоматически (метка `windows_gui.txt` — программа без консоли, `system_libs.txt` — дополнительные
системные библиотеки, как `ws2_32` у чата).

### Готовые архивы выпуска

К каждому выпуску на GitHub и Gitverse приложены архивы — движок можно подключить без сборки из исходников:

| Архив | Чем собран | Где работает |
|---|---|---|
| `gran-ui-X.Y.Z-linux-x86_64-gcc10.tar.gz` | GCC 10.3 в образе Альт p10 | программы на GCC 10 и новее, glibc 2.30+: Альт p10 и p11, Astra 1.8, РЕД ОС 8, Ubuntu 22.04+ |
| `gran-ui-X.Y.Z-linux-x86_64-gcc8.tar.gz` | GCC 8.3 в образе Astra Linux SE 1.7 | программы на GCC 8, glibc 2.27+: Astra 1.7, РЕД ОС 7.3 |
| `gran-ui-X.Y.Z-windows-x86_64-mingw.zip` | MinGW-w64 GCC (рантайм внутри, `-static`) | Windows 10 и 11 x64 |
| `gran-ui-X.Y.Z-windows-x86_64-msvc.zip` | MSVC, Release, статический рантайм `/MT` | Windows 10 и 11 x64, программы с `/MT` |
| `gran-ui-X.Y.Z-windows-x86_64-msvc-md.zip` | MSVC, Release, динамический рантайм `/MD` | Windows 10 и 11 x64, программы с `/MD` (по умолчанию у нового проекта Visual Studio) |

Внутри: `include/` (заголовки Грани и «Берёсты»), `lib/` (`libgran` и `libberesta` — статические, `gran_c` —
общая библиотека Си-интерфейса для обёрток), `bin/` (`gran_rcc`, `gran_snapshot`), `schemas/`, `samples/`, `docs/`,
лицензии и `BUILD_INFO.txt` (коммит, версия «Берёсты», компилятор). Статические библиотеки MinGW, MSVC `/MT` и
MSVC `/MD` между собой несовместимы — берите архив под свой компилятор и рантайм: иначе компоновщик MSVC отвечает
`LNK2038` («RuntimeLibrary» или «_ITERATOR_DEBUG_LEVEL»). Библиотеки в архивах — только Release; конфигурацию Debug
своей программы с ними собирайте с тем же рантаймом и `_ITERATOR_DEBUG_LEVEL=0` (например, `/MD /Od /Zi
/D_ITERATOR_DEBUG_LEVEL=0` с архивом `msvc-md`) или соберите движок из исходников (CMake, `-DGRAN_MSVC_DYNAMIC_RUNTIME=ON`
для `/MD`). Целостность — `sha256sum -c SHA256SUMS.txt` рядом с архивами.
Компоновка — как выше, только пути из архива: `-Igran-ui/include gran-ui/lib/libgran.a gran-ui/lib/libberesta.a`
(MSVC — `gran.lib beresta_static.lib`). Встроенного браузера (`<WebView>`) в архивах нет: для него движок
собирается из исходников с WebView2 SDK.

**Почему архивов Linux два.** Статические библиотеки C++ привязаны к компилятору: `std::filesystem` (им пользуется
«Берёста») в GCC 8 экспериментальный, а в GCC 9 его двоичная раскладка изменилась — программа на GCC 13, собранная
со статическими библиотеками GCC 8, падает при первом обращении к пути (проверено на Альт p11). Поэтому программе на
GCC 8 нужен архив `gcc8` (компоновка с `-lstdc++fs`), программе на GCC 10 и новее — архив `gcc10` (проверен с GCC 12
на Astra 1.8 и GCC 13 на Альт p11). Инструменты из `bin/` и `libgran_c.so` (Си-интерфейс для обёрток) от компилятора
программы не зависят: их можно брать из любого архива, подходящего по glibc. Для запуска им нужны X11 и загрузчик
Vulkan системы: Astra и Debian — `libx11-6 libvulkan1`, Альт — `libX11 libvulkan1`, РЕД ОС — `libX11 vulkan-loader`;
без видеокарты — ещё `mesa-vulkan-drivers` (программный Vulkan).

## Проверка

Выбор API: `AppOptions::backend` / `BackendOptions::backend` (`AUTO`, `D3D11`, `D3D12`, `VULKAN`).
Галерея и снимки принимают `--d3d11`, `--d3d12`, `--warp`; галерея с `--watch` перезагружает XML.
`make test-render` запускает снимки обоих API на WARP, `make update-render-baselines` обновляет эталоны явно.

### Непрерывная интеграция (CI)

Каждый push проверяется в двух местах. GitHub Actions (`.github/workflows`): `linux.yml` — Makefile GCC на x86_64
и aarch64, CMake Clang, ядро `NATIVE=0`, GPU-проверки на lavapipe, ASan + UBSan и TSan; `windows.yml` — MinGW-w64
и MSVC с WARP; `macos.yml` — см. [macOS](#macos). Gitverse (`.gitverse/workflows/ci.yml`, в настройках CI/CD
репозитория выбрана конфигурация из `.gitverse/workflows`): та же матрица Linux x86_64 — облачный раннер Gitverse
только Linux x86_64 с ограничением 30 минут на задачу, поэтому проверки разбиты на короткие задачи, а пакеты и
«Берёсту» каждая готовит сама (`tools/ci/gitverse_prepare.sh`).

Выпуск по тегу `vX.Y.Z`: `.github/workflows/release.yml` проверяет, что тег совпадает с `gran_version.h` и в
CHANGELOG есть раздел версии, собирает [архивы выпуска](#готовые-архивы-выпуска) (Linux — в контейнерах Astra 1.7 и Альт p10,
`tools/release/build_linux.sh`; Windows — `tools/release/build_windows.sh mingw|msvc`) после модульных тестов, считает
SHA-256 и создаёт релиз GitHub с описанием из CHANGELOG. Ручной запуск этого workflow с ветки собирает архивы без
релиза — проверка до тега. `.gitverse/workflows/release.yml` ждёт релиз GitHub, сверяет суммы и повторяет его на
Gitverse (секрет `GV_API_TOKEN`).

### Статический анализ (clang-tidy)

Правила — в [`.clang-tidy`](../.clang-tidy): группы `bugprone`, `cert`, `clang-analyzer` (в том числе
`optin.core.EnumCastOutOfRange` — приведение числа вне перечисления), `concurrency`, `performance`; что отключено
и почему — там же в комментарии. Нужна база команд сборки `compile_commands.json`: CMake пишет её сам с
`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

```bash
cmake -S . -B build-tidy -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-tidy
run-clang-tidy -p build-tidy -quiet '/(src|examples)/'
```

clang-tidy не входит в компилятор; без прав администратора его можно поставить из PyPI: `pip install clang-tidy`.
Проверки аргументов в коде видны анализатору: в Си-интерфейсе `require` — шаблон в заголовке над `[[noreturn]]`
функцией, иначе после `require(p != nullptr)` он считал бы `p` нулевым.

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
| Astra Linux, Debian | `g++ make libx11-dev libx11-xcb-dev libxcb1-dev libvulkan-dev` (+ `mesa-vulkan-drivers` для CPU) |
| Альт | `gcc-c++ make libX11-devel libvulkan-devel` |
| РЕД ОС | `gcc-c++ make libX11-devel vulkan-loader-devel` (+ `mesa-vulkan-drivers`) |

Диалоги файлов на Linux показывает системная программа `zenity` или `kdialog` (если ни одной нет — ошибка
с подсказкой); перетаскивание файлов в окно и AT-SPI пока не реализованы.

X11: клавиатура/XIM с UTF-8, мышь, колесо, курсоры, CLIPBOARD, DPI из `Xft.dpi` (переопределение
`GRAN_UI_SCALE=1.5`), системная и собственная рамка, EWMH resize/maximize/minimize, ожидание событий
и thread-safe wake через pipe. Нативный Wayland, AT-SPI и браузер (`<WebView>`) на Linux и macOS пока не реализованы.
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

## macOS

На macOS собираются **окно Cocoa и графика Metal** (`NATIVE=1` — умолчание с шага M6, как на Windows и Linux):
библиотека, все примеры, кроме `chat` (у него сеть Winsock), и Си-интерфейс для обёрток. Нужны **Xcode** (компилятор
Metal `xcrun metal`; в одних Command Line Tools его нет — тогда сборка сразу говорит об этом) и GNU make 4+: штатный
`make` macOS — версии 3.81, Makefile движка ему не подходит и говорит об этом сам.

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

Без Xcode собирается **ядро без окна ОС** (`NATIVE=0` / `-DGRAN_NATIVE=OFF`, хватает Command Line Tools:
`xcode-select --install`): разметка, стили, компоновка, текст, ввод, привязки, модели, программа и все модульные
тесты — на безоконной платформе (`AppOptions::headless`, `AppDriver`) и `NullBackend`. Вызов окна ОС или бэкенда GPU
в такой сборке — исключение с понятным текстом, примеры и Си-интерфейс не собираются.

```bash
gmake NATIVE=0 BERESTA_DIR=../beresta all test test-crash
```

Почему `NATIVE=1` стал умолчанием: так `make` на всех трёх ОС даёт одно и то же — программы с окном; Xcode для
программ macOS всё равно нужен (подпись, пакет `.app`), а без него ошибка появляется сразу и подсказывает `NATIVE=0`.

Код собирается для macOS 11.0 и новее (`MACOS_MIN` в Makefile, `CMAKE_OSX_DEPLOYMENT_TARGET` в CMake) — как
«Берёста». Универсальная сборка Intel + Apple Silicon — через CMake:

```bash
cmake -S . -B build -DGRAN_BERESTA_DIR=../beresta -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

```bash
cmake --build build -j
```

```bash
ctest --test-dir build --output-on-failure
```

### Окно Cocoa

Платформа `src/platform/cocoa/` (Objective-C++ с ARC) выполняет контракт `platform::` ([архитектура](architecture.md)):

- **Окно** — свой подкласс `NSWindow`, содержимое — вид со слоем `CAMetalLayer` (`nativeHandle`). Цикл событий —
  свой (`nextEventMatchingMask:untilDate:`, `[NSApp run]` не вызывается), `wake()` из любого потока кладёт событие
  в очередь AppKit. Программа, запущенная из терминала без пакета `.app`, всё равно получает Dock, строку меню
  и фокус; «Завершить» (Cmd+Q, Dock) и кнопка закрытия дают обычный `CLOSE` — решает программа.
- **Retina:** логический пиксель — пункт, `scale()` — `backingScaleFactor`; перенос окна на монитор с другим
  масштабом — `DPI_CHANGED` и `RESIZE`.
- **Мышь** — в пунктах от левого верхнего угла; `clickCount` ОС; Control+щелчок — правая кнопка (соглашение macOS).
  **Колесо и трекпад:** точная прокрутка (`hasPreciseScrollingDeltas`) делится на 48 пунктов на «щелчок», инерцию даёт ОС.
- **Клавиатура:** клавиши — по физическому коду (Cmd+C работает в русской раскладке), Cmd — `Mod`
  (`MODIFIER_CONTROL`), физический Control — `META`, Control+Tab — Mod+Tab. Текст — через метод ввода
  (`interpretKeyEvents` → `NSTextInputClient`): мёртвые клавиши и Option набирают символы, Option с символом приходит
  без `ALT`. IME — минимум контракта: зафиксированная строка приходит символами, набираемая движку пока не видна
  (`COMPOSITION` — пункт 2.5), окно кандидатов — у левого верхнего угла окна.
- **Буфер обмена** — `NSPasteboard` (UTF-8), **курсоры** — `NSCursor`, **диалоги файлов** — `NSOpenPanel` / `NSSavePanel`.
- **Своя рамка** (`titleBar="custom"`) — содержимое под прозрачным заголовком, «светофор» macOS остаётся; участок
  с ролью перетаскивания двигает окно, двойной щелчок — по настройке системы.
- **Пока нет:** перетаскивание файлов в окно (`NSDraggingDestination`) и мост доступности `NSAccessibility`
  (VoiceOver видит только окно) — следующие шаги.

Бэкенд Metal рисует кадр в свою текстуру, в конце кадра копирует её в `drawable` слоя и показывает; поверхность окна —
BGRA8 (CAMetalLayer не умеет RGBA8), цвета — в sRGB, `vsync` — `displaySyncEnabled`. Кадр окна читается так же, как вне
экрана (`SnapshotSource`), без прав «Запись экрана».

### Metal вне экрана

Бэкенд **Metal** (`src/rhi/metal/`, Objective-C++ с ARC) без окна рисует кадр в свою текстуру и читает её обратно —
как `gran_snapshot` на других ОС (ключ `--metal`). Шейдеры `src/rhi/shaders_gen/msl/*.metal` собираются в
`gran.metallib` при сборке и встраиваются в библиотеку, в движке шейдеры не компилируются. Тексты MSL получаются из того
же HLSL (`make shaders-metal` на Linux, WSL или macOS: dxc со SPIR-V и spirv-cross), `make metal-check` проверяет их
компилятором Apple и сверяет раскладку структур GPU с C++.

`test-render` (`bin/gran_macos_tests`) проверяет бэкенд (кадры в полёте, размытие, смена размера, слоты текстур, отказ
на неверные данные), затем **окно Cocoa в процессе** — настоящее окно с `CAMetalLayer`: кадры через `nextDrawable`,
пиксели кадра, ввод через очередь AppKit (`[NSApp postEvent:]`, без прав «Универсального доступа»: мышь, Shift+A с
символом, Cmd+C, Control+Tab, точная прокрутка), смена размера, буфер обмена, `wake` из потока, закрытие — и сверяет
снимки сцен `tests/render/scenes` с эталонами D3D12 WARP: весь кадр — в допуске 2 на канал, выбросы на самых краях фигур —
не больше 0,1 % пикселей и не дальше 48 (Metal растеризует края чуть иначе, чем WARP). Отладочный слой Metal включается
только переменными окружения: `MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`.

Отчёт о сбое на macOS — как на Linux (сигналы на своём стеке, backtrace, байты стека), вместо `/proc/self/maps` в
отчёт идут образы dyld с адресами загрузки (для `atos -l`). Шрифты ищутся в `/System/Library/Fonts`,
`/Library/Fonts` и `~/Library/Fonts`.

Своего Mac у разработчиков пока нет: каждый push проверяется на раннере GitHub Actions macOS 15
(`.github/workflows/macos.yml`): ядро `NATIVE=0` на arm64 (тесты, отчёт о сбое, шейдеры Metal), окно Cocoa и Metal
(Makefile и CMake: тесты, `test-render` с окном в процессе, снимки) и универсальная сборка CMake с тестами обеих
архитектур (x86_64 — через Rosetta). Живое окно на настоящем Mac (трекпад, IME, Retina, VoiceOver) проверяет человек.

`NATIVE=0` работает и на Linux: ядро без X11 и Vulkan — например, для серверной проверки разметки или контейнера CI
без графических пакетов.
