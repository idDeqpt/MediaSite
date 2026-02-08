mkdir build
cd build

cmake ..
cmake --build .

cd ..
xcopy /s ".\src\resources\" ".\bin\Debug\resources\" /Y /e /i
