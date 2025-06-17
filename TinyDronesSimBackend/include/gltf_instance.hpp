#pragma once

#include "../tinydronessim_backend.hpp"

class Environment;

namespace filament { namespace gltfio { class FilamentInstance; }}

class glTF_Instance {
public:
    glTF_Instance() : gltf_instance(nullptr), root_entity_id(), associated_env(nullptr) {}
    glTF_Instance(filament::gltfio::FilamentInstance* gltf_instance, Environment* associated_env);

    bool is_valid() { return gltf_instance != nullptr; }
    
    filament::gltfio::FilamentInstance* gltf_instance;
    Entity_ID root_entity_id;
    Environment* associated_env;
};
