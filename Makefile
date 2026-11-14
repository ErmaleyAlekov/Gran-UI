# ============================================================================
#  Gran UI — кроссплатформенный Makefile (GCC / Clang / MinGW)
#
#  Цели:
#    make               — библиотека bin/libgran.a, примеры и инструменты
#    make test          — собрать и запустить модульные тесты
#    make shaders       — HLSL → DXIL (+ SPIR-V при VULKAN=1) → src/rhi/shaders_gen/*.cpp (нужен dxc)
#    make beresta       — собрать статическую «Берёсту» в BERESTA_DIR
#    make clean / fclean / re / platform
#
#  Переменные:
#    BERESTA_DIR=путь   — каталог «Берёсты» (по умолчанию ../Dom lib C++)
#    DEBUG=1            — -O0 -g, отладочные слои D3D12 / Vulkan
#    VULKAN=1           — собрать бэкенд Vulkan (нужен Vulkan SDK: VULKAN_SDK)
#    NATIVE=0           — ядро без окна ОС и GPU: только безоконная платформа и NullBackend (Linux без X11;
#                         на macOS — по умолчанию, пока нет Cocoa и Metal)
#    DXC=путь           — компилятор шейдеров dxc
#    WEBVIEW2_SDK=путь  — build/native из NuGet-пакета Microsoft.Web.WebView2 (Windows): элемент <WebView>
#                         получает браузер, рядом с программами кладётся WebView2Loader.dll (после смены — make re)
# ============================================================================
#  Gran UI — a cross-platform Makefile (GCC / Clang / MinGW)
#
#  Targets:
#    make               — the bin/libgran.a library, the examples and the tools
#    make test          — build and run the unit tests
#    make shaders       — HLSL → DXIL (+ SPIR-V with VULKAN=1) → src/rhi/shaders_gen/*.cpp (needs dxc)
#    make beresta       — build the static Beresta library in BERESTA_DIR
#    make clean / fclean / re / platform
#
#  Variables:
#    BERESTA_DIR=path   — the Beresta directory (default ../Dom lib C++)
#    DEBUG=1            — -O0 -g, the D3D12 / Vulkan debug layers
#    VULKAN=1           — build the Vulkan backend (needs the Vulkan SDK: VULKAN_SDK)
#    NATIVE=0           — the core without an OS window and GPU: only the windowless platform and NullBackend (Linux
#                         without X11; on macOS — the default until Cocoa and Metal exist)
#    DXC=path           — the dxc shader compiler
#    WEBVIEW2_SDK=path  — build/native of the Microsoft.Web.WebView2 NuGet package (Windows): the <WebView> element
#                         gets a browser, WebView2Loader.dll is put next to the programs (after a change — make re)
# ============================================================================

# ---- Версия make --------------------------------------------------------------
# Нужен GNU make 4.0+: функция $(file …) (отметка флагов, system_libs.txt). На macOS штатный make — 3.81
# (2006 год): там `brew install make` и запуск gmake.
# ---- The make version --------------------------------------------------------------
# GNU make 4.0+ is needed: the $(file …) function (the flags stamp, system_libs.txt). The stock macOS make is 3.81
# (year 2006): there run `brew install make` and use gmake.
ifneq ($(filter 3.%,$(MAKE_VERSION)),)
    $(error GNU make 4.0+ is required (this is $(MAKE_VERSION)); on macOS: brew install make, then run gmake)
endif

# ---- Имена и каталоги -------------------------------------------------------
# ---- Names and directories -------------------------------------------------------
NAME        := libgran.a
.DEFAULT_GOAL := all
SRC_DIR     := src
INC_DIR     := include
OBJ_DIR     := obj
DEP_DIR     := dep
BIN_DIR     := bin
GEN_DIR     := gen
TEST_DIR    := tests
EXAMPLE_DIR := examples
TOOL_DIR    := tools

# Путь к «Берёсте» может содержать пробелы: он используется только в кавычках
# The Beresta path may contain spaces: it is used only inside quotes
BERESTA_DIR ?= ../Dom lib C++
BERESTA_LIB  = "$(BERESTA_DIR)/bin/libberesta.a"

# ---- Определение ОС ----------------------------------------------------------
# ---- Detecting the OS ----------------------------------------------------------
# На Windows явно используется cmd.exe: поведение не зависит от sh.exe в PATH
# On Windows cmd.exe is used explicitly: the behavior does not depend on sh.exe in PATH
ifeq ($(OS),Windows_NT)
    PLATFORM_OS := windows
else
    UNAME_S     := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATFORM_OS := macos
    else
        PLATFORM_OS := linux
    endif
endif

DEBUG       ?= 0
# Платформа ОС (окна, GPU): на macOS её пока нет (Cocoa — шаг M6, Metal — M5), поэтому там NATIVE=0
# The OS platform (windows, GPU): macOS has none yet (Cocoa — step M6, Metal — M5), therefore NATIVE=0 there
ifeq ($(PLATFORM_OS),macos)
NATIVE      ?= 0
else
NATIVE      ?= 1
endif
# Vulkan по умолчанию — только у платформы Linux / Vulkan by default — only with the Linux platform
ifeq ($(PLATFORM_OS)-$(NATIVE),linux-1)
VULKAN      ?= 1
else
VULKAN      ?= 0
endif
WEBVIEW2_SDK ?=
WEBVIEW2_ARCH ?= x64

