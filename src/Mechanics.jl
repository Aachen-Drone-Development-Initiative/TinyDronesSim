
export get_pos,
    get_mass,
    get_orientation,
    get_velocity,
    get_angular_velocity,
    set_pos!,
    set_mass!,
    set_orientation!,
    set_velocity!,
    set_angular_velocity!,
    
    get_relative_pos,
    get_total_mass,
    get_expr_center_of_mass,
    get_center_of_mass,
    get_center_of_mass_local_frame,
    get_inertia_matrix,
    get_inertia_matrix_point_mass,
    
    get_mechanical_reaction_parent_frame,
    get_mechanical_reaction_local_frame_component,
    get_mechanical_reaction_parent_frame_component,
    get_gravity_reaction_parent_frame_component,

    integrate_physics_euler!

include("MechanicsTypes.jl")

### Mechanics Interface ###

# define these functions for your '<: Generic' object to make it compatible with
# the mechanics implementation of TinyDronesSim
# 1) Not all functions need to be defined
# 2) You can also overwrite all generic functions listed below this section

function get_pos(obj::Nothing)::Vec3f end
function get_mass(obj::Nothing)::Float64 end
function get_velocity(obj::Nothing)::Vec3f end
function get_orientation(obj::Nothing)::Quaternion end
function get_angular_velocity(obj::Nothing)::Vec3f end

function set_pos!(obj::Nothing, x::Vec3f) end
function set_mass!(obj::Nothing, m::Float64) end
function set_velocity!(obj::Nothing, u::Vec3f) end
function set_orientation!(obj::Nothing, r::Quaternion) end
function set_angular_velocity!(obj::Nothing, angular_u::Vec3f) end

function get_mechanical_reaction_local_frame_component(obj::Nothing)::Resultant3D end
function get_mechanical_reaction_parent_frame_component(obj::Nothing)::Resultant3D end

### Generic Mechanics Functions ###

@generated function get_relative_pos(obj_list...) end
@generated function get_total_mass(obj::ObjT) where {ObjT <: Generic} end

function get_expr_center_of_mass(obj_name::Symbol, obj_type::Type)::Expr end
@generated function get_center_of_mass(obj::ObjT) where {ObjT <: Generic} end
@generated function get_center_of_mass_local_frame(obj::ObjT) where {ObjT <: Generic} end

@generated function get_inertia_matrix(obj::ObjT) where {ObjT <: Generic} end
function get_inertia_matrix_point_mass(obj::ObjT, center_of_mass::Vec3f)::Mat33f where {ObjT <: Generic} end

## Mechanical Reaction ##

# returns the mechanical reaction in the reference frame of objects on the same hierarchy level.
@generated function get_mechanical_reaction_parent_frame(obj::ObjT) where {ObjT <: Generic} end

function get_gravity_reaction_parent_frame_component(obj::ObjT)::Resultant3D where {ObjT <: Generic} end

## Time Integration ##

function integrate_physics_euler!(obj::ObjT, dt::Float64) where {ObjT <: Generic} end
