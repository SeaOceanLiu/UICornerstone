cmake_minimum_required(VERSION 3.16)
project(UICornerstone VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UICORNERSTONE_BACKEND "SDL3" CACHE STRING "Backend library (SDL3|SFML|RAYLIB)")
set_property(CACHE UICORNERSTONE_BACKEND PROPERTY STRINGS SDL3 SFML RAYLIB)

if(WIN32)
    add_definitions(-D_HAS_STD_BYTE=0)
    add_compile_options(/FS)
endif()

# ============================================================
# Common dependency paths
# ============================================================
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/json" JSON_INCLUDE_DIR)

message(STATUS "UICORNERSTONE_BACKEND: ${UICORNERSTONE_BACKEND}")
message(STATUS "JSON Include Dir: ${JSON_INCLUDE_DIR}")

# ============================================================
# Library target (must be before target_* calls)
# ============================================================
add_library(UICornerstone STATIC
    src/Actor.cpp
    src/BackendManager.cpp
    src/Bench.cpp
    src/Button.cpp
    src/CallbackAdapters.cpp
    src/CheckBox.cpp
    src/ConstDef.cpp
    src/ControlBase.cpp
    src/DataContext.cpp
    src/WinFrame.cpp
    src/EditBox.cpp
    src/EventQueue.cpp
    src/GraphTool.cpp
    src/HotReloader.cpp
    src/Label.cpp
    src/MainWindow.cpp
    src/LayoutEngine.cpp
    src/LayoutParser.cpp
    src/Material.cpp
    src/Panel.cpp
    src/Menu.cpp
    src/ProgressBar.cpp
    src/ResourceProvider.cpp
    src/ScrollBar.cpp
    src/TextArea.cpp
    src/Theme.cpp
    src/UICornerstoneAPI.cpp
)

target_include_directories(UICornerstone PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/subModules
    ${JSON_INCLUDE_DIR}/include
)

# ============================================================
# Backend-specific configuration
# ============================================================
set(BACKEND_SOURCES
    src/backend/${UICORNERSTONE_BACKEND}/BackendPlugin.cpp
    src/backend/${UICORNERSTONE_BACKEND}/RenderDevice.cpp
    src/backend/${UICORNERSTONE_BACKEND}/TextRenderer.cpp
    src/backend/${UICORNERSTONE_BACKEND}/Window.cpp
    src/backend/${UICORNERSTONE_BACKEND}/InputBackend.cpp
    src/backend/${UICORNERSTONE_BACKEND}/Cursor.cpp
)

target_sources(UICornerstone PRIVATE ${BACKEND_SOURCES})

set(BACKEND_DLLS)

string(TOUPPER "${UICORNERSTONE_BACKEND}" _BACKEND_UPPER)
if(_BACKEND_UPPER STREQUAL "SDL3")
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/libs" SDL3_LIB_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3" SDL3_INCLUDE_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3_ttf" SDL3_TTF_INCLUDE_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3_image" SDL3_IMAGE_INCLUDE_DIR)

    message(STATUS "SDL3 Include Dir: ${SDL3_INCLUDE_DIR}")
    message(STATUS "SDL3 TTF Include Dir: ${SDL3_TTF_INCLUDE_DIR}")
    message(STATUS "SDL3 Image Include Dir: ${SDL3_IMAGE_INCLUDE_DIR}")

    target_include_directories(UICornerstone PUBLIC
        ${SDL3_INCLUDE_DIR}
        ${SDL3_TTF_INCLUDE_DIR}
        ${SDL3_IMAGE_INCLUDE_DIR}/include
    )

    target_link_libraries(UICornerstone PUBLIC
        ${SDL3_LIB_DIR}/SDL3.lib
        ${SDL3_LIB_DIR}/SDL3_ttf.lib
        ${SDL3_LIB_DIR}/SDL3_image.lib
    )

    list(APPEND BACKEND_DLLS
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${SDL3_LIB_DIR}/SDL3_ttf.dll"
        "${SDL3_LIB_DIR}/SDL3_image.dll"
    )

elseif(_BACKEND_UPPER STREQUAL "SFML")
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SFML/lib" SFML_LIB_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SFML/include" SFML_INCLUDE_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SFML/bin" SFML_BIN_DIR)

    message(STATUS "SFML Include Dir: ${SFML_INCLUDE_DIR}")
    message(STATUS "SFML Lib Dir: ${SFML_LIB_DIR}")
    message(STATUS "SFML Bin Dir: ${SFML_BIN_DIR}")

    target_include_directories(UICornerstone PUBLIC
        ${SFML_INCLUDE_DIR}
    )

    target_link_libraries(UICornerstone PUBLIC
        debug ${SFML_LIB_DIR}/sfml-graphics-d.lib
        optimized ${SFML_LIB_DIR}/sfml-graphics.lib
        debug ${SFML_LIB_DIR}/sfml-window-d.lib
        optimized ${SFML_LIB_DIR}/sfml-window.lib
        debug ${SFML_LIB_DIR}/sfml-system-d.lib
        optimized ${SFML_LIB_DIR}/sfml-system.lib
        opengl32.lib
    )

    list(APPEND BACKEND_DLLS
        "${SFML_BIN_DIR}/sfml-graphics-3.dll"
        "${SFML_BIN_DIR}/sfml-graphics-d-3.dll"
        "${SFML_BIN_DIR}/sfml-window-3.dll"
        "${SFML_BIN_DIR}/sfml-window-d-3.dll"
        "${SFML_BIN_DIR}/sfml-system-3.dll"
        "${SFML_BIN_DIR}/sfml-system-d-3.dll"
    )

elseif(_BACKEND_UPPER STREQUAL "RAYLIB")
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/raylib/include" RAYLIB_INCLUDE_DIR)
    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/raylib/lib" RAYLIB_LIB_DIR)

    message(STATUS "raylib Include Dir: ${RAYLIB_INCLUDE_DIR}")
    message(STATUS "raylib Lib Dir: ${RAYLIB_LIB_DIR}")

    target_include_directories(UICornerstone PUBLIC
        ${RAYLIB_INCLUDE_DIR}
    )

    target_link_libraries(UICornerstone PUBLIC
        ${RAYLIB_LIB_DIR}/raylib.lib
        winmm.lib
    )

    list(APPEND BACKEND_DLLS "${RAYLIB_LIB_DIR}/raylib.dll")
endif()

string(TOUPPER "${UICORNERSTONE_BACKEND}" _BACKEND_UPPER_DEFINE)
target_compile_definitions(UICornerstone PUBLIC
    UICORNERSTONE_BACKEND_${_BACKEND_UPPER_DEFINE}=1
)

if(WIN32)
    target_link_libraries(UICornerstone PRIVATE user32 kernel32)
endif()

# ============================================================
# POST_BUILD: copy DLLs
# ============================================================
if(WIN32 AND BACKEND_DLLS)
    foreach(dll ${BACKEND_DLLS})
        add_custom_command(TARGET UICornerstone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                $<TARGET_FILE_DIR:UICornerstone>)
    endforeach()

endif()

add_subdirectory(test)