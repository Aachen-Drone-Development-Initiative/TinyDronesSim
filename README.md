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
   1. `cd TinyDronesSim/EnvironmentBackend`
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
3. Install [Julia](https://julialang.org/install/).
4. `julia --project=.` starting julia and activating the project (don't forget the `=.` !!)
5. `using Pkg; Pkg.add(["StaticArrays", "StaticStrings"])` adding dependencies. This only needs to be done **once**.
6. `include("examples/environment_test.jl")` executing an example script

## Using TinyDronesSim

At the center of the TinyDronesSim-Workflow is the Julia-REPL (the Julia command line interface).
You should get to know the Julia-REPL it's very powerful.  
Here are some resouces:
- Julia REPL Mastery Workshop: https://www.youtube.com/watch?v=bHLXEUt5KLc&t=2475s
- If you prefer reading: https://github.com/miguelraz/REPLMasteryWorkshop
Some important basics:
- `varinfo()` - show all global variables, structs, functions

#### Basic Workflow example

1. Create a .jl file (let's call it `defs.jl`) for your definitions of objects and their functions. 
   Get inspiration from `./examples/simple_drone_declaration.jl`
2. Create another .jl (let's call it `objs.jl`) where you create all global objects, for example `env = Env.create_environment()` or `drone = MyDrone()`.
   Here you can also create your `sim_loop()` function for showing and updating your window(s) and advancing the simulation.
   Get inspiration from `./examples/simple_drone_simulation.jl`
3. Start the Julia REPL and `include("defs.jl")` `include("objs.jl")`.
4. Run the `sim_loop()`, stop the sim, inspect some values, plot some things, change some functions in `defs.jl`, reinclude `defs.jl`.
   Run the sim, have an idea that requires a new object field, stop the sim, make the change, restart Julia
   reinclude `defs.jl` and `objs.jl` run the sim ...
   
Sadly, when changing structs, you need to restart Julia (This will be fixed with Julia 1.12.0).
