#include <object_manager.hpp>

#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <viewport.hpp>
#include <joystick.hpp>
#include <logging.hpp>

#include <gltfio/FilamentInstance.h>

// This global variable owns and manages all the objects of the TinyDronesSim,
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

Viewport_ID Object_Manager::add_object(Viewport* viewport)
{
    UUID id = g_objm.create_id();
    auto ins = m_viewports.insert({{id}, viewport});
    assert(ins.second);
    return {id};
}

Entity_ID Object_Manager::add_object(Entity filament_entity)
{
    UUID id = g_objm.create_id();
    auto ins = m_entities.insert({{id}, filament_entity});
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

Joystick_ID Object_Manager::add_object(Joystick* joystick)
{
    UUID id = g_objm.create_id();
    auto ins = m_joysticks.insert({{id}, joystick});
    assert(ins.second);
    return {id};
}

Environment* Object_Manager::get_object(Environment_ID id)
{
    auto itr = m_environments.find(id);
    if (itr == m_environments.end()) {
        tds_soft_error("Couldn't find the Environment with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Frame* Object_Manager::get_object(Frame_ID id)
{
    auto itr = m_frames.find(id);
    if (itr == m_frames.end()) {
        tds_soft_error("Couldn't find the Frame with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Camera* Object_Manager::get_object(Camera_ID id)
{
    auto itr = m_cameras.find(id);
    if (itr == m_cameras.end()) {
        tds_soft_error("Couldn't find the Camera with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Viewport* Object_Manager::get_object(Viewport_ID id)
{
    auto itr = m_viewports.find(id);
    if (itr == m_viewports.end()) {
        tds_soft_error("Couldn't find the Viewport with id: %d", id.id);
        return nullptr;
    }
    return itr.value();
}

Entity Object_Manager::get_object(Entity_ID id) {
    auto itr = m_entities.find(id);
    if (itr == m_entities.end()) {
        tds_soft_error("Couldn't find the Filament-Entity with id: %d", id.id);
        return {};
    }
    return itr.value();
}

glTF_Instance Object_Manager::get_object(glTF_Instance_ID id)
{
    auto itr = m_gltf_instances.find(id);
    if (itr == m_gltf_instances.end()) {
        tds_soft_error("Couldn't find the glTF-Instance with id: %d", id.id);
        return {};
    }
    return itr.value();
}

Joystick* Object_Manager::get_object(Joystick_ID id)
{
    auto itr = m_joysticks.find(id);
    if (itr == m_joysticks.end()) {
        tds_soft_error("Couldn't find the Joystick with id: %d", id.id);
        return {};
    }
    return itr.value();
}

bool Object_Manager::destroy_object(Environment_ID id)
{
    if (id == active_env_id) {
        active_env_id = {TDS_INVALID_ID};
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

bool Object_Manager::destroy_object(Viewport_ID id)
{
    if (id == active_viewport_id) {
        active_viewport_id = {TDS_INVALID_ID};
        active_viewport = nullptr;
    }

    Viewport* viewport = get_object(id);
    if (!viewport) return false;
    delete viewport;
    m_viewports.erase(id);
    return true;
}

bool Object_Manager::destroy_object(Joystick_ID id)
{
    Joystick* joystick = get_object(id);
    if (!joystick) return false;
    delete joystick;
    m_joysticks.erase(id);
    return true;
}

bool Object_Manager::destroy_all_objects()
{
    // Destroy all objects in reverse order of creation.
    // This is possible because the ids are 'sorted by creation' (next_id = prev_id + 1).
    UUID current_max_id = get_current_max_id();
    for (UUID uuid = current_max_id; uuid.id > m_guaranteed_invalid_ids_between_here_and_0.id; --uuid.id)
    {
        if (m_environments.find({uuid}) != m_environments.end())    destroy_object(Environment_ID{uuid});
        else if (m_frames.find({uuid}) != m_frames.end())           destroy_object(Frame_ID{uuid});
        else if (m_cameras.find({uuid}) != m_cameras.end())         destroy_object(Camera_ID{uuid});
        else if (m_viewports.find({uuid}) != m_viewports.end())     destroy_object(Viewport_ID{uuid});
        else if (m_joysticks.find({uuid}) != m_joysticks.end())     destroy_object(Joystick_ID{uuid});
    }

    m_environments.clear();
    m_frames.clear();
    m_cameras.clear();
    m_viewports.clear();
    m_entities.clear();
    m_gltf_instances.clear();
    m_joysticks.clear();
    
    // Since these ids got deleted they can't be used again. Therefore all previous ids guaranteed to be not in use.
    m_guaranteed_invalid_ids_between_here_and_0 = current_max_id;
    return true;
}

bool Object_Manager::environment_activate(Environment_ID id)
{
    Environment* env = g_objm.get_object(id);
    if (!env) return false;
    
    active_env = env;
    active_env_id = id;
    return true;
}
    
bool Object_Manager::viewport_activate(Viewport_ID id)
{
    Viewport* viewport = g_objm.get_object(id);
    if (!viewport) return false;

    active_viewport = viewport;
    active_viewport_id = id;
    return true;
}

Environment* Object_Manager::get_active_environment()
{
    if (!active_env) {
        tds_soft_error("No active environment has been set.");
    }
    return active_env;
}

Environment_ID Object_Manager::get_active_environment_id()
{
    if (active_env_id == TDS_INVALID_ID) {
        tds_soft_error("No active environment has been set.");
    }
    return active_env_id;
}
    
Viewport* Object_Manager::get_active_viewport()
{
    if (!active_viewport) {
        tds_soft_error("No active viewport has been set.");
    }
    return active_viewport;
}

Viewport_ID Object_Manager::get_active_viewport_id()
{
    if (active_viewport_id == TDS_INVALID_ID) {
        tds_soft_error("No active viewport has been set.");
    }
    return active_viewport_id;
}

TDS_API bool env_activate(Environment_ID id)   { return g_objm.environment_activate(id); }
TDS_API bool viewport_activate(Viewport_ID id) { return g_objm.viewport_activate(id); }

TDS_API bool env_exists(Environment_ID id)         { return g_objm.object_exists(id); }
TDS_API bool frame_exists(Frame_ID id)             { return g_objm.object_exists(id); }
TDS_API bool camera_exists(Camera_ID id)           { return g_objm.object_exists(id); }
TDS_API bool viewport_exists(Viewport_ID id)       { return g_objm.object_exists(id); }
TDS_API bool entity_exists(Entity_ID id)           { return g_objm.object_exists(id); }
TDS_API bool gltf_inst_exists(glTF_Instance_ID id) { return g_objm.object_exists(id); }
TDS_API bool joystick_exists(Joystick_ID id)       { return g_objm.object_exists(id); }

TDS_API bool env_destroy(Environment_ID id)   { return g_objm.destroy_object(id); }
TDS_API bool frame_destroy(Frame_ID id)       { return g_objm.destroy_object(id); }
TDS_API bool camera_destroy(Camera_ID id)     { return g_objm.destroy_object(id); }
TDS_API bool viewport_destroy(Viewport_ID id) { return g_objm.destroy_object(id); }
TDS_API bool joystick_destroy(Joystick_ID id) { return g_objm.destroy_object(id); }

TDS_API bool destroy_everything() { return g_objm.destroy_all_objects(); }

TDS_API bool is_active_viewport_set() { return g_objm.is_active_viewport_set(); }
