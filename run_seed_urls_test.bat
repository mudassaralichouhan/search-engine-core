@echo off
echo Running Seed URLs test...

:: Navigate to build directory
cd build

:: Run the specific test
echo Running test...
ctest -C Debug -R "Seed URLs" -V --output-on-failure

:: Return to original directory
cd ..

echo Done! 