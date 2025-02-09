# WORK IN PROGRESS!

# TinyDronesSim

This project aims to be a flexible software development foundation for tinyDrones projects.  

## Overview

The 'entry point' of any project based on TinyDronesSim is inside a Julia file/script,
where you define types objects and functions to describe whatever you want to do.
TinyDronesSim is designed to assist you by providing interfaces for your objects,
which integrate them into an expanding set of useful functionalities like 3D visualization or rigid body mechanics.
Interfaces are really just functions intended to be overloaded, you could for example define `get_mass(obj::MyType)`
to let everybody know that your object has a mass.
You can also easily put low-level code into action, because of Julias straightforward
[interoperatability with languages like C](https://docs.julialang.org/en/v1/manual/calling-c-and-fortran-code/).

## Getting started

1. Install [Julia](https://julialang.org/downloads/).
2. `git clone https://github.com/Aachen-Drone-Development-Initiative/TinyDronesSim.git`
3. `cd TinyDronesSim`
4. `julia --project=.`
5. `include("examples/drone_simulation.jl")`

Step (4) and (5) need to be repeated everytime you want to start/restart julia.
This is very cumbersum, because a restart is required everytime you change the julia code.
You can use [Revise](https://timholy.github.io/Revise.jl/stable/) to recompile automatically,
but this won't erase the current state.

## TinyDronesRender

### Build

run `./build.bat` in `/TinyDronesRender`
