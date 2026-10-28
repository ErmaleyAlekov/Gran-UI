# Один HLSL → пять встроенных SPIR-V; эта цель нужна только при правке шейдеров.
# One HLSL source → five embedded SPIR-V blobs; this target is only needed when editing shaders.
if(GRAN_DXC)
    set(vk_commands COMMAND "${CMAKE_COMMAND}" -E make_directory "obj/shaders")
    foreach(shader "ui_vs|vs_6_0|vsMain|UI_VS_SPV" "ui_ps|ps_6_0|psMain|UI_PS_SPV"
        "fullscreen_vs|vs_6_0|vsFullscreen|FULLSCREEN_VS_SPV"
        "blur_down_ps|ps_6_0|psBlurDown|BLUR_DOWN_PS_SPV" "blur_up_ps|ps_6_0|psBlurUp|BLUR_UP_PS_SPV")
        string(REPLACE "|" ";" parts "${shader}")
        list(GET parts 0 name)
        list(GET parts 1 profile)
        list(GET parts 2 entry)
        list(GET parts 3 symbol)
        set(invert "")
        if(profile STREQUAL "vs_6_0")
            set(invert -fvk-invert-y)
        endif()
        list(APPEND vk_commands
            COMMAND "${GRAN_DXC}" -spirv -fspv-target-env=vulkan1.0 -DGRAN_VULKAN=1 ${invert}
                -T ${profile} -E ${entry} -O3 -Fo "obj/shaders/${name}.spv"
                "${CMAKE_CURRENT_SOURCE_DIR}/shaders/hlsl/ui.hlsl"
            COMMAND gran_embed "obj/shaders/${name}.spv"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/rhi/shaders_gen/${name}_spv.cpp" ${symbol})
    endforeach()
    add_custom_target(gran_shaders_vulkan ${vk_commands} DEPENDS gran_embed
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" VERBATIM)
endif()
