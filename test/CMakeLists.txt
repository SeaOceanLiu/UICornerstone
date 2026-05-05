cmake_minimum_required(VERSION 3.16)
project(UIControlsTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/libs" ABSOLUTE)
get_filename_component(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3" ABSOLUTE)
get_filename_component(SDL3_TTF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3_ttf" ABSOLUTE)
get_filename_component(SDL3_IMAGE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/SDL3_image" ABSOLUTE)
get_filename_component(JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/subModules/json" ABSOLUTE)
get_filename_component(DEBUG_TRACE_DIR "${CMAKE_SOURCE_DIR}/subModules/DebugTrace" ABSOLUTE)
get_filename_component(ASSETS_DIR "${CMAKE_SOURCE_DIR}/subModules/assets" ABSOLUTE)

set(UI_TEST_EXECUTABLES
    test_menu
    test_label
    test_editbox
    test_checkbox
    test_progressbar
    test_button
)

foreach(test_name ${UI_TEST_EXECUTABLES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp")
        add_executable(${test_name}
            ${test_name}.cpp
        )

        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/subModules
            ${SDL3_INCLUDE_DIR}
            ${SDL3_TTF_INCLUDE_DIR}
            ${SDL3_IMAGE_INCLUDE_DIR}/include
            ${JSON_INCLUDE_DIR}/include
            ${DEBUG_TRACE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )

        target_link_libraries(${test_name} PRIVATE
            UIControls
            ${SDL3_LIB_DIR}/SDL3.lib
            ${SDL3_LIB_DIR}/SDL3_ttf.lib
            ${SDL3_LIB_DIR}/SDL3_image.lib
        )

        if(WIN32)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL3_LIB_DIR}/SDL3.dll"
                    $<TARGET_FILE_DIR:${test_name}>)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL3_LIB_DIR}/SDL3_ttf.dll"
                    $<TARGET_FILE_DIR:${test_name}>)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL3_LIB_DIR}/SDL3_image.dll"
                    $<TARGET_FILE_DIR:${test_name}>)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEBUG_TRACE_DIR}/DebugInfoX64.dll"
                    $<TARGET_FILE_DIR:${test_name}>)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ASSETS_DIR}"
                    $<TARGET_FILE_DIR:${test_name}>/assets)
        endif()
    endif()
endforeach()