#include "../environments.hpp"
#include "filament_object_wrappers.hpp"
#include <environment.hpp>

#include <camera.hpp>
#include <math.hpp>
#include <logging.hpp>
#include <object_manager.hpp>

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/Material.h>
#include <filament/IndirectLight.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/RenderableManager.h>
#include <filament/IndexBuffer.h>
#include <filament/VertexBuffer.h>
#include <utils/EntityManager.h>
#include <utils/Path.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/TextureProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/FilamentInstance.h>
#include <filament-iblprefilter/IBLPrefilterContext.h>
#include <math/norm.h>

#include <stb_image.h>

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace futils = utils;
namespace fmath = filament::math;

#define ASSET_PATH "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/"
#define DEFAULT_LIT_MATERIAL_PATH ASSET_PATH "sandboxLit.filamat"
#define DEFAULT_UNLIT_MATERIAL_PATH ASSET_PATH "sandboxUnlit.filamat"

constexpr filament::backend::Backend ENGINE_BACKEND = fmt::Engine::Backend::OPENGL;

static bool read_entire_file(const char* path, uint8_t** data, size_t* size)
{
    int fd = open(path, O_RDONLY);
    
    if (fd >= 0) {
        *size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        *data = new uint8_t[*size];
        read(fd, *data, *size);
        
        if (!*data) {
            env_hard_error(ENV_ERR_MEM_ALLOC);
        }
        close(fd);
        return true;
    }
    else {
        env_soft_error("Unable to open file '%s'", path);
        return false;
    }
}

static fmt::Material* load_material_from_file(filament::Engine* engine, const char* path)
{
    fmt::Material* material = nullptr;

    size_t size = 0;
    uint8_t* data = nullptr;

    if (read_entire_file(path, &data, &size)) {
        material = fmt::Material::Builder().package(data, size).build(*engine);
        delete[] data;
    }
    else {
        env_soft_error("Failed to load material",  material);
    }
    
    return material;
}

Environment_ID create_environment()
{
    Environment* env = new Environment;

    env->engine = fmt::Engine::create(ENGINE_BACKEND);
    env->scene = env->engine->createScene();
    
    env->base_lit_material = load_material_from_file(env->engine, DEFAULT_LIT_MATERIAL_PATH);
    env->base_unlit_material = load_material_from_file(env->engine, DEFAULT_UNLIT_MATERIAL_PATH);

    env->gltf.material_provider = fgltfio::createJitShaderProvider(env->engine);
    env->gltf.texture_provider = fgltfio::createStbProvider(env->engine);

    fgltfio::AssetConfiguration asset_loader_config{
        .engine = env->engine,
        .materials = env->gltf.material_provider,
        .entities = &futils::EntityManager::get(),
    };
        
    env->gltf.asset_loader = fgltfio::AssetLoader::create(asset_loader_config);
    
    Environment_ID env_id = g_objm.add_object(env);
    g_objm.environment_activate(env_id);
    
    add_lit_material("DefaultMaterial");
    return env_id;
}

Environment::~Environment()
{
    // destroy gltf stuff
    for (fgltfio::FilamentAsset* asset : gltf.assets) {
        scene->removeEntities(asset->getEntities(), asset->getEntityCount());
        gltf.asset_loader->destroyAsset(asset);
    }

    material_registry.unregisterAll();
        
    gltf.material_provider->destroyMaterials();
    delete gltf.material_provider;
    delete gltf.texture_provider;
    fgltfio::AssetLoader::destroy(&gltf.asset_loader);

    // destroy handles
    engine->destroy(scene);

    // FIXME: WE SHOULD DESTROY THE ENGINE, BUT FILAMENT CRASHES HERE SOMETIMES
    // fmt::Engine::destroy(&engine);
}

