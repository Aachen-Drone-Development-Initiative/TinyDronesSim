#include "../tinydronessim_backend.hpp"

#include "entity.hpp"
#include "math/mathfwd.h"
#include "par_shapes.h"

#include <environment.hpp>

#include <embedded_asset_info.hpp>
#include <camera.hpp>
#include <math.hpp>
#include <logging.hpp>
#include <object_manager.hpp>

#include <filament/Engine.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filament/Scene.h>
#include <filament/Material.h>
#include <filament/IndirectLight.h>
#include <filament/Skybox.h>
#include <filament/TransformManager.h>
#include <filament/RenderableManager.h>
#include <filament/IndexBuffer.h>
#include <filament/VertexBuffer.h>
#include <filament/LightManager.h>
#include <filament/Fence.h>
#include <utils/EntityManager.h>
#include <utils/Path.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/TextureProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/FilamentInstance.h>
#include <filament-iblprefilter/IBLPrefilterContext.h>
#include <math/norm.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <stb_image.h>

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

constexpr filament::backend::Backend ENGINE_BACKEND = fmt::Engine::Backend::VULKAN;

static bool read_entire_file(const char* path, uint8_t** data, size_t* size)
{
    int fd = open(path, O_RDONLY);
    
    if (fd >= 0) {
        *size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        *data = new uint8_t[*size];
        read(fd, *data, *size);
        
        if (!*data) {
            tds_hard_error(ENV_ERR_MEM_ALLOC);
        }
        close(fd);
        return true;
    }
    else {
        tds_soft_error("Unable to open file '%s'", path);
        return false;
    }
}

static fmt::Material* load_material_from_buffer(filament::Engine* engine, const uint8_t* buffer, unsigned int size)
{
    fmt::Material* material = fmt::Material::Builder().package(buffer, size).build(*engine);
    if (!material) {
        tds_soft_error("Failed to load material");
    }
    return material;
}

static void init_google_filament_if_not_init()
{
    static bool initialized = false;
    if (initialized) return;

    // HACK: Disabling VSync because waiting for the green light from vsync somehow introduces a lot of lag
    if (ENGINE_BACKEND == fmt::Engine::Backend::VULKAN) {
        setenv("MESA_VK_WSI_PRESENT_MODE", "immediate", 1);
    }
    else if (ENGINE_BACKEND == fmt::Engine::Backend::OPENGL) {
        setenv("vblank_mode", "0", 1);
    }
    
    initialized = true;
}

Environment_ID Environment::create()
{
    Environment* env = new Environment;

    init_google_filament_if_not_init();

    env->m_engine = fmt::Engine::Builder().backend(ENGINE_BACKEND).build();

    if (!env->m_engine) {
        tds_soft_error("Failed to create a Filament Engine.");
        delete env;
        return {TDS_INVALID_ID};
    }
    
    env->m_scene = env->m_engine->createScene();

    // Load the material "templates"
    env->m_base_lit_material = load_material_from_buffer(env->m_engine, __assets_sandboxLit_filamat, __assets_sandboxLit_filamat_len);
    env->m_base_unlit_material = load_material_from_buffer(env->m_engine, __assets_sandboxUnlit_filamat, __assets_sandboxUnlit_filamat_len);

    if (!env->m_base_lit_material || !env->m_base_unlit_material) {
        tds_soft_error("Failed to load Material Shaders from memory.");
        delete env;
        return {TDS_INVALID_ID};
    }
    
    if (!env->add_lit_material("DefaultMaterial", {1.f, 1.f, 1.f}, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f)) {
        tds_soft_error("Failed to create the Default Material instance.");
        delete env;
        return {TDS_INVALID_ID};
    }

    // initialize gltf-loader
    env->m_gltf.material_provider = fgltfio::createJitShaderProvider(env->m_engine);
    env->m_gltf.texture_provider = fgltfio::createStbProvider(env->m_engine);

    env->m_gltf.asset_loader = fgltfio::AssetLoader::create({
            .engine = env->m_engine,
            .materials = env->m_gltf.material_provider,
            .entities = &futils::EntityManager::get()});

    env->m_id = g_objm.add_object(env);
    g_objm.environment_activate(env->m_id);

    return env->m_id;
}

TDS_API Environment_ID env_create() { return Environment::create(); }

