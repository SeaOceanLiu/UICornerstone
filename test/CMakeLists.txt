cmake_minimum_required(VERSION 3.16)
project(UICornerstoneTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/json" ABSOLUTE)
get_filename_component(ASSETS_DIR "${CMAKE_SOURCE_DIR}/subModules/assets" ABSOLUTE)

# Retrieve the backend choice from the parent project
if(NOT UICORNERSTONE_BACKEND)
    set(UICORNERSTONE_BACKEND "SDL3")
endif()

string(TOUPPER "${UICORNERSTONE_BACKEND}" _BACKEND_UPPER)

# ============================================================
# Backend-specific test configuration
# ============================================================
 set(TEST_LIBS UICornerstone)
set(TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/subModules
    ${JSON_INCLUDE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(TEST_DLLS)

if(_BACKEND_UPPER STREQUAL "SDL3")
    get_filename_component(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/libs" ABSOLUTE)
    get_filename_component(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3" ABSOLUTE)
    get_filename_component(SDL3_TTF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3_ttf" ABSOLUTE)
    get_filename_component(SDL3_IMAGE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3_image" ABSOLUTE)

    list(APPEND TEST_INCLUDES
        ${SDL3_INCLUDE_DIR}
        ${SDL3_TTF_INCLUDE_DIR}
        ${SDL3_IMAGE_INCLUDE_DIR}/include
    )

    list(APPEND TEST_LIBS
        ${SDL3_LIB_DIR}/SDL3.lib
        ${SDL3_LIB_DIR}/SDL3_ttf.lib
        ${SDL3_LIB_DIR}/SDL3_image.lib
    )

    list(APPEND TEST_DLLS
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${SDL3_LIB_DIR}/SDL3_ttf.dll"
        "${SDL3_LIB_DIR}/SDL3_image.dll"
    )

elseif(_BACKEND_UPPER STREQUAL "SFML")
    get_filename_component(SFML_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/SFML/lib" ABSOLUTE)
    get_filename_component(SFML_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SFML/include" ABSOLUTE)
    get_filename_component(SFML_BIN_DIR "${CMAKE_SOURCE_DIR}/subModules/SFML/bin" ABSOLUTE)

    list(APPEND TEST_INCLUDES ${SFML_INCLUDE_DIR})

    list(APPEND TEST_LIBS
        debug ${SFML_LIB_DIR}/sfml-graphics-d.lib
        optimized ${SFML_LIB_DIR}/sfml-graphics.lib
        debug ${SFML_LIB_DIR}/sfml-window-d.lib
        optimized ${SFML_LIB_DIR}/sfml-window.lib
        debug ${SFML_LIB_DIR}/sfml-system-d.lib
        optimized ${SFML_LIB_DIR}/sfml-system.lib
        opengl32.lib
    )

    list(APPEND TEST_DLLS
        "${SFML_BIN_DIR}/sfml-graphics-3.dll"
        "${SFML_BIN_DIR}/sfml-graphics-d-3.dll"
        "${SFML_BIN_DIR}/sfml-window-3.dll"
        "${SFML_BIN_DIR}/sfml-window-d-3.dll"
        "${SFML_BIN_DIR}/sfml-system-3.dll"
        "${SFML_BIN_DIR}/sfml-system-d-3.dll"
    )

elseif(_BACKEND_UPPER STREQUAL "RAYLIB")
    get_filename_component(RAYLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/raylib/include" ABSOLUTE)
    get_filename_component(RAYLIB_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/raylib/lib" ABSOLUTE)

    list(APPEND TEST_INCLUDES ${RAYLIB_INCLUDE_DIR})

    list(APPEND TEST_LIBS ${RAYLIB_LIB_DIR}/raylib.lib)

    list(APPEND TEST_DLLS "${RAYLIB_LIB_DIR}/raylib.dll")
endif()

# ============================================================
# Test executables
# ============================================================
set(UI_TEST_EXECUTABLES
    test_menu
    test_label
    test_editbox
    test_checkbox
    test_progressbar
    test_button
    test_graphtool
    test_layout
    test_layout_advanced
    test_winframe
)

foreach(test_name ${UI_TEST_EXECUTABLES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp")
        add_executable(${test_name}
            ${test_name}.cpp
        )

        target_include_directories(${test_name} PRIVATE ${TEST_INCLUDES})
        target_link_libraries(${test_name} PRIVATE ${TEST_LIBS})

        if(WIN32)
            foreach(dll ${TEST_DLLS})
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}"
                        $<TARGET_FILE_DIR:${test_name}>)
            endforeach()

            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ASSETS_DIR}"
                    $<TARGET_FILE_DIR:${test_name}>/assets)
            if(${test_name} STREQUAL "test_layout" OR ${test_name} STREQUAL "test_layout_advanced")
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_SOURCE_DIR}/layouts"
                        $<TARGET_FILE_DIR:${test_name}>/layouts)
            endif()
        endif()
    endif()
endforeach()
