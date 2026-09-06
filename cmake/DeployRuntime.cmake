#[=======================================================================[.rst:
# DeployRuntime.cmake
# -------------------
# 自动把第三方运行时(VTK/ITK/DCMTK 的 DLL)与 Qt 依赖拷贝到目标输出目录,
# 使得编译完成后 exe 可直接双击运行, 无需配置 PATH。
#
# 用法(在 find_package 完成之后):
#   include(DeployRuntime)
#   deploy_runtime(<目标名>)
#]=======================================================================]

function(deploy_runtime TARGET_NAME)
    if(NOT WIN32)
        return()
    endif()

    # ---- 1. 收集第三方库 bin 目录下的全部 DLL ------------------------------
    set(_runtime_dirs)
    foreach(_dir IN ITEMS "${VTK_RUNTIME_DIR}" "${ITK_RUNTIME_DIR}" "${DCMTK_RUNTIME_DIR}")
        if(_dir AND IS_DIRECTORY "${_dir}")
            list(APPEND _runtime_dirs "${_dir}")
        endif()
    endforeach()

    foreach(_dir IN LISTS _runtime_dirs)
        file(GLOB _dlls "${_dir}/*.dll")
        if(_dlls)
            # copy_if_different 支持多源文件 + 目标目录, 每个 bin 目录一条命令
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${_dlls} "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                COMMENT "[DeployRuntime] 拷贝 ${_dir} 下的运行时 DLL")
        endif()
    endforeach()

    # ---- 2. Qt 依赖: windeployqt ------------------------------------------
    if(TARGET Qt6::qmake)
        get_target_property(_qmake_loc Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_loc}" DIRECTORY)
        find_program(_windeployqt windeployqt.exe HINTS "${_qt_bin_dir}" NO_DEFAULT_PATH)
        if(_windeployqt)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND "${_windeployqt}"
                        --no-translations
                        --no-system-d3d-compiler
                        --no-opengl-sw
                        "$<TARGET_FILE:${TARGET_NAME}>"
                COMMENT "[DeployRuntime] windeployqt 部署 Qt 依赖")
        else()
            message(WARNING "[DeployRuntime] 未找到 windeployqt, Qt 依赖需手动部署")
        endif()
    endif()
endfunction()
