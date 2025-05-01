#include <filament_object_wrappers.hpp>

#include <object_manager.hpp>

#include <gltfio/FilamentInstance.h>

glTF_Instance::glTF_Instance(filament::gltfio::FilamentInstance* gltf_instance, Environment* associated_env)
    : gltf_instance(gltf_instance), root_entity_id(g_objm.add_object({gltf_instance->getRoot(), associated_env})), associated_env(associated_env) {}

ENV_API Filament_Entity_ID get_gltf_instance_filament_entity(glTF_Instance_ID gltf_instance_id)
{
    glTF_Instance gltf_instance = g_objm.get_object(gltf_instance_id);
    if (!gltf_instance.is_valid()) return {ENV_INVALID_UUID};

    return gltf_instance.root_entity_id;
}
