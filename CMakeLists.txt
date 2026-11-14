# ============================================================================
#  Gran UI — кроссплатформенная сборка CMake (повторяет Makefile)
#  Компиляторы: MSVC, MinGW, GCC, Clang. ОС: Windows (D3D12), Linux (Vulkan/X11); macOS — пока только ядро без окна
#  (GRAN_NATIVE=OFF: безоконная платформа и NullBackend; Cocoa и Metal — шаги M5–M6).
#
#  Цели:
#    gran                — статическая библиотека движка (libgran.a / gran.lib)
#    gran_embed          — инструмент встраивания файлов в C++ (собирается первым)
#    gran_<пример>       — каждый каталог examples/<пример>/ (chat — только Windows)
#    gran_tests          — модульные тесты (-DGRAN_BUILD_TESTS=ON, по умолчанию ON), запуск: ctest
#    gran_shaders        — HLSL → DXIL → src/rhi/shaders_gen/*.cpp (по запросу, нужен dxc)
#
#  Параметры:
#    -DGRAN_BERESTA_DIR=путь   каталог исходников «Берёсты» (по умолчанию ../Dom lib C++);
#                              она собирается тем же компилятором как подпроект (beresta_static)
#    -DGRAN_WITH_VULKAN=ON     бэкенд Vulkan (нужен Vulkan SDK)
#    -DGRAN_NATIVE=OFF         ядро без окна ОС и GPU (Linux без X11; на macOS — по умолчанию)
#    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"  macOS: универсальная сборка (не ниже macOS 11.0)
#    -DGRAN_DXC=путь           компилятор шейдеров dxc
#    -DGRAN_WEBVIEW2_SDK=путь  build/native из NuGet-пакета Microsoft.Web.WebView2 (Windows): браузер для <WebView>,
#                              WebView2Loader.dll кладётся в <сборка>/bin
#    -DCMAKE_BUILD_TYPE=Debug  -O0/-Od, отладочные слои D3D12 / Vulkan (GRAN_DEBUG=1)
#
#  Примеры:
#    cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
#    cmake -S . -B build-msvc -G "Visual Studio 18 2026" && cmake --build build-msvc --config Release
#    cmake --build build --target gran_shaders
# ============================================================================
#  Gran UI — a cross-platform CMake build (mirrors the Makefile)
#  Compilers: MSVC, MinGW, GCC, Clang. OS: Windows (D3D12), Linux (Vulkan/X11); macOS — so far only the core without
#  windows (GRAN_NATIVE=OFF: the windowless platform and NullBackend; Cocoa and Metal are steps M5–M6).
#
#  Targets:
#    gran                — the engine static library (libgran.a / gran.lib)
#    gran_embed          — the tool embedding files into C++ (built first)
#    gran_<example>      — each examples/<example>/ directory (chat — Windows only)
#    gran_tests          — the unit tests (-DGRAN_BUILD_TESTS=ON, ON by default), running: ctest
#    gran_shaders        — HLSL → DXIL → src/rhi/shaders_gen/*.cpp (on request, needs dxc)
#
#  Options:
#    -DGRAN_BERESTA_DIR=path   the Beresta source directory (default ../Dom lib C++);
#                              it is built by the same compiler as a subproject (beresta_static)
#    -DGRAN_WITH_VULKAN=ON     the Vulkan backend (needs the Vulkan SDK)
#    -DGRAN_NATIVE=OFF         the core without an OS window and GPU (Linux without X11; on macOS — the default)
#    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"  macOS: a universal build (not lower than macOS 11.0)
#    -DGRAN_DXC=path           the dxc shader compiler
#    -DGRAN_WEBVIEW2_SDK=path  build/native of the Microsoft.Web.WebView2 NuGet package (Windows): a browser for <WebView>,
#                              WebView2Loader.dll goes to <build>/bin
#    -DCMAKE_BUILD_TYPE=Debug  -O0/-Od, the D3D12 / Vulkan debug layers (GRAN_DEBUG=1)
# ============================================================================
cmake_minimum_required(VERSION 3.16)