Environment::~Environment()
{
    // destroy gltf stuff
    for (fgltfio::FilamentAsset* asset : m_gltf.assets) {
        m_scene->removeEntities(asset->getEntities(), asset->getEntityCount());
        m_gltf.asset_loader->destroyAsset(asset);
    }

    std::vector<filament::MaterialInstance*> material_instances(m_material_registry.numRegistered());
    m_material_registry.getRegisteredMaterials(material_instances.data());
    for (auto mat_i : material_instances) {
        m_engine->destroy(mat_i);
    }
    m_material_registry.unregisterAll();
    
    m_gltf.material_provider->destroyMaterials();
    delete m_gltf.material_provider;
    delete m_gltf.texture_provider;
    fgltfio::AssetLoader::destroy(&m_gltf.asset_loader);

    // destroy handles
    m_engine->destroy(m_scene);

    // FIXME: WE SHOULD DESTROY THE ENGINE, BUT FILAMENT CRASHES HERE SOMETIMES
    // fmt::Engine::destroy(&m_engine);
}

bool Environment::add_ibl_skybox(futils::Path file_path)
{
    int width, height, n_channels;
    stbi_info(file_path.getAbsolutePath().c_str(), &width, &height, nullptr);
    // load image as float
    size_t size = width * height * sizeof(fmath::float3);
    fmath::float3* data = (fmath::float3*)stbi_loadf(file_path.getAbsolutePath().c_str(), &width, &height, &n_channels, 3);
    fmt::Texture::PixelBufferDescriptor::Callback destroy_callback = [](void* data, size_t, void*) {
        stbi_image_free(data);
    };

    if (data == nullptr || n_channels != 3) {
        tds_soft_error("Could not decode image: %s", file_path.c_str());
        destroy_callback(data, size, nullptr);
        return false;
    }

    if (width != height * 2) {
        tds_soft_error("Not an equirectangular image: %s", file_path.c_str());
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
        .build(*m_engine);

    equirect->setImage(*m_engine, 0, std::move(buffer));

    IBLPrefilterContext context(*m_engine);
    IBLPrefilterContext::EquirectangularToCubemap equirectangularToCubemap(context);
    IBLPrefilterContext::SpecularFilter specularFilter(context);
    IBLPrefilterContext::IrradianceFilter irradianceFilter(context);

    fmt::Texture* skybox_texture = equirectangularToCubemap(equirect);
    fmt::Texture* texture = specularFilter(skybox_texture);
    
    fmt::IndirectLight* indirect_light = fmt::IndirectLight::Builder()
        .reflections(texture)
        .intensity(20000.0f)
        .build(*m_engine);

    fmt::Skybox* skybox = fmt::Skybox::Builder()
            .environment(skybox_texture)
            .showSun(true)
            .build(*m_engine);

    m_scene->setIndirectLight(indirect_light);
    m_scene->setSkybox(skybox);

    return true;
}


TDS_API bool env_add_ibl_skybox(const char* file_path)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;

    return env->add_ibl_skybox({file_path});
}

Entity_ID Environment::add_filamesh_from_file(futils::Path file_path)
{
    fmesh::MeshReader::Mesh mesh = filamesh::MeshReader::loadMeshFromFile(m_engine, file_path, m_material_registry);
    if (!mesh.renderable) return {TDS_INVALID_ID};
    
    m_scene->addEntity(mesh.renderable);
    m_meshes.push_back(mesh);
        
    // add transform component to the mesh (make it transformable)
    m_engine->getTransformManager().create(mesh.renderable);

    return g_objm.add_object({mesh.renderable, this});
}

TDS_API Entity_ID env_add_filamesh_from_file(const char* file_path)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};
    
    return env->add_filamesh_from_file({file_path});
}

Entity_ID Environment::add_sun_light(float intenstiy_lux, fmath::float3 position)
{
    futils::Entity light = futils::EntityManager::get().create();
    if (!light) return {TDS_INVALID_ID};
    
    fmt::LightManager::Builder(fmt::LightManager::Type::SUN)
        .color(fmt::Color::toLinear<fmt::ACCURATE>({0.98f, 0.92f, 0.89f}))
        .intensity(intenstiy_lux).castShadows(true)
        .position(position)
        .build(*m_engine, light);
    m_scene->addEntity(light);

    m_engine->getTransformManager().create(light);

    return g_objm.add_object({light, this});
}

