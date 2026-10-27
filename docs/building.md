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
| Windows 10 1909+ или Windows 11, видеокарта с Direct3D 12 | Окно и отрисовка (без видеокарты — программный адаптер WARP) |
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
| `make shaders` | HLSL → DXIL (и SPIR-V при `VULKAN=1`) → `src/rhi/shaders_gen/*.cpp`, нужен `dxc` |
| `make beresta` | Собрать статическую «Берёсту» в `BERESTA_DIR` |
| `make clean` / `fclean` / `re` | Удалить объектные файлы / всё собранное / пересобрать |

| Переменная | Значение |
|---|---|
| `BERESTA_DIR=путь` | Каталог «Берёсты» (по умолчанию `../Dom lib C++`) |
| `DEBUG=1` | `-O0 -g`, отладочный слой D3D12 доступен через `--debug` у примеров |
| `VULKAN=1` | Бэкенд Vulkan (в разработке, нужен Vulkan SDK и переменная `VULKAN_SDK`) |
| `WEBVIEW2_SDK=путь` | Браузер для `<WebView>` (Windows): `build/native` пакета NuGet `Microsoft.Web.WebView2`; `WebView2Loader.dll` копируется в `bin/`. После смены — `make re` |

Очистка работает и в CMD/PowerShell, и в Unix-оболочке.

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

| Параметр | Значение |
|---|---|
| `-DGRAN_BERESTA_DIR=путь` | Каталог исходников «Берёсты» |
| `-DGRAN_BUILD_TESTS=ON` | Собирать модульные тесты (по умолчанию включено) |
| `-DGRAN_WITH_VULKAN=ON` | Бэкенд Vulkan (в разработке) |
| `-DGRAN_WEBVIEW2_SDK=путь` | Браузер для `<WebView>` (Windows): `build/native` пакета `Microsoft.Web.WebView2`; загрузчик копируется к программам |
| `-DCMAKE_BUILD_TYPE=Debug` | Отладочная сборка (`GRAN_DEBUG=1`) |

Цель `gran_shaders` пересобирает шейдеры, если найден `dxc`.

**SDK WebView2** нужен только для элемента `<WebView>`. Это архив NuGet: его можно скачать с
`https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2` и распаковать как zip; путь для сборки —
каталог `build/native` внутри (в нём `include/WebView2.h` и `x64/WebView2Loader.dll`). Путь без пробелов.

Проверенные сочетания: MinGW-w64 + Ninja, Visual Studio 2026 (MSVC 14.5). Makefile и CMake дают одинаковые
кадры пиксель в пиксель.

## Шейдеры

Исходник один — `shaders/hlsl/*.hlsl`. Байт-код DXIL лежит в репозитории как сгенерированные массивы
`src/rhi/shaders_gen/*.cpp`, поэтому для обычной сборки `dxc` не нужен. После правки шейдера:

```bash
make shaders
```

Сгенерированные файлы руками не правятся.

## Подключение к своей программе

Публичные заголовки — `include/gran/`, главный для приложения — `gran/app/application.hpp`. Программа линкуется
с `libgran.a`, `libberesta.a` и системными библиотеками Windows:

```bash
g++ -std=c++17 -I"Gran-UI/include" -I"beresta/include" main.cpp Gran-UI/bin/libgran.a beresta/bin/libberesta.a -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32 -static -o app.exe
```

Ключ `-mwindows` убирает консольное окно у программы с интерфейсом.

С CMake проще всего добавить свою программу каталогом `examples/<имя>/`: каждый такой каталог собирается в
`gran_<имя>` автоматически (метка `windows_gui.txt` — программа без консоли, `system_libs.txt` — дополнительные
системные библиотеки, как `ws2_32` у чата).

## Проверка

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