# macOS: не ниже 11.0, как у «Берёсты» (первая версия с Apple Silicon). Задаётся до project(), иначе не действует;
# «Берёста» как подпроект берёт то же значение из кэша.
# macOS: not lower than 11.0, as in Beresta (the first version with Apple Silicon). It is set before project(), otherwise
# it has no effect; Beresta as a subproject takes the same value from the cache.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Минимальная версия macOS / The minimum macOS version")

# ---- Версия — из include/gran/gran_version.h (единый источник, docs/versioning.md) -----
# ---- The version comes from include/gran/gran_version.h (a single source, docs/versioning.md) -----
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranVersion.cmake")
gran_read_version("${CMAKE_CURRENT_SOURCE_DIR}/include/gran/gran_version.h")

project(gran_ui VERSION ${GRAN_VERSION} LANGUAGES CXX)
message(STATUS "Gran UI ${GRAN_VERSION}${GRAN_VERSION_SUFFIX}")

# ---- Тип сборки по умолчанию — Release (однотипные генераторы: Make, Ninja) -----
# ---- The default build type is Release (single-config generators: Make, Ninja) -----
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Тип сборки / The build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Статический runtime MSVC — как у «Берёсты» (смешивать /MT и /MD в одной программе нельзя)
# и как -static у MinGW: программа — один файл без vcruntime140.dll
# The static MSVC runtime — as Beresta has (mixing /MT and /MD in one program is impossible)
# and as -static with MinGW: a program is one file without vcruntime140.dll
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Программы кладутся в <сборка>/bin, а не в bin/ исходников (там живут программы Makefile)
# Programs go to <build>/bin, not to the source bin/ (the Makefile programs live there)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(GRAN_BUILD_TESTS "Собирать модульные тесты / Build the unit tests" ON)
option(GRAN_BUILD_RENDER_TESTS "GPU-тесты WARP/Vulkan / WARP/Vulkan GPU tests" OFF)
# Платформа ОС (окна, GPU): на macOS её пока нет (Cocoa — шаг M6, Metal — M5) — там ядро без окна
# The OS platform (windows, GPU): macOS has none yet (Cocoa — step M6, Metal — M5) — the core without windows there
set(GRAN_NATIVE_DEFAULT ON)
if(APPLE)
    set(GRAN_NATIVE_DEFAULT OFF)
endif()
option(GRAN_NATIVE "Платформа ОС и GPU (OFF — только безоконная) / The OS platform and GPU (OFF — windowless only)"
    ${GRAN_NATIVE_DEFAULT})
if(APPLE AND GRAN_NATIVE)
    message(FATAL_ERROR "Gran UI: the macOS platform (Cocoa, Metal) is not ready yet, use -DGRAN_NATIVE=OFF")
elseif(WIN32 AND NOT GRAN_NATIVE)
    message(FATAL_ERROR "Gran UI: GRAN_NATIVE=OFF (the build without the OS platform) is for Linux and macOS")
elseif(NOT WIN32 AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Gran UI: currently supported platforms are Windows, Linux and macOS")
endif()
set(GRAN_VULKAN_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GRAN_NATIVE)
    set(GRAN_VULKAN_DEFAULT ON)
endif()
option(GRAN_WITH_VULKAN "Vulkan Linux backend (SDK headers at build time)" ${GRAN_VULKAN_DEFAULT})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GRAN_NATIVE AND NOT GRAN_WITH_VULKAN)
    message(FATAL_ERROR "Gran UI: Linux requires -DGRAN_WITH_VULKAN=ON (or -DGRAN_NATIVE=OFF for the core without windows)")
endif()
if(NOT GRAN_NATIVE AND GRAN_WITH_VULKAN)
    message(FATAL_ERROR "Gran UI: GRAN_NATIVE=OFF builds no GPU backend, GRAN_WITH_VULKAN is not allowed")
