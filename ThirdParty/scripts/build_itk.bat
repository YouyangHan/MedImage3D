@echo off
REM ============================================================================
REM 编译 ITK 5.4.7 (Release / 共享库 / ITKVtkGlue 桥接 VTK)
REM 注意: 必须先完成 build_vtk.bat (ITKVtkGlue 依赖 VTK_DIR)
REM       VTK 的 Qt 支持要求配置期能找到 Qt6, 且 Qt 6.12 要求 CMake >= 3.25,
REM       因此使用 ThirdParty/_tools 下的绿色版 CMake 3.31.6
REM ============================================================================
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "TP_ROOT=%SCRIPT_DIR%.."
set "SRC_DIR=%TP_ROOT%\_src\InsightToolkit-5.4.7"
set "BUILD_DIR=%TP_ROOT%\_build\ITK"
set "INSTALL_DIR=%TP_ROOT%\ITK"
set "VTK_INSTALL=%TP_ROOT%\VTK"
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
    echo [错误] 未找到 ITK 源码: %SRC_DIR%
    exit /b 1
)
if not exist "%VTK_INSTALL%\lib\cmake" (
    echo [错误] 未找到已安装的 VTK, 请先执行 build_vtk.bat
    exit /b 1
)

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%NINJA_DIR%;%PATH%"

echo [1/3] 配置 ITK (使用 %CMAKE_EXE%) ...
"%CMAKE_EXE%" -G Ninja -S "%SRC_DIR%" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DBUILD_SHARED_LIBS=ON ^
    -DBUILD_TESTING=OFF ^
    -DBUILD_EXAMPLES=OFF ^
    -DITK_BUILD_DEFAULT_MODULES=ON ^
    -DITKGroup_IO=ON ^
    -DModule_ITKVtkGlue=ON ^
    -DVTK_DIR="%VTK_INSTALL:\=/%/lib/cmake/vtk-9.6" ^
    -DQt6_DIR="%QT_DIR:\=/%/lib/cmake/Qt6" ^
    || exit /b 1

echo [2/3] 编译 ITK (32 核并行) ...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel 32 || exit /b 1

echo [3/3] 安装 ITK 到 %INSTALL_DIR% ...
"%CMAKE_EXE%" --install "%BUILD_DIR%" || exit /b 1

echo [完成] ITK 5.4.7 已安装到 %INSTALL_DIR%
endlocal
