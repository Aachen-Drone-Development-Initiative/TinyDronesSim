
export Vec3f,
    Vec3f_32,
    Mat33f,
    cross_product,
    zero_mat33f,
    euclid_norm,
    norm_vector

const Vec3f = SVector{3, Float64}
const Vec3f_32 = SVector{3, Float32}
const Mat33f = SMatrix{3, 3, Float64}

cross_product(a::Vec3f, b::Vec3f) = Vec3f(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)

zero_mat33f() = Mat33f([0 0 0; 0 0 0; 0 0 0])

euclid_norm(a) = sqrt(a'a)
norm_vector(a) = a ./ euclid_norm(a)