# ---- Компилятор и флаги -------------------------------------------------------
# ---- The compiler and flags -------------------------------------------------------
# На macOS g++ — лишь псевдоним clang++: имя пишется честно / On macOS g++ is only an alias of clang++: the name is written honestly
ifeq ($(origin CXX),default)
    ifeq ($(PLATFORM_OS),macos)
        CXX := clang++
    else
        CXX := g++
    endif
endif
CXXFLAGS    := -std=c++17 -Wall -Wextra -Wpedantic -Wshadow
INCLUDES    := -I$(INC_DIR) -I"$(BERESTA_DIR)/include"
ifeq ($(DEBUG),1)
    CXXFLAGS += -O0 -g -DGRAN_DEBUG=1
else
    CXXFLAGS += -O2 -DNDEBUG
endif
# Выпуск: удаляем также отладку из статических библиотек компилятора при компоновке.
# Release: also remove debug information from the compiler's static libraries at link time.
# Компоновщик macOS (ld64) ключ -s считает устаревшим и пропускает с предупреждением: там отладку убирает strip.
# The macOS linker (ld64) treats the -s key as obsolete and skips it with a warning: there strip removes debug information.
ifeq ($(DEBUG)-$(PLATFORM_OS),0-macos)
else ifeq ($(DEBUG),0)
    LDFLAGS += -s
endif
# macOS: не ниже MACOS_MIN, как у «Берёсты» (11.0 — первая с Apple Silicon); иначе ld64 предупреждает
# о разных версиях объектов libberesta.a и наших
# macOS: not lower than MACOS_MIN, as in Beresta (11.0 is the first with Apple Silicon); otherwise ld64 warns
# about different versions of the libberesta.a objects and ours
MACOS_MIN   ?= 11.0
ifeq ($(PLATFORM_OS),macos)
    CXXFLAGS += -mmacosx-version-min=$(MACOS_MIN)
endif
# Санитайзеры GCC/Clang: make DEBUG=1 SANITIZE=address,undefined test (thread — отдельно, с address несовместим).
# Флаги идут в CXXFLAGS, потому что CXXFLAGS участвует и в компоновке. MinGW ASan не поддерживает —
# на Windows санитайзеры только через CMake и MSVC (-DGRAN_SANITIZE=address), см. docs/development.md.
# GCC/Clang sanitizers: make DEBUG=1 SANITIZE=address,undefined test (thread is separate, incompatible with address).
# The flags go into CXXFLAGS because CXXFLAGS also takes part in linking. MinGW does not support ASan —
# on Windows sanitizers are available only through CMake and MSVC (-DGRAN_SANITIZE=address), see docs/development.md.
SANITIZE    ?=
ifneq ($(strip $(SANITIZE)),)
    ifeq ($(OS),Windows_NT)
        $(error SANITIZE: MinGW does not support sanitizers, use CMake + MSVC with -DGRAN_SANITIZE=address)
    endif
    CXXFLAGS += -fsanitize=$(SANITIZE) -fno-omit-frame-pointer -fno-sanitize-recover=undefined -g
endif
ifeq ($(VULKAN),1)
    CXXFLAGS += -DGRAN_WITH_VULKAN=1
    INCLUDES += -I"$(VULKAN_SDK)/Include" -I"$(VULKAN_SDK)/include"
endif
ARFLAGS     := rcs

# ---- Платформенные особенности -------------------------------------------------
# PLATFORM_DIRS — подкаталоги src/, которые собираются только на этой ОС
# PLATFORM_DIRS — the src/ subdirectories that are built only on this OS
ifeq ($(PLATFORM_OS),windows)
    SHELL          := cmd.exe
    .SHELLFLAGS    := /C
    EXE            := .exe
    PLATFORM_DIRS  := platform/win32 rhi/dx12 rhi/dx11
    SYSTEM_LIBS    := -static -ld3d11 -ld3d12 -ldxgi -ldxguid -ldwmapi -lshcore -limm32 \
                      -luiautomationcore -loleaut32 -lole32 -luuid -luser32 -lgdi32 -lshell32
    native          = $(subst /,\,$(1))
    MKDIR           = if not exist "$(call native,$(1))" mkdir "$(call native,$(1))"
    RMDIR           = if exist "$(call native,$(1))" rmdir /S /Q "$(call native,$(1))"
    RMFILE          = if exist "$(call native,$(1))" del /Q "$(call native,$(1))"
    COPYFILE        = copy /Y "$(call native,$(1))" "$(call native,$(2))" >nul
    DXC            ?= C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe
