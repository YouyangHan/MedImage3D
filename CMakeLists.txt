cmake_minimum_required(VERSION 3.25)

project(MedImage3D VERSION 0.1.0 LANGUAGES CXX)

# ============================================================================
# 全局设置
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    # 第三方库仅编译了 Release(/MD), 日常开发用 RelWithDebInfo 保证 CRT 一致
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "构建类型" FORCE)
endif()

# ============================================================================
# 运行时库一致性 (CRT)
#  第三方库(VTK/ITK/DCMTK/Qt)均编译为 Release 动态库(/MD)。
#  Debug 配置(/MDd + _ITERATOR_DEBUG_LEVEL=2)与之链接会产生 CRT 不匹配
#  (RuntimeLibrary / _ITERATOR_DEBUG_LEVEL 冲突, MSVCRT 默认库冲突)。
#  因此多配置生成器(Visual Studio)下仅保留 Release 与 RelWithDebInfo(带符号, /MD)。
# ============================================================================
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release"
        CACHE STRING "构建配置(仅 Release/RelWithDebInfo, 第三方库为 /MD)" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Qt6 安装前缀: 优先环境变量 QTDIR, 其次 -DQt6_ROOT=<路径>; 不硬编码绝对路径
if(NOT DEFINED Qt6_ROOT AND DEFINED ENV{QTDIR})
    set(Qt6_ROOT "$ENV{QTDIR}")
endif()
if(NOT DEFINED Qt6_ROOT)
    message(FATAL_ERROR
        "未指定 Qt6 安装前缀。请设置环境变量 QTDIR (指向 Qt 的 msvc2022_64 目录), "
        "或通过 -DQt6_ROOT=<路径> 传入。")
endif()
set(Qt6_ROOT "${Qt6_ROOT}" CACHE PATH "Qt6 安装前缀")
list(APPEND CMAKE_PREFIX_PATH "${Qt6_ROOT}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ============================================================================
# 第三方依赖 (cmake/FindXXX.cmake -> ThirdParty/<NAME>)
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(VTK REQUIRED COMPONENTS
    GUISupportQt            # QVTKOpenGLNativeWidget
    RenderingQt
    RenderingVolumeOpenGL2  # 体绘制
    RenderingOpenGL2
    InteractionStyle
    InteractionWidgets
    RenderingFreeType
    RenderingAnnotation
    IOImage                 # vtkDICOMImageReader 等
    ImagingGeneral
    FiltersCore
    FiltersSources
    CommonColor
)

find_package(ITK REQUIRED COMPONENTS
    ITKCommon
    ITKIOImageBase
    ITKIOGDCM               # GDCMImageIO 读取 DICOM 像素
    ITKImageFunction
    ITKImageGrid
    ITKImageIntensity
    ITKVtkGlue              # itkImageToVTKImageFilter 桥接
)

find_package(DCMTK REQUIRED COMPONENTS
    ofstd
    oflog
    dcmdata
    dcmimgle
    dcmimage
)

# ============================================================================
# 主程序
# ============================================================================
qt_standard_project_setup()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qrc"
)

qt_add_executable(MedImage3D WIN32 ${PROJECT_SOURCES})

# 统一以 src/ 为根的头文件搜索路径, 便于各层互相 include (如 "common/Logging.h")
target_include_directories(MedImage3D PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(MedImage3D PRIVATE
    Qt6::Widgets
    ${VTK_LIBRARIES}
    ${ITK_LIBRARIES}
    DCMTK::dcmdata
    DCMTK::dcmimgle
    DCMTK::dcmimage
    DCMTK::ofstd
    DCMTK::oflog
)

# VTK 9 模块化初始化的标准做法: 自动注册所需的对象工厂
vtk_module_autoinit(
    TARGETS MedImage3D
    MODULES ${VTK_LIBRARIES}
)

# ============================================================================
# 自动拷贝运行时 DLL + windeployqt (cmake/DeployRuntime.cmake)
# ============================================================================
include(DeployRuntime)
deploy_runtime(MedImage3D)
