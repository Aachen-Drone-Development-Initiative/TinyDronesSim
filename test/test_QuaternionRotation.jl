

val = normed_axis_angle_to_quaternion(Float64(π/2), Vec3f(1,0,0))
sol = Quaternion(0.7071067811865476, [0.7071067811865475, 0.0, 0.0])
@test val.real ≈ sol.real && val.im ≈ sol.im

val = normed_axis_angle_to_quaternion(Float64(-3/8 * π), Vec3f(1/√3, -1/√3, 1/√3))
sol = Quaternion(0.8314696123025452, [-0.3207586235876105, 0.3207586235876105, -0.3207586235876105])
@test val.real ≈ sol.real && val.im ≈ sol.im

