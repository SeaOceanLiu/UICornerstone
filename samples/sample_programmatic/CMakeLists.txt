string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)

set(_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/build/sample/sample_programmatic/${_BACKEND_LOWER}")

add_executable(sample_programmatic sample_programmatic.c)
target_link_libraries(sample_programmatic PRIVATE UICornerstone)
target_compile_definitions(sample_programmatic PRIVATE UICORNERSTONE_BACKEND_NAME="${UICORNERSTONE_BACKEND}")

set_target_properties(sample_programmatic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_SAMPLE_DIR}/$<CONFIG>"
)

if(WIN32 AND BACKEND_DLLS)
    file(MAKE_DIRECTORY "${_SAMPLE_DIR}/Debug")
    file(MAKE_DIRECTORY "${_SAMPLE_DIR}/Release")

    get_filename_component(ASSETS_DIR "${CMAKE_SOURCE_DIR}/subModules/assets" ABSOLUTE)

    set(_COPY_CMDS)
    foreach(dll ${BACKEND_DLLS})
        get_filename_component(_DLL_NAME "${dll}" NAME)
        list(APPEND _COPY_CMDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "${_SAMPLE_DIR}/$<CONFIG>/${_DLL_NAME}"
        )
    endforeach()
    list(APPEND _COPY_CMDS
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "${_SAMPLE_DIR}/$<CONFIG>/assets"
    )

    add_custom_command(TARGET sample_programmatic POST_BUILD ${_COPY_CMDS})
endif()
