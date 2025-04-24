# cmake/GenerateMSDFFont.cmake

function(generate_msdf_font FONT_PATH OUTPUT_BASENAME)
    set(IMAGE_OUT "${CMAKE_BINARY_DIR}/assets/fonts/${OUTPUT_BASENAME}.png")
    set(JSON_OUT "${CMAKE_BINARY_DIR}/assets/fonts/${OUTPUT_BASENAME}.json")

    add_custom_command(
        OUTPUT ${IMAGE_OUT} ${JSON_OUT}
        COMMAND msdf-atlas-gen
        -font "${FONT_PATH}"
        -type msdf
        -size 48
        -imageout "${IMAGE_OUT}"
        -json "${JSON_OUT}"
        DEPENDS "${FONT_PATH}"
        COMMENT "Generating MSDF atlas for ${OUTPUT_BASENAME}"
    )

    add_custom_target(generate_font_${OUTPUT_BASENAME} ALL
        DEPENDS ${IMAGE_OUT} ${JSON_OUT}
    )

    # Optionally register the outputs as variables to use elsewhere
    set(${OUTPUT_BASENAME}_ATLAS ${IMAGE_OUT} PARENT_SCOPE)
    set(${OUTPUT_BASENAME}_METADATA ${JSON_OUT} PARENT_SCOPE)
endfunction()