else
    EXE            :=
    CXXFLAGS       += -fPIC -fvisibility=hidden -fvisibility-inlines-hidden
    native          = $(1)
    MKDIR           = mkdir -p "$(1)"
    RMDIR           = rm -rf "$(1)"
    RMFILE          = rm -f "$(1)"
    COPYFILE        = cp "$(1)" "$(2)"
    DXC            ?= dxc
    ifeq ($(NATIVE),0)
        # Ядро без платформы ОС: заглушки окна, диалогов и бэкенда (src/platform/none/, src/rhi/none/)
        # The core without the OS platform: the window, dialog and backend stubs (src/platform/none/, src/rhi/none/)
        PLATFORM_DIRS := platform/none rhi/none
        # Потоки: на Linux нужен -pthread, на macOS они в libSystem (ключ там лишний)
        # Threads: Linux needs -pthread, on macOS they are in libSystem (the key is redundant there)
        ifeq ($(PLATFORM_OS),linux)
            SYSTEM_LIBS := -pthread
        else
            SYSTEM_LIBS :=
        endif
    else ifeq ($(PLATFORM_OS),macos)
        $(error Gran UI: the macOS platform (Cocoa, Metal) is not ready yet, build the core with NATIVE=0)
    else
        PLATFORM_DIRS := platform/x11
        # libvulkan не компонуется: загрузчик открывается через dlopen (vulkan_loader.cpp)
        # libvulkan is not linked: the loader is opened through dlopen (vulkan_loader.cpp)
        SYSTEM_LIBS   := -ldl -pthread -lX11
    endif
endif

# ---- std::filesystem на GCC 8 -------------------------------------------------
# В libstdc++ до GCC 9 реализация std::filesystem лежит в отдельной статической
# библиотеке libstdc++fs.a (Astra Linux SE 1.7, Debian 10) — как в «Берёсте».
# Clang узнаётся по слову clang в --version.
# ---- std::filesystem on GCC 8 -------------------------------------------------
# In libstdc++ before GCC 9 the std::filesystem implementation lives in a separate
# static library libstdc++fs.a (Astra Linux SE 1.7, Debian 10) — as in Beresta.
# Clang is recognized by the word clang in --version.
ifneq ($(PLATFORM_OS),windows)
    CXX_VERSION_TEXT := $(shell $(CXX) --version)
    CXX_MAJOR        := $(firstword $(subst ., ,$(shell $(CXX) -dumpversion)))
    ifeq ($(findstring clang,$(CXX_VERSION_TEXT)),)
        ifneq ($(filter 5 6 7 8,$(CXX_MAJOR)),)
            SYSTEM_LIBS += -lstdc++fs
        endif
    endif
endif
ifeq ($(VULKAN),1)
    PLATFORM_DIRS += rhi/vulkan
endif
ifeq ($(PLATFORM_OS),windows)
    ifeq ($(NATIVE),0)
        $(error NATIVE=0: the build without the OS platform is for Linux and macOS)
    endif
endif
ifeq ($(PLATFORM_OS)-$(NATIVE),linux-1)
    ifneq ($(VULKAN),1)
        $(error Gran UI Linux requires VULKAN=1 (or NATIVE=0 for the core without windows))
    endif
else ifeq ($(NATIVE),0)
    ifeq ($(VULKAN),1)
        $(error NATIVE=0 builds no GPU backend: VULKAN=1 is not allowed)
    endif
else
    ifeq ($(VULKAN),1)
        $(error Gran UI Vulkan currently targets Linux; use Direct3D on Windows)
    endif
endif

# Встроенный браузер <WebView>: только Windows и только с SDK WebView2; без него — заглушка (web_view_fallback.cpp)
# The embedded <WebView> browser: Windows only and only with the WebView2 SDK; without it — a stub (web_view_fallback.cpp)
WEBVIEW2_DLL :=
ifneq ($(and $(filter windows,$(PLATFORM_OS)),$(WEBVIEW2_SDK)),)
    PLATFORM_DIRS += platform/webview2
    CXXFLAGS      += -DGRAN_HAS_WEBVIEW2 -isystem "$(WEBVIEW2_SDK)/include"
    WEBVIEW2_DLL  := $(BIN_DIR)/WebView2Loader.dll
endif

# Все платформенные каталоги: из общего набора исходников они исключаются
# All platform directories: they are excluded from the common source set
ALL_PLATFORM_DIRS := platform/win32 platform/x11 platform/cocoa platform/webview2 platform/none \n                     rhi/dx12 rhi/dx11 rhi/vulkan rhi/none

# ---- Вспомогательные функции ------------------------------------------------
# Рекурсивный поиск файлов средствами самого make (без внешней утилиты find)
# A recursive file search with make itself (without the external find utility)
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Путь к .d-файлу для объектного файла: obj/x/y.o -> dep/x/y.d
# The path to the .d file for an object file: obj/x/y.o -> dep/x/y.d
dep_of    = $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(1))

# Каталог файла без завершающего слеша: obj/x/y.o -> obj/x
# The file directory without the trailing slash: obj/x/y.o -> obj/x
dir_of    = $(patsubst %/,%,$(dir $(1)))

