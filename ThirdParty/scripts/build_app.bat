@echo off
REM ============================================================================
REM 构建主工程 MedImage3D (需先执行 configure_app.bat)
REM ============================================================================
setlocal

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."
set "CMAKE_EXE=%SCRIPT_DIR%..\_tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"
set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"

set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%PROJECT_ROOT%\build\ninja"

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul

echo [构建] 目录: %BUILD_DIR%
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel 32 || exit /b 1

echo [完成] 可执行文件位于 %BUILD_DIR%\MedImage3D.exe
endlocal
