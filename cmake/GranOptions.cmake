# ============================================================================
#  Флаги компиляции и компоновки, системные библиотеки по ОС
#  Compile and link flags, the system libraries by OS
# ============================================================================

# Системные библиотеки платформы (как SYSTEM_LIBS в Makefile)
# The platform system libraries (as SYSTEM_LIBS in the Makefile)
if(WIN32)
    set(GRAN_SYSTEM_LIBS d3d11 d3d12 dxgi dxguid dcomp dwmapi shcore imm32 uiautomationcore oleaut32 ole32 uuid
        user32 gdi32 shell32)
elseif(APPLE)
    set(GRAN_SYSTEM_LIBS "-framework Cocoa" "-framework QuartzCore")
else()
    find_package(Threads REQUIRED)
    set(GRAN_SYSTEM_LIBS ${CMAKE_DL_LIBS} Threads::Threads)
endif()

# Предупреждения и кодировка; GRAN_DEBUG=1 в отладочной сборке (отладочные слои GPU).
# /utf-8 для MSVC обязателен: литералы с кириллицей в UTF-8, иначе MSVC читает их в ANSI.
# Warnings and encoding; GRAN_DEBUG=1 in the debug build (the GPU debug layers).
# /utf-8 for MSVC is mandatory: Cyrillic literals are UTF-8, otherwise MSVC reads them as ANSI.
function(gran_apply_compile_options target)
    # /Zc:preprocessor — стандартный препроцессор (__VA_ARGS__ в TEST_ASSERT_NEAR раскрывается как в GCC);
    # _CRT_SECURE_NO_WARNINGS — getenv и fopen стандартны, «безопасные» _s-версии есть только в MSVC
    # /Zc:preprocessor — the standard preprocessor (__VA_ARGS__ in TEST_ASSERT_NEAR expands as in GCC);
    # _CRT_SECURE_NO_WARNINGS — getenv and fopen are standard, the "secure" _s versions exist only in MSVC
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc /Zc:preprocessor)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    endif()
    target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:GRAN_DEBUG=1>)
    gran_apply_sanitizers(${target})
endfunction()

# Санитайзеры по -DGRAN_SANITIZE=address[,undefined|thread]: GCC/Clang — -fsanitize=…, MSVC — только address.
# Флаги нужны и при компиляции, и при компоновке; MinGW санитайзеры не поддерживает.
# Sanitizers by -DGRAN_SANITIZE=address[,undefined|thread]: GCC/Clang — -fsanitize=…, MSVC — address only.
# The flags are needed both when compiling and when linking; MinGW does not support sanitizers.
set(GRAN_SANITIZE "" CACHE STRING "Sanitizers: address, undefined, thread (comma-separated); MSVC: address")
function(gran_apply_sanitizers target)
    if(GRAN_SANITIZE STREQUAL "")
        return()
    endif()
    if(MINGW)
        message(FATAL_ERROR "GRAN_SANITIZE: MinGW does not support sanitizers, use MSVC or Linux GCC/Clang")
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
        target_link_options(${target} PRIVATE /INCREMENTAL:NO /DEBUG)
    else()
        set(flags -fsanitize=${GRAN_SANITIZE} -fno-omit-frame-pointer -fno-sanitize-recover=undefined -g)
        target_compile_options(${target} PRIVATE ${flags})
        target_link_options(${target} PRIVATE ${flags})
    endif()
endfunction()

# MinGW: -static, как в Makefile — программе не нужны libstdc++-6.dll и libgcc
# MinGW: -static, as in the Makefile — a program needs neither libstdc++-6.dll nor libgcc
# Только Release/MinSizeRel: -s удаляет отладку, в том числе из статического runtime.
# Release/MinSizeRel only: -s removes debug information, including that from the static runtime.
function(gran_apply_link_options target)
    if(MINGW)
        target_link_options(${target} PRIVATE -static)
        target_link_options(${target} PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>)
    endif()
endfunction()
