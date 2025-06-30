#include "../tinydronessim_backend.hpp"

#include <limits>
#include <object_manager.hpp>
#include <entity.hpp>
#include <environment.hpp>
#include <math.hpp>
#include <logging.hpp>

#include <filament/Engine.h>
#include <filament/TransformManager.h>
#include <math/mat3.h>
#include <math/mat4.h>
#include <math/quat.h>
#include <typeinfo>

namespace fmt = filament;

TDS_API double3 entity_position(Entity_ID id)
{
    Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return {};
    
    fmt::TransformManager& trans_m = fentity.associated_env->get_transform_manager();
    auto entity_instance = trans_m.getInstance(fentity.entity);
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    return { mat[3].x, mat[3].y, mat[3].z };
}

TDS_API bool entity_set_position(Entity_ID id, double3 pos)
{
    Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->get_transform_manager();
    auto entity_instance = trans_m.getInstance(fentity.entity);
    
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    mat[3] = {fmath::double3{pos.x, pos.y, pos.z}, 1};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

TDS_API bool entity_set_orientation(Entity_ID id, Quaternion orientation)
{
    Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->get_transform_manager();
    auto entity_instance = trans_m.getInstance(fentity.entity);

    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    fmath::mat3 rotation_mat{quat_to_fquat(orientation)};
    mat[0] = {rotation_mat[0], 0};
    mat[1] = {rotation_mat[1], 0};
    mat[2] = {rotation_mat[2], 0};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

TDS_API bool entity_set_position_and_orientation(Entity_ID id, double3 pos, Quaternion orientation)
{
    Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->get_transform_manager();
    auto entity_instance = trans_m.getInstance(fentity.entity);

    fmath::mat4 mat{};
    fmath::mat3 rotation_mat{quat_to_fquat(orientation)};
    mat[0] = {rotation_mat[0], 0};
    mat[1] = {rotation_mat[1], 0};
    mat[2] = {rotation_mat[2], 0};
    mat[3] = {fmath::double3{pos.x, pos.y, pos.z}, 1};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

TDS_API bool entity_set_scale(Entity_ID id, double3 scale, bool in_local_frame)
{
    Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;

    fmt::TransformManager& trans_m = fentity.associated_env->get_transform_manager();
    auto entity_instance = trans_m.getInstance(fentity.entity);

    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    fmath::mat3 rot_mat = mat.upperLeft();

    // remove any prior scaling
    rot_mat[0] = normalize(rot_mat[0]);
    rot_mat[1] = normalize(rot_mat[1]);
    rot_mat[2] = normalize(rot_mat[2]);
    
    if (abs(scale.x) < std::numeric_limits<float>::epsilon()) {
        scale.x = std::numeric_limits<float>::epsilon();
    }
    if (abs(scale.y) < std::numeric_limits<float>::epsilon()) {
        scale.y = std::numeric_limits<float>::epsilon();
    }
    if (abs(scale.z) < std::numeric_limits<float>::epsilon()) {
        scale.z = std::numeric_limits<float>::epsilon();
    }
    
    fmath::mat3 scale_mat(fmath::float3{scale.x, 0, 0},
                          fmath::float3{0, scale.y, 0},
                          fmath::float3{0, 0, scale.z});
    
    if (in_local_frame) {
        rot_mat = rot_mat * scale_mat;
    }
    else {
        rot_mat = scale_mat * rot_mat;
    }
    
    mat[0] = {rot_mat[0], 0};
    mat[1] = {rot_mat[1], 0};
    mat[2] = {rot_mat[2], 0};
    trans_m.setTransform(entity_instance, mat);
    return true;
}
