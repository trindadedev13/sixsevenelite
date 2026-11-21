cmake -B .build
mv .build/compile_commands.json .
cmake --build .build