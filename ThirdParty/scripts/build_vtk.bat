@echo off
REM ============================================================================
REM 编译 VTK 9.6.2 (Release / 共享库 / Qt6 支持)
REM 用法: 双击或在终端执行 build_vtk.bat
REM 说明: Qt 6.12 要求 CMake >= 3.25, 优先使用 ThirdParty/_tools 绿色版 CMake;
REM       若退回系统 CMake 3.24, 则依靠 QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT
REM ============================================================================
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "TP_ROOT=%SCRIPT_DIR%.."
set "SRC_DIR=%TP_ROOT%\_src\VTK-9.6.2"
set "BUILD_DIR=%TP_ROOT%\_build\VTK"
set "INSTALL_DIR=%TP_ROOT%\VTK"
set "QT_DIR=%QTDIR%"
if not defined QT_DIR (
    echo [错误] 请先设置 QTDIR 环境变量, 指向 Qt 的 msvc2022_64 目录, 例如: set QTDIR=C:\Qt\6.12.0\msvc2022_64
    exit /b 1
)
set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "NINJA_DIR=%VS_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "CMAKE_EXE=%TP_ROOT%\_tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"

if not exist "%SRC_DIR%\CMakeLists.txt" (
    echo [错误] 未找到 VTK 源码: %SRC_DIR%
    exit /b 1
)

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%NINJA_DIR%;%PATH%"

echo [1/3] 配置 VTK (使用 %CMAKE_EXE%) ...
"%CMAKE_EXE%" -G Ninja -S "%SRC_DIR%" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DBUILD_SHARED_LIBS=ON ^
    -DBUILD_EXAMPLES=OFF ^
    -DVTK_BUILD_TESTING=OFF ^
    -DVTK_BUILD_DOCUMENTATION=OFF ^
    -DVTK_WRAP_PYTHON=OFF ^
    -DVTK_WRAP_JAVA=OFF ^
    -DVTK_QT_VERSION=6 ^
    -DVTK_GROUP_ENABLE_Qt=WANT ^
    -DVTK_MODULE_ENABLE_VTK_GUISupportQt=YES ^
    -DVTK_MODULE_ENABLE_VTK_RenderingQt=YES ^
    -DVTK_MODULE_ENABLE_VTK_ViewsQt=YES ^
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick=NO ^
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtSQL=NO ^
    -DQt6_DIR="%QT_DIR:\=/%/lib/cmake/Qt6" ^
    -DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.24 ^
    || exit /b 1

echo [2/3] 编译 VTK (32 核并行) ...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel 32 || exit /b 1

echo [3/3] 安装 VTK 到 %INSTALL_DIR% ...
"%CMAKE_EXE%" --install "%BUILD_DIR%" || exit /b 1

echo [完成] VTK 9.6.2 已安装到 %INSTALL_DIR%
endlocal
