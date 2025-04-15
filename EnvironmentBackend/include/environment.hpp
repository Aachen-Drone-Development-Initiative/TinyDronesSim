#include <filameshio/MeshReader.h>

#include <vector>

namespace filament {
    class Scene;
    class Engine;
    class Material;
    
    namespace gltfio {
        class AssetLoader;
        class MaterialProvider;
        class TextureProvider;
        class FilamentAsset;
    }
}

namespace fmt = filament;
namespace fmesh = filamesh;
namespace fgltfio = filament::gltfio;

struct Environment {
    ~Environment();
    
    fmt::Scene* scene = nullptr;
    fmt::Engine* engine = nullptr;

    fmesh::MeshReader::MaterialRegistry material_registry; // keeps track of materials
    fmt::Material* base_lit_material = nullptr;
    fmt::Material* base_unlit_material = nullptr;
    struct {
        fgltfio::MaterialProvider* material_provider = nullptr;
        fgltfio::TextureProvider* texture_provider = nullptr;
        fgltfio::AssetLoader* asset_loader = nullptr;
        std::vector<fgltfio::FilamentAsset*> assets;
    } gltf;
};

// void __destroy_all_gltf_instances_and_asset(fgltfio::FilamentInstance* instace, Environment* env);
