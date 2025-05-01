#pragma once

#include "../environments.hpp"
#include <filament_object_wrappers.hpp>
#include <logging.hpp>

#include <tsl/robin_map.h>
#include <utils/Entity.h>

struct UUID_Hasher {
    uint64_t operator()(const UUID& uuid) const { return uuid.id; }
};

struct Environment;
struct Frame;
struct Camera;
struct Window;

struct Object_Manager {

    Environment_ID     add_object(Environment* env);
    Frame_ID           add_object(Frame* frame);
    Camera_ID          add_object(Camera* camera);
    Window_ID          add_object(Window* window);
    Filament_Entity_ID add_object(Filament_Entity filament_entity);
    glTF_Instance_ID   add_object(glTF_Instance gltf_instance);

    Environment*     get_object(Environment_ID id);
    Frame*           get_object(Frame_ID id);
    Camera*          get_object(Camera_ID id);
    Window*          get_object(Window_ID id);
    Filament_Entity  get_object(Filament_Entity_ID id);
    glTF_Instance    get_object(glTF_Instance_ID id);
    
    bool object_exists(Environment_ID id)     { return m_environments.find(id) != m_environments.end(); }
    bool object_exists(Frame_ID id)           { return m_frames.find(id) != m_frames.end(); }
    bool object_exists(Camera_ID id)          { return m_cameras.find(id) != m_cameras.end(); }
    bool object_exists(Window_ID id)          { return m_windows.find(id) != m_windows.end(); }
    bool object_exists(Filament_Entity_ID id) { return m_filament_entities.find(id) != m_filament_entities.end(); }
    bool object_exists(glTF_Instance_ID id)   { return m_gltf_instances.find(id) != m_gltf_instances.end(); }

    bool destroy_object(Environment_ID id);
    bool destroy_object(Frame_ID id);
    bool destroy_object(Camera_ID id);
    bool destroy_object(Window_ID id);
    
    bool destroy_all_objects();
    
    bool environment_activate(Environment_ID id);
    bool window_activate(Window_ID id);
    Environment* get_active_environment();
    Environment_ID get_active_environment_id();
    Window* get_active_window();
    Window_ID get_active_window_id();
    bool is_active_window_set() { return active_window != nullptr; }

    const tsl::robin_map<Environment_ID, Environment*, UUID_Hasher>& get_environments()             { return m_environments; }
    const tsl::robin_map<Frame_ID, Frame*, UUID_Hasher>& get_frames()                               { return m_frames; }
    const tsl::robin_map<Camera_ID, Camera*, UUID_Hasher>& get_cameras()                            { return m_cameras; }
    const tsl::robin_map<Window_ID, Window*, UUID_Hasher>& get_windows()                            { return m_windows; }
    const tsl::robin_map<Filament_Entity_ID, Filament_Entity, UUID_Hasher>& get_filament_entities() { return m_filament_entities; }
    const tsl::robin_map<glTF_Instance_ID, glTF_Instance, UUID_Hasher>& get_gltf_instances()        { return m_gltf_instances; }

private:

    /*
     * We are using the 'robin_map' with robin hood hashing, this is also what Filament uses
     * for its entities. They argue that it is efficient with removes, which a vector is not.
     * Because the library is intended to be used from a julia repl, where things are often
     * repeatedly created and destroyed it seems like a good fit.
     */

    tsl::robin_map<Environment_ID, Environment*, UUID_Hasher>        m_environments;
    tsl::robin_map<Frame_ID, Frame*, UUID_Hasher>                    m_frames;
    tsl::robin_map<Camera_ID, Camera*, UUID_Hasher>                  m_cameras;
    tsl::robin_map<Window_ID, Window*, UUID_Hasher>                  m_windows;
    tsl::robin_map<Filament_Entity_ID, Filament_Entity, UUID_Hasher> m_filament_entities;
    tsl::robin_map<glTF_Instance_ID, glTF_Instance, UUID_Hasher>     m_gltf_instances;
    
    /*
     * Universal Unique Identifier implementation
     */
    
    UUID m_last_id = {0}; // zero is the invalid id
    UUID m_guaranteed_invalid_ids_between_here_and_0 = {0};

    UUID create_id() {
        m_last_id.id++;
        return m_last_id;
    }

    UUID get_current_max_id() { return m_last_id; }

    /*
     * Current active Environment and Window
     */

    Window* active_window = nullptr;
    Window_ID active_window_id = {ENV_INVALID_UUID};

    Environment* active_env = nullptr;
    Environment_ID active_env_id = {ENV_INVALID_UUID};
};

extern Object_Manager g_objm;
