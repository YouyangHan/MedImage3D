#[=======================================================================[.rst:
# FindVTK.cmake
# -------------
# 在 ThirdParty 目录中定位本项目预编译的 VTK 9.6, 并转发到官方 VTKConfig.cmake。
#
# 用法:
#   find_package(VTK REQUIRED COMPONENTS GUISupportQt RenderingQt ...)
#
# 可覆盖变量 (cache / 环境变量):
#   VTK_ROOT - VTK 安装前缀, 默认 ${PROJECT_SOURCE_DIR}/ThirdParty/VTK
#
# 导出变量:
#   VTK_RUNTIME_DIR - VTK 运行时 DLL 目录, 供 DeployRuntime 拷贝
#   (其余 VTK_LIBRARIES / vtk_module_autoinit 等由官方 config 提供)
#]=======================================================================]

if(NOT DEFINED VTK_ROOT)
    if(DEFINED ENV{VTK_ROOT})
        set(VTK_ROOT "$ENV{VTK_ROOT}")
    else()
        set(VTK_ROOT "${PROJECT_SOURCE_DIR}/ThirdParty/VTK")
    endif()
endif()
set(VTK_ROOT "${VTK_ROOT}" CACHE PATH "VTK 9.6 安装前缀" FORCE)

# 定位 VTK 9 的 config 文件(文件名为小写 vtk-config.cmake)
find_path(_VTK_CONFIG_DIR
    NAMES vtk-config.cmake VTKConfig.cmake
    HINTS "${VTK_ROOT}"
    PATH_SUFFIXES lib/cmake/vtk-9.6 lib/cmake/vtk lib/cmake
    NO_DEFAULT_PATH
)

if(NOT _VTK_CONFIG_DIR)
    message(FATAL_ERROR
        "未在 ${VTK_ROOT} 下找到 vtk-config.cmake。\n"
        "请先执行 ThirdParty/scripts/build_vtk.bat 编译安装 VTK, "
        "或通过 -DVTK_ROOT=<路径> 指定安装位置。")
endif()

# 转发到官方 config 包(透传 COMPONENTS 请求)
if(VTK_FIND_COMPONENTS)
    find_package(VTK CONFIG REQUIRED NO_DEFAULT_PATH
        PATHS "${_VTK_CONFIG_DIR}" COMPONENTS ${VTK_FIND_COMPONENTS})
else()
    find_package(VTK CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${_VTK_CONFIG_DIR}")
endif()

set(VTK_RUNTIME_DIR "${VTK_ROOT}/bin")
message(STATUS "[FindVTK] VTK ${VTK_VERSION}  @ ${VTK_ROOT}")
