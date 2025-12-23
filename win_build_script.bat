@echo off
mkdir build 2>nul
if not exist build (
    echo Directory 'build' could not be created.
    exit /b 1
)
cd build
if %errorlevel% neq 0 (
    echo Failed to change directory to 'build'.
    exit /b %errorlevel%
)
cmake ..
if %errorlevel% neq 0 (
    echo CMake command failed.
    exit /b %errorlevel%
)