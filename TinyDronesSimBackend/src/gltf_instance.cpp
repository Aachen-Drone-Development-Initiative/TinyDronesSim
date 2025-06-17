#include <gltf_instance.hpp>

#include <object_manager.hpp>

#include <gltfio/FilamentInstance.h>

glTF_Instance::glTF_Instance(filament::gltfio::FilamentInstance* gltf_instance, Environment* associated_env)
    : gltf_instance(gltf_instance),
      root_entity_id(g_objm.add_object({gltf_instance->getRoot(), associated_env})),
      associated_env(associated_env) {}

TDS_API double3 gltf_inst_position(glTF_Instance_ID gltf_inst_id)
{
    glTF_Instance gltf_inst = g_objm.get_object(gltf_inst_id);
    if (!gltf_inst.is_valid()) return {};

    return entity_position(gltf_inst.root_entity_id);
}

TDS_API bool gltf_inst_set_position(glTF_Instance_ID gltf_inst_id, double3 pos)
{
    glTF_Instance gltf_inst = g_objm.get_object(gltf_inst_id);
    if (!gltf_inst.is_valid()) return false;

    return entity_set_position(gltf_inst.root_entity_id, pos);
}

TDS_API bool gltf_inst_set_orientation(glTF_Instance_ID gltf_inst_id, Quaternion orientation)
{
    glTF_Instance gltf_inst = g_objm.get_object(gltf_inst_id);
    if (!gltf_inst.is_valid()) return false;

    return entity_set_orientation(gltf_inst.root_entity_id, orientation);
}

TDS_API bool gltf_inst_set_position_and_orientation(glTF_Instance_ID gltf_inst_id, double3 pos, Quaternion orientation)
{
    glTF_Instance gltf_inst = g_objm.get_object(gltf_inst_id);
    if (!gltf_inst.is_valid()) return false;

    return entity_set_position_and_orientation(gltf_inst.root_entity_id, pos, orientation);
}
