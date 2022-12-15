cmake -S . -B build ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ^
    -GNinja
cmake --build build
build\smtc-demo.exe
