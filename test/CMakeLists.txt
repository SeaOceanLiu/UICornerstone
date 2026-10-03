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
string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)

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
    test_colorpicker
    test_colorfixes
    test_combobox
    test_dialog
    test_graphtool
    test_handlecontrol
    test_slider
    test_shape
    test_listview
    test_statusbar
    test_contextmenu
    test_tabcontrol
    test_numericupdown
    test_splitter
    test_treeview
    test_layout
    test_layout_advanced
    test_winframe
    test_multi_instance
    test_multiviewport
    test_viewport_scale
    test_window
    test_luotiani
    test_animation
    test_aniviewer
)

foreach(test_name ${UI_TEST_EXECUTABLES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp")
        add_executable(${test_name}
            ${test_name}.cpp
        )

        target_include_directories(${test_name} PRIVATE ${TEST_INCLUDES})
        target_link_libraries(${test_name} PRIVATE ${TEST_LIBS})

        if(${test_name} STREQUAL "test_aniviewer" OR ${test_name} STREQUAL "test_window")
            # 命令行视觉校验工具：CreateInstanceFromPlugin 需要后端名
            string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
            target_compile_definitions(${test_name} PRIVATE
                UICORNERSTONE_BACKEND_NAME="${_BACKEND_LOWER}")
        endif()

        if(WIN32)
            foreach(dll ${TEST_DLLS})
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}"
                        $<TARGET_FILE_DIR:${test_name}>)
            endforeach()

            if(UICORNERSTONE_BUILD_DLL)
                # DLL 模式下标准测试运行需要核心 DLL 与后端插件 DLL：
                # 与 test_api 一致，POST_BUILD 同步最新构建产物，
                # 避免 test/Debug 下 DLL 陈旧导致版本错配（如 view/resize 行为差异）。
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                        "$<TARGET_FILE_DIR:${test_name}>")
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll"
                        "$<TARGET_FILE_DIR:${test_name}>")

                # 标准测试静态链接 UICornerstone，不依赖 UICornerstone_dll 导入库；
                # 若不强制顺序，MSBuild 并行下 POST_BUILD 可能在核心 DLL 构建完成前
                # 执行（copy_if_different 读到旧文件视为相同而跳过），导致 test/Debug
                # 长期同步不到最新库代码。add_dependencies 只约束构建顺序，不增加链接。
                add_dependencies(${test_name} UICornerstone_dll UIBackend_${_BACKEND_LOWER})
            endif()

            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ASSETS_DIR}"
                    $<TARGET_FILE_DIR:${test_name}>/assets)
            # 测试专用资源（不入 subModules 子模块）：合并进测试输出 assets/
            if(EXISTS "${CMAKE_SOURCE_DIR}/test/assets")
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_SOURCE_DIR}/test/assets"
                        $<TARGET_FILE_DIR:${test_name}>/assets)
            endif()
            if(${test_name} STREQUAL "test_layout" OR ${test_name} STREQUAL "test_layout_advanced")
                add_custom_command(TARGET ${test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_SOURCE_DIR}/layouts"
                    $<TARGET_FILE_DIR:${test_name}>/layouts)
            endif()
        endif()
    endif()
endforeach()

# ============================================================
# Scale visual tests (C ABI 集成测试) — 仅在 DLL 模式构建树编译，
# 输出遵循规范：build/{backend}_dll/test/Debug/（见 design/guidelines/build.md）
# ============================================================
foreach(scale_test IN ITEMS test_scale_cabi test_scale_json)
    if(UICORNERSTONE_BUILD_DLL AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${scale_test}.cpp")
        add_executable(${scale_test} ${scale_test}.cpp)
        target_include_directories(${scale_test} PRIVATE ${TEST_INCLUDES})
        target_link_libraries(${scale_test} PRIVATE ${TEST_LIBS})

        if(WIN32)
            foreach(dll ${TEST_DLLS})
                add_custom_command(TARGET ${scale_test} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}"
                        $<TARGET_FILE_DIR:${scale_test}>)
            endforeach()

            # 与标准测试同理：静态链接核心库，仅保证 POST_BUILD 同步到最新 DLL
            add_dependencies(${scale_test} UICornerstone_dll UIBackend_${_BACKEND_LOWER})
            add_custom_command(TARGET ${scale_test} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                    "$<TARGET_FILE_DIR:${scale_test}>")
            add_custom_command(TARGET ${scale_test} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll"
                    "$<TARGET_FILE_DIR:${scale_test}>")

            add_custom_command(TARGET ${scale_test} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ASSETS_DIR}"
                    $<TARGET_FILE_DIR:${scale_test}>/assets)
        endif()
    endif()
endforeach()