bool add_ibl_skybox(const char* file_path_cstr)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;
    
    futils::Path path{file_path_cstr};

    int width, height, n_channels;
    stbi_info(path.getAbsolutePath().c_str(), &width, &height, nullptr);
    // load image as float
    size_t size = width * height * sizeof(fmath::float3);
    fmath::float3* data = (fmath::float3*)stbi_loadf(path.getAbsolutePath().c_str(), &width, &height, &n_channels, 3);
    fmt::Texture::PixelBufferDescriptor::Callback destroy_callback = [](void* data, size_t, void*) {
        stbi_image_free(data);
    };

    if (data == nullptr || n_channels != 3) {
        env_soft_error("Could not decode image: %s", file_path_cstr);
        destroy_callback(data, size, nullptr);
        return false;
    }

    if (width != height * 2) {
        env_soft_error("Not an equirectangular image: %s", file_path_cstr);
        destroy_callback(data, size, nullptr);
        return false;
    }

    // load the texture
    fmt::Texture::PixelBufferDescriptor buffer(
        data, size, fmt::Texture::Format::RGB, fmt::Texture::Type::FLOAT, destroy_callback, nullptr);
    
    fmt::Texture* equirect = fmt::Texture::Builder()
        .width((uint32_t)width)
        .height((uint32_t)height)
        .levels(0xff)
        .format(fmt::Texture::InternalFormat::R11F_G11F_B10F)
        .sampler(fmt::Texture::Sampler::SAMPLER_2D)
        .build(*env->engine);

    equirect->setImage(*env->engine, 0, std::move(buffer));

    IBLPrefilterContext context(*env->engine);
    IBLPrefilterContext::EquirectangularToCubemap equirectangularToCubemap(context);
    IBLPrefilterContext::SpecularFilter specularFilter(context);
    IBLPrefilterContext::IrradianceFilter irradianceFilter(context);

    fmt::Texture* skybox_texture = equirectangularToCubemap(equirect);
    fmt::Texture* texture = specularFilter(skybox_texture);
    
    fmt::IndirectLight* indirect_light = fmt::IndirectLight::Builder()
        .reflections(texture)
        .intensity(20000.0f)
        .build(*env->engine);

    fmt::Skybox* skybox = fmt::Skybox::Builder()
            .environment(skybox_texture)
            .showSun(true)
            .build(*env->engine);

    env->scene->setIndirectLight(indirect_light);
    env->scene->setSkybox(skybox);

    return true;
}

Filament_Entity_ID add_filamesh_from_file(const char* path)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {ENV_INVALID_UUID};
    
    fmesh::MeshReader::Mesh mesh = filamesh::MeshReader::loadMeshFromFile(
        env->engine, futils::Path(path), env->material_registry);

    env->scene->addEntity(mesh.renderable);

    // add transform component to the mesh (make it transformable)
    env->engine->getTransformManager().create(mesh.renderable);

    return g_objm.add_object({mesh.renderable, env});
}

static fmt::MaterialInstance* create_material_instance(Environment* env, float3 base_color, float roughness, float metallic, float reflectance, float sheen_color, float clear_coat, float clear_coat_roughness)
{
    fmt::MaterialInstance* mat_i = env->base_lit_material->createInstance();
    mat_i->setParameter("baseColor", fmt::RgbType::sRGB, f3_to_ff3(base_color));
    mat_i->setParameter("roughness", roughness);
    mat_i->setParameter("metallic", metallic);
    mat_i->setParameter("reflectance", reflectance);
    mat_i->setParameter("sheenColor", sheen_color);
    mat_i->setParameter("clearCoat", clear_coat);
    mat_i->setParameter("clearCoatRoughness", clear_coat_roughness);
    return mat_i;
}

static fmt::MaterialInstance* create_material_instance(Environment* env, float3 base_color, float4 emmisive)
{
    fmt::MaterialInstance* mat_i = env->base_unlit_material->createInstance();
    mat_i->setParameter("baseColor", fmt::RgbType::sRGB, fmath::float3{base_color.x, base_color.y, base_color.z});
    mat_i->setParameter("emissive", f4_to_ff4(emmisive));
    return mat_i;
}

