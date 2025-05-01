#pragma once

#include "../environments.hpp"

#include <utils/Entity.h>

struct Environment;

struct Filament_Entity {
    Filament_Entity() : entity(), associated_env(nullptr) {}
    Filament_Entity(utils::Entity entity, Environment* associated_env) : entity(entity), associated_env(associated_env) {}

    bool is_valid() { return !entity.isNull(); }
    
    utils::Entity entity;
    Environment* associated_env;
};

namespace filament { namespace gltfio { class FilamentInstance; }}

struct glTF_Instance {
    glTF_Instance() : gltf_instance(nullptr), root_entity_id(), associated_env(nullptr) {}
    glTF_Instance(filament::gltfio::FilamentInstance* gltf_instance, Environment* associated_env);

    bool is_valid() { return gltf_instance != nullptr; }
    
    filament::gltfio::FilamentInstance* gltf_instance;
    Filament_Entity_ID root_entity_id;
    Environment* associated_env;
};
