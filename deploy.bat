@echo off
setlocal enabledelayedexpansion

echo =======================================
echo HuYanReader 部署脚本
echo =======================================

:: 检查参数
set BUILD_TYPE=RelWithDebInfo
if "%1"=="Release" set BUILD_TYPE=Release
if "%1"=="Debug" set BUILD_TYPE=Debug

echo 构建类型: %BUILD_TYPE%
echo.

:: 设置路径
set BUILD_DIR=%~dp0build_vs
set BIN_DIR=!BUILD_DIR!\bin\RelWithDebInfo
set DEPLOY_DIR=%~dp0deploy

echo 构建目录: !BUILD_DIR!
echo 可执行文件目录: !BIN_DIR!
echo 部署目录: !DEPLOY_DIR!
echo.

:: 检查可执行文件是否存在
if not exist "!BIN_DIR!\ProtectEye.exe" (
    echo 错误: 找不到 ProtectEye.exe 在 !BIN_DIR!
    echo 请先构建项目: cmake --build build_vs --config !BUILD_TYPE!
    pause
    exit /b 1
)

:: 创建部署目录
if not exist "!DEPLOY_DIR!" mkdir "!DEPLOY_DIR!"

:: 清空部署目录
echo 清空部署目录...
rmdir /s /q "!DEPLOY_DIR!" 2>nul
mkdir "!DEPLOY_DIR!"

:: 复制可执行文件
echo 复制可执行文件...
copy "!BIN_DIR!\ProtectEye.exe" "!DEPLOY_DIR!\" >nul
if errorlevel 1 (
    echo 错误: 复制 ProtectEye.exe 失败
    pause
    exit /b 1
)

:: 复制配置文件
echo 复制配置文件...
if exist "!BIN_DIR!\*.ini" copy "!BIN_DIR!\*.ini" "!DEPLOY_DIR!\" >nul

:: 复制资源目录
echo 复制资源文件...
if exist "%~dp0resources" xcopy /s /e /i "%~dp0resources" "!DEPLOY_DIR!\resources" >nul

:: 复制SSL DLL
echo 复制SSL库...
if exist "!BIN_DIR!\libcrypto-1_1-x64.dll" copy "!BIN_DIR!\libcrypto-1_1-x64.dll" "!DEPLOY_DIR!\" >nul
if exist "!BIN_DIR!\libssl-1_1-x64.dll" copy "!BIN_DIR!\libssl-1_1-x64.dll" "!DEPLOY_DIR!\" >nul

:: 查找并运行 windeployqt
echo 查找 windeployqt...
where windeployqt >nul 2>&1
if errorlevel 1 (
    echo 警告: 找不到 windeployqt，尝试手动查找...
    set WINDEPLOYQT_PATH=
    for %%i in ("D:\Software\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\windeployqt.exe") do (
        if exist "%%i" set WINDEPLOYQT_PATH=%%i
    )
    if "!WINDEPLOYQT_PATH!"=="" (
        echo 错误: 找不到 windeployqt.exe
        echo 请确保 Qt 的 bin 目录在 PATH 中，或修改此脚本中的路径
        pause
        exit /b 1
    )
) else (
    set WINDEPLOYQT_PATH=windeployqt
)

:: 运行 windeployqt
echo 运行 windeployqt 部署 Qt 依赖...
if "!BUILD_TYPE!"=="Release" (
    "!WINDEPLOYQT_PATH!" --release --webenginewidgets "!DEPLOY_DIR!\ProtectEye.exe"
) else (
    "!WINDEPLOYQT_PATH!" --debug --webenginewidgets "!DEPLOY_DIR!\ProtectEye.exe"
)

if errorlevel 1 (
    echo 错误: windeployqt 运行失败
    pause
    exit /b 1
)

:: 复制其他必需的 DLL
echo 复制额外的 DLL...
if exist "!BIN_DIR!\libcrypto-1_1-x64.dll" copy "!BIN_DIR!\libcrypto-1_1-x64.dll" "!DEPLOY_DIR!\" >nul
if exist "!BIN_DIR!\libssl-1_1-x64.dll" copy "!BIN_DIR!\libssl-1_1-x64.dll" "!DEPLOY_DIR!\" >nul

echo.
echo =======================================
echo 部署完成！
echo =======================================
echo 部署目录: !DEPLOY_DIR!
echo.
echo 现在可以将 deploy 目录中的所有文件复制到其他电脑运行。
echo.
echo 测试部署:
echo 1. 运行 !DEPLOY_DIR!\ProtectEye.exe 确认程序正常启动
echo 2. 如果运行正常，可以将整个 deploy 目录打包分发
echo.
pause