bool add_lit_material(const char* material_name, float3 base_color, float roughness, float metallic, float reflectance, float sheen_color, float clear_coat, float clear_coat_roughness)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;

    fmt::MaterialInstance* mat_i = create_material_instance(env, base_color, roughness, metallic, reflectance, sheen_color, clear_coat, clear_coat_roughness);
    env->material_registry.registerMaterialInstance(futils::CString{material_name}, mat_i);
    return true;
}

bool add_unlit_material(const char* material_name, float3 base_color, float4 emmisive)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;

    fmt::MaterialInstance* mat_i = create_material_instance(env, base_color, emmisive);
    env->material_registry.registerMaterialInstance(futils::CString{material_name}, mat_i);
    return true;
}

glTF_Instance_ID add_gltf_asset_and_create_instance(const char* filepath)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {ENV_INVALID_UUID};

    uint8_t* data = nullptr;
    size_t size = 0;
    fgltfio::FilamentAsset* asset = nullptr;
    if (read_entire_file(filepath, &data, &size)) {
        asset = env->gltf.asset_loader->createAsset(data, size);
        delete[] data;
    }
    else {
        return {ENV_INVALID_UUID};
    }

    fgltfio::ResourceLoader resource_loader({env->engine, filepath, true});
    resource_loader.addTextureProvider("image/png", env->gltf.texture_provider);
    resource_loader.addTextureProvider("image/jpeg", env->gltf.texture_provider);
    resource_loader.loadResources(asset);
    
    env->gltf.assets.push_back(asset);

    // Never remove cpu side data, because we always want to be able to create new instances
    // asset->releaseSourceData();

    fgltfio::FilamentInstance* instance = asset->getInstance();
    env->scene->addEntities(instance->getEntities(), instance->getEntityCount());
    
    return g_objm.add_object({instance, env});
}

glTF_Instance_ID create_gltf_instance_sibling(glTF_Instance_ID gltf_instance_id)
{
    glTF_Instance instance = g_objm.get_object(gltf_instance_id);
    if (!instance.is_valid()) return {ENV_INVALID_UUID};
    
    // we are violating constness here, but I don't think this is an issue.
    fgltfio::FilamentInstance* sibling_instance = instance.associated_env->gltf.asset_loader->createInstance(
        (fgltfio::FilamentAsset*)instance.gltf_instance->getAsset());
    instance.associated_env->scene->addEntities(sibling_instance->getEntities(), sibling_instance->getEntityCount());
    return g_objm.add_object({sibling_instance, instance.associated_env});
}

