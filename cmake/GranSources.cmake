# ============================================================================
#  Исходники библиотеки и встроенные файлы
#  The library sources and the embedded files
# ============================================================================

# Платформенные каталоги src/: собираются только на своей ОС (как PLATFORM_DIRS в Makefile)
# The platform src/ directories: built only on their OS (as PLATFORM_DIRS in the Makefile)
set(GRAN_ALL_PLATFORM_DIRS platform/win32 platform/x11 platform/cocoa platform/webview2 rhi/dxgi rhi/dx12 rhi/dx11 rhi/vulkan)
if(WIN32)
    set(GRAN_PLATFORM_DIRS platform/win32 rhi/dxgi rhi/dx12 rhi/dx11)
elseif(APPLE)
    set(GRAN_PLATFORM_DIRS platform/cocoa)
else()
    set(GRAN_PLATFORM_DIRS platform/x11)
endif()
if(WIN32 AND GRAN_WEBVIEW2_SDK)
    list(APPEND GRAN_PLATFORM_DIRS platform/webview2)
endif()
if(GRAN_WITH_VULKAN)
    list(APPEND GRAN_PLATFORM_DIRS rhi/vulkan)
endif()

# Все src/**/*.cpp минус чужие платформенные каталоги / All src/**/*.cpp minus the foreign platform directories
function(gran_collect_sources out)
    set(root "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(kept "")
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${root}/*.cpp")
    # Сравнение префиксом относительного пути, а не регулярным выражением: в путях бывают «+» и скобки
    # Comparing by the relative path prefix, not by a regular expression: paths may contain "+" and brackets
    foreach(source IN LISTS sources)
        file(RELATIVE_PATH relative "${root}" "${source}")
        set(foreign FALSE)
        foreach(directory IN LISTS GRAN_ALL_PLATFORM_DIRS)
            string(FIND "${relative}" "${directory}/" position)
            if(position EQUAL 0 AND NOT directory IN_LIST GRAN_PLATFORM_DIRS)
                set(foreign TRUE)
            endif()
        endforeach()
        if(NOT foreign)
            list(APPEND kept "${source}")
        endif()
    endforeach()
    set(${out} ${kept} PARENT_SCOPE)
endfunction()

# Файл → массив байтов gran::generated::<symbol> в <сборка>/gen/<output>; пересборка при смене файла
# или инструмента. Результат добавляется к GRAN_GENERATED_SOURCES.
# A file → the byte array gran::generated::<symbol> in <build>/gen/<output>; rebuilt when the file
# or the tool changes. The result is appended to GRAN_GENERATED_SOURCES.
function(gran_embed_file input output symbol)
    set(generated "${CMAKE_BINARY_DIR}/gen/${output}")
    get_filename_component(directory "${generated}" DIRECTORY)
    add_custom_command(
        OUTPUT "${generated}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${directory}"
        COMMAND gran_embed "${input}" "${generated}" ${symbol}
        DEPENDS "${input}" gran_embed
        COMMENT "gran_embed ${output}"
        VERBATIM)
    set(GRAN_GENERATED_SOURCES ${GRAN_GENERATED_SOURCES} "${generated}" PARENT_SCOPE)
endfunction()
