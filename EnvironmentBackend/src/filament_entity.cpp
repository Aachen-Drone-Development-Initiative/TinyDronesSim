#include "../environments.h"
#include <object_manager.hpp>
#include <environment.hpp>

#include <filament/Engine.h>
#include <filament/TransformManager.h>
#include <math/mat3.h>
#include <math/mat4.h>
#include <math/quat.h>

namespace fmt = filament;

double3 get_position(UUID obj_id)
{
    Env_Object obj = g_objm.get_object(obj_id);
    futils::Entity fentity = obj.get_representing_filament_entity();
    
    fmt::TransformManager& trans_m = obj.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity);
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    return { mat[3].x, mat[3].y, mat[3].z };
}

// Quaternion get_orientation(Environment* env, EntityID entity)
// {
//     fmt::TransformManager& trans_m = env->engine->getTransformManager();
//     futils::Entity object = futils::Entity::import(entity);
//     auto entity_instance = trans_m.getInstance(object);
//     fmath::mat4 mat(trans_m.getTransform(entity_instance));

//     // TODO
// }

bool set_position(UUID obj_id, double3 pos)
{
    Env_Object obj = g_objm.get_object(obj_id);
    futils::Entity fentity = obj.get_representing_filament_entity();
    
    fmt::TransformManager& trans_m = obj.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity);
    
    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    mat[3] = {fmath::double3{pos.x, pos.y, pos.z}, 1};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

bool set_orientation(UUID obj_id, Quaternion orientation)
{
    Env_Object obj = g_objm.get_object(obj_id);
    futils::Entity fentity = obj.get_representing_filament_entity();
    
    fmt::TransformManager& trans_m = obj.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity);

    fmath::mat4 mat(trans_m.getTransform(entity_instance));
    fmath::mat3 rotation_mat{quat_to_fquat(orientation)};
    mat[0] = {rotation_mat[0], 0};
    mat[1] = {rotation_mat[1], 0};
    mat[2] = {rotation_mat[2], 0};
    trans_m.setTransform(entity_instance, mat);
    return true;
}

bool set_position_and_orientation(UUID obj_id, double3 pos, Quaternion orientation)
{
    Env_Object obj = g_objm.get_object(obj_id);
    futils::Entity fentity = obj.get_representing_filament_entity();
    
    fmt::TransformManager& trans_m = obj.associated_env->engine->getTransformManager();
    auto entity_instance = trans_m.getInstance(fentity);

    fmath::mat4 mat{};
    fmath::mat3 rotation_mat{quat_to_fquat(orientation)};
    mat[0] = {rotation_mat[0], 0};
    mat[1] = {rotation_mat[1], 0};
    mat[2] = {rotation_mat[2], 0};
    mat[3] = {fmath::double3{pos.x, pos.y, pos.z}, 1};
    trans_m.setTransform(entity_instance, mat);
    return true;
}