TDS_API Entity_ID env_add_sun_light(float intenstiy_lux, float3 position)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};
    
    return env->add_sun_light(intenstiy_lux, f3_to_ff3(position));
}

bool Environment::add_lit_material(const char* material_name, float3 base_color, float roughness, float metallic, float reflectance, float sheen_color, float clear_coat, float clear_coat_roughness)
{
    fmt::MaterialInstance* mat_i = m_base_lit_material->createInstance();
    if (!mat_i) return false;
    
    mat_i->setParameter("baseColor", fmt::RgbType::sRGB, f3_to_ff3(base_color));
    mat_i->setParameter("roughness", roughness);
    mat_i->setParameter("metallic", metallic);
    mat_i->setParameter("reflectance", reflectance);
    mat_i->setParameter("sheenColor", sheen_color);
    mat_i->setParameter("clearCoat", clear_coat);
    mat_i->setParameter("clearCoatRoughness", clear_coat_roughness);
    m_material_registry.registerMaterialInstance(futils::CString{material_name}, mat_i);
    
    return true;
}

bool Environment::add_unlit_material(const char* material_name, float3 base_color, float4 emmisive)
{
    fmt::MaterialInstance* mat_i = m_base_unlit_material->createInstance();
    if (!mat_i) return false;
    
    mat_i->setParameter("baseColor", fmt::RgbType::sRGB, f3_to_ff3(base_color));
    mat_i->setParameter("emissive", f4_to_ff4(emmisive));
    m_material_registry.registerMaterialInstance(futils::CString{material_name}, mat_i);
    
    return true;
}

TDS_API bool env_add_lit_material(const char* material_name, float3 base_color, float roughness, float metallic, float reflectance, float sheen_color, float clear_coat, float clear_coat_roughness)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;
    
    return env->add_lit_material(material_name, base_color, roughness, metallic, reflectance, sheen_color, clear_coat, clear_coat_roughness);
}

TDS_API bool env_add_unlit_material(const char* material_name, float3 base_color, float4 emmisive)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return false;
    
    return env->add_unlit_material(material_name, base_color, emmisive);
}

glTF_Instance_ID Environment::add_gltf_asset_and_create_instance(const char* file_path)
{
    uint8_t* data = nullptr;
    size_t size = 0;
    fgltfio::FilamentAsset* asset = nullptr;
    if (read_entire_file(file_path, &data, &size)) {
        asset = m_gltf.asset_loader->createAsset(data, size);
        delete[] data;
    }
    else {
        return {TDS_INVALID_ID};
    }

    fgltfio::ResourceLoader resource_loader({m_engine, file_path, true});
    resource_loader.addTextureProvider("image/png", m_gltf.texture_provider);
    resource_loader.addTextureProvider("image/jpeg", m_gltf.texture_provider);
    resource_loader.loadResources(asset);
    
    m_gltf.assets.push_back(asset);

    // Never remove cpu side data, because we always want to be able to create new instances
    // asset->releaseSourceData();

    fgltfio::FilamentInstance* instance = asset->getInstance();
    m_scene->addEntities(instance->getEntities(), instance->getEntityCount());
    
    return g_objm.add_object({instance, this});
}

TDS_API glTF_Instance_ID env_add_gltf_asset_and_create_instance(const char* file_path)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};
    
    return env->add_gltf_asset_and_create_instance(file_path);
}

glTF_Instance_ID Environment::create_gltf_instance_sibling(glTF_Instance gltf_instance)
{
    // we are violating constness here, but I don't think this is an issue.
    fgltfio::FilamentInstance* sibling_instance = m_gltf.asset_loader->createInstance((fgltfio::FilamentAsset*)gltf_instance.gltf_instance->getAsset());
    m_scene->addEntities(sibling_instance->getEntities(), sibling_instance->getEntityCount());
    return g_objm.add_object({sibling_instance, gltf_instance.associated_env});
}

TDS_API glTF_Instance_ID gltf_inst_create_sibling(glTF_Instance_ID gltf_instance_id)
{
    glTF_Instance instance = g_objm.get_object(gltf_instance_id);
    if (!instance.is_valid()) return {TDS_INVALID_ID};
    if (!instance.associated_env) return {TDS_INVALID_ID};
    
    return instance.associated_env->create_gltf_instance_sibling(instance);
}

