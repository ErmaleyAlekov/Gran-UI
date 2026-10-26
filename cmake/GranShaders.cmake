# ============================================================================
#  Шейдеры по запросу: cmake --build <сборка> --target gran_shaders
#  Один исходник HLSL → DXIL (dxc) → src/rhi/shaders_gen/*.cpp через gran_embed.
#  Сгенерированные файлы лежат в исходниках (как у make shaders): обычной сборке dxc не нужен.
#
#  Shaders on request: cmake --build <build> --target gran_shaders
#  One HLSL source → DXIL (dxc) → src/rhi/shaders_gen/*.cpp through gran_embed.
#  The generated files live in the sources (as with make shaders): a normal build does not need dxc.
# ============================================================================

# dxc: из Windows SDK 10.0.26100, из PATH или из Vulkan SDK / dxc: from Windows SDK 10.0.26100, from PATH or from the Vulkan SDK
find_program(GRAN_DXC dxc
    HINTS "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64" "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
    DOC "Компилятор шейдеров dxc / The dxc shader compiler")

# Шейдеры: имя файла, тип, точка входа, имя массива / The shaders: the file name, the type, the entry point, the array name
set(GRAN_SHADERS
    "ui_vs|vs_6_0|vsMain|UI_VS_DXIL"
    "ui_ps|ps_6_0|psMain|UI_PS_DXIL"
    "blur_down_ps|ps_6_0|psBlurDown|BLUR_DOWN_PS_DXIL"
    "blur_up_ps|ps_6_0|psBlurUp|BLUR_UP_PS_DXIL"
    "fullscreen_vs|vs_6_0|vsFullscreen|FULLSCREEN_VS_DXIL")

# Команды идут из папки сборки с относительным путём obj/shaders: gran_embed пишет путь в комментарий
# сгенерированного файла, и он совпадает с make shaders (без путей машины в исходниках)
# The commands run from the build directory with the relative path obj/shaders: gran_embed writes the path into
# the generated file comment, and it matches make shaders (no machine paths in the sources)
if(GRAN_DXC)
    set(source "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hlsl/ui.hlsl")
    set(temporary "obj/shaders")
    set(commands COMMAND "${CMAKE_COMMAND}" -E make_directory "${temporary}")
    foreach(shader IN LISTS GRAN_SHADERS)
        string(REPLACE "|" ";" parts "${shader}")
        list(GET parts 0 name)
        list(GET parts 1 profile)
        list(GET parts 2 entry)
        list(GET parts 3 symbol)
        list(APPEND commands
            COMMAND "${GRAN_DXC}" -nologo -T ${profile} -E ${entry} -O3 -Fo "${temporary}/${name}.dxil" "${source}"
            COMMAND gran_embed "${temporary}/${name}.dxil"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/rhi/shaders_gen/${name}_dxil.cpp" ${symbol})
    endforeach()
    add_custom_target(gran_shaders ${commands} DEPENDS gran_embed WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "HLSL → DXIL → src/rhi/shaders_gen" VERBATIM)
else()
    add_custom_target(gran_shaders
        COMMAND "${CMAKE_COMMAND}" -E echo "Gran UI: dxc не найден, задайте -DGRAN_DXC / dxc not found, set -DGRAN_DXC"
        COMMAND "${CMAKE_COMMAND}" -E false)
endif()
