#include <math.hpp>

Quaternion operator*(Quaternion q, Quaternion r)
{
    return Quaternion{q.w * r.x + q.x * r.w - q.y * r.z + q.z * r.y,
                      q.w * r.y + q.x * r.z + q.y * r.w - q.z * r.x,
                      q.w * r.z - q.x * r.y + q.y * r.x + q.z * r.w,
                      q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z};
}

double3 quaternion_rotate_vector(double3 v, Quaternion r) {
    Quaternion q = r * Quaternion{v.x, v.y, v.z, 0.0} * quaternion_conjugate(r);
    return double3{q.x, q.y, q.z};
}