Entity_ID Environment::add_and_delete_par_mesh(par_shapes_mesh* par_mesh, const char* material_name)
{
    fmath::float3 aabb[2] = {};
    // we precompute the aabb, because we are freeing the memory before we use the aabb
    par_shapes_compute_aabb(par_mesh, (float*) aabb);
    
    // Creating Vertex Buffer

    auto delete_par_points = [](void* buffer, size_t, void*) -> void {
        PAR_FREE((decltype(par_mesh->points)) buffer);
    };

    fmath::short4* tangent_frame = nullptr;
    auto delete_tangent_frame = [](void* buffer, size_t, void*) -> void {
        PAR_FREE((decltype(tangent_frame)) buffer);
    };

    auto vertex_buffer_builder = fmt::VertexBuffer::Builder()
        .vertexCount(par_mesh->npoints);
    
    if (par_mesh->normals) {
        // Converting Normals to tangent-frame (Tangent, Bitangent, Normal)
        // Usually the tangent-frame also describes the orientation of the meshes texture,
        // though we assume, that there is no texture. So we just compute an arbitray tangent/bitangent.
        
        // tds_assert(par_mesh->tcoords == nullptr);
        
        tangent_frame = new fmath::short4[par_mesh->npoints];
        for (int i = 0; i < par_mesh->npoints; ++i) {
            fmath::float3 normal = ((fmath::float3*) par_mesh->normals)[i];
            fmath::float3 tangent_1 = cross(normal, fmath::float3{0.0, 0.0, 1.0});
            fmath::float3 tangent_2 = cross(normal, fmath::float3{0.0, 1.0, 0.0});

            fmath::float3 tangent;
            if (length(tangent_1) > length(tangent_2)) {
                tangent = normalize(tangent_1);
            }
            else {
                tangent = normalize(tangent_2);
            }

            fmath::float3 bitangent = normalize(cross(tangent, normal));
            
            tangent_frame[i] = fmath::packSnorm16(
                fmath::mat3f::packTangentFrame(
                    fmath::mat3f{tangent, bitangent, normal}).xyzw);
        }
        
        vertex_buffer_builder.bufferCount(2)
            .attribute(fmt::VertexAttribute::TANGENTS, 1, fmt::VertexBuffer::AttributeType::SHORT4)
            .normalized(fmt::VertexAttribute::TANGENTS);
    }
    else {
        vertex_buffer_builder.bufferCount(1);
    }

    fmt::VertexBuffer* vertex_buffer = vertex_buffer_builder
        .attribute(fmt::VertexAttribute::POSITION, 0, fmt::VertexBuffer::AttributeType::FLOAT3)
        .build(*m_engine);
        
    vertex_buffer->setBufferAt(*m_engine, 0, fmt::VertexBuffer::BufferDescriptor(
                                   par_mesh->points, vertex_buffer->getVertexCount() * sizeof(fmath::float3), delete_par_points));
    par_mesh->points = nullptr; // now the vertex_buffer owns the memory

    
    if (tangent_frame) {
        vertex_buffer->setBufferAt(*m_engine, 1, fmt::VertexBuffer::BufferDescriptor(
                                       tangent_frame, vertex_buffer->getVertexCount() * sizeof(tangent_frame[0]), delete_tangent_frame));
    }

    // Creating Index-Buffer

    auto delete_par_indices = [](void* buffer, size_t, void*) -> void {
        PAR_FREE((decltype(par_mesh->triangles)) buffer);
    };

    tds_assert(sizeof(PAR_SHAPES_T) == sizeof(uint16_t)); // using USHORT
    
    fmt::IndexBuffer* index_buffer = fmt::IndexBuffer::Builder()
        .bufferType(fmt::IndexBuffer::IndexType::USHORT)
        .indexCount(par_mesh->ntriangles * 3)
        .build(*m_engine);

    index_buffer->setBuffer(*m_engine, fmt::IndexBuffer::BufferDescriptor(
                                par_mesh->triangles, index_buffer->getIndexCount() * sizeof(uint32_t), delete_par_indices));
    par_mesh->triangles = nullptr; // now the index_buffer owns the memory

    // Creating a renderable entity

    futils::Entity plane_renderable = utils::EntityManager::get().create();
    fmt::RenderableManager::Builder(1)
        .boundingBox(fmt::Box().set(aabb[0], aabb[1]))
        .material(0, m_material_registry.getMaterialInstance(futils::CString(material_name)))
        .geometry(0, fmt::RenderableManager::PrimitiveType::TRIANGLES, vertex_buffer, index_buffer, 0, par_mesh->ntriangles * 3)
        .culling(true)
        .receiveShadows(true)
        .castShadows(true)
        .build(*m_engine, plane_renderable);

    m_scene->addEntity(plane_renderable);

    par_shapes_free_mesh(par_mesh); // Frees everything that was stil managed by the par_mesh (ie. not nulled)
    
    return g_objm.add_object({plane_renderable, this});
}

