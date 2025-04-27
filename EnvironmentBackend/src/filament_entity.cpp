#include "../environments.hpp"

#include <object_manager.hpp>
#include <filament_object_wrappers.hpp>
#include <environment.hpp>

#include <filament/Engine.h>
#include <filament/TransformManager.h>
#include <math/mat3.h>
#include <math/mat4.h>
#include <math/quat.h>

namespace fmt = filament;

double3 get_position(Filament_Entity_ID id)
{
    Filament_Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return {};
    
    fmt::TransformManager& trans_m = fentity.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity.entity);
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    return { mat[3].x, mat[3].y, mat[3].z };
}

bool set_position(Filament_Entity_ID id, double3 pos)
{
    Filament_Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity.entity);
    
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    mat[3] = {fmath::double3{pos.x, pos.y, pos.z}, 1};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

bool set_orientation(Filament_Entity_ID id, Quaternion orientation)
{
    Filament_Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity.entity);

    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    fmath::mat3 rotation_mat{quat_to_fquat(orientation)};
    mat[0] = {rotation_mat[0], 0};
    mat[1] = {rotation_mat[1], 0};
    mat[2] = {rotation_mat[2], 0};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

bool set_position_and_orientation(Filament_Entity_ID id, double3 pos, Quaternion orientation)
{
    Filament_Entity fentity = g_objm.get_object(id);
    if (!fentity.is_valid()) return false;
    
    fmt::TransformManager& trans_m = fentity.associated_env->engine->getTransformManager();
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
