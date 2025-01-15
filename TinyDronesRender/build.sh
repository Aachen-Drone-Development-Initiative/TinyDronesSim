mkdir bin
cd bin
# rm lib_tinydronesrender.a tiny_drones_render.o
rm libtinydronesrender.so tiny_drones_render.o
g++ ../src/tiny_drones_render.cpp -c
gcc -shared -o libtinydronesrender.so tiny_drones_render.o ../raylib/raudio.o ../raylib/rcore.o  ../raylib/rglfw.o  ../raylib/rmodels.o  ../raylib/rshapes.o  ../raylib/rtext.o  ../raylib/rtextures.o  ../raylib/utils.o
# ar rvs lib_tinydronesrender.a tiny_drones_render.o ../raylib/raudio.o ../raylib/rcore.o  ../raylib/rglfw.o  ../raylib/rmodels.o  ../raylib/rshapes.o  ../raylib/rtext.o  ../raylib/rtextures.o  ../raylib/utils.o
cd ..
