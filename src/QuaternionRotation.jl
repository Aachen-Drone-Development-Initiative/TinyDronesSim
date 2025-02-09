# Quaternions for rotation
# based on: https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html

export Quaternion,
    combine,
    rotate,
    normed_axis_angle_to_quaternion,
    axis_angle_to_quaternion,
    identity_quaternion

struct Quaternion
    real::Float64
    im::Vec3f
end

identity_quaternion() = Quaternion(1, Vec3f(0, 0, 0))

# convert axis angles (which need to be normed first) to a quaternion
normed_axis_angle_to_quaternion(theta::Float64, v::Vec3f) = Quaternion(cos(theta * 0.5), v * sin(theta * 0.5))

# the magnitude of the vector is theta, returns the identity quaternion if theta is smaller than epsilon
function axis_angle_to_quaternion(v::Vec3f, epsilon::Float64)
    theta = euclid_norm(v)
    if theta > epsilon
        return Quaternion(cos(theta * 0.5), (v ./ theta) * sin(theta * 0.5))
    end
    return identity_quaternion()
end

import Base.+
(+)(q::Quaternion, r::Quaternion) = Quaternion(q.real + r.real, q.im + r.im)

import Base.*
(*)(q::Quaternion, r::Quaternion) = Quaternion(
    q.real * r.real - q.im.x * r.im.x - q.im.y * r.im.y - q.im.z * r.im.z,
    Vec3f(q.real * r.im.x + q.im.x * r.real - q.im.y * r.im.z + q.im.z * r.im.y,
          q.real * r.im.y + q.im.x * r.im.z + q.im.y * r.real - q.im.z * r.im.x,
          q.real * r.im.z - q.im.x * r.im.y + q.im.y * r.im.x + q.im.z * r.real))

conjugate(q::Quaternion) = Quaternion(q.real, -q.im)

function combine(q::Quaternion, r::Quaternion)::Quaternion
    return r * q # https://math.stackexchange.com/questions/331539/combining-rotation-quaternions
end

function rotate(v::Vec3f, r::Quaternion)::Vec3f
    q = r * Quaternion(0, v) * conjugate(r)
    return q.im
end

function inv_rotate(v::Vec3f, r::Quaternion)::Vec3f
    q = Quaternion(0, v)
    return conjugate(r) * q * r
end