# ============================================================
# C ABI test (pure C, only uses UICornerstoneAPI.h)
# When DLL mode is on, test_api links the shared library for real
# DLL loading. Otherwise it stays static (code linked into exe).
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_api.c")
    add_executable(test_api test_api.c)
    target_include_directories(test_api PRIVATE ${TEST_INCLUDES})
    string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
    target_compile_definitions(test_api PRIVATE UICORNERSTONE_BACKEND_NAME="${_BACKEND_LOWER}")

    if(UICORNERSTONE_BUILD_DLL)
        target_link_libraries(test_api PRIVATE UICornerstone_dll)
        target_compile_definitions(test_api PRIVATE UICORNERSTONE_BUILD_SHARED=1)

        if(WIN32)
            # Copy UICornerstone.dll from the core DLL output dir
            add_custom_command(TARGET test_api POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                    "$<TARGET_FILE_DIR:test_api>")

            # Copy the backend plugin DLL (e.g. UIBackend_sdl3.dll)
            add_custom_command(TARGET test_api POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll"
                    "$<TARGET_FILE_DIR:test_api>")
        endif()
    else()
        target_link_libraries(test_api PRIVATE ${TEST_LIBS})
    endif()
endif()

# ============================================================
# test_p0_getter (pure C): P0 配合项验证（SetControlId / getBoolProperty /
# font-size 转发）。链接与 test_api 完全一致。
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_p0_getter.c")
    add_executable(test_p0_getter test_p0_getter.c)
    target_include_directories(test_p0_getter PRIVATE ${TEST_INCLUDES})
    string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
    target_compile_definitions(test_p0_getter PRIVATE UICORNERSTONE_BACKEND_NAME="${_BACKEND_LOWER}")

    if(UICORNERSTONE_BUILD_DLL)
        target_link_libraries(test_p0_getter PRIVATE UICornerstone_dll)
        target_compile_definitions(test_p0_getter PRIVATE UICORNERSTONE_BUILD_SHARED=1)

        if(WIN32)
            add_custom_command(TARGET test_p0_getter POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                    "$<TARGET_FILE_DIR:test_p0_getter>")
            add_custom_command(TARGET test_p0_getter POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll"
                    "$<TARGET_FILE_DIR:test_p0_getter>")
        endif()
    else()
        target_link_libraries(test_p0_getter PRIVATE ${TEST_LIBS})
    endif()
endif()

