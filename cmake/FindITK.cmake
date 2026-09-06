#[=======================================================================[.rst:
# FindITK.cmake
# -------------
# 在 ThirdParty 目录中定位本项目预编译的 ITK 5.4, 并转发到官方 ITKConfig.cmake。
#
# 用法:
#   find_package(ITK REQUIRED COMPONENTS ITKCommon ITKIOImageBase ITKVtkGlue)
#
# 可覆盖变量 (cache / 环境变量):
#   ITK_ROOT - ITK 安装前缀, 默认 ${PROJECT_SOURCE_DIR}/ThirdParty/ITK
#
# 导出变量:
#   ITK_RUNTIME_DIR - ITK 运行时 DLL 目录, 供 DeployRuntime 拷贝
#   (ITK_LIBRARIES / ITK_USE_FILE 等由官方 config 提供)
#]=======================================================================]

if(NOT DEFINED ITK_ROOT)
    if(DEFINED ENV{ITK_ROOT})
        set(ITK_ROOT "$ENV{ITK_ROOT}")
    else()
        set(ITK_ROOT "${PROJECT_SOURCE_DIR}/ThirdParty/ITK")
    endif()
endif()
set(ITK_ROOT "${ITK_ROOT}" CACHE PATH "ITK 5.4 安装前缀" FORCE)

find_path(_ITK_CONFIG_DIR
    NAMES ITKConfig.cmake
    HINTS "${ITK_ROOT}"
    PATH_SUFFIXES lib/cmake/ITK-5.4 lib/cmake/ITK lib/cmake
    NO_DEFAULT_PATH
)

if(NOT _ITK_CONFIG_DIR)
    message(FATAL_ERROR
        "未在 ${ITK_ROOT} 下找到 ITKConfig.cmake。\n"
        "请先执行 ThirdParty/scripts/build_itk.bat 编译安装 ITK, "
        "或通过 -DITK_ROOT=<路径> 指定安装位置。")
endif()

if(ITK_FIND_COMPONENTS)
    find_package(ITK CONFIG REQUIRED NO_DEFAULT_PATH
        PATHS "${_ITK_CONFIG_DIR}" COMPONENTS ${ITK_FIND_COMPONENTS})
else()
    find_package(ITK CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${_ITK_CONFIG_DIR}")
endif()

include(${ITK_USE_FILE})

set(ITK_RUNTIME_DIR "${ITK_ROOT}/bin")
message(STATUS "[FindITK] ITK ${ITK_VERSION}  @ ${ITK_ROOT}")