endif()
if(WIN32 AND GRAN_WITH_VULKAN)
    message(FATAL_ERROR "Gran UI: Vulkan backend currently targets Linux; use Direct3D on Windows")
endif()
set(GRAN_WEBVIEW2_SDK "" CACHE PATH
    "build/native пакета Microsoft.Web.WebView2 (пусто — <WebView> без браузера) / build/native of Microsoft.Web.WebView2 (empty — <WebView> without a browser)")
set(GRAN_BERESTA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Dom lib C++" CACHE PATH
    "Каталог исходников «Берёсты» / The Beresta source directory")

# ---- «Берёста»: подпроект, только статическая библиотека ------------------------
# EXCLUDE_FROM_ALL: собирается лишь то, от чего зависят цели Gran UI (beresta_static),
# а не её динамическая библиотека и обёртки.
# ---- Beresta: a subproject, only the static library ------------------------
# EXCLUDE_FROM_ALL: only what the Gran UI targets depend on (beresta_static) is built,
# not its shared library and bindings.
if(NOT EXISTS "${GRAN_BERESTA_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Gran UI: не найдена «Берёста» / Beresta not found: ${GRAN_BERESTA_DIR}")
endif()

set(BERESTA_BUILD_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory("${GRAN_BERESTA_DIR}" "${CMAKE_BINARY_DIR}/beresta" EXCLUDE_FROM_ALL)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranOptions.cmake")
# Санитайзер — и для «Берёсты»: MSVC ASan размечает std::string/vector (annotate_string), и объекты
# с разметкой и без неё не компонуются вместе (LNK2038); найдено на MSVC 14.44, стенд 2026-11-08.
# beresta_static собирается из объектной библиотеки beresta_objects — флаги нужны именно ей.
# The sanitizer is applied to Beresta too: MSVC ASan annotates std::string/vector (annotate_string), and
# objects with and without annotations do not link together (LNK2038); found on MSVC 14.44, stand 2026-11-08.
# beresta_static is built from the beresta_objects object library — that is where the flags are needed.
gran_apply_sanitizers(beresta_objects)
gran_apply_sanitizers(beresta_static)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranSources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranShaders.cmake")

# ---- Инструмент встраивания: без библиотеки, собирается до генерации ------------
# ---- The embedding tool: without the library, built before the generation ------------
add_executable(gran_embed "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed/embed_main.cpp")
gran_apply_compile_options(gran_embed)
gran_apply_link_options(gran_embed)

# ---- gran_rcc: ресурсы программы внутри exe (нужна библиотека — цель объявлена после неё) ----------
# ---- gran_rcc: the program resources inside the exe (needs the library — the target is declared after it) ----------
# gran_add_resources(цель манифест): манифест → <сборка>/gen/<цель>/resources.cpp (с файлом зависимостей) → в цель
# gran_add_resources(target manifest): the manifest → <build>/gen/<target>/resources.cpp (with a dependency file) → into the target
function(gran_add_resources target manifest)
    set(generated "${CMAKE_BINARY_DIR}/gen/${target}/resources.cpp")
    add_custom_command(
        OUTPUT "${generated}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/gen/${target}"
        COMMAND gran_rcc "${manifest}" "${generated}" --depfile "${generated}.d"
        DEPENDS "${manifest}" gran_rcc
        DEPFILE "${generated}.d"
        COMMENT "gran_rcc ${target}"
        VERBATIM)
    target_sources(${target} PRIVATE "${generated}")
endfunction()

# ---- Встроенные схема разметки и тема по умолчанию ------------------------------
# ---- The embedded markup schema and the default theme ------------------------------
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/gran-ui.xsd" markup/gran_ui_schema.cpp GRAN_UI_SCHEMA)
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/default_theme.xml" markup/default_theme.cpp DEFAULT_THEME)
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/gran-resources.xsd" res/gran_resources_schema.cpp GRAN_RESOURCES_SCHEMA)
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/gran-settings.xsd" settings/gran_settings_schema.cpp GRAN_SETTINGS_SCHEMA)

# ---- Библиотека движка ----------------------------------------------------------
# ---- The engine library ----------------------------------------------------------
gran_collect_sources(GRAN_SOURCES)
add_library(gran STATIC ${GRAN_SOURCES} ${GRAN_GENERATED_SOURCES})
target_include_directories(gran PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gran PUBLIC beresta_static ${GRAN_SYSTEM_LIBS})
# Инструмент ресурсов использует библиотеку (схема и разбор манифеста) / The resource tool uses the library (the schema and the manifest parsing)
add_executable(gran_rcc "${CMAKE_CURRENT_SOURCE_DIR}/tools/rcc/rcc_main.cpp")
target_link_libraries(gran_rcc PRIVATE gran)
gran_apply_compile_options(gran_rcc)
gran_apply_link_options(gran_rcc)
# std::filesystem в libstdc++ до GCC 9 — отдельная libstdc++fs (Astra Linux SE 1.7)
# std::filesystem in libstdc++ before GCC 9 is a separate libstdc++fs (Astra Linux SE 1.7)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(gran PUBLIC stdc++fs)
endif()
gran_apply_compile_options(gran)
set_target_properties(gran beresta_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(gran PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)

# Общий C ABI для тонких языковых обёрток; экспортируются только функции gran.h.
# A shared C ABI for thin language bindings; only gran.h functions are exported.
# Без платформы ОС Си-интерфейс не создал бы ни одного окна: по умолчанию не собирается, явный ON — ошибка (ревью Т-48)
# Without the OS platform the C interface could not create a single window: not built by default, an explicit ON is an error (review T-48)
option(GRAN_BUILD_BINDINGS "Build the shared C ABI" ${GRAN_NATIVE})
if(GRAN_BUILD_BINDINGS AND NOT GRAN_NATIVE)
    message(FATAL_ERROR "Gran UI: the C ABI (gran_c) needs the OS platform, use -DGRAN_BUILD_BINDINGS=OFF with GRAN_NATIVE=OFF")
endif()
if(GRAN_BUILD_BINDINGS)
    file(GLOB GRAN_CAPI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/capi/*.cpp")
    add_library(gran_c SHARED ${GRAN_CAPI_SOURCES})
    target_link_libraries(gran_c PRIVATE gran)
    target_compile_definitions(gran_c PRIVATE GRAN_CAPI_BUILD=1)
    target_include_directories(gran_c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(gran_c PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
    gran_apply_compile_options(gran_c)
    gran_apply_link_options(gran_c)
endif()
# Встроенный браузер: определение и заголовки только для библиотеки; загрузчик — рядом с программами
# The embedded browser: the definition and the headers only for the library; the loader — next to the programs
if(WIN32 AND GRAN_WEBVIEW2_SDK)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(GRAN_WEBVIEW2_ARCH x64)
    else()
        set(GRAN_WEBVIEW2_ARCH x86)
    endif()
    target_compile_definitions(gran PRIVATE GRAN_HAS_WEBVIEW2=1)
    target_include_directories(gran SYSTEM PRIVATE "${GRAN_WEBVIEW2_SDK}/include")
    set(GRAN_WEBVIEW2_DLL "${GRAN_WEBVIEW2_SDK}/${GRAN_WEBVIEW2_ARCH}/WebView2Loader.dll")
endif()
if(GRAN_WITH_VULKAN)
    # Нужны только заголовки: загрузчик libvulkan.so.1 открывается через dlopen (vulkan_loader.cpp)
    # Only the headers are needed: the libvulkan.so.1 loader is opened through dlopen (vulkan_loader.cpp)
    find_path(GRAN_VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS ${Vulkan_INCLUDE_DIR} "$ENV{VULKAN_SDK}/include")
    if(NOT GRAN_VULKAN_INCLUDE_DIR)
        message(FATAL_ERROR "Gran UI: Vulkan headers (vulkan/vulkan.h) are required for Linux")
    endif()
    target_compile_definitions(gran PUBLIC GRAN_WITH_VULKAN=1)
    target_link_libraries(gran PUBLIC ${CMAKE_DL_LIBS})
    target_include_directories(gran SYSTEM PRIVATE ${GRAN_VULKAN_INCLUDE_DIR})
    find_path(GRAN_X11_INCLUDE_DIR X11/Xlib.h)
    find_library(GRAN_X11_LIBRARY NAMES X11 libX11.so.6)
    if(NOT GRAN_X11_INCLUDE_DIR OR NOT GRAN_X11_LIBRARY)
        message(FATAL_ERROR "Gran UI: system X11 headers/library are required for Linux windows")
    endif()
    target_include_directories(gran SYSTEM PRIVATE ${GRAN_X11_INCLUDE_DIR})
    target_link_libraries(gran PUBLIC ${GRAN_X11_LIBRARY})
endif()

# ---- Примеры: каждый подкаталог examples/<имя>/ — программа gran_<имя> ------------
# ---- Examples: each examples/<name>/ subdirectory is a program gran_<name> ------------
file(GLOB GRAN_EXAMPLE_DIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/examples/*")
foreach(example_dir IN LISTS GRAN_EXAMPLE_DIRS)
    get_filename_component(example "${example_dir}" NAME)
    # Без платформы ОС примеры не собираются: всем нужно окно или GPU (ревью Т-48)
    # Without the OS platform the examples are not built: all of them need a window or a GPU (review T-48)
    if(GRAN_NATIVE AND IS_DIRECTORY "${example_dir}" AND (WIN32 OR NOT example STREQUAL "chat"))
        file(GLOB_RECURSE example_sources CONFIGURE_DEPENDS "${example_dir}/*.cpp")
        add_executable(gran_${example} ${example_sources})
        # Заголовки примера подключаются от его корня; свои системные библиотеки — из system_libs.txt
        # The example headers are included from its root; its own system libraries come from system_libs.txt
        target_include_directories(gran_${example} PRIVATE "${example_dir}")
        target_link_libraries(gran_${example} PRIVATE gran)
        # Встроенные ресурсы примера (resources.xml → gran_rcc) / The example embedded resources (resources.xml → gran_rcc)
        if(EXISTS "${example_dir}/resources.xml")
            gran_add_resources(gran_${example} "${example_dir}/resources.xml")
        endif()
        if(EXISTS "${example_dir}/system_libs.txt")
            file(STRINGS "${example_dir}/system_libs.txt" example_libs)
            target_link_libraries(gran_${example} PRIVATE ${example_libs})
        endif()
        # Метка windows_gui.txt — оконная программа без консоли; MSVC при этом нужен вход main, а не WinMain
        # The windows_gui.txt marker — a windowed program without a console; MSVC then needs the main entry, not WinMain
        if(WIN32 AND EXISTS "${example_dir}/windows_gui.txt")
            set_target_properties(gran_${example} PROPERTIES WIN32_EXECUTABLE TRUE)
            if(MSVC)
                target_link_options(gran_${example} PRIVATE /ENTRY:mainCRTStartup)
            endif()
        endif()
        gran_apply_compile_options(gran_${example})
        # Загрузчик WebView2 — в каталог программы (у многоконфигурационных генераторов это bin/<конфигурация>)
        # The WebView2 loader — into the program directory (bin/<configuration> for multi-config generators)
        if(GRAN_WEBVIEW2_DLL)
            add_custom_command(TARGET gran_${example} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${GRAN_WEBVIEW2_DLL}" "$<TARGET_FILE_DIR:gran_${example}>"
                VERBATIM)
        endif()
        gran_apply_link_options(gran_${example})
    endif()
endforeach()

# Проверка отчёта о сбое (как make test-crash): программа падает сама и проверяет отчёты.
# С санитайзером не собирается: ASan перехватывает намеренный сбой раньше обработчика движка,
# и проверка ждёт отчёт до таймаута (MSVC ASan, стенд 2026-11-08).
# The crash report check (as make test-crash): the program crashes itself and checks the reports.
# Not built with a sanitizer: ASan intercepts the deliberate crash before the engine handler,
# and the check waits for the report until the timeout (MSVC ASan, stand 2026-11-08).
if(GRAN_BUILD_TESTS AND GRAN_SANITIZE STREQUAL "")
    add_executable(gran_crash_probe "${CMAKE_CURRENT_SOURCE_DIR}/tests/crash/crash_probe.cpp")
    target_link_libraries(gran_crash_probe PRIVATE gran)
    gran_apply_compile_options(gran_crash_probe)
    gran_apply_link_options(gran_crash_probe)
    add_test(NAME gran_crash_reports COMMAND gran_crash_probe --check "${CMAKE_BINARY_DIR}/crash-check")
endif()

# Отдельный набор GPU-тестов, включаемый явно. / A separate GPU suite enabled explicitly.
if(WIN32 AND GRAN_BUILD_RENDER_TESTS)
    enable_testing()
    file(GLOB GRAN_RENDER_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/render/*.cpp")
    add_executable(gran_render_tests ${GRAN_RENDER_TEST_SOURCES})
    target_link_libraries(gran_render_tests PRIVATE gran)
    gran_apply_compile_options(gran_render_tests)
    gran_apply_link_options(gran_render_tests)
    foreach(backend d3d12 d3d11)
        add_test(NAME gran_render_${backend} COMMAND gran_render_tests --${backend}
            --output "${CMAKE_BINARY_DIR}/render-${backend}")
        set_tests_properties(gran_render_${backend} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endforeach()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND GRAN_NATIVE AND GRAN_BUILD_RENDER_TESTS)
    enable_testing()
    file(GLOB GRAN_LINUX_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/linux/*.cpp")
    add_executable(gran_linux_tests ${GRAN_LINUX_TEST_SOURCES})
    target_include_directories(gran_linux_tests SYSTEM PRIVATE ${GRAN_X11_INCLUDE_DIR})
    target_link_libraries(gran_linux_tests PRIVATE gran)
    gran_apply_compile_options(gran_linux_tests)
    add_test(NAME gran_vulkan_cpu COMMAND gran_linux_tests)
    set_tests_properties(gran_vulkan_cpu PROPERTIES TIMEOUT 30)
    option(GRAN_BUILD_WINDOW_TESTS "Enable tests requiring a running X11/XWayland server" OFF)
    if(GRAN_BUILD_WINDOW_TESTS)
        add_test(NAME gran_x11_vulkan COMMAND gran_linux_tests --window)
        set_tests_properties(gran_x11_vulkan PROPERTIES TIMEOUT 30)
    endif()
endif()

# ---- Модульные тесты: запускаются из корня исходников (пути schemas/, samples/) ----
# ---- The unit tests: run from the source root (the schemas/, samples/ paths) ----
if(GRAN_BUILD_TESTS)
    enable_testing()
    file(GLOB_RECURSE GRAN_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/src/*.cpp")
    # Переносимая часть примера chat (JSON, chunked, NDJSON, HTTP) проверяется теми же тестами
    # The portable part of the chat example (JSON, chunked, NDJSON, HTTP) is checked by the same tests
    file(GLOB_RECURSE GRAN_CHAT_COMMON CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/chat/common/*.cpp")
    add_executable(gran_tests ${GRAN_TEST_SOURCES} ${GRAN_CHAT_COMMON})
    target_include_directories(gran_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/chat")
    target_link_libraries(gran_tests PRIVATE gran)
    gran_apply_compile_options(gran_tests)
    gran_apply_link_options(gran_tests)
    add_test(NAME gran_tests COMMAND gran_tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
