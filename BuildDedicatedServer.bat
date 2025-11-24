@echo off
setlocal EnableDelayedExpansion

REM Create logs directory if it doesn't exist
if not exist "Logs" mkdir "Logs"

REM Set log file with timestamp
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%" & set "YYYY=%dt:~0,4%" & set "MM=%dt:~4,2%" & set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%" & set "Min=%dt:~10,2%" & set "Sec=%dt:~12,2%"
set "datestamp=%YYYY%-%MM%-%DD%_%HH%-%Min%-%Sec%"
set "BUILD_LOG=Logs\build_log_%datestamp%.txt"

REM Function to log with timestamp
call :log "========================================"
call :log "Building Example Projecr Dedicated Server"
call :log "========================================"
call :log "Log file: %BUILD_LOG%"

REM Set the Unreal Engine path (modify this to match your UE installation)
set UE_ENGINE_PATH=C:\EpicDiogo\Engine
set UAT_PATH=%UE_ENGINE_PATH%\Build\BatchFiles\RunUAT.bat

REM Check if UAT exists
if not exist "%UAT_PATH%" (
    call :log "ERROR: UAT not found at %UAT_PATH%"
    pause
    exit /b 1
)

REM Set project details
set PROJECT_DIR=%~dp0
set PROJECT_FILE=%PROJECT_DIR%ExampleProject.uproject
set OUTPUT_DIR=%PROJECT_DIR%PackagedServer

REM Check if project file exists
if not exist "%PROJECT_FILE%" (
    call :log "ERROR: Project file not found at %PROJECT_FILE%"
    pause
    exit /b 1
)

REM Clean old packaged output
if exist "%OUTPUT_DIR%" (
    call :log "Cleaning old packaged output..."
    rmdir /s /q "%OUTPUT_DIR%"
)

REM Choose build config (can be changed via argument)
set BUILD_CONFIG=Shipping
set PLATFORM=Linux
set EnvoirmentPlatform=Win64
set TARGET=ExampleProjectServer
set MAP=Lvl_ThirdPerson

call :log ""
call :log "==============================="
call :log "Building and Cooking %TARGET% [%BUILD_CONFIG%]"
call :log "==============================="

REM Build with full logging
call :log "Starting UAT build process..."
call :log "Command: %UAT_PATH% BuildCookRun -nocompileeditor -project=\"%PROJECT_FILE%\" -noP4 -clientconfig=%BUILD_CONFIG% -serverconfig=%BUILD_CONFIG% -platform=%EnvoirmentPlatform% -targetplatform=%PLATFORM% -cook -build -stage -pak -archive -archivedirectory=\"%OUTPUT_DIR%\" -server -CrashReporter=False -prereqs -map=%MAP%"

"%UAT_PATH%" BuildCookRun ^
 -nocompileeditor ^
 -project="%PROJECT_FILE%" ^
 -noP4 ^
 -clientconfig=%BUILD_CONFIG% ^
 -serverconfig=%BUILD_CONFIG% ^
 -platform=%EnvoirmentPlatform% ^
 -targetplatform=%PLATFORM% ^
 -cook ^
 -build ^
 -stage ^
 -pak ^
 -archive ^
 -archivedirectory="%OUTPUT_DIR%" ^
 -server ^
 -CrashReporter=False ^
 -prereqs ^
 -map=%MAP% >> "%BUILD_LOG%" 2>&1

set BUILD_RESULT=%ERRORLEVEL%

if %BUILD_RESULT% NEQ 0 (
    call :log "ERROR: Server build failed with exit code %BUILD_RESULT%!"
    call :log "Check the log file for details: %BUILD_LOG%"
    echo.
    echo Build failed! Check the log file: %BUILD_LOG%
    pause
    exit /b 1
)

call :log ""
call :log "========================================"
call :log "Build completed successfully!"
call :log "Output Path: %OUTPUT_DIR%"
call :log "Executable:  %OUTPUT_DIR%\WindowsServer\BattleBumpersServer.exe"
call :log "Log saved to: %BUILD_LOG%"
call :log "========================================"

echo.
echo ========================================
echo Build completed successfully!
echo Output Path: %OUTPUT_DIR%
echo Executable:  %OUTPUT_DIR%\WindowsServer\BattleBumpersServer.exe
echo Log saved to: %BUILD_LOG%
echo ========================================
pause

exit /b 0

:log
echo [%date% %time%] %~1
echo [%date% %time%] %~1 >> "%BUILD_LOG%"
goto :eof
