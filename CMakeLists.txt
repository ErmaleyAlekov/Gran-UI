# ============================================================================
#  Gran UI — кроссплатформенная сборка CMake (повторяет Makefile)
#  Компиляторы: MSVC, MinGW, GCC, Clang. ОС: Windows (D3D12), Linux и macOS — каркас.
#
#  Цели:
#    gran                — статическая библиотека движка (libgran.a / gran.lib)
#    gran_embed          — инструмент встраивания файлов в C++ (собирается первым)
#    gran_<пример>       — каждый каталог examples/<пример>/ (gallery, snapshot и chat — только Windows)
#    gran_tests          — модульные тесты (-DGRAN_BUILD_TESTS=ON, по умолчанию ON), запуск: ctest
#    gran_shaders        — HLSL → DXIL → src/rhi/shaders_gen/*.cpp (по запросу, нужен dxc)
#
#  Параметры:
#    -DGRAN_BERESTA_DIR=путь   каталог исходников «Берёсты» (по умолчанию ../Dom lib C++);
#                              она собирается тем же компилятором как подпроект (beresta_static)
#    -DGRAN_WITH_VULKAN=ON     бэкенд Vulkan (нужен Vulkan SDK)
#    -DGRAN_DXC=путь           компилятор шейдеров dxc
#    -DCMAKE_BUILD_TYPE=Debug  -O0/-Od, отладочные слои D3D12 / Vulkan (GRAN_DEBUG=1)
#
#  Примеры:
#    cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
#    cmake -S . -B build-msvc -G "Visual Studio 18 2026" && cmake --build build-msvc --config Release
#    cmake --build build --target gran_shaders
# ============================================================================
#  Gran UI — a cross-platform CMake build (mirrors the Makefile)
#  Compilers: MSVC, MinGW, GCC, Clang. OS: Windows (D3D12), Linux and macOS — a skeleton.
#
#  Targets:
#    gran                — the engine static library (libgran.a / gran.lib)
#    gran_embed          — the tool embedding files into C++ (built first)
#    gran_<example>      — each examples/<example>/ directory (gallery, snapshot and chat — Windows only)
#    gran_tests          — the unit tests (-DGRAN_BUILD_TESTS=ON, ON by default), running: ctest
#    gran_shaders        — HLSL → DXIL → src/rhi/shaders_gen/*.cpp (on request, needs dxc)
#
#  Options:
#    -DGRAN_BERESTA_DIR=path   the Beresta source directory (default ../Dom lib C++);
#                              it is built by the same compiler as a subproject (beresta_static)
#    -DGRAN_WITH_VULKAN=ON     the Vulkan backend (needs the Vulkan SDK)
#    -DGRAN_DXC=path           the dxc shader compiler
#    -DCMAKE_BUILD_TYPE=Debug  -O0/-Od, the D3D12 / Vulkan debug layers (GRAN_DEBUG=1)
# ============================================================================
cmake_minimum_required(VERSION 3.16)

project(gran_ui LANGUAGES CXX)

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
option(GRAN_WITH_VULKAN "Бэкенд Vulkan (нужен Vulkan SDK) / The Vulkan backend (needs the Vulkan SDK)" OFF)
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
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranSources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GranShaders.cmake")

# ---- Инструмент встраивания: без библиотеки, собирается до генерации ------------
# ---- The embedding tool: without the library, built before the generation ------------
add_executable(gran_embed "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed/embed_main.cpp")
gran_apply_compile_options(gran_embed)
gran_apply_link_options(gran_embed)

# ---- Встроенные схема разметки и тема по умолчанию ------------------------------
# ---- The embedded markup schema and the default theme ------------------------------
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/gran-ui.xsd" markup/gran_ui_schema.cpp GRAN_UI_SCHEMA)
gran_embed_file("${CMAKE_CURRENT_SOURCE_DIR}/schemas/default_theme.xml" markup/default_theme.cpp DEFAULT_THEME)

# ---- Библиотека движка ----------------------------------------------------------
# ---- The engine library ----------------------------------------------------------
gran_collect_sources(GRAN_SOURCES)
add_library(gran STATIC ${GRAN_SOURCES} ${GRAN_GENERATED_SOURCES})
target_include_directories(gran PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gran PUBLIC beresta_static ${GRAN_SYSTEM_LIBS})
gran_apply_compile_options(gran)
if(GRAN_WITH_VULKAN)
    find_package(Vulkan REQUIRED)
    target_compile_definitions(gran PUBLIC GRAN_WITH_VULKAN=1)
    target_include_directories(gran PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()

# ---- Примеры: каждый подкаталог examples/<имя>/ — программа gran_<имя> ------------
# ---- Examples: each examples/<name>/ subdirectory is a program gran_<name> ------------
file(GLOB GRAN_EXAMPLE_DIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/examples/*")
foreach(example_dir IN LISTS GRAN_EXAMPLE_DIRS)
    get_filename_component(example "${example_dir}" NAME)
    if(IS_DIRECTORY "${example_dir}" AND (WIN32 OR NOT example MATCHES "^(gallery|snapshot|chat)$"))
        file(GLOB_RECURSE example_sources CONFIGURE_DEPENDS "${example_dir}/*.cpp")
        add_executable(gran_${example} ${example_sources})
        # Заголовки примера подключаются от его корня; свои системные библиотеки — из system_libs.txt
        # The example headers are included from its root; its own system libraries come from system_libs.txt
        target_include_directories(gran_${example} PRIVATE "${example_dir}")
        target_link_libraries(gran_${example} PRIVATE gran)
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
        gran_apply_link_options(gran_${example})
    endif()
endforeach()

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
