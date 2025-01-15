./build.sh
cd bin
rm ./tiny_drones_render.exe
g++ -g ../src/tiny_drones_render_test.cpp -L. -ltinydronesrender -o tiny_drones_render.exe
cd ..
