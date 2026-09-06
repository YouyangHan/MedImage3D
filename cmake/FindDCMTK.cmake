#[=======================================================================[.rst:
# FindDCMTK.cmake
# ---------------
# 在 ThirdParty 目录中定位本项目预编译的 DCMTK 3.7, 并转发到官方 DCMTKConfig.cmake。
#
# 用法:
#   find_package(DCMTK REQUIRED COMPONENTS dcmdata dcmimgle ofstd)
#
# 可覆盖变量 (cache / 环境变量):
#   DCMTK_ROOT - DCMTK 安装前缀, 默认 ${PROJECT_SOURCE_DIR}/ThirdParty/DCMTK
#
# 导出变量:
#   DCMTK_RUNTIME_DIR - DCMTK 运行时 DLL 目录, 供 DeployRuntime 拷贝
#   (DCMTK_LIBRARIES / DCMTK::xxx 导入目标由官方 config 提供)
#]=======================================================================]

if(NOT DEFINED DCMTK_ROOT)
    if(DEFINED ENV{DCMTK_ROOT})
        set(DCMTK_ROOT "$ENV{DCMTK_ROOT}")
    else()
        set(DCMTK_ROOT "${PROJECT_SOURCE_DIR}/ThirdParty/DCMTK")
    endif()
endif()
set(DCMTK_ROOT "${DCMTK_ROOT}" CACHE PATH "DCMTK 3.7 安装前缀" FORCE)

# DCMTK 在 Windows 下安装到 <prefix>/cmake, 其他平台多为 lib/cmake/dcmtk
find_path(_DCMTK_CONFIG_DIR
    NAMES DCMTKConfig.cmake
    HINTS "${DCMTK_ROOT}"
    PATH_SUFFIXES cmake lib/cmake/dcmtk lib/cmake share/dcmtk
    NO_DEFAULT_PATH
)

if(NOT _DCMTK_CONFIG_DIR)
    message(FATAL_ERROR
        "未在 ${DCMTK_ROOT} 下找到 DCMTKConfig.cmake。\n"
        "请先执行 ThirdParty/scripts/build_dcmtk.bat 编译安装 DCMTK, "
        "或通过 -DDCMTK_ROOT=<路径> 指定安装位置。")
endif()

if(DCMTK_FIND_COMPONENTS)
    find_package(DCMTK CONFIG REQUIRED NO_DEFAULT_PATH
        PATHS "${_DCMTK_CONFIG_DIR}" COMPONENTS ${DCMTK_FIND_COMPONENTS})
else()
    find_package(DCMTK CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${_DCMTK_CONFIG_DIR}")
endif()

set(DCMTK_RUNTIME_DIR "${DCMTK_ROOT}/bin")
message(STATUS "[FindDCMTK] DCMTK ${DCMTK_VERSION}  @ ${DCMTK_ROOT}")
