#include <object_manager.hpp>

#include "../environments.hpp"
#include "filament_object_wrappers.hpp"
#include <cstddef>

#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <window.hpp>
#include <logging.hpp>

#include <gltfio/FilamentInstance.h>

// This global variable owns and manages all the objects of the library,
Object_Manager g_objm;

Environment_ID Object_Manager::add_object(Environment* env)
{
    UUID id = g_objm.create_id();
    auto ins = m_environments.insert({{id}, env});
    assert(ins.second);
    return {id};
}

Frame_ID Object_Manager::add_object(Frame* frame)
{
    UUID id = g_objm.create_id();
    auto ins = m_frames.insert({{id}, frame});
    assert(ins.second);
    return {id};
}

Camera_ID Object_Manager::add_object(Camera* camera)
{
    UUID id = g_objm.create_id();
    auto ins = m_cameras.insert({{id}, camera});
    assert(ins.second);
    return {id};
}

Window_ID Object_Manager::add_object(Window* window)
{
    UUID id = g_objm.create_id();
    auto ins = m_windows.insert({{id}, window});
    assert(ins.second);
    return {id};
}

Filament_Entity_ID Object_Manager::add_object(Filament_Entity filament_entity)
{
    UUID id = g_objm.create_id();
    auto ins = m_filament_entities.insert({{id}, filament_entity});
    assert(ins.second);
    return {id};
}

glTF_Instance_ID Object_Manager::add_object(glTF_Instance gltf_instance)
{
    UUID id = g_objm.create_id();
    auto ins = m_gltf_instances.insert({{id}, gltf_instance});
    assert(ins.second);
    return {id};
}

