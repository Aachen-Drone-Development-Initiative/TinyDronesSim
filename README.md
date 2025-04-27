# WORK IN PROGRESS!

# TinyDronesSim

TinyDronesSim is a tool for simulating the *tinydrone*.  

## What this tool should do

1. Very simple mechanics for generic quadrocopter-like behaviour (low speed regime)
2. Interactive visualization of the drone and other information
3. Controlling of the virtual drone with gamepad
3. Easy integration and testing of the drones source code

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


## Getting started

1. Install [Julia](https://julialang.org/downloads/).
2. `git clone https://github.com/Aachen-Drone-Development-Initiative/TinyDronesSim.git`
3. `cd TinyDronesSim`
4. `julia --project=.` starting julia and activating the project
5. `using Pkg`
6. `Pkg.add(["StaticArrays"])` adding dependencies
7. `include("examples/drone_simulation.jl")` executing an example script

I recommend installing Julia tooling for the editor of your choice. With the default Julia setup you would have to repeat steps 4 and 7 every time you change the code.
