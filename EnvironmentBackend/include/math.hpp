#pragma once

#include <math/vec3.h>
#include <math/mat3.h>
#include <math/quat.h>

#define PI 3.141592653589793

namespace fmath = filament::math;

struct double3 {
    double x, y, z;
};

inline fmath::double3 d3_to_fd3(double3 v) { return fmath::double3 { v.x, v.y, v.z }; }
inline double3 fd3_to_d3(fmath::double3 v) { return double3 { v.x, v.y, v.z }; }

struct float3 {
    float x, y, z;
};

inline fmath::float3 f3_to_ff3(float3 v) { return fmath::float3 { v.x, v.y, v.z }; }
inline float3 ff3_to_f3(fmath::float3 v) { return float3 { v.x, v.y, v.z }; }

struct float4 {
    float x, y, z, w;
};

inline fmath::float4 f4_to_ff4(float4 v) { return fmath::float4 { v.x, v.y, v.z, v.w }; }
inline float4 ff4_to_f4(fmath::float4 v) { return float4 { v.x, v.y, v.z, v.w }; }

struct int2 {
    int x, y;
};

struct Quaternion {
    double x, y, z, w;
};

/*TODO*/ Quaternion fmat3_to_quat(fmath::mat3 m);
inline Quaternion fquat_to_quat(fmath::quat q) { return { q.x, q.y, q.z, q.w }; }
inline fmath::quat quat_to_fquat(Quaternion q) { return { q.w, q.x, q.y, q.z }; }

// convert axis angles (which need to be normed first) to a quaternion


constexpr Quaternion normed_axis_angle_to_quaternion(double theta, double3 v)
{
    double sin_theta_div2 = sin(theta * 0.5);
    return Quaternion{v.x * sin_theta_div2, v.y * sin_theta_div2, v.z * sin_theta_div2, cos(theta * 0.5)};
}

// the neutral element for quaternions
constexpr Quaternion identity_quaternion() { return Quaternion { 0.0, 0.0, 0.0, 1.0 }; }

// create quaternion for rotating 90 degrees clock-wise/counter-clock-wise around the x, y or z axis
constexpr Quaternion quaternion_ccw_90_x() { return normed_axis_angle_to_quaternion(PI * 0.5, {1.0, 0.0, 0.0}); }
constexpr Quaternion quaternion_cw_90_x() { return normed_axis_angle_to_quaternion(-PI * 0.5, {1.0, 0.0, 0.0}); }
constexpr Quaternion quaternion_ccw_90_y() { return normed_axis_angle_to_quaternion(PI * 0.5, {0.0, 1.0, 0.0}); }
constexpr Quaternion quaternion_cw_90_y() { return normed_axis_angle_to_quaternion(-PI * 0.5, {0.0, 1.0, 0.0}); }
constexpr Quaternion quaternion_ccw_90_z() { return normed_axis_angle_to_quaternion(PI * 0.5, {0.0, 0.0, 1.0}); }
constexpr Quaternion quaternion_cw_90_z() { return normed_axis_angle_to_quaternion(-PI * 0.5, {0.0, 0.0, 1.0}); }

Quaternion operator*(Quaternion q, Quaternion r);
inline Quaternion quaternion_conjugate(Quaternion q) { return Quaternion{-q.x, -q.y, -q.z, q.w}; }
double3 quaternion_rotate_vector(double3 v, Quaternion r);
