
## Physical Properties ##

function TinyDronesSim.get_pos(obj::ObjT)::Vec3f where {ObjT <: SimObject}              obj.x end
function TinyDronesSim.get_mass(obj::ObjT)::Float64 where {ObjT <: SimObject}           obj.m end
function TinyDronesSim.get_velocity(obj::ObjT)::Vec3f where {ObjT <: SimObject}         obj.u end
function TinyDronesSim.get_orientation(obj::ObjT)::Quaternion where {ObjT <: SimObject} obj.r end
function TinyDronesSim.get_angular_velocity(obj::ObjT)::Vec3f where {ObjT <: SimObject} obj.angular_u end

function TinyDronesSim.set_pos!(obj::ObjT, x::Vec3f) where {ObjT <: SimObject}                      obj.x = x end
function TinyDronesSim.set_mass!(obj::ObjT, m::Float64) where {ObjT <: SimObject}                   obj.m = m end
function TinyDronesSim.set_velocity!(obj::ObjT, u::Vec3f) where {ObjT <: SimObject}                 obj.u = u end
function TinyDronesSim.set_orientation!(obj::ObjT, r::Quaternion) where {ObjT <: SimObject}         obj.r = r end
function TinyDronesSim.set_angular_velocity!(obj::ObjT, angular_u::Vec3f) where {ObjT <: SimObject} obj.angular_u = angular_u end

#=
Returns an expression for the position of a child object relative to its parent / parent of parent / etc.
Requires the list of parents up until the relevant parent for example (car, seat, seatbelt) will return the
position of the seatbelt relative to the car. Supposed to be used in combination with 'find_fields_for_method_v' etc.
=#
function get_expr_relative_pos(typed_field_sets::Vector{Tuple{Symbol, DataType}})::Expr # FIXME: calculation wrong, cant just add positions together, must rotate
    expr = :(Vec3f(0, 0, 0))
    for i in 1:length(typed_field_sets)
        if has_pos(typed_field_sets[i][2]())
            typed_field_set = Vector{Tuple{Symbol, DataType}}()
            for j in 1:i
                push!(typed_field_set, typed_field_sets[j])
            end
            
            expr = :($expr + get_pos($(field_set_to_field_expr(typed_field_set))))
        end
    end
    return expr
end

@generated function get_relative_pos(obj_list...)
    expr = :(Vec3f(0, 0, 0))
    expr_parent_orientation = :(identity_quaternion())
    for i in 1:length(obj_list)
        if has_pos(obj_list[i]())
            expr = :($expr + rotate(get_pos(obj_list[$i]), $expr_parent_orientation))
            if has_orientation(obj_list[i]())
                expr_parent_orientation = :(combine(get_orientation(obj_list[$i]), $expr_parent_orientation))
            end
        end
    end
    return expr
end

@generated function TinyDronesSim.get_total_mass(obj::ObjT) where {ObjT <: SimObject}
    exprs_mass = find_fields_for_method(ObjT, get_mass)
    for i in 1:length(exprs_mass)
        exprs_mass[i] = Meta.parse("get_mass(obj.$(exprs_mass[i]))")
    end
    if length(exprs_mass) > 0
        return Expr(:call, :+, :($(exprs_mass...)))
    else
        return :(0)
    end
end

# TODO: THIS FUNCTION DOESNT WORK. CANT CHECK FOR TYPE ATTRIBUTES LIKE HAS_MASS ...

function TinyDronesSim.get_expr_center_of_mass(obj_name::Symbol, obj_type::Type)
    if has_pos(obj_type()) && has_mass(obj_type())
        return :(get_pos($obj_name))
    end
    field_sets = find_fields_for_methods_v(obj_type, [get_pos, get_mass])
    expr_total_mass = :(Float64(0))
    expr_weighted_x = :(Vec3f(0,0,0))
    if length(field_sets) > 0
        for i in 1:length(field_sets)
            incremental_field_list = Vector{Union{Expr, Symbol}}()
            push!(incremental_field_list, obj_name)
            for j in 1:length(field_sets[i])
                push!(incremental_field_list, :($(incremental_field_list[end]).$(field_sets[i][j])))
            end
            expr_weighted_x = :($expr_weighted_x + get_mass($obj_name.$(field_set_to_field_expr(field_sets[i]))) .*
                                      (get_relative_pos($(incremental_field_list...))))
            expr_total_mass = :($expr_total_mass + get_mass($obj_name.$(field_set_to_field_expr(field_sets[i]))))
        end
        return :($expr_weighted_x ./ $expr_total_mass)
    else
        return :(Vec3f(0, 0, 0))
    end
end

# function TinyDronesSim.get_expr_center_of_mass(obj_name::Symbol, obj_type::Type)
#     if has_pos(obj_type()) && has_mass(obj_type())
#         return :(get_pos($obj_name))
#     end
#     typed_field_sets = find_fields_for_methods_v_t(obj_type, [get_pos, get_mass])
#     expr_total_mass = :(Float64(0))
#     expr_weighted_x = :(Vec3f(0,0,0))
#     if length(typed_field_sets) > 0
#         for i in 1:length(typed_field_sets)
#             expr_weighted_x = Meta.parse("$expr_weighted_x + get_mass($obj_name.$(field_set_to_field_expr(typed_field_sets[i]))) .* \
#                                       ($(get_expr_relative_pos([(obj_name, obj_type), typed_field_sets[i]...])))")
#             expr_total_mass = Meta.parse("$expr_total_mass + get_mass($obj_name.$(field_set_to_field_expr(typed_field_sets[i])))")
#         end
#         return :($expr_weighted_x ./ $expr_total_mass)
#     else
#         return :(Vec3f(0, 0, 0))
#     end
# end

