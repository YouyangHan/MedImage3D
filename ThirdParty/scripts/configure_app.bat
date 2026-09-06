@echo off
REM ============================================================================
REM 配置主工程 MedImage3D (Ninja + MSVC 2022 + RelWithDebInfo)
REM 说明: Qt 6.12 要求 CMake >= 3.25, 使用 ThirdParty/_tools 绿色版 CMake 3.31.6
REM 用法: configure_app.bat [build_dir]
REM ============================================================================
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."
set "CMAKE_EXE=%SCRIPT_DIR%..\_tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"
set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "NINJA_DIR=%VS_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

if not defined QTDIR (
    echo [错误] 未设置 QTDIR 环境变量, 请先指向 Qt 的 msvc2022_64 目录, 例如:
    echo        setx QTDIR "C:\Qt\6.12.0\msvc2022_64" ^&^& 重开终端
    exit /b 1
)

set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%PROJECT_ROOT%\build\ninja"

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%NINJA_DIR%;%PATH%"

echo [配置] 使用 CMake: %CMAKE_EXE%
echo [配置] 项目根:    %PROJECT_ROOT%
echo [配置] 构建目录:  %BUILD_DIR%

"%CMAKE_EXE%" -G Ninja -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_DIR%\ninja.exe" ^
    || exit /b 1

echo.
echo [完成] 配置成功。构建命令:
echo   "%CMAKE_EXE%" --build "%BUILD_DIR%"
endlocal
