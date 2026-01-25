rd /s /q build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
start cmd /k build\src\Release\WiiURichPresence.exe
