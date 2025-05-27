@echo OFF
SetLocal EnableDelayedExpansion

REM Helper function to print colored text using PowerShell
set "ps_cmd=powershell -NoProfile -Command"

REM Color printing functions
call :ColorPrint "Cyan" "==========================================="
call :ColorPrint "Cyan" "           BUILDING GAME PROJECT          "
call :ColorPrint "Cyan" "==========================================="
echo.

SET PATH=%PATH%;C:\msys64\ucrt64\bin
SET config=debug
IF "%1"=="release" SET config=release
SET outputDir=.\build\bin\%config%

IF NOT EXIST ".\build" mkdir ".\build"
IF NOT EXIST ".\build\bin" mkdir ".\build\bin"
IF NOT EXIST "%outputDir%" mkdir "%outputDir%"

set cppFilenames=
for /R %%f in (*.cpp) do (
    set cppFilenames=!cppFilenames! "%%f"
)

set assembly=game
set includeFlags=-Isrc -Isrc/public 
set defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS -DDEBUG_BUILD
IF "%config%"=="release" set defines=-DRELEASE -D_CRT_SECURE_NO_WARNINGS

@REM REM Try AddressSanitizer
@REM call :ColorPrint "Blue" "[1/3] Attempting build with AddressSanitizer..."
@REM set compilerFlags=-g3 -Wall -Werror -std=c++23 -fsanitize=address -fno-omit-frame-pointer -O0
@REM set linkerFlags=-luser32 -lgdi32 -fsanitize=address

@REM g++ %cppFilenames% %compilerFlags% -o "%outputDir%\%assembly%.exe" %defines% %includeFlags% %linkerFlags% >nul 2>&1

@REM IF %ERRORLEVEL% EQU 0 (
@REM     call :ColorPrint "Green" "✓ SUCCESS: Build completed with AddressSanitizer!"
@REM     call :ColorPrint "Cyan" "  Executable: %outputDir%\%assembly%.exe"
@REM     call :ColorPrint "Magenta" "  ⚡ ENHANCED DEBUGGING: Detailed crash reports enabled!"
@REM     goto :success
@REM )

@REM REM Try UBSanitizer
@REM call :ColorPrint "Blue" "[2/3] Trying UndefinedBehaviorSanitizer..."
@REM set compilerFlags=-g3 -Wall -Werror -std=c++23 -fsanitize=undefined -fno-omit-frame-pointer -O0
@REM set linkerFlags=-luser32 -lgdi32 -fsanitize=undefined

@REM g++ %cppFilenames% %compilerFlags% -o "%outputDir%\%assembly%.exe" %defines% %includeFlags% %linkerFlags% >nul 2>&1

@REM IF %ERRORLEVEL% EQU 0 (
@REM     call :ColorPrint "Green" "✓ SUCCESS: Build completed with UBSanitizer!"
@REM     call :ColorPrint "Cyan" "  Executable: %outputDir%\%assembly%.exe"
@REM     goto :success
@REM )

REM Fallback
call :ColorPrint "Blue" "Using enhanced debugging flags..."
set compilerFlags=-g3 -Wall -Werror -std=c++23 -fstack-protector-all -fno-omit-frame-pointer -O0 -ggdb
set linkerFlags=-luser32 -lgdi32 -ldbghelp 

g++ %cppFilenames% %compilerFlags% -o "%outputDir%\%assembly%.exe" %defines% %includeFlags% %linkerFlags%

IF %ERRORLEVEL% EQU 0 (
    call :ColorPrint "Green" "SUCCESS: Build completed!"
    call :ColorPrint "Cyan" "  Executable: %outputDir%\%assembly%.exe"
    goto :success
) ELSE (
    call :ColorPrint "Red" "FAILED: Build failed with error code %ERRORLEVEL%"
    exit /b %ERRORLEVEL%
)

:success
echo.
call :ColorPrint "Green" "BUILD COMPLETE!"
call :ColorPrint "White" "Ready to debug crashes and issues."
echo.
goto :eof

REM Function to print colored text
:ColorPrint
%ps_cmd% "Write-Host '%~2' -ForegroundColor %~1"
goto :eof