# ---- Наборы исходников -------------------------------------------------------
# ---- Source sets -------------------------------------------------------
ALL_LIB_SRC   := $(call rwildcard,$(SRC_DIR),*.cpp)
EXCLUDED_SRC  := $(foreach d,$(ALL_PLATFORM_DIRS),$(filter $(SRC_DIR)/$(d)/%,$(ALL_LIB_SRC)))
INCLUDED_SRC  := $(foreach d,$(PLATFORM_DIRS),$(filter $(SRC_DIR)/$(d)/%,$(ALL_LIB_SRC)))
LIB_SRC       := $(filter-out $(EXCLUDED_SRC),$(ALL_LIB_SRC)) $(INCLUDED_SRC)
LIB_OBJ       := $(LIB_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Сгенерированные исходники: встроенная схема разметки (всегда) из schemas/
# Generated sources: the embedded markup schema (always) from schemas/
SCHEMA_XSD    := schemas/gran-ui.xsd
SCHEMA_CPP    := $(GEN_DIR)/markup/gran_ui_schema.cpp
SCHEMA_OBJ    := $(OBJ_DIR)/$(GEN_DIR)/markup/gran_ui_schema.o
THEME_XML     := schemas/default_theme.xml
THEME_CPP     := $(GEN_DIR)/markup/default_theme.cpp
THEME_OBJ     := $(OBJ_DIR)/$(GEN_DIR)/markup/default_theme.o
RES_SCHEMA_XSD := schemas/gran-resources.xsd
RES_SCHEMA_CPP := $(GEN_DIR)/res/gran_resources_schema.cpp
RES_SCHEMA_OBJ := $(OBJ_DIR)/$(GEN_DIR)/res/gran_resources_schema.o
SET_SCHEMA_XSD := schemas/gran-settings.xsd
SET_SCHEMA_CPP := $(GEN_DIR)/settings/gran_settings_schema.cpp
SET_SCHEMA_OBJ := $(OBJ_DIR)/$(GEN_DIR)/settings/gran_settings_schema.o
LIB_OBJ       += $(SCHEMA_OBJ) $(THEME_OBJ) $(RES_SCHEMA_OBJ) $(SET_SCHEMA_OBJ)

# Инструмент встраивания файлов в C++ (собирается первым, без библиотеки)
# The tool that embeds files into C++ (built first, without the library)
EMBED_SRC     := $(TOOL_DIR)/embed/embed_main.cpp
EMBED_OBJ     := $(OBJ_DIR)/$(TOOL_DIR)/embed/embed_main.o
EMBED_BIN     := $(BIN_DIR)/gran_embed$(EXE)
# Встраивание ресурсов программы в exe (gran_rcc, нужна библиотека) / Embedding the program resources into the exe (gran_rcc, needs the library)
RCC_OBJ       := $(OBJ_DIR)/$(TOOL_DIR)/rcc/rcc_main.o
RCC_BIN       := $(BIN_DIR)/gran_rcc$(EXE)

TEST_SRC      := $(call rwildcard,$(TEST_DIR)/src,*.cpp)
TEST_OBJ      := $(TEST_SRC:$(TEST_DIR)/src/%.cpp=$(OBJ_DIR)/$(TEST_DIR)/%.o)
TEST_BIN      := $(BIN_DIR)/gran_tests$(EXE)

# Переносимая часть примера chat (JSON, chunked, NDJSON, HTTP) проверяется модульными тестами
# The portable part of the chat example (JSON, chunked, NDJSON, HTTP) is checked by the unit tests
CHAT_DIR      := $(EXAMPLE_DIR)/chat
CHAT_COMMON   := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(call rwildcard,$(CHAT_DIR)/common,*.cpp))

