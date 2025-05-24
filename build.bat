@echo OFF
SetLocal EnableDelayedExpansion

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
set compilerFlags=-g -Wall -Werror -std=c++23
IF "%config%"=="release" set compilerFlags=-O2 -Wall -Werror -std=c++23

set includeFlags=-Isrc -Isrc/public 
set linkerFlags=-luser32  -lgdi32
set defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS
IF "%config%"=="release" set defines=-DRELEASE -D_CRT_SECURE_NO_WARNINGS

echo Building %assembly% in %config% mode...
echo Output directory: %outputDir%
echo Compiler flags: %compilerFlags%
echo Include flags: %includeFlags%
echo Defines: %defines%

g++ %cppFilenames% %compilerFlags% -o "%outputDir%\%assembly%.exe" %defines% %includeFlags% %linkerFlags%

IF %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
    echo Executable: %outputDir%\%assembly%.exe
) ELSE (
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)