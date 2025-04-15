# Quaternions for rotation
# based on: https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html

export Quaternion,
    identity_quaternion,
    quaternion_ccw_x,
    quaternion_cw_x,
    quaternion_ccw_y,
    quaternion_cw_y,
    quaternion_ccw_z,
    quaternion_cw_z,
    combine,
    rotate_vector,
    inv_rotate_vector,
    conjugate,
    normed_axis_angle_to_quaternion,
    axis_angle_to_quaternion,
    quaternion_to_normed_axis_angle

# q = w + x * i + y * j + z * k
# this is conform with filaments Quaternion memory layout
struct Quaternion
    x::Float64
    y::Float64
    z::Float64
    w::Float64
end

identity_quaternion() = Quaternion(0.0, 0.0, 0.0, 1.0)

# create quaternion for rotating 90 degrees clock-wise/counter-clock-wise around the x, y or z axis
quaternion_ccw_x(radians::Float64) = normed_axis_angle_to_quaternion(radians, Float64_3(1.0, 0.0, 0.0))
quaternion_cw_x(radians::Float64) = normed_axis_angle_to_quaternion(-radians, Float64_3(1.0, 0.0, 0.0))
quaternion_ccw_y(radians::Float64) = normed_axis_angle_to_quaternion(radians, Float64_3(0.0, 1.0, 0.0))
quaternion_cw_y(radians::Float64) = normed_axis_angle_to_quaternion(-radians, Float64_3(0.0, 1.0, 0.0))
quaternion_ccw_z(radians::Float64) = normed_axis_angle_to_quaternion(radians, Float64_3(0.0, 0.0, 1.0))
quaternion_cw_z(radians::Float64) = normed_axis_angle_to_quaternion(-radians, Float64_3(0.0, 0.0, 1.0))

# convert axis angles (which need to be normed first) to a quaternion
normed_axis_angle_to_quaternion(theta::Float64, v::Float64_3) = Quaternion((v .* sin(theta * 0.5))..., cos(theta * 0.5))

# The magnitude of 'v' is interpreted as the angle of rotation in radians ('theta')
function axis_angle_to_quaternion(v::Float64_3, epsilon::Float64)
    theta = euclid_norm(v)
    if theta > epsilon
        return Quaternion(((v ./ theta) * sin(theta * 0.5))..., cos(theta * 0.5))
    end
    return identity_quaternion()
end

function quaternion_to_normed_axis_angle(q::Quaternion)::Tuple{Float64, Float64_3}
    theta = 2.0 * acos(q.w)
    return theta, Float64_3(q.x, q.y, q.z) ./ sin(theta * 0.5)
end

import Base.+
(+)(q::Quaternion, r::Quaternion) = Quaternion(q.x + r.x, q.y + r.y, q.z + r.z, q.w + r.w)

import Base.*
(*)(q::Quaternion, r::Quaternion) = Quaternion(
    q.w * r.x + q.x * r.w - q.y * r.z + q.z * r.y,
    q.w * r.y + q.x * r.z + q.y * r.w - q.z * r.x,
    q.w * r.z - q.x * r.y + q.y * r.x + q.z * r.w,
    q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z)

conjugate(q::Quaternion) = Quaternion(-q.x, -q.y, -q.z, q.w)

# The resulting quaternion behaves as if q is applied *first* and r is applied *afterterwards*
function combine(q::Quaternion, r::Quaternion)::Quaternion
    return q * r
end

function rotate_vector(v::Float64_3, r::Quaternion)::Float64_3
    q = r * Quaternion(v.x, v.y, v.z, 0.0) * conjugate(r)
    return Float64_3(q.x, q.y, q.z)
end

function inv_rotate_vector(v::Float64_3, r::Quaternion)::Float64_3
    q = Quaternion(v.x, v.y, v.z, 0.0)
    return conjugate(r) * q * r
end
