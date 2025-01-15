
# TinyDronesSim

This project aims to be a flexible software development foundation for tinyDrones projects.  

## Overview

The 'main loop' of any TinyDronesSim-derived project is inside a Julia file/script.
Here types, objects and functions/overloads can be defined to describe your project.
You can also call external code, for example with the convenient [C-interface](https://docs.julialang.org/en/v1/manual/calling-c-and-fortran-code/).
TinyDronesSim essentially consists of a set of predefined generic or specific functions and types,
which are supposed to simplify your job and provide some structure.
Currently there are implementations for basic mechanics on somewhat generic objects and for very simple 3D rendering.

## Getting started

1. Install [Julia](https://julialang.org/downloads/).
2. `git clone https://github.com/Aachen-Drone-Development-Initiative/TinyDronesSim.git`
3. `cd TinyDronesSim`
4. `julia --project=.`
5. `include("examples/Example.jl")`

Step (4) and (5) need to be repeated everytime you want to start/restart julia.
This is very cumbersum, because a restart is required everytime you change the julia code.
You can use [Revise](https://timholy.github.io/Revise.jl/stable/) to recompile automatically,
but this won't erase the current state.

## Basic usage

### Conventions

#### local and parent frame of reference

Consider the object:
```julia
struct Obj
    x::Vec3f
    r::Quaternion
    m::Float64
end
```
`x` Is the position and `r` the orientation relative to its parent object, which is the object 
that contains an object of type `Obj` (can also be the global scope).
Anything which consideres `x` and `r` is in the parents frame of reference and anything which 
doesn't consider it, is in the local frame of reference.
In other words the local frame of reference is whatever must be moved by `x` and then rotated 
by `r` to arrive at the objects position/orientation relative to its parent.

### Mechanics

#### `get_mechanical_reaction_parent_frame(obj::ObjT)`

This function attempts to calculate a resultant for `obj` with type `ObjT <: SimObject`.
The resultant contains a force and momentum vector at the center of mass of `obj`, in the parent frame of reference.
It essentially combines all resultants recursively downwards from `obj`, each object (`obj` included) can have
overloads for their type for the functions `get_mechanical_reaction_local_frame_component(obj)` and/or
`get_mechanical_reaction_parent_frame_component(obj)`, which each return a resultant in the respective frame of reference.
The child objects will only be checked recursively, if both are not defined for an objects type.