/* DONT FORGET: LEAKING MEMORY (just temporary) */
Filament_Entity_ID add_plane(double3 center, double length_x, double length_z, const char* material_name, Quaternion rotation)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {ENV_INVALID_UUID};

    uint32_t* indices = new uint32_t[6]{ 0, 1, 2, 2, 3, 0 };

    filament::math::float3* vertices = new filament::math::float3[4]{
        d3_to_fd3(quaternion_rotate_vector({ -length_x / 2.0, 0, -length_z / 2.0 }, rotation)) + d3_to_fd3(center),
        d3_to_fd3(quaternion_rotate_vector({ -length_x / 2.0, 0,  length_z / 2.0 }, rotation)) + d3_to_fd3(center),
        d3_to_fd3(quaternion_rotate_vector({  length_x / 2.0, 0,  length_z / 2.0 }, rotation)) + d3_to_fd3(center),
        d3_to_fd3(quaternion_rotate_vector({  length_x / 2.0, 0, -length_z / 2.0 }, rotation)) + d3_to_fd3(center)
    };

    fmath::short4 tbn = fmath::packSnorm16(
        fmath::mat3f::packTangentFrame(
            fmath::mat3f{d3_to_fd3(quaternion_rotate_vector({ 1.0f, 0.0f, 0.0f }, rotation)),
                         d3_to_fd3(quaternion_rotate_vector({ 0.0f, 0.0f, 1.0f }, rotation)),
                         d3_to_fd3(quaternion_rotate_vector({ 0.0f, 1.0f, 0.0f }, rotation))}
            ).xyzw);

    fmath::short4* normals = new fmath::short4[4]{ tbn, tbn, tbn, tbn };

    fmt::VertexBuffer* vertex_buffer = fmt::VertexBuffer::Builder()
        .vertexCount(4)
        .bufferCount(2)
        .attribute(fmt::VertexAttribute::POSITION, 0, fmt::VertexBuffer::AttributeType::FLOAT3)
        .attribute(fmt::VertexAttribute::TANGENTS, 1, fmt::VertexBuffer::AttributeType::SHORT4)
        .normalized(fmt::VertexAttribute::TANGENTS)
        .build(*env->engine);

    vertex_buffer->setBufferAt(*env->engine, 0, fmt::VertexBuffer::BufferDescriptor(
                                  vertices, vertex_buffer->getVertexCount() * sizeof(vertices[0])));
    vertex_buffer->setBufferAt(*env->engine, 1, fmt::VertexBuffer::BufferDescriptor(
                                  normals, vertex_buffer->getVertexCount() * sizeof(normals[0])));

    fmt::IndexBuffer* index_buffer = fmt::IndexBuffer::Builder()
        .indexCount(6)
        .build(*env->engine);

    index_buffer->setBuffer(*env->engine, fmt::IndexBuffer::BufferDescriptor(
                               indices, index_buffer->getIndexCount() * sizeof(uint32_t)));

    futils::Entity plane_renderable = utils::EntityManager::get().create();
    fmt::RenderableManager::Builder(1)
        .boundingBox({{0, 0, 0},
                      { length_x / 2.0, 1e-4f, length_z / 2.0 }}) // this is bullshit, but still working (this code is depricated anyway)
        // .material(0, create_material_instance(env, material_settings))
        .material(0, env->material_registry.getMaterialInstance(futils::CString(material_name)))
        .geometry(0, fmt::RenderableManager::PrimitiveType::TRIANGLES,
                  vertex_buffer, index_buffer, 0, 6)
        .culling(false)
        .receiveShadows(true)
        .castShadows(false)
        .build(*env->engine, plane_renderable);

    env->scene->addEntity(plane_renderable);
    
    return g_objm.add_object({plane_renderable, env});
}

/* DONT FORGET: LEAKING MEMORY (just temporary) */
Filament_Entity_ID add_line(double3 begin, double3 end, const char* material_name)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {ENV_INVALID_UUID};
    
    uint32_t* indices = new uint32_t[2] { 0, 1 };
    
    filament::math::float3* vertices = new filament::math::float3[2] {
        d3_to_fd3(begin), d3_to_fd3(end)
    };

    fmt::VertexBuffer* vertex_buffer = fmt::VertexBuffer::Builder()
        .vertexCount(2)
        .bufferCount(1)
        .attribute(fmt::VertexAttribute::POSITION, 0, fmt::VertexBuffer::AttributeType::FLOAT3)
        .build(*env->engine);

    vertex_buffer->setBufferAt(*env->engine, 0, fmt::VertexBuffer::BufferDescriptor(
                                   vertices, vertex_buffer->getVertexCount() * sizeof(vertices[0])));

    fmt::IndexBuffer* index_buffer = fmt::IndexBuffer::Builder()
        .indexCount(2)
        .build(*env->engine);

    index_buffer->setBuffer(*env->engine, fmt::IndexBuffer::BufferDescriptor(
                                indices, index_buffer->getIndexCount() * sizeof(uint32_t)));

    
    futils::Entity line_renderable = utils::EntityManager::get().create();
    fmt::RenderableManager::Builder(1)
        .boundingBox({d3_to_fd3(begin),
                      d3_to_fd3(end)})
        .material(0, env->material_registry.getMaterialInstance(futils::CString(material_name)))
        .geometry(0, fmt::RenderableManager::PrimitiveType::LINES,
                  vertex_buffer, index_buffer, 0, 2)
        .culling(false)
        .receiveShadows(false)
        .castShadows(false)
        .build(*env->engine, line_renderable);

    env->scene->addEntity(line_renderable);
    
    return g_objm.add_object({line_renderable, env});
}
