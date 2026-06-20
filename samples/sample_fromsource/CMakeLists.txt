# Only build in DLL mode (needs UICornerstone.dll)
if(NOT UICORNERSTONE_BUILD_DLL)
    return()
endif()

string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
set(_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/build/sample/sample_fromsource/${_BACKEND_LOWER}")

# Backend source files compiled into the exe (not in a plugin DLL)
set(_BACKEND_SOURCES
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/Window.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/RenderDevice.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/TextRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/InputBackend.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/Cursor.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/BackendPlugin.cpp"
)

add_executable(sample_fromsource
    sample_fromsource.c
    ${_BACKEND_SOURCES}
)

target_include_directories(sample_fromsource PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/subModules
    ${JSON_INCLUDE_DIR}/include
    ${BACKEND_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/src/backend"
)

target_compile_definitions(sample_fromsource PRIVATE
    UICORNERSTONE_BUILD_SHARED=1
    UICORNERSTONE_BACKEND_PLUGIN=0
)

target_link_libraries(sample_fromsource PRIVATE
    UICornerstone_dll
    ${BACKEND_LIBS}
)

set_target_properties(sample_fromsource PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_SAMPLE_DIR}/$<CONFIG>"
)

# POST_BUILD: copy UICornerstone.dll, third-party DLLs, and assets
if(WIN32)
    file(MAKE_DIRECTORY "${_SAMPLE_DIR}/Debug")
    file(MAKE_DIRECTORY "${_SAMPLE_DIR}/Release")

    get_filename_component(ASSETS_DIR "${CMAKE_SOURCE_DIR}/subModules/assets" ABSOLUTE)

    set(_COPY_CMDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
            "${_SAMPLE_DIR}/$<CONFIG>/UICornerstone.dll"
    )
    foreach(dll ${BACKEND_DLLS})
        get_filename_component(_DLL_NAME "${dll}" NAME)
        list(APPEND _COPY_CMDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "${_SAMPLE_DIR}/$<CONFIG>/${_DLL_NAME}"
        )
    endforeach()
    list(APPEND _COPY_CMDS
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "${_SAMPLE_DIR}/$<CONFIG>/assets"
    )

    add_custom_command(TARGET sample_fromsource POST_BUILD ${_COPY_CMDS})
endif()
