@echo off
REM ============================================================================
REM 编译 DCMTK 3.7.0 (Release / 共享库)
REM ============================================================================
setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "TP_ROOT=%SCRIPT_DIR%.."
set "SRC_DIR=%TP_ROOT%\_src\dcmtk-DCMTK-3.7.0"
set "BUILD_DIR=%TP_ROOT%\_build\DCMTK"
set "INSTALL_DIR=%TP_ROOT%\DCMTK"
set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "NINJA_DIR=%VS_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "CMAKE_EXE=%TP_ROOT%\_tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe"
if not exist "%CMAKE_EXE%" set "CMAKE_EXE=cmake"

if not exist "%SRC_DIR%\CMakeLists.txt" (
    echo [错误] 未找到 DCMTK 源码: %SRC_DIR%
    exit /b 1
)

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%NINJA_DIR%;%PATH%"

echo [1/3] 配置 DCMTK (使用 %CMAKE_EXE%) ...
"%CMAKE_EXE%" -G Ninja -S "%SRC_DIR%" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%" ^
    -DBUILD_SHARED_LIBS=ON ^
    -DBUILD_APPS=OFF ^
    -DDCMTK_WITH_DOXYGEN=OFF ^
    -DDCMTK_WITH_OPENSSL=OFF ^
    -DDCMTK_WITH_ICONV=OFF ^
    || exit /b 1

echo [2/3] 编译 DCMTK (32 核并行) ...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel 32 || exit /b 1

echo [3/3] 安装 DCMTK 到 %INSTALL_DIR% ...
"%CMAKE_EXE%" --install "%BUILD_DIR%" || exit /b 1

echo [完成] DCMTK 3.7.0 已安装到 %INSTALL_DIR%
endlocal