@generated function TinyDronesSim.get_center_of_mass(obj::ObjT) where {ObjT <: SimObject}
    return get_expr_center_of_mass(:obj, ObjT)
end

@generated function TinyDronesSim.get_center_of_mass_local_frame(obj::ObjT) where {ObjT <: SimObject}
    return :($(get_expr_center_of_mass(:obj, ObjT)) - get_pos(obj))
end
    
@generated function TinyDronesSim.get_inertia_matrix(obj::ObjT) where {ObjT <: SimObject}
    exprs_fields = find_fields_for_method_multiargs(ObjT, FunctionCall(get_inertia_matrix, (), (Vec3f,)))
    exprs_inertia = :($(zero_mat33f()))
    for i in 1:length(exprs_fields)
        exprs_inertia = Meta.parse("$exprs_inertia + get_inertia_matrix(obj.$(exprs_fields[i]), center_of_mass_local)")
    end
    return quote
        center_of_mass_local = get_center_of_mass_local_frame(obj)
        return $exprs_inertia
    end
end

function TinyDronesSim.get_inertia_matrix_point_mass(obj::T, center_of_mass::Vec3f)::Mat33f where {T <: SimObject}
    c = center_of_mass
    x = get_pos(obj)
    m = get_mass(obj)
    return Mat33f([
        m * ((x.y - c.y)^2 + (x.z - c.z)^2), m * (x.x - c.x) * (x.y - c.y),       m * (x.x - c.x) * (x.z - c.z),
        m * (x.x - c.x) * (x.y - c.y),       m * ((x.x - c.x)^2 + (x.z - c.z)^2), m * (x.y - c.y) * (x.z - c.z),
        m * (x.x - c.x) * (x.z - c.z),       m * (x.y - c.y) * (x.z - c.z),       m * ((x.x - c.x)^2 + (x.y - c.y)^2)
    ])
end

## Mechanical Reaction ##

# returns the mechanical reaction in the reference frame of objects on the same hierarchy level.
@generated function TinyDronesSim.get_mechanical_reaction_parent_frame(obj::ObjT) where {ObjT <: SimObject}
    expr = :(:nothing)
    
    is_defined_local_frame_reaction = hasmethod(get_mechanical_reaction_local_frame_component, (ObjT,))
    is_defined_parent_frame_reaction = hasmethod(get_mechanical_reaction_parent_frame_component, (ObjT,))

    if is_defined_local_frame_reaction && is_defined_parent_frame_reaction
        if is_defined_local_frame_reaction
            expr = :(resultant = get_mechanical_reaction_local_frame_component(obj))
            
            if (has_orientation(ObjT()))
                expr = quote
                    $expr
                    resultant = rotate_resultant(resultant, get_orientation(obj))
                end
            end

            if (has_pos(ObjT()))
                expr = quote
                    $expr
                    set_pos!(resultant, get_pos(obj))
                end
            end
        end

        if is_defined_parent_frame_reaction
            expr = quote
                resultant = Vec3f(0,0,0)
                $expr
                return combine_resultants(get_center_of_mass(obj), resultant, get_mechanical_reaction_parent_frame_component(obj))
            end
        end
    else
        expr = :(center_of_mass_local = get_center_of_mass_local_frame(obj))
        # they are local, because the parent of the child is local to the parent again
        exprs_local = find_fields_for_method(ObjT, get_mechanical_reaction_parent_frame)
        for i in 1:length(exprs_local)
            exprs_local[i] = Meta.parse("get_mechanical_reaction_parent_frame(obj.$(exprs_local[i]))")
        end
        expr = quote
            $expr
            resultant = combine_resultants(center_of_mass_local, $(exprs_local...))
        end
        
        if (has_orientation(ObjT()))
            expr = quote
                $expr
                resultant = rotate_resultant(resultant, get_orientation(obj))
            end
        end

        if (has_pos(ObjT()))
            expr = quote
                $expr
                set_pos!(resultant, get_pos(obj))
            end
        end
        expr = quote
            $expr
            return resultant
        end
    end
    return expr
end

function TinyDronesSim.get_gravity_reaction_parent_frame_component(obj::ObjT)::Resultant3D where {ObjT <: SimObject}
    force = Vec3f(0, 0, -get_mass(obj) * gravitational_acceleration)
    return Resultant3D(force, Vec3f(0, 0, 0), get_pos(obj))
end

## Time Integration ##

function TinyDronesSim.integrate_physics_euler!(obj::ObjT, dt::Float64) where {ObjT <: SimObject}
    if (!has_pos(ObjT())
        || !has_velocity(ObjT())
        || !has_angular_velocity(ObjT())
        || !has_orientation(ObjT()))
        @error("Requires has_pos, has_velocity, has_angular_velocity, has_orientation")
    end
    
    resultant = get_mechanical_reaction_parent_frame(obj)

    # update velocity, position
    mass = get_total_mass(obj)
    u_dot = resultant.force / mass

    set_velocity!(obj, get_velocity(obj) + u_dot * dt)
    set_pos!(obj, get_pos(obj) + get_velocity(obj) * dt)
    
    # update angular_velocity, orientation
    I = get_inertia_matrix(obj)
    angular_u = get_angular_velocity(obj)
    angular_u_dot = inv(I) * (resultant.torque - cross_product(angular_u, I * angular_u))
    set_angular_velocity!(obj, angular_u + angular_u_dot * dt)
    
    orientation_change = get_angular_velocity(obj) * dt
    theta = euclid_norm(orientation_change) # rotation around the axis
    
    set_orientation!(obj, combine(get_orientation(obj), normed_axis_angle_to_quaternion(theta, orientation_change ./ theta)))
end
