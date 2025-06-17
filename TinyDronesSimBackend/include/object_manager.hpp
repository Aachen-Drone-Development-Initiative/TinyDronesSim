#pragma once

#include "../tinydronessim_backend.hpp"

#include <tsl/robin_map.h>

struct UUID_Hasher {
    uint64_t operator()(const UUID& uuid) const { return uuid.id; }
};

class Environment;
class Frame;
class Camera;
class Viewport;
#include <entity.hpp>
#include <gltf_instance.hpp>
class Joystick;

struct Object_Manager {
    
    Environment_ID     add_object(Environment* env);
    Frame_ID           add_object(Frame* frame);
    Camera_ID          add_object(Camera* camera);
    Viewport_ID        add_object(Viewport* viewport);
    Entity_ID          add_object(Entity filament_entity);
    glTF_Instance_ID   add_object(glTF_Instance gltf_instance);
    Joystick_ID        add_object(Joystick* joystick);

    Environment*     get_object(Environment_ID id);
    Frame*           get_object(Frame_ID id);
    Camera*          get_object(Camera_ID id);
    Viewport*        get_object(Viewport_ID id);
    Entity           get_object(Entity_ID id);
    glTF_Instance    get_object(glTF_Instance_ID id);
    Joystick*        get_object(Joystick_ID id);
    
    bool object_exists(Environment_ID id)   { return m_environments.find(id) != m_environments.end(); }
    bool object_exists(Frame_ID id)         { return m_frames.find(id) != m_frames.end(); }
    bool object_exists(Camera_ID id)        { return m_cameras.find(id) != m_cameras.end(); }
    bool object_exists(Viewport_ID id)      { return m_viewports.find(id) != m_viewports.end(); }
    bool object_exists(Entity_ID id)        { return m_entities.find(id) != m_entities.end(); }
    bool object_exists(glTF_Instance_ID id) { return m_gltf_instances.find(id) != m_gltf_instances.end(); }
    bool object_exists(Joystick_ID id)      { return m_joysticks.find(id) != m_joysticks.end(); }

    bool destroy_object(Environment_ID id);
    bool destroy_object(Frame_ID id);
    bool destroy_object(Camera_ID id);
    bool destroy_object(Viewport_ID id);
    bool destroy_object(Joystick_ID id);
    
    bool destroy_all_objects();
    
    bool environment_activate(Environment_ID id);
    bool viewport_activate(Viewport_ID id);
    Environment* get_active_environment();
    Environment_ID get_active_environment_id();
    Viewport* get_active_viewport();
    Viewport_ID get_active_viewport_id();
    bool is_active_viewport_set() { return active_viewport != nullptr; }

    const tsl::robin_map<Environment_ID, Environment*, UUID_Hasher>& get_environments()      { return m_environments; }
    const tsl::robin_map<Frame_ID, Frame*, UUID_Hasher>& get_frames()                        { return m_frames; }
    const tsl::robin_map<Camera_ID, Camera*, UUID_Hasher>& get_cameras()                     { return m_cameras; }
    const tsl::robin_map<Viewport_ID, Viewport*, UUID_Hasher>& get_viewports()               { return m_viewports; }
    const tsl::robin_map<Entity_ID, Entity, UUID_Hasher>& get_filament_entities()            { return m_entities; }
    const tsl::robin_map<glTF_Instance_ID, glTF_Instance, UUID_Hasher>& get_gltf_instances() { return m_gltf_instances; }

private:

    /*
     * We are using the 'robin_map' with robin hood hashing, this is also what Filament uses
     * for its entities. They argue that it is efficient with removes, which a vector is not,
     * but still offers faster iteration over elements than other hash-maps.
     * Because the TinyDronesSim is intended to be used from a julia repl, where things are
     * often repeatedly created and destroyed, it seems like a good fit.
     */

    tsl::robin_map<Environment_ID, Environment*, UUID_Hasher>    m_environments;
    tsl::robin_map<Frame_ID, Frame*, UUID_Hasher>                m_frames;
    tsl::robin_map<Camera_ID, Camera*, UUID_Hasher>              m_cameras;
    tsl::robin_map<Viewport_ID, Viewport*, UUID_Hasher>          m_viewports;
    tsl::robin_map<Entity_ID, Entity, UUID_Hasher>               m_entities;
    tsl::robin_map<glTF_Instance_ID, glTF_Instance, UUID_Hasher> m_gltf_instances;
    tsl::robin_map<Joystick_ID, Joystick*, UUID_Hasher>          m_joysticks;
    
    // Universal Unique Identifier implementation
    UUID m_last_id = {0}; // zero is the invalid id
    UUID m_guaranteed_invalid_ids_between_here_and_0 = {0};

    UUID create_id() {
        m_last_id.id++;
        return m_last_id;
    }

    UUID get_current_max_id() { return m_last_id; }

    // active viewport
    Viewport* active_viewport = nullptr;
    Viewport_ID active_viewport_id = {TDS_INVALID_ID};

    // active environment
    Environment* active_env = nullptr;
    Environment_ID active_env_id = {TDS_INVALID_ID};
};

extern Object_Manager g_objm;
