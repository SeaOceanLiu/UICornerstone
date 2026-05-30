cmake_minimum_required(VERSION 3.16)
project(UIControls VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/libs" SDL3_LIB_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3" SDL3_INCLUDE_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3_ttf" SDL3_TTF_INCLUDE_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/SDL3_image" SDL3_IMAGE_INCLUDE_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/json" JSON_INCLUDE_DIR)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/subModules/DebugTrace" DEBUG_TRACE_DIR)

message(STATUS "SDL3 Include Dir: ${SDL3_INCLUDE_DIR}")
message(STATUS "SDL3 TTF Include Dir: ${SDL3_TTF_INCLUDE_DIR}")
message(STATUS "SDL3 Image Include Dir: ${SDL3_IMAGE_INCLUDE_DIR}")
message(STATUS "JSON Include Dir: ${JSON_INCLUDE_DIR}")
message(STATUS "Debug Trace Dir: ${DEBUG_TRACE_DIR}")

add_library(UIControls STATIC
    src/Actor.cpp
    src/Bench.cpp
    src/Button.cpp
    src/CheckBox.cpp
    src/ConstDef.cpp
    src/ControlBase.cpp
    src/Dialog.cpp
    src/EditBox.cpp
    src/EventQueue.cpp
    src/GraphTool.cpp
    src/Label.cpp
    src/LayoutEngine.cpp
    src/LayoutParser.cpp
    src/Material.cpp
    src/Panel.cpp
    src/ResourceLoader.cpp
    src/Menu.cpp
    src/ProgressBar.cpp
    src/ScrollBar.cpp
    src/TextArea.cpp
    src/Theme.cpp
    ${DEBUG_TRACE_DIR}/DebugTrace.cpp
)

target_include_directories(UIControls PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/subModules
    ${SDL3_INCLUDE_DIR}
    ${SDL3_TTF_INCLUDE_DIR}
    ${SDL3_IMAGE_INCLUDE_DIR}/include
    ${JSON_INCLUDE_DIR}/include
    ${DEBUG_TRACE_DIR}
)

target_link_libraries(UIControls PUBLIC
    ${SDL3_LIB_DIR}/SDL3.lib
    ${SDL3_LIB_DIR}/SDL3_ttf.lib
    ${SDL3_LIB_DIR}/SDL3_image.lib
)

if(WIN32)
    target_link_libraries(UIControls PRIVATE user32 kernel32)
    
    add_custom_command(TARGET UIControls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_LIB_DIR}/SDL3.dll"
            $<TARGET_FILE_DIR:UIControls>)
    add_custom_command(TARGET UIControls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_LIB_DIR}/SDL3_ttf.dll"
            $<TARGET_FILE_DIR:UIControls>)
    add_custom_command(TARGET UIControls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_LIB_DIR}/SDL3_image.dll"
            $<TARGET_FILE_DIR:UIControls>)
    add_custom_command(TARGET UIControls POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DEBUG_TRACE_DIR}/DebugInfoX64.dll"
            $<TARGET_FILE_DIR:UIControls>)
endif()

add_subdirectory(test)