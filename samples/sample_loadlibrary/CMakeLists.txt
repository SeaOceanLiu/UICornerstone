# Only build in DLL mode (needs UICornerstone.dll)
if(NOT UICORNERSTONE_BUILD_DLL)
    return()
endif()

string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
set(_SAMPLE_DIR "${CMAKE_SOURCE_DIR}/build/sample/sample_loadlibrary/${_BACKEND_LOWER}")

add_executable(sample_loadlibrary
    sample_loadlibrary.cpp
)

target_include_directories(sample_loadlibrary PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/subModules
    ${JSON_INCLUDE_DIR}/include
    ${BACKEND_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/src/backend"
)

# 注意：不定义 UICORNERSTONE_BUILD_SHARED，CORE_API 为空宏。
# 这样 Surface::registerFactories 等声明不产生 dllimport，
# 依赖由本文件的 stub 满足，无需链接 UICornerstone_dll.lib。
target_compile_definitions(sample_loadlibrary PRIVATE
    UICORNERSTONE_BACKEND_PLUGIN=0
)

# 不链接 UICornerstone_dll：C ABI 走 GetProcAddress，
# 后端源码中的 CORE 符号由本文件的 stub 提供。
target_link_libraries(sample_loadlibrary PRIVATE
    ${BACKEND_LIBS}
)

set_target_properties(sample_loadlibrary PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_SAMPLE_DIR}/$<CONFIG>"
)

# POST_BUILD: copy UICornerstone.dll, third-party DLLs, and assets
if(WIN32)
    file(MAKE_DIRECTORY "${_SAMPLE_DIR}/Debug")

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

    add_custom_command(TARGET sample_loadlibrary POST_BUILD ${_COPY_CMDS})
endif()
