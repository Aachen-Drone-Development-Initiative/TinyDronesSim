# TinyDronesSim-EnvironmentBackend

This is the backend of `Environment.jl`, which builds on [Google-Filament](https://github.com/google/filament).

## Building

### Installing/Building Dependencies on Linux

1. Clone and build [Google-Filament](https://github.com/google/filament).
   1. Get the dependencies for Filament.
      - `clang-14` or higher
      - `libglu1-mesa-dev`
      - `libc++-14-dev` (`libcxx-devel` and `libcxx-static` on Fedora) or higher
      - `libc++abi-14-dev` (`libcxxabi-static` on Fedora) or higher
      - `ninja-build`
      - `libxi-dev`
      - `libxcomposite-dev` (`libXcomposite-devel` on Fedora)
      - `libxxf86vm-dev` (`libXxf86vm-devel` on Fedora)
   2. Clone filament, specify `depth=1` to do a shallow clone:  
      `git clone https://github.com/google/filament.git --depth=1`
   3. Enter the directory: `cd filament`
   4. Generate build files:  `CC=clang CXX=clang++ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../release/filament -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fPIC" -DCMAKE_C_FLAGS="-fPIC"`  
   This command is mostly copied from the BUILDING.md in the filament directory, the only relevant change is the added `-fPIC` flag for generating Position Independent Code, which is required for building this project as a shared library.
   5. Run Ninja: `ninja`
   
2. Copy all the libraries from the `filament/out/cmake-release` folder to the `TinyDronesSim/EnvironmentBackend/filament/lib` folder.
   1. Create the `filament` and `filament/lib` folders inside `TinyDronesSim/EnvironmentBackend/`.
   2. Now return to your `filament` directory and scrape all the libraries you just compiled:  
      `for file in $(find . -name "*.a"); do cp "$file" <path-to-the-filament-lib-folder>; done`
   Most of these libraries are not used, you can inspect which ones are actually required inside `nob.c` and remove the rest.

### Building on Linux

We are using the experimental build system [nob.h](https://github.com/tsoding/nob.h).  

1. Create the new C-header file `nob_config.h` inside `EnvironmentBackend/` and add the line `#define FILAMENT_PATH "/your/path/to/filament/"` to specify your filament directory **you need to add a trailing '/'**. This file is 'gitignored'.
1. Compile `nob.c` with your favorite C-compiler. `cc nob.c -o nob` should work on Linux.
2. Run `./nob` (can be executed from any directory)
