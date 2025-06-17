
#####################
# Vectors
#####################

# some vectors are already defined in "./TinyDronesSimBackend.jl"
const Float64_4 = SVector{4, Float64}

vec_cross(a::V, b::V) where {V} = V(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
vec_norm2(a) = sqrt(a'a)
vec_normalize(a) = a ./ vec_norm2(a)

#####################
# Matrices
#####################

const Float64_3x3 = SMatrix{3, 3, Float64}

mat33f_zero() = Float64_3x3([0 0 0; 0 0 0; 0 0 0])

#####################
# Quaternions
#####################

# based on: https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html
# note that we use a different quaternion memory layout than in the above source:
# q = w + x * i + y * j + z * k

# definition for Quaternion is in "./TinyDronesSimBackend.jl"

quat_identity() = Quaternion(0.0, 0.0, 0.0, 1.0)

# create quaternion for rotating 90 degrees clock-wise/counter-clock-wise around the x, y or z axis
quat_ccw_x(radians) = quat_from_normed_axis_angle(radians, Float64_3(1.0, 0.0, 0.0))
quat_cw_x(radians) = quat_from_normed_axis_angle(-radians, Float64_3(1.0, 0.0, 0.0))
quat_ccw_y(radians) = quat_from_normed_axis_angle(radians, Float64_3(0.0, 1.0, 0.0))
quat_cw_y(radians) = quat_from_normed_axis_angle(-radians, Float64_3(0.0, 1.0, 0.0))
quat_ccw_z(radians) = quat_from_normed_axis_angle(radians, Float64_3(0.0, 0.0, 1.0))
quat_cw_z(radians) = quat_from_normed_axis_angle(-radians, Float64_3(0.0, 0.0, 1.0))

"Convert axis angles (which need to be normed first) to a quaternion."
quat_from_normed_axis_angle(θ, v) = Quaternion((v .* sin(θ * 0.5))..., cos(θ * 0.5))

"The magnitude of 'v' is interpreted as the angle of rotation in radians ('theta')."
function quat_from_axis_angle(v)
    θ = vec_norm2(v)
    if θ > eps(typeof(θ))
        return Quaternion(((v ./ θ) * sin(θ * 0.5))..., cos(θ * 0.5))
    end
    return quat_identity()
end

function quat_to_normed_axis_angle(q::Quaternion)::Tuple{Float64, Float64_3}
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

quat_conjugate(q::Quaternion) = Quaternion(-q.x, -q.y, -q.z, q.w)

"The resulting quaternion behaves as if q is applied *first* and r is applied *afterterwards*."
function quat_combine(q::Quaternion, r::Quaternion)::Quaternion
    return q * r
end

function quat_normalize(q::Quaternion)::Quaternion
    d = sqrt(q.x^2 + q.y^2 + q.z^2 + q.w^2)
    return Quaternion(q.x / d, q.y / d, q.z / d, q.w / d)
end

function quat_rotate_vector(v::V, r::Quaternion)::V where {V}
    q = r * Quaternion(v.x, v.y, v.z, 0.0) * quat_conjugate(r)
    return V(q.x, q.y, q.z)
end
