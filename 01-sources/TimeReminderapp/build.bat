@echo off
REM ============================================================================
REM TimeReminder 编译脚本
REM 使用 Visual Studio 2026 编译器编译生成 TimeReminder.exe
REM ============================================================================

echo ============================================
echo   TimeReminder 编译脚本
echo ============================================
echo.

REM 设置Visual Studio 2013编译环境
set VCVARSALL="D:\AppGallery\Downloads\VS2026\PATCH\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %VCVARSALL% (
    echo [错误] 未找到 Visual Studio 2013 编译环境！
    echo 请确保已安装 Visual Studio 2013 或修改脚本中的路径。
    pause
    exit /b 1
)

echo [1/3] 初始化编译环境...
call %VCVARSALL% x86

echo.
echo [2/3] 编译 TimeReminder.exe...
cl.exe /EHsc /W3 /O2 /D "UNICODE" /D "_UNICODE" TimeReminder.cpp /Fe:TimeReminder.exe /link user32.lib comctl32.lib shell32.lib gdi32.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo [错误] 编译失败！
    pause
    exit /b 1
)

echo.
echo [3/3] 清理临时文件...
if exist TimeReminder.obj del TimeReminder.obj

echo.
echo ============================================
echo   编译成功！
echo   输出文件: TimeReminder.exe
echo ============================================
echo.

REM 询问是否运行
set /p RUN="是否立即运行程序？(Y/N): "
if /i "%RUN%"=="Y" (
    echo 启动程序...
    start TimeReminder.exe
)

pause