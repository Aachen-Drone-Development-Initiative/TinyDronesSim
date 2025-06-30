#include "../tinydronessim_backend.hpp"

#include <math/vec3.h>
#include <filameshio/MeshReader.h>
#include <utils/EntityManager.h>

#include <par_shapes.h>

#include <vector>

namespace filament {
    class Scene;
    class Engine;
    class Material;
    class Camera;
    class View;
    class Viewport;
    class Renderer;
    class SwapChain;
    class Fence;
    class TransformManager;
    
    namespace gltfio {
        class AssetLoader;
        class MaterialProvider;
        class TextureProvider;
        class FilamentAsset;
    }
}

namespace fmt = filament;
namespace fmath = filament::math;
namespace futils = utils;
namespace fmesh = filamesh;
namespace fgltfio = filament::gltfio;

struct UI_Panel {
    UI_Panel();
    virtual bool update() = 0;
    
private:
    const char* name;
};

class glTF_Instance;

class Environment {
public:
    static Environment_ID create();
    ~Environment();

    bool add_camera_viewport(Camera_ID camera_id);
    bool remove_camera_viewport(Camera_ID camera_id);

    bool add_lit_material(const char* material_name, float3 base_color, float roughness, float metallic, float reflectance, float sheen_color, float clear_coat, float clear_coat_roughness);
    bool add_unlit_material(const char* material_name, float3 base_color, float4 emmisive);
    bool add_ibl_skybox(futils::Path file_path);
    glTF_Instance_ID add_gltf_asset_and_create_instance(const char* filepath);
    glTF_Instance_ID create_gltf_instance_sibling(glTF_Instance gltf_instance);

    Entity_ID add_filamesh_from_file(futils::Path file_path);
    Entity_ID add_plane(double length_x, double length_z, const char* material_name);
    Entity_ID add_cylinder(double radius, double height, const char* material_name);
    Entity_ID add_arrow(double radius, double length, const char* material_name);
    Entity_ID add_line(double3 begin, double3 end, const char* material_name);

    Entity_ID add_sun_light(float intenstiy_lux, fmath::float3 position);

    std::pair<fmt::Camera*, Entity_ID> create_filament_camera();
    fmt::View* create_filament_view();
    bool destroy_view(fmt::View* view);
    fmt::Renderer* create_filament_renderer();
    bool destroy_renderer(fmt::Renderer* renderer);
    fmt::SwapChain* create_swap_chain(uint32_t width, uint32_t height);
    fmt::SwapChain* create_swap_chain(void* native_window);
    bool destroy_swap_chain(fmt::SwapChain* swap_chain);
    fmt::Fence* create_fence();
    fmt::TransformManager& get_transform_manager();
    
private:
    Environment() {};
    Environment_ID m_id;

    Entity_ID add_and_delete_par_mesh(par_shapes_mesh* par_mesh, const char* material_name);

    fmt::Scene* m_scene = nullptr;
    fmt::Engine* m_engine = nullptr;
    futils::EntityManager& m_entity_manager = futils::EntityManager::get();

    std::vector<fmesh::MeshReader::Mesh> m_meshes;

    fmesh::MeshReader::MaterialRegistry m_material_registry; // keeps track of material instances
    fmt::Material* m_base_lit_material = nullptr;
    fmt::Material* m_base_unlit_material = nullptr;
    
    struct {
        fgltfio::MaterialProvider* material_provider = nullptr;
        fgltfio::TextureProvider* texture_provider = nullptr;
        fgltfio::AssetLoader* asset_loader = nullptr;
        std::vector<fgltfio::FilamentAsset*> assets;
    } m_gltf;
};
