#pragma once

#include <mesh.hpp>

#include <tsl/robin_map.h>
#include <utils/Entity.h>
#include <logging.hpp>

#include <cstdint>


enum Env_Object_Type {
    ENVO_INVALID = 0,
    ENVO_ENVIRONMENT,
    ENVO_FRAME,
    ENVO_CAMERA,
    ENVO_WINDOW,
    ENVO_FILAMENT_ENTITY,
    ENVO_FILAMENT_GLTF_INSTANCE,
    ENVO_MESH,
    SIZE
};

static const char* env_object_type_name[Env_Object_Type::SIZE] = {
    "Invalid",
    "Environment",
    "Frame",
    "Camera",
    "Filament Entity",
    "Filament glTF Instance",
    "Mesh"
};

struct Environment;
struct Frame;
struct Camera;
struct Window;

namespace filament { namespace gltfio { class FilamentInstance; }}

namespace futils = utils;
namespace fgltfio = filament::gltfio;

typedef uint64_t UUID;
struct UUID_Hasher {
    uint64_t operator()(UUID id) const { return id; }
};
#define ENV_INVALID_UUID UUID(0)

struct Env_Object {

    Env_Object();
    Env_Object(Environment* env);
    Env_Object(Frame* frame);
    Env_Object(Camera* camera);
    Env_Object(Window* window);
    Env_Object(futils::Entity fentity, Environment* associated_env);
    Env_Object(fgltfio::FilamentInstance* finstance, Environment* associated_env);
    Env_Object(Mesh* mesh);

    bool operator==(Env_Object other) noexcept { return id == other.id; }

    void destroy_object();
    
    UUID id;
    Env_Object_Type type;

    union {
        Environment* env;
        Frame* frame;
        Camera* camera;
        Window* window;
        futils::Entity fentity;
        fgltfio::FilamentInstance* gltf_instance;
        // Mesh* mesh;
        uint64_t handle; // this is just for adressing the memory of the union without "thinking" about a specific field
    };

    Environment* associated_env;

    futils::Entity get_representing_filament_entity();
};

struct Object_State {
    
    friend struct Env_Object;
    
    // returns the invalid 0 id, if an object with the same id already exists.
    Env_Object add_object(Env_Object obj);
    Env_Object get_object(UUID obj_id);
    bool destroy_object(UUID obj_id);
    bool destroy_all_objects();

    Environment* get_as_environment(UUID obj_id);
    Frame* get_as_frame(UUID obj_id);
    Camera* get_as_camera(UUID obj_id);
    Window* get_as_window(UUID obj_id);
    futils::Entity get_as_filament_entity(UUID obj_id);
    fgltfio::FilamentInstance* get_as_gltf_instance(UUID obj_id);

private:
    // We are using the 'robin_map' with robin hood hashing, this is also what Filament uses
    // for its entities. They argue that it is efficient with removes, which a vector is not.
    // Because the library is intended to be used from a julia repl, where things are often
    // repeatedly created and destroyed it seems like a good fit.
    tsl::robin_map<UUID, Env_Object, UUID_Hasher> m_objects;

    // tsl::robin_map requires thes attributes
    static_assert(std::is_nothrow_swappable<std::pair<UUID, Env_Object>>::value
                  && std::is_nothrow_move_constructible<std::pair<UUID, Env_Object>>::value);

    UUID m_last_id = 0; // zero is the invalid id
    UUID m_guaranteed_invalid_ids_between_here_and_0 = 0;

    UUID create_id() {
        m_last_id++;
        return m_last_id;
    }

    UUID get_current_max_id() { return m_last_id; }
};

extern Object_State state;

inline Environment* Object_State::get_as_environment(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_ENVIRONMENT) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_ENVIRONMENT);
        return nullptr;
    }
    return obj.env;
}

inline Frame* Object_State::get_as_frame(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_FRAME) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_FRAME);
        return nullptr;
    }
    return obj.frame;
}

inline Camera* Object_State::get_as_camera(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_CAMERA) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_CAMERA);
        return nullptr;
    }
    return obj.camera;
}

inline Window* Object_State::get_as_window(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_WINDOW) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_WINDOW);
        return nullptr;
    }
    return obj.window;
}

inline futils::Entity Object_State::get_as_filament_entity(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_FILAMENT_ENTITY) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_FILAMENT_ENTITY);
        return futils::Entity{};
    }
    return obj.fentity;
}

inline fgltfio::FilamentInstance* Object_State::get_as_gltf_instance(UUID obj_id) {
    Env_Object obj = get_object(obj_id);
    if (obj.type != ENVO_FILAMENT_GLTF_INSTANCE) {
        ENV_OBJ_TYPE_ARGUMENT_ERROR(obj.type, ENVO_FILAMENT_GLTF_INSTANCE);
        return nullptr;
    }
    return obj.gltf_instance;
}