Environment* Object_Manager::get_object(Environment_ID id)
{
    auto itr = m_environments.find(id);
    if (itr == m_environments.end()) {
        env_soft_error("Couldn't find the Environment with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Frame* Object_Manager::get_object(Frame_ID id)
{
    auto itr = m_frames.find(id);
    if (itr == m_frames.end()) {
        env_soft_error("Couldn't find the Frame with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Camera* Object_Manager::get_object(Camera_ID id)
{
    auto itr = m_cameras.find(id);
    if (itr == m_cameras.end()) {
        env_soft_error("Couldn't find the Camera with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Window* Object_Manager::get_object(Window_ID id)
{
    auto itr = m_windows.find(id);
    if (itr == m_windows.end()) {
        env_soft_error("Couldn't find the Window with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Filament_Entity Object_Manager::get_object(Filament_Entity_ID id) {
    auto itr = m_filament_entities.find(id);
    if (itr == m_filament_entities.end()) {
        env_soft_error("Couldn't find the Filament-Entity with id: %d", id.id);
        return {};
    }
    return itr.value();
}

glTF_Instance Object_Manager::get_object(glTF_Instance_ID id)
{
    auto itr = m_gltf_instances.find(id);
    if (itr == m_gltf_instances.end()) {
        env_soft_error("Couldn't find the glTF-Instance with id: %d", id.id);
        return {};
    }
    return itr.value();
}

bool Object_Manager::destroy_object(Environment_ID id)
{
    if (id == active_env_id) {
        active_env_id = {ENV_INVALID_UUID};
        active_env = nullptr;
    }

    Environment* env = get_object(id);
    if (!env) return false;
    delete env;
    m_environments.erase(id);
    return true;
}

bool Object_Manager::destroy_object(Frame_ID id)
{
    Frame* frame = get_object(id);
    if (!frame) return false;
    delete frame;
    m_frames.erase(id);
    return true;
}

bool Object_Manager::destroy_object(Camera_ID id)
{
    Camera* camera = get_object(id);
    if (!camera) return false;
    delete camera;
    m_cameras.erase(id);
    return true;
}

bool Object_Manager::destroy_object(Window_ID id)
{
    if (id == active_window_id) {
        active_window_id = {ENV_INVALID_UUID};
        active_window = nullptr;
    }

    Window* window = get_object(id);
    if (!window) return false;
    delete window;
    m_windows.erase(id);
    return true;
}

bool Object_Manager::destroy_all_objects()
{
    // Destroy all objects in reverse order of creation.
    // This is possible because the ids are 'sorted by creation' (next_id = prev_id + 1).
    UUID current_max_id = get_current_max_id();
    for (UUID uuid = current_max_id; uuid.id > m_guaranteed_invalid_ids_between_here_and_0.id; --uuid.id)
    {
        if (m_environments.find({uuid}) != m_environments.end()) { destroy_object(Environment_ID{uuid}); continue; }
        if (m_frames.find({uuid}) != m_frames.end())             { destroy_object(Frame_ID{uuid}); continue; }
        if (m_cameras.find({uuid}) != m_cameras.end())           { destroy_object(Camera_ID{uuid}); continue; }
        if (m_windows.find({uuid}) != m_windows.end())           { destroy_object(Window_ID{uuid}); continue; }
    }

    m_environments.clear();
    m_frames.clear();
    m_cameras.clear();
    m_windows.clear();
    m_filament_entities.clear();
    m_gltf_instances.clear();
    
    // Since these ids got deleted they can't be used again. Therefore all previous ids guaranteed to be not in use.
    m_guaranteed_invalid_ids_between_here_and_0 = current_max_id;
    return true;
}

bool Object_Manager::set_active_environment(Environment_ID id)
{
    Environment* env = g_objm.get_object(id);
    if (!env) return false;
    
    active_env = env;
    active_env_id = id;
    return true;
}
    
bool Object_Manager::set_active_window(Window_ID id)
{
    Window* window = g_objm.get_object(id);
    if (!window) return false;

    active_window = window;
    active_window_id = id;
    return true;
}

Environment* Object_Manager::get_active_environment()
{
    if (!active_env) {
        env_soft_error("No active environment has been set.");
    }
    return active_env;
}

Environment_ID Object_Manager::get_active_environment_id()
{
    if (active_env_id == ENV_INVALID_UUID) {
        env_soft_error("No active environment has been set.");
    }
    return active_env_id;
}
    
Window* Object_Manager::get_active_window()
{
    if (!active_window) {
        env_soft_error("No active window has been set.");
    }
    return active_window;
}

Window_ID Object_Manager::get_active_window_id()
{
    if (active_window_id == ENV_INVALID_UUID) {
        env_soft_error("No active window has been set.");
    }
    return active_window_id;
}

/*
 * ENV_API function implementations
 */

ENV_API bool set_active_environment(Environment_ID id) { return g_objm.set_active_environment(id); }
ENV_API bool set_active_window(Window_ID id)           { return g_objm.set_active_window(id); }

ENV_API bool environment_exists(Environment_ID id)         { return g_objm.object_exists(id); }
ENV_API bool frame_exists(Frame_ID id)                     { return g_objm.object_exists(id); }
ENV_API bool camera_exists(Camera_ID id)                   { return g_objm.object_exists(id); }
ENV_API bool window_exists(Window_ID id)                   { return g_objm.object_exists(id); }
ENV_API bool filament_entity_exists(Filament_Entity_ID id) { return g_objm.object_exists(id); }
ENV_API bool gltf_instance_exists(glTF_Instance_ID id)     { return g_objm.object_exists(id); }

ENV_API bool destroy_environment(Environment_ID id)         { return g_objm.destroy_object(id); }
ENV_API bool destroy_frame(Frame_ID id)                     { return g_objm.destroy_object(id); }
ENV_API bool destroy_camera(Camera_ID id)                   { return g_objm.destroy_object(id); }
ENV_API bool destroy_window(Window_ID id)                   { return g_objm.destroy_object(id); }

ENV_API bool destroy_everything() { return g_objm.destroy_all_objects(); }

ENV_API bool is_active_window_set() { return g_objm.is_active_window_set(); }
