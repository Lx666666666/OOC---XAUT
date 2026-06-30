@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if %errorlevel%==0 (
    py -3 auto_run.py
    exit /b %errorlevel%
)

where python >nul 2>nul
if %errorlevel%==0 (
    python auto_run.py
    exit /b %errorlevel%
)

echo 未检测到 Python，改为直接启动程序。
if exist "bin\Project7_static.exe" (
    start "" "%~dp0bin\Project7_static.exe"
) else if exist "bin\Project7.exe" (
    start "" "%~dp0bin\Project7.exe"
) else (
    echo 找不到可执行程序。
    pause
    exit /b 1
)
endlocal