# Примеры: каждый подкаталог examples/<имя>/ — отдельная программа bin/gran_<имя>
# Examples: each examples/<name>/ subdirectory is a separate program bin/gran_<name>
EXAMPLES      := $(notdir $(patsubst %/,%,$(wildcard $(EXAMPLE_DIR)/*/)))
ifneq ($(PLATFORM_OS),windows)
    EXAMPLES  := $(filter-out chat,$(EXAMPLES))
endif
# NATIVE=0: всем примерам нужно окно ОС или GPU (gallery, editor — окно, snapshot — бэкенд), без платформы они только
# бросили бы исключение — поэтому не собираются (ревью Т-48). Инструменты gran_embed и gran_rcc собираются.
# NATIVE=0: every example needs an OS window or a GPU (gallery, editor — a window, snapshot — a backend), without the
# platform they would only throw — therefore they are not built (review T-48). The gran_embed and gran_rcc tools are built.
ifeq ($(NATIVE),0)
    EXAMPLES  :=
endif
EXAMPLE_BINS  := $(foreach e,$(EXAMPLES),$(BIN_DIR)/gran_$(e)$(EXE))
# Объект встроенных ресурсов примера (есть examples/<имя>/resources.xml) / The example embedded resource object (when examples/<name>/resources.xml exists)
example_resources = $(if $(wildcard $(EXAMPLE_DIR)/$(1)/resources.xml),$(OBJ_DIR)/$(GEN_DIR)/$(EXAMPLE_DIR)/$(1)/resources.o)

# Свои системные библиотеки примера: по имени в строке файла examples/<имя>/system_libs.txt
# The example's own system libraries: one name per line of the examples/<name>/system_libs.txt file
example_libs  = $(if $(wildcard $(EXAMPLE_DIR)/$(1)/system_libs.txt),$(addprefix -l,$(strip $(file < $(EXAMPLE_DIR)/$(1)/system_libs.txt))))

# Оконная программа без консоли (подсистема WINDOWS): пример с файлом-меткой examples/<имя>/windows_gui.txt
# A windowed program without a console (the WINDOWS subsystem): an example with the examples/<name>/windows_gui.txt marker file
example_gui   = $(if $(and $(filter windows,$(PLATFORM_OS)),$(wildcard $(EXAMPLE_DIR)/$(1)/windows_gui.txt)),-mwindows)

LIB_BIN       := $(BIN_DIR)/$(NAME)
ALL_OBJ       := $(LIB_OBJ) $(TEST_OBJ) $(EMBED_OBJ) $(RCC_OBJ) $(CHAT_COMMON) \
                 $(foreach e,$(EXAMPLES),$(call example_resources,$(e))) \
                 $(foreach e,$(EXAMPLES),$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(call rwildcard,$(EXAMPLE_DIR)/$(e),*.cpp)))
ALL_DEP       := $(call dep_of,$(ALL_OBJ))
ALL_DIRS      := $(sort $(call dir_of,$(ALL_OBJ) $(ALL_DEP) $(SCHEMA_CPP) $(RES_SCHEMA_CPP) $(SET_SCHEMA_CPP)) $(BIN_DIR))

# -MMD/-MP: зависимости от заголовков в dep/, удалённый заголовок не ломает сборку
# -MMD/-MP: the header dependencies go to dep/, a removed header does not break the build
DEPFLAGS      = -MMD -MP -MF $(call dep_of,$@) -MT $@

# ---- Основные цели ----------------------------------------------------------
# ---- Main targets ----------------------------------------------------------
.PHONY: all test shaders beresta platform clean fclean re

# GPU-тесты запускаются отдельно; эталоны обновляются только явной целью.
# GPU tests run separately; baselines update only through an explicit target.
ifeq ($(PLATFORM_OS)-$(NATIVE),linux-1)
RENDER_TEST_SRC := $(wildcard tests/linux/*.cpp)
RENDER_TEST_BIN := $(BIN_DIR)/gran_linux_tests
.PHONY: test-render test-window update-render-baselines
$(RENDER_TEST_BIN): $(RENDER_TEST_SRC) tests/linux/linux_tests.hpp $(LIB_BIN)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $(RENDER_TEST_SRC) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@
test-render: $(RENDER_TEST_BIN)
	$(RENDER_TEST_BIN)
test-window: $(RENDER_TEST_BIN)
	$(RENDER_TEST_BIN) --window
update-render-baselines:
	@echo Gran UI: reference baselines require Windows D3D12 WARP
	@false
else ifeq ($(NATIVE),0)
# Без GPU снимков нет: цель сообщает об этом и завершается ошибкой / Without a GPU there are no snapshots: the target says so and fails
.PHONY: test-render update-render-baselines
test-render update-render-baselines:
	@echo Gran UI: NATIVE=0 has no GPU backend, render tests need the OS platform
	@false
else
RENDER_TEST_SRC := $(wildcard tests/render/*.cpp)
RENDER_TEST_BIN := $(BIN_DIR)/gran_render_tests$(EXE)
.PHONY: test-render update-render-baselines
$(RENDER_TEST_BIN): $(RENDER_TEST_SRC) $(wildcard tests/render/*.hpp) $(LIB_BIN)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $(RENDER_TEST_SRC) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@
test-render: $(RENDER_TEST_BIN)
	$(call native,$(RENDER_TEST_BIN)) --d3d12
	$(call native,$(RENDER_TEST_BIN)) --d3d11
update-render-baselines: $(RENDER_TEST_BIN)
	$(call native,$(RENDER_TEST_BIN)) --update
endif

all: $(LIB_BIN) $(RCC_BIN) $(EXAMPLE_BINS) $(WEBVIEW2_DLL)

# Языки загружают одну библиотеку; C++-примеры продолжают использовать статическую.
# Languages load one library; C++ examples continue to use the static library.
ifeq ($(PLATFORM_OS),windows)
    CAPI_BIN := $(BIN_DIR)/gran_c.dll
else ifeq ($(PLATFORM_OS),macos)
    CAPI_BIN := $(BIN_DIR)/libgran_c.dylib
else
    CAPI_BIN := $(BIN_DIR)/libgran_c.so
endif
CAPI_SRC := $(wildcard src/capi/*.cpp)
.PHONY: bindings
# В Си-интерфейсе нет безоконного режима: без платформы ОС библиотека для обёрток не создала бы ни одного окна (ревью Т-48)
# The C interface has no windowless mode: without the OS platform the binding library could not create a single window (review T-48)
ifeq ($(NATIVE),0)
bindings:
	@echo Gran UI: the C ABI gran_c needs the OS platform, it is not built with NATIVE=0
	@false
else
bindings: $(CAPI_BIN) $(WEBVIEW2_DLL)
endif
$(CAPI_BIN): $(CAPI_SRC) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) -DGRAN_CAPI_BUILD=1 -shared $(CAPI_SRC) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@

test: $(TEST_BIN)
	$(call native,$(TEST_BIN))

# Отчёт о сбое: программа падает сама (SEGV, terminate, abort) и проверяет свои отчёты
# The crash report: the program crashes itself (SEGV, terminate, abort) and checks its reports
CRASH_SRC := $(wildcard tests/crash/*.cpp)
CRASH_BIN := $(BIN_DIR)/gran_crash_probe$(EXE)
.PHONY: test-crash
$(CRASH_BIN): $(CRASH_SRC) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $(CRASH_SRC) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@
test-crash: $(CRASH_BIN)
	$(call native,$(CRASH_BIN)) --check $(BIN_DIR)/crash_check

# Статическая «Берёста» собирается её собственным Makefile
# The static Beresta is built by its own Makefile
beresta:
	$(MAKE) -C "$(BERESTA_DIR)" bin/libberesta.a

# Архив собирается заново, чтобы объекты удалённых исходников не остались в нём
# The archive is rebuilt so that the objects of removed sources do not stay in it
$(LIB_BIN): $(LIB_OBJ) | $(BIN_DIR)
	@$(call RMFILE,$@)
	$(AR) $(ARFLAGS) $@ $(LIB_OBJ)

$(TEST_BIN): $(TEST_OBJ) $(CHAT_COMMON) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TEST_OBJ) $(CHAT_COMMON) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@

# Шаблон компоновки примера: его объекты + библиотека + «Берёста» + системные библиотеки
# The example link template: its objects + the library + Beresta + the system libraries
define EXAMPLE_RULE
$(BIN_DIR)/gran_$(1)$(EXE): $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(call rwildcard,$(EXAMPLE_DIR)/$(1),*.cpp)) $(call example_resources,$(1)) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $$(filter %.o,$$^) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) $(call example_libs,$(1)) $(call example_gui,$(1)) -o $$@
endef

# Ресурсы примера: examples/<имя>/resources.xml → gen/examples/<имя>/resources.cpp (gran_rcc) → объект в exe;
# файл .d от gran_rcc пересобирает .cpp при правке любого встроенного файла
# The example resources: examples/<name>/resources.xml → gen/examples/<name>/resources.cpp (gran_rcc) → an object in the exe;
# the .d file from gran_rcc rebuilds the .cpp when any embedded file is edited
define RESOURCE_RULE
$(GEN_DIR)/$(EXAMPLE_DIR)/$(1)/resources.cpp: $(EXAMPLE_DIR)/$(1)/resources.xml $(RCC_BIN)
	@$(call MKDIR,$(GEN_DIR)/$(EXAMPLE_DIR)/$(1))
	@$(call MKDIR,$(OBJ_DIR)/$(GEN_DIR)/$(EXAMPLE_DIR)/$(1))
	$(call native,$(RCC_BIN)) $(EXAMPLE_DIR)/$(1)/resources.xml $$@ --depfile $(GEN_DIR)/$(EXAMPLE_DIR)/$(1)/resources.d
-include $(GEN_DIR)/$(EXAMPLE_DIR)/$(1)/resources.d
endef
$(foreach e,$(EXAMPLES),$(if $(wildcard $(EXAMPLE_DIR)/$(e)/resources.xml),$(eval $(call RESOURCE_RULE,$(e)))))

$(RCC_BIN): $(RCC_OBJ) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(RCC_OBJ) $(LIB_BIN) $(BERESTA_LIB) $(SYSTEM_LIBS) -o $@
$(foreach e,$(EXAMPLES),$(eval $(call EXAMPLE_RULE,$(e))))

# Загрузчик WebView2 рядом с программами: движок ищет его только в каталоге программы и System32
# The WebView2 loader next to the programs: the engine looks for it only in the program directory and System32
$(BIN_DIR)/WebView2Loader.dll: | $(BIN_DIR)
	$(call COPYFILE,$(WEBVIEW2_SDK)/$(WEBVIEW2_ARCH)/WebView2Loader.dll,$@)

$(EMBED_BIN): $(EMBED_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(EMBED_OBJ) -o $@

# Схема разметки превращается в массив байтов gran::generated::GRAN_UI_SCHEMA
# The markup schema becomes the byte array gran::generated::GRAN_UI_SCHEMA
$(SCHEMA_CPP): $(SCHEMA_XSD) $(EMBED_BIN) | $(call dir_of,$(SCHEMA_CPP))
	$(call native,$(EMBED_BIN)) $(SCHEMA_XSD) $@ GRAN_UI_SCHEMA

# Схема манифеста ресурсов gran_rcc — GRAN_RESOURCES_SCHEMA / The gran_rcc resource manifest schema — GRAN_RESOURCES_SCHEMA
$(RES_SCHEMA_CPP): $(RES_SCHEMA_XSD) $(EMBED_BIN) | $(call dir_of,$(RES_SCHEMA_CPP))
	$(call native,$(EMBED_BIN)) $(RES_SCHEMA_XSD) $@ GRAN_RESOURCES_SCHEMA

# Схема файла настроек — GRAN_SETTINGS_SCHEMA / The settings file schema — GRAN_SETTINGS_SCHEMA
$(SET_SCHEMA_CPP): $(SET_SCHEMA_XSD) $(EMBED_BIN) | $(call dir_of,$(SET_SCHEMA_CPP))
	$(call native,$(EMBED_BIN)) $(SET_SCHEMA_XSD) $@ GRAN_SETTINGS_SCHEMA

# Тема по умолчанию — разметка, встроенная как DEFAULT_THEME / The default theme is markup embedded as DEFAULT_THEME
$(THEME_CPP): $(THEME_XML) $(EMBED_BIN) | $(call dir_of,$(THEME_CPP))
	$(call native,$(EMBED_BIN)) $(THEME_XML) $@ DEFAULT_THEME

# ---- Шейдеры (по запросу, нужен dxc) ------------------------------------------
# ---- Shaders (on request, needs dxc) ------------------------------------------
SHADER_SRC    := shaders/hlsl/ui.hlsl
SHADER_OUT    := $(SRC_DIR)/rhi/shaders_gen
SHADER_TMP    := $(OBJ_DIR)/shaders
FXC ?= C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe

# DXBC из общего HLSL для Shader Model 5.0 / DXBC from shared HLSL for Shader Model 5.0
.PHONY: shaders-dx11
shaders-dx11: $(EMBED_BIN)
	@$(call MKDIR,$(SHADER_TMP))
	"$(FXC)" /nologo /D GRAN_D3D11=1 /T vs_5_0 /E vsMain /O3 /Fo $(SHADER_TMP)/ui_vs.dxbc $(SHADER_SRC)
	"$(FXC)" /nologo /D GRAN_D3D11=1 /T ps_5_0 /E psMain /O3 /Fo $(SHADER_TMP)/ui_ps.dxbc $(SHADER_SRC)
	"$(FXC)" /nologo /D GRAN_D3D11=1 /T ps_5_0 /E psBlurDown /O3 /Fo $(SHADER_TMP)/blur_down_ps.dxbc $(SHADER_SRC)
	"$(FXC)" /nologo /D GRAN_D3D11=1 /T ps_5_0 /E psBlurUp /O3 /Fo $(SHADER_TMP)/blur_up_ps.dxbc $(SHADER_SRC)
	"$(FXC)" /nologo /D GRAN_D3D11=1 /T vs_5_0 /E vsFullscreen /O3 /Fo $(SHADER_TMP)/fullscreen_vs.dxbc $(SHADER_SRC)
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/ui_vs.dxbc $(SHADER_OUT)/ui_vs_dxbc.cpp UI_VS_DXBC
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/ui_ps.dxbc $(SHADER_OUT)/ui_ps_dxbc.cpp UI_PS_DXBC
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/blur_down_ps.dxbc $(SHADER_OUT)/blur_down_ps_dxbc.cpp BLUR_DOWN_PS_DXBC
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/blur_up_ps.dxbc $(SHADER_OUT)/blur_up_ps_dxbc.cpp BLUR_UP_PS_DXBC
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/fullscreen_vs.dxbc $(SHADER_OUT)/fullscreen_vs_dxbc.cpp FULLSCREEN_VS_DXBC

shaders: $(EMBED_BIN)
	@$(call MKDIR,$(SHADER_TMP))
	"$(DXC)" -nologo -T vs_6_0 -E vsMain -O3 -Fo $(SHADER_TMP)/ui_vs.dxil $(SHADER_SRC)
	"$(DXC)" -nologo -T ps_6_0 -E psMain -O3 -Fo $(SHADER_TMP)/ui_ps.dxil $(SHADER_SRC)
	"$(DXC)" -nologo -T ps_6_0 -E psBlurDown -O3 -Fo $(SHADER_TMP)/blur_down_ps.dxil $(SHADER_SRC)
	"$(DXC)" -nologo -T ps_6_0 -E psBlurUp -O3 -Fo $(SHADER_TMP)/blur_up_ps.dxil $(SHADER_SRC)
	"$(DXC)" -nologo -T vs_6_0 -E vsFullscreen -O3 -Fo $(SHADER_TMP)/fullscreen_vs.dxil $(SHADER_SRC)
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/ui_vs.dxil $(SHADER_OUT)/ui_vs_dxil.cpp UI_VS_DXIL
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/ui_ps.dxil $(SHADER_OUT)/ui_ps_dxil.cpp UI_PS_DXIL
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/blur_down_ps.dxil $(SHADER_OUT)/blur_down_ps_dxil.cpp BLUR_DOWN_PS_DXIL
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/blur_up_ps.dxil $(SHADER_OUT)/blur_up_ps_dxil.cpp BLUR_UP_PS_DXIL
	$(call native,$(EMBED_BIN)) $(SHADER_TMP)/fullscreen_vs.dxil $(SHADER_OUT)/fullscreen_vs_dxil.cpp FULLSCREEN_VS_DXIL

# ---- Отметка флагов сборки ----------------------------------------------------
# ---- The build flags stamp ----------------------------------------------------
# Объектные файлы выпуска и отладки лежат в одном obj/, а make сравнивает только даты файлов: без отметки
# `make DEBUG=1` после выпуска оставлял объектники -O2 без отладочной информации, и отладчик не ставил точки
# останова (найдено в ручной проверке Т-2, 2026-11-06). Поэтому полная строка флагов хранится в obj/.flags;
# если она изменилась, файл переписывается при чтении Makefile и становится новее всех .o — они пересобираются.
# Сравнение — по содержимому, решение о пересборке — по дате, поэтому `make -n` тоже безопасен.
# The release and debug object files live in one obj/, and make compares only file dates: without a stamp
# `make DEBUG=1` after a release build kept -O2 object files without debug information, and the debugger set
# no breakpoints (found in the manual check T-2, 2026-11-06). Therefore the full flags line is kept in obj/.flags;
# if it changed, the file is rewritten while the Makefile is read and becomes newer than every .o — they are rebuilt.
# The comparison is by content, the rebuild decision is by date, so `make -n` is safe as well.
FLAGS_STAMP := $(OBJ_DIR)/.flags
FLAGS_NOW   := $(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $(SYSTEM_LIBS)
FLAGS_OLD   := $(file <$(FLAGS_STAMP))
ifneq '$(FLAGS_OLD)' '$(FLAGS_NOW)'
    $(shell $(call MKDIR,$(OBJ_DIR)))
    $(file >$(FLAGS_STAMP),$(FLAGS_NOW))
endif
# Пустое правило: после clean в том же вызове отметки нет — make считает её обновлённой и пересобирает всё
# An empty rule: after clean in the same call there is no stamp — make treats it as updated and rebuilds everything
$(FLAGS_STAMP): ;
# Объектники и программы, которые компилируются прямо из исходников, зависят от отметки
# The object files and the programs compiled straight from sources depend on the stamp
$(ALL_OBJ) $(RENDER_TEST_BIN) $(CAPI_BIN) $(CRASH_BIN): $(FLAGS_STAMP)

# ---- Правила компиляции -----------------------------------------------------
# ---- Compilation rules -----------------------------------------------------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/$(GEN_DIR)/%.o: $(GEN_DIR)/%.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/src/%.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(TEST_DIR)/include -I$(CHAT_DIR) $(DEPFLAGS) -c $< -o $@

# Корень примера (examples/<имя>) — в путях включения: его заголовки подключаются от корня
# The example root (examples/<name>) is in the include paths: its headers are included from the root
$(OBJ_DIR)/$(EXAMPLE_DIR)/%.o: $(EXAMPLE_DIR)/%.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(EXAMPLE_DIR)/$(word 2,$(subst /, ,$<)) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TOOL_DIR)/%.o: $(TOOL_DIR)/%.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# gran_rcc пользуется библиотекой (схема, разбор манифеста) — ему нужны её заголовки
# gran_rcc uses the library (the schema, the manifest parsing) — it needs its headers
$(RCC_OBJ): $(TOOL_DIR)/rcc/rcc_main.cpp | $(ALL_DIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(ALL_DIRS):
	@$(call MKDIR,$@)

# ---- Служебные цели -----------------------------------------------------------
# ---- Service targets -----------------------------------------------------------
platform:
	@echo OS: $(PLATFORM_OS)
	@echo Native platform: $(NATIVE)
	@echo Platform sources: $(PLATFORM_DIRS)
	@echo Beresta: $(BERESTA_DIR)

clean:
	@$(call RMDIR,$(OBJ_DIR))
	@$(call RMDIR,$(DEP_DIR))
	@$(call RMDIR,$(GEN_DIR))

fclean: clean
	@$(call RMDIR,$(BIN_DIR))

re: fclean
	$(MAKE) all

-include $(ALL_DEP)

# SPIR-V встроен; SDK-компилятор нужен только при изменении HLSL.
# SPIR-V is embedded; the SDK compiler is only needed when changing HLSL.
.PHONY: shaders-vulkan
shaders-vulkan: $(EMBED_BIN)
	@$(call MKDIR,$(OBJ_DIR)/shaders)
	"$(DXC)" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 -fvk-invert-y -T vs_6_0 -E vsMain -O3 -Fo $(OBJ_DIR)/shaders/ui_vs.spv shaders/hlsl/ui.hlsl
	$(EMBED_BIN) $(OBJ_DIR)/shaders/ui_vs.spv src/rhi/shaders_gen/ui_vs_spv.cpp UI_VS_SPV
	"$(DXC)" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 -T ps_6_0 -E psMain -O3 -Fo $(OBJ_DIR)/shaders/ui_ps.spv shaders/hlsl/ui.hlsl
	$(EMBED_BIN) $(OBJ_DIR)/shaders/ui_ps.spv src/rhi/shaders_gen/ui_ps_spv.cpp UI_PS_SPV
	"$(DXC)" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 -fvk-invert-y -T vs_6_0 -E vsFullscreen -O3 -Fo $(OBJ_DIR)/shaders/fullscreen_vs.spv shaders/hlsl/ui.hlsl
	$(EMBED_BIN) $(OBJ_DIR)/shaders/fullscreen_vs.spv src/rhi/shaders_gen/fullscreen_vs_spv.cpp FULLSCREEN_VS_SPV
	"$(DXC)" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 -T ps_6_0 -E psBlurDown -O3 -Fo $(OBJ_DIR)/shaders/blur_down_ps.spv shaders/hlsl/ui.hlsl
	$(EMBED_BIN) $(OBJ_DIR)/shaders/blur_down_ps.spv src/rhi/shaders_gen/blur_down_ps_spv.cpp BLUR_DOWN_PS_SPV
	"$(DXC)" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 -T ps_6_0 -E psBlurUp -O3 -Fo $(OBJ_DIR)/shaders/blur_up_ps.spv shaders/hlsl/ui.hlsl
	$(EMBED_BIN) $(OBJ_DIR)/shaders/blur_up_ps.spv src/rhi/shaders_gen/blur_up_ps_spv.cpp BLUR_UP_PS_SPV