Entity_ID Environment::add_plane(double length_x, double length_z, const char* material_name)
{
    par_shapes_mesh* plane = par_shapes_create_plane(1, 1); // creates "unit" (1x1) plane with (slices * stacks * 2) = 2 triangles
    float axis[3] = {1.0, 0.0, 0.0};
    par_shapes_rotate(plane, -PAR_PI / 2.0, axis);
    
    { // Bottom part (Plane should be visible from both sides)
        par_shapes_mesh* plane_bottom = par_shapes_create_plane(1, 1); // creates "unit" (1x1) plane with (slices * stacks * 2) = 2 triangles
        par_shapes_rotate(plane_bottom, -PAR_PI / 2.0, axis);
        par_shapes_invert(plane_bottom, 0, 0);
        par_shapes_merge_and_free(plane, plane_bottom);
    }
    
    par_shapes_translate(plane, -0.5, 0, 0.5);
    par_shapes_scale(plane, length_x, 0, length_z);
    
    return add_and_delete_par_mesh(plane, material_name);
}

TDS_API Entity_ID env_add_plane(double length_x, double length_z, const char* material_name)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};

    return env->add_plane(length_x, length_z, material_name);
}

Entity_ID Environment::add_cylinder(double radius, double height, const char* material_name)
{
    const int slices = 100;
    par_shapes_mesh* cylinder = par_shapes_create_cylinder(slices, 1); // creates "unit" (1x1) cylinder with 50 faces on the curvature
    float bottom_center[3] = {0, 0, 0};
    float bottom_normal[3] = {0, 0, 1};
    par_shapes_mesh* bottom = par_shapes_create_disk(1, slices, bottom_center, bottom_normal);
    par_shapes_invert(bottom, 0, 0); // normal = {0, 0, -1}; does not work. This is an alternative
    par_shapes_merge_and_free(cylinder, bottom);
    
    float top_center[3] = {0, 0, 1};
    float top_normal[3] = {0, 0, 1};
    par_shapes_merge_and_free(cylinder, par_shapes_create_disk(1, slices, top_center, top_normal));

    float axis[3] = {1, 0, 0};
    par_shapes_rotate(cylinder, -PAR_PI / 2.0, axis);
    par_shapes_scale(cylinder, radius, height, radius);
    
    return add_and_delete_par_mesh(cylinder, material_name);
}

TDS_API Entity_ID env_add_cylinder(double radius, double height, const char* material_name)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};

    return env->add_cylinder(radius, height, material_name);
}

Entity_ID Environment::add_arrow(double radius, double length, const char* material_name)
{
    const int slices = 50;
    par_shapes_mesh* arrow;
    
    { // Base
        arrow = par_shapes_create_cylinder(slices, 1); // creates "unit" (1x1) cylinder with 50 faces on the curvature
        float bottom_center[3] = {0, 0, 0};
        float bottom_normal[3] = {0, 0, 1};
        par_shapes_mesh* bottom = par_shapes_create_disk(1, slices, bottom_center, bottom_normal);
        par_shapes_invert(bottom, 0, 0); // normal = {0, 0, -1}; does not work. This is an alternative
        par_shapes_merge_and_free(arrow, bottom);
    
        par_shapes_scale(arrow, radius, radius, length);
    }

    { // Cone
        par_shapes_mesh* cone = par_shapes_create_cone(slices, 1);
        float bottom_center[3] = {0, 0, 0};
        float bottom_normal[3] = {0, 0, 1};
        par_shapes_mesh* bottom = par_shapes_create_disk(1, slices, bottom_center, bottom_normal);
        par_shapes_invert(bottom, 0, 0); // normal = {0, 0, -1}; does not work. This is an alternative
        par_shapes_merge_and_free(cone, bottom);

        par_shapes_scale(cone, radius * 2, radius * 2, radius * 4);
        par_shapes_translate(cone, 0, 0, length);
        par_shapes_merge_and_free(arrow, cone);
    }
    
    float axis[3] = {1, 0, 0};
    par_shapes_rotate(arrow, -PAR_PI / 2.0, axis);
    
    return Entity_ID{ add_and_delete_par_mesh(arrow, material_name) };
}

