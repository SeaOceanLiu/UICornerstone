string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)

# Output to build/sample/<sample>/<backend>/<config>/
set(_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/build/sample/hello_uicornerstone/${_BACKEND_LOWER}")

add_executable(hello_uicornerstone hello_uicornerstone.c)
target_link_libraries(hello_uicornerstone PRIVATE UICornerstone)
target_compile_definitions(hello_uicornerstone PRIVATE UICORNERSTONE_BACKEND_NAME="${UICORNERSTONE_BACKEND}")

set_target_properties(hello_uicornerstone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_SAMPLE_DIR}/$<CONFIG>"
)

# Pre-create output directories and copy third-party DLLs + assets
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

    add_custom_command(TARGET hello_uicornerstone POST_BUILD ${_COPY_CMDS})
endif()
