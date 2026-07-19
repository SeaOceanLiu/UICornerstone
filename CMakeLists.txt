cmake_minimum_required(VERSION 3.16)
project(UICornerstone VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option: build UICornerstone_dll (shared library) for C ABI consumers.
# Tests always link the static UICornerstone target.
option(UICORNERSTONE_BUILD_DLL "Build UICornerstone DLL in addition to static lib" OFF)

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
message(STATUS "UICORNERSTONE_BUILD_DLL: ${UICORNERSTONE_BUILD_DLL}")

# ============================================================
# Core library source files
# ============================================================
set(CORE_SOURCES
    src/Actor.cpp
    src/BackendManager.cpp
    src/Bench.cpp
    src/Button.cpp
    src/CallbackAdapters.cpp
    src/CheckBox.cpp
    src/ColorPicker.cpp
    src/ComboBox.cpp
    src/ConstDef.cpp
    src/ControlBase.cpp
    src/Cursor.cpp
    src/DataContext.cpp
    src/Dialog.cpp
    src/WinFrame.cpp
    src/EditBox.cpp
    src/EventQueue.cpp
    src/FocusManager.cpp
    src/GraphTool.cpp
    src/HandleControl.cpp
    src/HotReloader.cpp
    src/Label.cpp
    src/MainWindow.cpp
    src/LayoutEngine.cpp
    src/LayoutParser.cpp
    src/Material.cpp
    src/Panel.cpp
    src/Menu.cpp
    src/NumericUpDown.cpp
    src/ProgressBar.cpp
    src/ResourceProvider.cpp
    src/ScrollBar.cpp
    src/Slider.cpp
    src/Surface.cpp
    src/TextArea.cpp
    src/Theme.cpp
    src/UICornerstoneAPI.cpp
)

# ============================================================
# Backend-specific sources and dependencies
# ============================================================
set(BACKEND_SOURCES
    src/backend/${UICORNERSTONE_BACKEND}/BackendPlugin.cpp
    src/backend/${UICORNERSTONE_BACKEND}/RenderDevice.cpp
    src/backend/${UICORNERSTONE_BACKEND}/TextRenderer.cpp
    src/backend/${UICORNERSTONE_BACKEND}/Window.cpp
    src/backend/${UICORNERSTONE_BACKEND}/InputBackend.cpp
    src/backend/${UICORNERSTONE_BACKEND}/Cursor.cpp
)

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

    set(BACKEND_INCLUDE_DIRS
        ${SDL3_INCLUDE_DIR}
        ${SDL3_TTF_INCLUDE_DIR}
        ${SDL3_IMAGE_INCLUDE_DIR}/include
    )

    set(BACKEND_LIBS
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

    set(BACKEND_INCLUDE_DIRS ${SFML_INCLUDE_DIR})

    set(BACKEND_LIBS
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

    set(BACKEND_INCLUDE_DIRS ${RAYLIB_INCLUDE_DIR})

    set(BACKEND_LIBS
        ${RAYLIB_LIB_DIR}/raylib.lib
        winmm.lib
    )

    list(APPEND BACKEND_DLLS "${RAYLIB_LIB_DIR}/raylib.dll")
endif()

string(TOUPPER "${UICORNERSTONE_BACKEND}" _BACKEND_UPPER_DEFINE)

# ============================================================
# UICornerstone — static library (for tests and internal use)
# ============================================================
add_library(UICornerstone STATIC ${CORE_SOURCES} ${BACKEND_SOURCES})

target_include_directories(UICornerstone PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/subModules
    ${JSON_INCLUDE_DIR}/include
    ${BACKEND_INCLUDE_DIRS}
)

target_link_libraries(UICornerstone PUBLIC ${BACKEND_LIBS})

target_compile_definitions(UICornerstone PUBLIC
    UICORNERSTONE_BACKEND_${_BACKEND_UPPER_DEFINE}=1
)

if(WIN32)
    target_link_libraries(UICornerstone PRIVATE user32 kernel32)
endif()

# ============================================================
# UICornerstone_dll — shared library (optional, for C ABI consumers)
# Outputs UICornerstone.dll (not UICornerstone_dll.dll).
# Contains only core sources — backend is a separate plugin DLL.
# ============================================================
if(UICORNERSTONE_BUILD_DLL)
    add_library(UICornerstone_dll SHARED ${CORE_SOURCES})

    set_target_properties(UICornerstone_dll PROPERTIES
        OUTPUT_NAME "UICornerstone"
        ARCHIVE_OUTPUT_NAME "UICornerstone_dll"
    )

    target_include_directories(UICornerstone_dll PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/subModules
        ${JSON_INCLUDE_DIR}/include
    )

    target_compile_definitions(UICornerstone_dll
        PRIVATE UICORNERSTONE_API_EXPORT=1
        PRIVATE UICORNERSTONE_CORE_API_EXPORT=1
        PUBLIC UICORNERSTONE_BUILD_SHARED=1
        PUBLIC UICORNERSTONE_BACKEND_${_BACKEND_UPPER_DEFINE}=1
    )

    target_link_libraries(UICornerstone_dll PRIVATE)

    if(WIN32)
        target_link_libraries(UICornerstone_dll PRIVATE user32 kernel32)
    endif()

    message(STATUS "UICornerstone_dll target created → UICornerstone.dll")
endif()

# ============================================================
# UIBackend_xxx — backend plugin DLL (optional)
# Separate shared library that exports GetUIBackendCallbacks.
# Dynamically loaded by UICornerstone_InitFromPlugin.
# ============================================================
if(UICORNERSTONE_BUILD_DLL)
    string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)

    set(UIBACKEND_TARGET "UIBackend_${_BACKEND_LOWER}")

    add_library(${UIBACKEND_TARGET} SHARED ${BACKEND_SOURCES})

    target_include_directories(${UIBACKEND_TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/subModules
        ${JSON_INCLUDE_DIR}/include
        ${BACKEND_INCLUDE_DIRS}
    )

    target_compile_definitions(${UIBACKEND_TARGET}
        PRIVATE UICORNERSTONE_BACKEND_PLUGIN=1
        PRIVATE UICORNERSTONE_BACKEND_${_BACKEND_UPPER_DEFINE}=1
        PUBLIC UICORNERSTONE_BUILD_SHARED=1
    )

    target_link_libraries(${UIBACKEND_TARGET} PRIVATE UICornerstone_dll ${BACKEND_LIBS})

    if(WIN32)
        target_link_libraries(${UIBACKEND_TARGET} PRIVATE user32 kernel32)
    endif()

    if(WIN32 AND BACKEND_DLLS)
        foreach(dll ${BACKEND_DLLS})
            add_custom_command(TARGET ${UIBACKEND_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll}"
                    $<TARGET_FILE_DIR:${UIBACKEND_TARGET}>)
        endforeach()
    endif()

    message(STATUS "${UIBACKEND_TARGET} target created → ${UIBACKEND_TARGET}.dll")
endif()

# ============================================================
# POST_BUILD: copy backend DLLs for static library output
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
add_subdirectory(samples)