TDS_API Entity_ID env_add_arrow(double radius, double length, const char* material_name)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};

    return env->add_arrow(radius, length, material_name);
}

/* DONT FORGET: LEAKING MEMORY (just temporary) */
Entity_ID Environment::add_line(double3 begin, double3 end, const char* material_name)
{
    uint32_t* indices = new uint32_t[2] { 0, 1 };
    
    filament::math::float3* vertices = new filament::math::float3[2] {
        d3_to_fd3(begin), d3_to_fd3(end)
    };

    fmt::VertexBuffer* vertex_buffer = fmt::VertexBuffer::Builder()
        .vertexCount(2)
        .bufferCount(1)
        .attribute(fmt::VertexAttribute::POSITION, 0, fmt::VertexBuffer::AttributeType::FLOAT3)
        .build(*m_engine);

    vertex_buffer->setBufferAt(*m_engine, 0, fmt::VertexBuffer::BufferDescriptor(
                                   vertices, vertex_buffer->getVertexCount() * sizeof(vertices[0])));

    fmt::IndexBuffer* index_buffer = fmt::IndexBuffer::Builder()
        .indexCount(2)
        .build(*m_engine);

    index_buffer->setBuffer(*m_engine, fmt::IndexBuffer::BufferDescriptor(
                                indices, index_buffer->getIndexCount() * sizeof(uint32_t)));

    futils::Entity line_renderable = utils::EntityManager::get().create();
    fmt::RenderableManager::Builder(1)
        .boundingBox({d3_to_fd3(begin),
                      d3_to_fd3(end)})
        .material(0, m_material_registry.getMaterialInstance(futils::CString(material_name)))
        .geometry(0, fmt::RenderableManager::PrimitiveType::LINES,
                  vertex_buffer, index_buffer, 0, 2)
        .culling(false)
        .receiveShadows(false)
        .castShadows(false)
        .build(*m_engine, line_renderable);

    m_scene->addEntity(line_renderable);
    
    return g_objm.add_object({line_renderable, this});
}

TDS_API Entity_ID env_add_line(double3 begin, double3 end, const char* material_name)
{
    Environment* env = g_objm.get_active_environment();
    if (!env) return {TDS_INVALID_ID};

    return env->add_line(begin, end, material_name);
}

std::pair<fmt::Camera*, Entity_ID> Environment::create_filament_camera()
{
    Entity_ID camera_entity = g_objm.add_object({m_entity_manager.create(), this});
    fmt::Camera* camera = m_engine->createCamera(g_objm.get_object(camera_entity).entity);
    return {camera, camera_entity};
}

fmt::View* Environment::create_filament_view()
{
    fmt::View* view = m_engine->createView();
    if (!view) return nullptr;
    
    view->setScene(m_scene);
    return view;
}

bool Environment::destroy_view(fmt::View* view) { return m_engine->destroy(view); }

fmt::Renderer* Environment::create_filament_renderer()
{
    fmt::Renderer* renderer = m_engine->createRenderer();
    if (!renderer) return nullptr;
    
    return renderer;
}

bool Environment::destroy_renderer(fmt::Renderer* renderer) { return m_engine->destroy(renderer); }

fmt::SwapChain* Environment::create_swap_chain(uint32_t width, uint32_t height) { return m_engine->createSwapChain(width, height); }
fmt::SwapChain* Environment::create_swap_chain(void* native_window) { return m_engine->createSwapChain(native_window); }
bool Environment::destroy_swap_chain(fmt::SwapChain* swap_chain) { return m_engine->destroy(swap_chain); }

fmt::Fence* Environment::create_fence() { return m_engine->createFence(); }

fmt::TransformManager& Environment::get_transform_manager() { return m_engine->getTransformManager(); };