# ============================================================
# Multi-instance / multiviewport C ABI tests (pure DLL plugin loading)
# Dynamic loading: LoadLibrary("UICornerstone.dll") + GetProcAddress,
# create instances via CreateInstanceFromPlugin → core DLL loads
# UIBackend_xxx.dll. Unlike test_api (import-lib link), these do NOT
# link UICornerstone_dll — everything is resolved at runtime.
# ============================================================
foreach(cabi_test_name test_multi_instance_cabi test_multiviewport_cabi test_multiinstance_visual_cabi test_multiviewport_visual_cabi test_overlay_viewport_cabi test_image test_capture_cabi test_handlecontrol_cabi test_wheel_cabi test_wheel_nested_cabi)
    if(UICORNERSTONE_BUILD_DLL AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${cabi_test_name}.cpp")
        add_executable(${cabi_test_name} ${cabi_test_name}.cpp)

        target_include_directories(${cabi_test_name} PRIVATE ${TEST_INCLUDES})
        string(TOLOWER "${UICORNERSTONE_BACKEND}" _BACKEND_LOWER)
        target_compile_definitions(${cabi_test_name} PRIVATE
            UICORNERSTONE_BACKEND_NAME="${_BACKEND_LOWER}"
        )
        # 动态加载方式（LoadLibrary + GetProcAddress）：不链接 UICornerstone_dll 导入库

        if(WIN32)
            # Copy UICornerstone.dll from the core DLL output dir
            add_custom_command(TARGET ${cabi_test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                    "$<TARGET_FILE_DIR:${cabi_test_name}>")

            # Copy the backend plugin DLL (e.g. UIBackend_sdl3.dll)
            add_custom_command(TARGET ${cabi_test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UIBackend_${_BACKEND_LOWER}>/UIBackend_${_BACKEND_LOWER}.dll"
                    "$<TARGET_FILE_DIR:${cabi_test_name}>")

            foreach(dll ${TEST_DLLS})
                add_custom_command(TARGET ${cabi_test_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}"
                        "$<TARGET_FILE_DIR:${cabi_test_name}>")
            endforeach()

            # 与标准测试同理：动态加载不链接核心库，强制顺序避免 POST_BUILD 竞态
            add_dependencies(${cabi_test_name} UICornerstone_dll UIBackend_${_BACKEND_LOWER})
        endif()
    endif()
endforeach()

# ============================================================
# From-source build tests (DLL bridge + backend source compiled in)
# ============================================================
# Collect backend source files for the current backend
set(FROMSOURCE_BACKEND_SOURCES
    "${CMAKE_SOURCE_DIR}/src/RenderDevice.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/Window.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/RenderDevice.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/TextRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/InputBackend.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/Cursor.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/${_BACKEND_LOWER}/BackendPlugin.cpp"
)

# Backend-specific libraries for fromsource targets
# (TEST_LIBS includes UICornerstone itself; we need only the 3rd-party deps)
set(FROMSOURCE_BACKEND_LIBS)
if(_BACKEND_UPPER STREQUAL "SDL3")
    get_filename_component(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/libs" ABSOLUTE)
    set(FROMSOURCE_BACKEND_LIBS
        ${SDL3_LIB_DIR}/SDL3.lib
        ${SDL3_LIB_DIR}/SDL3_ttf.lib
        ${SDL3_LIB_DIR}/SDL3_image.lib
    )
elseif(_BACKEND_UPPER STREQUAL "SFML")
    get_filename_component(SFML_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/SFML/lib" ABSOLUTE)
    set(FROMSOURCE_BACKEND_LIBS
        debug ${SFML_LIB_DIR}/sfml-graphics-d.lib
        optimized ${SFML_LIB_DIR}/sfml-graphics.lib
        debug ${SFML_LIB_DIR}/sfml-window-d.lib
        optimized ${SFML_LIB_DIR}/sfml-window.lib
        debug ${SFML_LIB_DIR}/sfml-system-d.lib
        optimized ${SFML_LIB_DIR}/sfml-system.lib
        opengl32.lib
    )
elseif(_BACKEND_UPPER STREQUAL "RAYLIB")
    get_filename_component(RAYLIB_LIB_DIR "${CMAKE_SOURCE_DIR}/subModules/raylib/lib" ABSOLUTE)
    set(FROMSOURCE_BACKEND_LIBS ${RAYLIB_LIB_DIR}/raylib.lib winmm.lib)
endif()

macro(add_fromsource_target target_name source_file)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
        add_executable(${target_name} ${source_file} ${FROMSOURCE_BACKEND_SOURCES})

        target_include_directories(${target_name} PRIVATE
            ${TEST_INCLUDES}
            "${CMAKE_SOURCE_DIR}/src/backend"
        )

        target_compile_definitions(${target_name} PRIVATE UICORNERSTONE_BUILD_SHARED=1)
        target_compile_definitions(${target_name} PUBLIC UICORNERSTONE_BACKEND_PLUGIN=0)

        if(_BACKEND_UPPER STREQUAL "SDL3")
            target_compile_definitions(${target_name} PRIVATE
                BACKEND_SHORT_NAME="sdl3" BACKEND_DISPLAY_NAME="SDL3")
        elseif(_BACKEND_UPPER STREQUAL "SFML")
            target_compile_definitions(${target_name} PRIVATE
                BACKEND_SHORT_NAME="sfml" BACKEND_DISPLAY_NAME="SFML")
        elseif(_BACKEND_UPPER STREQUAL "RAYLIB")
            target_compile_definitions(${target_name} PRIVATE
                BACKEND_SHORT_NAME="raylib" BACKEND_DISPLAY_NAME="Raylib")
        endif()

        target_link_libraries(${target_name} PRIVATE
            UICornerstone_dll
            ${FROMSOURCE_BACKEND_LIBS}
        )

        if(WIN32)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE_DIR:UICornerstone_dll>/UICornerstone.dll"
                    "$<TARGET_FILE_DIR:${target_name}>")

            foreach(dll ${TEST_DLLS})
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}"
                        "$<TARGET_FILE_DIR:${target_name}>")
            endforeach()

            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ASSETS_DIR}"
                    "$<TARGET_FILE_DIR:${target_name}>/assets")
        endif()
    endif()
endmacro()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_fromsource_cabi test_fromsource_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_fromsource_cabi test_fromsource_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_fromsource_cabi test_fromsource_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_dialog_cabi test_dialog_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_dialog_cabi test_dialog_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_dialog_cabi test_dialog_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_combobox_cabi test_combobox_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_combobox_cabi test_combobox_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_combobox_cabi test_combobox_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_resourceprovider_cabi test_resourceprovider_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_resourceprovider_cabi test_resourceprovider_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_resourceprovider_cabi test_resourceprovider_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_property_cabi test_property_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_property_cabi test_property_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_property_cabi test_property_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_numericupdown_cabi test_numericupdown_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_numericupdown_cabi test_numericupdown_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_numericupdown_cabi test_numericupdown_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_splitter_cabi test_splitter_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_splitter_cabi test_splitter_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_splitter_cabi test_splitter_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_treeview_cabi test_treeview_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_treeview_cabi test_treeview_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_treeview_cabi test_treeview_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_menu_cabi test_menu_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_menu_cabi test_menu_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_menu_cabi test_menu_cabi.cpp)
    endif()
endif()

if(UICORNERSTONE_BUILD_DLL)
    if(_BACKEND_UPPER STREQUAL "SDL3")
        add_fromsource_target(test_gaps_cabi test_gaps_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "SFML")
        add_fromsource_target(test_gaps_cabi test_gaps_cabi.cpp)
    endif()

    if(_BACKEND_UPPER STREQUAL "RAYLIB")
        add_fromsource_target(test_gaps_cabi test_gaps_cabi.cpp)
    endif()
endif()


