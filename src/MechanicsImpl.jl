const tds = TinyDronesSim

### Resultant3D implementation ###

tds.set_pos!(resultant::Resultant3D, pos::Vec3f) = resultant.pos = pos

function combine_resultants(pos::Vec3f, resultants::Resultant3D...)::Resultant3D
    resultant = Resultant3D(pos=pos)
    for i in 1:length(resultants)
        distance = euclid_norm(resultants[i].pos)
        resultant.force += resultants[i].force
        resultant.torque += resultants[i].torque + cross_product(resultants[i].pos - pos, resultants[i].force)
    end
    return resultant
end

function rotate_resultant(resultant::Resultant3D, rotation::Quaternion)::Resultant3D
    return Resultant3D(rotate(resultant.force, rotation),
                       rotate(resultant.torque, rotation),
                       resultant.pos)
end

tds.get_pos(obj::MechanicalPoint)::Vec3f      = obj.x
tds.get_mass(obj::MechanicalPoint)::Float64   = obj.m
tds.get_velocity(obj::MechanicalPoint)::Vec3f = obj.u
tds.set_pos!(obj::MechanicalPoint, x::Vec3f)      = obj.x = x
tds.set_mass!(obj::MechanicalPoint, m::Float64)   = obj.m = m
tds.set_velocity!(obj::MechanicalPoint, u::Vec3f) = obj.u = u

function tds.render(ctxid::RenderContextID, obj::MechanicalPoint)::Nothing
    render(ctxid, obj.x, 0.01, BLACK)
end

### Generic Mechanics Functions ###

@generated function get_relative_pos(obj_list...)
    expr = :(Vec3f(0, 0, 0))
    expr_parent_orientation = :(identity_quaternion())
    for i in 1:length(obj_list)
        if @has(get_pos, obj_list[i])
            expr = :($expr + rotate(get_pos(obj_list[$i]), $expr_parent_orientation))
            if @has(get_orientation, obj_list[i])
                expr_parent_orientation = :(combine(get_orientation(obj_list[$i]), $expr_parent_orientation))
            end
        end
    end
    return expr
end

@generated function tds.get_total_mass(obj::ObjT) where {ObjT <: Generic}
    exprs_mass = find_fields_for_method(ObjT, get_mass)
    if isempty(exprs_mass)
        @error_nonsensecall(obj, "get_total_mass", "no child objects with 'get_mass'")
    else
        for i in 1:length(exprs_mass)
            exprs_mass[i] = Meta.parse("get_mass(obj.$(exprs_mass[i]))")
        end
        return Expr(:call, :+, :($(exprs_mass...)))
    end
end

# TODO: Need to rework this function, dont want any get_expr functions
# TODO: THIS FUNCTION DOESNT WORK. CANT CHECK FOR TYPE ATTRIBUTES LIKE HAS_MASS ...
function tds.get_expr_center_of_mass(obj_name::Symbol, obj_type::Type)
    if @has(get_pos, obj_type) && @has(get_mass, obj_type)
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

@generated function tds.get_center_of_mass(obj::ObjT) where {ObjT <: Generic}
    return get_expr_center_of_mass(:obj, ObjT)
end

@generated function tds.get_center_of_mass_local_frame(obj::ObjT) where {ObjT <: Generic}
    return :($(get_expr_center_of_mass(:obj, ObjT)) - get_pos(obj))
end
    
@generated function tds.get_inertia_matrix(obj::ObjT) where {ObjT <: Generic}
    exprs_fields = find_fields_for_method_multiargs(ObjT, FunctionCall(get_inertia_matrix, (), (Vec3f,)))
    if isempty(exprs_fields)
        @error_nonsensecall(obj, "get_inertia_matrix")
    else
        exprs_inertia = :($(zero_mat33f()))
        for i in 1:length(exprs_fields)
            exprs_inertia = Meta.parse("$exprs_inertia + get_inertia_matrix(obj.$(exprs_fields[i]), center_of_mass_local)")
        end
        return quote
            center_of_mass_local = get_center_of_mass_local_frame(obj)
            return $exprs_inertia
        end
    end
end

function tds.get_inertia_matrix_point_mass(obj::ObjT, center_of_mass::Vec3f)::Mat33f where {ObjT <: Generic}
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
@generated function tds.get_mechanical_reaction_parent_frame(obj::ObjT) where {ObjT <: Generic}
    expr = :(:nothing)
    
    is_defined_local_frame_reaction = @has(get_mechanical_reaction_local_frame_component, ObjT)
    is_defined_parent_frame_reaction = @has(get_mechanical_reaction_parent_frame_component, ObjT)

    if is_defined_local_frame_reaction && is_defined_parent_frame_reaction
        if is_defined_local_frame_reaction
            expr = :(resultant = get_mechanical_reaction_local_frame_component(obj))
            
            if (@has(get_orientation, ObjT))
                expr = quote
                    $expr
                    resultant = rotate_resultant(resultant, get_orientation(obj))
                end
            end

            if (@has(get_pos, ObjT))
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
        # they are local, because 'parent of child' is local to the parent again
        exprs_local = find_fields_for_method(ObjT, get_mechanical_reaction_parent_frame)
        if (isempty(exprs_local))
            @error_nonsensecall(obj, "get_mechanical_reaction_parent_frame", "Obj doesn't have explicit mechanical-reaction-components, \
                                                                              but no child has get_mechanical_reaction_parent_frame either")
        else
            for i in 1:length(exprs_local)
                exprs_local[i] = Meta.parse("get_mechanical_reaction_parent_frame(obj.$(exprs_local[i]))")
            end
            expr = quote
                $expr
                resultant = combine_resultants(center_of_mass_local, $(exprs_local...))
            end
            
            if @has(get_orientation, ObjT)
                expr = quote
                    $expr
                    resultant = rotate_resultant(resultant, get_orientation(obj))
                end
            end

            if @has(get_pos, ObjT)
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
    end
    return expr
end

function tds.get_gravity_reaction_parent_frame_component(obj::ObjT)::Resultant3D where {ObjT <: Generic}
    force = Vec3f(0, 0, -get_mass(obj) * gravitational_acceleration)
    return Resultant3D(force, Vec3f(0, 0, 0), get_pos(obj))
end

## Time Integration ##

function tds.integrate_physics_euler!(obj::ObjT, dt::Float64) where {ObjT <: Generic}
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
