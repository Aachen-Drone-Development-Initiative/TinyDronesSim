#include "../environments.h"
#include <object_manager.hpp>

#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <window.hpp>
#include <logging.hpp>

#include <gltfio/FilamentInstance.h>

/*
 * This global variable contains the entire 'state' of the library,
 * which is currently just a map of objects.
 */
Object_State state;

Env_Object::Env_Object()
    : id(0), type(ENVO_INVALID), handle(0), associated_env(nullptr) {} // used as error return value
Env_Object::Env_Object(Environment* env)
    : id(state.create_id()), type(ENVO_ENVIRONMENT), env(env), associated_env(env) {}
Env_Object::Env_Object(Frame* frame)
    : id(state.create_id()), type(ENVO_FRAME), frame(frame), associated_env(frame->env) {}
Env_Object::Env_Object(Camera* camera)
    : id(state.create_id()), type(ENVO_CAMERA), camera(camera), associated_env(camera->env) {}
Env_Object::Env_Object(Window* window)
    : id(state.create_id()), type(ENVO_WINDOW), window(window), associated_env(window->camera->env) {}
Env_Object::Env_Object(futils::Entity fentity, Environment* associated_env)
    : id(state.create_id()), type(ENVO_FILAMENT_ENTITY), fentity(fentity), associated_env(associated_env) {}
Env_Object::Env_Object(fgltfio::FilamentInstance* finstance, Environment* associated_env)
    : id(state.create_id()), type(ENVO_FILAMENT_GLTF_INSTANCE), gltf_instance(finstance), associated_env(associated_env) {}
// Env_Object::Env_Object(Mesh* mesh)
    // : id(state.create_id()), type(ENVO_MESH), mesh(mesh), associated_env(nullptr) {}

static bool is_destroyable(Env_Object_Type type)
{
    return !(type == ENVO_FILAMENT_ENTITY || type == ENVO_FILAMENT_GLTF_INSTANCE);
}

void Env_Object::destroy_object()
{
    switch(type) {
    case ENVO_ENVIRONMENT:
        delete env;
        break;
    case ENVO_FRAME:
        delete frame;
        break;
    case ENVO_CAMERA:
        delete camera;
        break;
    case ENVO_WINDOW:
        delete window;
        break;
    case ENVO_FILAMENT_ENTITY:
        env_soft_error("Cannot destroy '%s', this will be done automatically.",
                       env_object_type_name[ENVO_FILAMENT_ENTITY]);
        break;
    case ENVO_FILAMENT_GLTF_INSTANCE:
        env_soft_error("Cannot destroy '%s', this will be done automatically.",
                       env_object_type_name[ENVO_FILAMENT_GLTF_INSTANCE]);
        break;
    default:
        env_soft_error("Cannot destroy '%s' Object.", env_object_type_name[ENVO_INVALID]);
        break;
    // case ENVO_MESH:
        // delete mesh;
        // break;
    }
    handle = 0;
}

futils::Entity Env_Object::get_representing_filament_entity()
{
    switch(type) {
    case ENVO_CAMERA:
        return camera->camera_fentity;
    case ENVO_FILAMENT_ENTITY:
        return fentity;
    case ENVO_FILAMENT_GLTF_INSTANCE:
        return gltf_instance->getRoot();
    default:
        return futils::Entity{};
    }
}

Env_Object Object_State::add_object(Env_Object obj)
{
    auto ins = m_objects.insert({obj.id, obj});
    if (!ins.second) {
        env_soft_error("An object with the same unique id '%s' already exists", obj.id);
        return Env_Object{};
    }
    return obj;
}

Env_Object Object_State::get_object(UUID obj_id)
{
    auto itr = m_objects.find(obj_id);
    if (itr == m_objects.end()) {
        env_soft_error("The object with id '%d' doesn't exist", obj_id);
        return Env_Object{};
    }

    return itr->second;
}

bool Object_State::destroy_object(UUID obj_id)
{
    auto itr = m_objects.find(obj_id);
    if (itr == m_objects.end()) {
        env_soft_error("The object with id '%d' doesn't exist", obj_id);
        return false;
    }

    Env_Object obj = itr->second;
    obj.destroy_object();

    m_objects.erase(itr);
    
    return true;
}

bool Object_State::destroy_all_objects()
{
    // Destroy all objects in reverse order of creation,
    // this is possible because the ids are 'sorted by creation' (next_id = prev_id + 1).
    UUID current_max_id = get_current_max_id();
    for (UUID id = current_max_id; id > m_guaranteed_invalid_ids_between_here_and_0; --id) {
        auto itr = m_objects.find(id);
        if (itr != m_objects.end()) {
            Env_Object obj = itr->second;
            if (is_destroyable(obj.type)) {
                obj.destroy_object();
            }                
        }
    }

    m_objects.clear(); // empty the map
    // since these ids got deleted they can't be used again.
    m_guaranteed_invalid_ids_between_here_and_0 = current_max_id;
    return true;
}

bool destroy(UUID obj_id)
{
    return state.destroy_object(obj_id);
}

bool destroy_everything()
{
    return state.destroy_all_objects();
}
