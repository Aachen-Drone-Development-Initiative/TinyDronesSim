
export has_pos,
    has_mass,
    has_velocity,
    has_orientation,
    has_angular_velocity,
    
    get_pos,
    get_mass,
    get_orientation,
    get_velocity,
    get_angular_velocity,
    set_pos!,
    set_mass!,
    set_orientation!,
    set_velocity!,
    set_angular_velocity!,
    get_expr_relative_pos,
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

### Attributes ###

function has_pos(::ObjT) where {ObjT <: SimObject} return false end
function has_mass(::ObjT) where {ObjT <: SimObject} return false end
function has_velocity(::ObjT) where {ObjT <: SimObject} return false end
function has_orientation(::ObjT) where {ObjT <: SimObject} return false end
function has_angular_velocity(::ObjT) where {ObjT <: SimObject} return false end

## Physical Properties ##

function get_pos(obj::ObjT)::Vec3f where {ObjT <: SimObject} end
function get_mass(obj::ObjT)::Float64 where {ObjT <: SimObject} end
function get_velocity(obj::ObjT)::Vec3f where {ObjT <: SimObject} end
function get_orientation(obj::ObjT)::Quaternion where {ObjT <: SimObject} end
function get_angular_velocity(obj::ObjT)::Vec3f where {ObjT <: SimObject} end

function set_pos!(obj::ObjT, x::Vec3f) where {ObjT <: SimObject} end
function set_mass!(obj::ObjT, m::Float64) where {ObjT <: SimObject} end
function set_velocity!(obj::ObjT, u::Vec3f) where {ObjT <: SimObject} end
function set_orientation!(obj::ObjT, r::Quaternion) where {ObjT <: SimObject} end
function set_angular_velocity!(obj::ObjT, angular_u::Vec3f) where {ObjT <: SimObject} end

function get_expr_relative_pos(exprs_objs::Symbol...)::Expr end
@generated function get_relative_pos(obj_list...) end

@generated function get_total_mass(obj::ObjT) where {ObjT <: SimObject} end

function get_expr_center_of_mass(obj_name::Symbol, obj_type::Type)::Expr end
@generated function get_center_of_mass(obj::ObjT) where {ObjT <: SimObject} end
@generated function get_center_of_mass_local_frame(obj::ObjT) where {ObjT <: SimObject} end

@generated function get_inertia_matrix(obj::ObjT) where {ObjT <: SimObject} end
function get_inertia_matrix_point_mass(obj::ObjT, center_of_mass::Vec3f)::Mat33f where {ObjT <: SimObject} end

## Mechanical Reaction ##

# returns the mechanical reaction in the reference frame of objects on the same hierarchy level.
@generated function get_mechanical_reaction_parent_frame(obj::ObjT) where {ObjT <: SimObject} end

function get_mechanical_reaction_local_frame_component()::Resultant3D end
function get_mechanical_reaction_parent_frame_component()::Resultant3D end

function get_gravity_reaction_parent_frame_component(obj::ObjT)::Resultant3D where {ObjT <: SimObject} end

## Time Integration ##

function integrate_physics_euler!(obj::ObjT, dt::Float64) where {ObjT <: SimObject} end
