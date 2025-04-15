
export Int32_2,
    Float64_3,
    Float64_4,
    Float32_3,
    Float32_4,
    Float64_3x3,
    cross_product,
    zero_mat33f,
    euclid_norm,
    norm_vector

const Int32_2 = SVector{2, Int32}
const Float64_3 = SVector{3, Float64}
const Float64_4 = SVector{4, Float64}
const Float32_3 = SVector{3, Float32}
const Float32_4 = SVector{4, Float32}
const Float64_3x3 = SMatrix{3, 3, Float64}

cross_product(a::Float64_3, b::Float64_3) = Float64_3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)

zero_mat33f() = Float64_3x3([0 0 0; 0 0 0; 0 0 0])

euclid_norm(a) = sqrt(a'a)
norm_vector(a) = a ./ euclid_norm(a)

