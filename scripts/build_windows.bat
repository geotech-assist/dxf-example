@echo off
REM Windows build script for DXF Processor
REM Supports Visual Studio 2019 and 2022

echo DXF Processor - Windows Build Script
echo ====================================

REM Check if we're in the right directory
if not exist CMakeLists.txt (
    echo Error: CMakeLists.txt not found. Please run this script from the project root directory.
    pause
    exit /b 1
)

REM Create build directory
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

REM Detect Visual Studio version
set VS_GENERATOR=""
if defined VS170COMNTOOLS (
    set VS_GENERATOR="Visual Studio 17 2022"
    echo Detected Visual Studio 2022
) else if defined VS160COMNTOOLS (
    set VS_GENERATOR="Visual Studio 16 2019"
    echo Detected Visual Studio 2019
) else (
    echo Warning: Visual Studio not detected in environment variables.
    echo Trying default generator...
    set VS_GENERATOR="Visual Studio 17 2022"
)

echo.
echo Configuring project with CMake...
echo Generator: %VS_GENERATOR%

cmake .. -G %VS_GENERATOR% -A x64
if errorlevel 1 (
    echo Error: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Building project (Release configuration)...
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo Error: Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Building project (Debug configuration)...
cmake --build . --config Debug --parallel
if errorlevel 1 (
    echo Error: Debug build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo Build completed successfully!
echo.
echo Executables are available at:
echo   Release: build\bin\Release\dxf_processor.exe
echo   Debug:   build\bin\Debug\dxf_processor.exe
echo.
echo You can open the Visual Studio solution at:
echo   build\dxf_processor.sln
echo.
echo To test the application, run:
echo   build\bin\Release\dxf_processor.exe "data\Design Pit.dxf"

pause