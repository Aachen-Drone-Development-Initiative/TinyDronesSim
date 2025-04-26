#include "../environments.h"
#include <cstddef>
#include <object_manager.hpp>

#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <window.hpp>
#include <logging.hpp>

#include <gltfio/FilamentInstance.h>

// This global variable owns and manages all the objects of the library,
Object_Manager g_objm;

static bool is_destroyable(Env_Object_Type type)
{
    return !(type == ENVO_FILAMENT_ENTITY || type == ENVO_FILAMENT_GLTF_INSTANCE);
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

Env_Object Object_Manager::add_object() { return add_object(Env_Object {}); }

Env_Object Object_Manager::add_object(Environment* env)
{
    return add_object({.id=g_objm.create_id(), .type=ENVO_ENVIRONMENT, .env=env, .associated_env=env});
}

Env_Object Object_Manager::add_object(Frame* frame)
{
    return add_object({.id=g_objm.create_id(), .type=ENVO_FRAME, .frame=frame, .associated_env=frame->env});
}

Env_Object Object_Manager::add_object(Camera* camera)
{
    return add_object({ .id = g_objm.create_id(), .type = ENVO_CAMERA, .camera = camera, .associated_env = camera->env });
}

Env_Object Object_Manager::add_object(Window* window)
{
    Env_Object obj = add_object({ .id = g_objm.create_id(), .type = ENVO_WINDOW, .window = window, .associated_env = window->camera->env });
    m_all_window_ids.push_back(obj.id);
    return obj;
}

Env_Object Object_Manager::add_object(futils::Entity fentity, Environment* associated_env)
{
    return add_object({ .id = g_objm.create_id(), .type = ENVO_FILAMENT_ENTITY, .fentity = fentity, .associated_env = associated_env });
}

Env_Object Object_Manager::add_object(fgltfio::FilamentInstance* finstance, Environment* associated_env)
{
    return add_object({.id=g_objm.create_id(), .type=ENVO_FILAMENT_GLTF_INSTANCE, .gltf_instance=finstance, .associated_env=associated_env});
}

Env_Object Object_Manager::add_object(Env_Object obj)
{
    auto ins = m_objects.insert({obj.id, obj});
    if (!ins.second) {
        env_soft_error("An object with the same unique id '%s' already exists", obj.id);
        return Env_Object{};
    }
    return obj;
}

Env_Object Object_Manager::get_object(UUID obj_id)
{
    auto itr = m_objects.find(obj_id);
    if (itr == m_objects.end()) {
        env_soft_error("The object with id '%d' doesn't exist", obj_id);
        return Env_Object{};
    }

    return itr->second;
}

bool Object_Manager::destroy_object(Env_Object obj)
{
    switch(obj.type) {
    case ENVO_ENVIRONMENT:
        if (obj.id == active_env_id) {
            active_env_id = ENV_INVALID_UUID;
            active_env = nullptr;
        }
        delete obj.env;
        break;
    case ENVO_FRAME:
        delete obj.frame;
        break;
    case ENVO_CAMERA:
        delete obj.camera;
        break;
    case ENVO_WINDOW:
        if (obj.id == active_window_id) {
            active_window_id = ENV_INVALID_UUID;
            active_window = nullptr;
        }
        // also remove window from the window-exclusive vector
        for (size_t i = 0; i < m_all_window_ids.size(); ++i) {
            if (m_all_window_ids[i] == obj.id) {
                m_all_window_ids.erase(m_all_window_ids.cbegin() + i);
            }
        }
        delete obj.window;
        break;
    case ENVO_FILAMENT_ENTITY:
        env_soft_error("Cannot destroy '%s', this will be done automatically.",
                       env_object_type_name[ENVO_FILAMENT_ENTITY]);
        return false;
    case ENVO_FILAMENT_GLTF_INSTANCE:
        env_soft_error("Cannot destroy '%s', this will be done automatically.",
                       env_object_type_name[ENVO_FILAMENT_GLTF_INSTANCE]);
        return false;
    default:
        env_soft_error("Cannot destroy '%s' Object.", env_object_type_name[obj.type]);
        return false;
    // case ENVO_MESH:
        // delete mesh;
        // break;
    }
    obj.handle = 0;
    
    return true;
}

bool Object_Manager::destroy_object(UUID obj_id)
{
    auto itr = m_objects.find(obj_id);
    if (itr == m_objects.end()) {
        env_soft_error("The object with id '%d' doesn't exist", obj_id);
        return false;
    }

    Env_Object obj = itr->second;
    
    destroy_object(obj);

    m_objects.erase(itr);
    
    return true;
}

bool Object_Manager::destroy_all_objects()
{
    // Destroy all objects in reverse order of creation,
    // this is possible because the ids are 'sorted by creation' (next_id = prev_id + 1).
    UUID current_max_id = get_current_max_id();
    for (UUID id = current_max_id; id > m_guaranteed_invalid_ids_between_here_and_0; --id) {
        auto itr = m_objects.find(id);
        if (itr != m_objects.end()) {
            Env_Object obj = itr->second;
            if (is_destroyable(obj.type)) {
                destroy_object(obj);
            }                
        }
    }

    m_objects.clear(); // empty the map
    // since these ids got deleted they can't be used again.
    m_guaranteed_invalid_ids_between_here_and_0 = current_max_id;
    return true;
}

bool Object_Manager::object_exists(UUID obj_id)
{
    return m_objects.find(obj_id) != m_objects.end();
}

bool set_active_environment(UUID env_id)
{
    Environment* env = g_objm.get_as_environment(env_id);
    if (!env) return false;

    g_objm.set_active_environment(env, env_id);
    return true;
}

bool set_active_window(UUID window_id)
{
    Window* window = g_objm.get_as_window(window_id);
    if (!window) return false;

    g_objm.set_active_window(window, window_id);
    return true;
}

bool is_active_window_set()
{
    return g_objm.is_active_window_set();
}

bool destroy(UUID obj_id)
{
    return g_objm.destroy_object(obj_id);
}

bool destroy_everything()
{
    return g_objm.destroy_all_objects();
}

bool exists(UUID obj_id)
{
    return g_objm.object_exists(obj_id);
}
