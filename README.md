# WORK IN PROGRESS!

# TinyDronesSim

TinyDronesSim is a tool for simulating the *tinydrone*.  

## What this tool should do

1. Very simple mechanics for generic quadrocopter-like behaviour (low speed regime)
2. Interactive visualization of the drone and other information
3. Controlling of the virtual drone with gamepad
3. Easy integration and testing of the drones source code

## Getting started (Linux only)

1. `git clone https://github.com/Aachen-Drone-Development-Initiative/TinyDronesSim.git`
2. Building the `libenvironment.so`
   1.`cd TinyDronesSim/EnvironmentBackend`
   2. Get Google-Filament
      - `git clone https://github.com/google/filament.git --depth=1` specifying `depth=1` to do a shallow clone (still 2 GB)
   3. Get the build-environment dependencies. They are required for compiling Google-Filament.
      - `clang-14` **or higher**
      - `libglu1-mesa-dev`
      - `libc++-14-dev` (`libcxx-devel` and `libcxx-static` on Fedora) **or higher**
      - `libc++abi-14-dev` (`libcxxabi-static` on Fedora) **or higher**
      - `ninja-build`
      - `libxi-dev`
      - `libxcomposite-dev` (`libXcomposite-devel` on Fedora)
      - `libxxf86vm-dev` (`libXxf86vm-devel` on Fedora)
   4. Compile `nob.c` with your favorite C-compiler. You only have to do this once, `nob.c` recompiles automatically when changed.
      - `cc nob.c -o nob` should work on Linux.
      - *Optionally* do `./nob help` to learn more about this build tool.
   5. Compile Goolge-Filament. *This might take a while (10-20 mins).* **AND** compile `libenvironment.so`.
      - `./nob filament libenv`
   7. `cd ..` return to the `TinyDronesSim/` directory 
3. Install [Julia](https://julialang.org/downloads/).
4. `julia --project=.` starting julia and activating the project (don't forget the `=.` !!)
5. `using Pkg; Pkg.add(["StaticArrays", "StaticStrings"])` adding dependencies. This only needs to be done **once**.
6. `include("examples/environment_test.jl")` executing an example script

## Using TinyDronesSim



## TODOS
- [x] Bugs with high fps (much faster walking and simulation)
- [x] implement `set_active_environment()`
- [x] Line drawing
- [x] fix annoying unsafe_convert to Cstring
- [x] change material parameter to material name of previously added material instead of settings fro new material
- [x] Better internal managing of objects (destroy all option + hash-stored id instead of pointer)
- [x] Replace *_Settings structs with function arguments
- [x] reimplement activating things like environment and window (to avoid stupid UUID conversion)
- [x] add controller input support
- [x] Refactor single robin_map to many maps, one for each object type, while keeping UUID
- [] implement Mesh constructor
