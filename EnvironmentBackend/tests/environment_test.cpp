
#include <filament/FilamentAPI.h>
#include <filament/Engine.h>

#include <utils/Path.h>

#include <filameshio/MeshReader.h>
#include <filamat/MaterialBuilder.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>

#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>

#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filament/Camera.h>
#include <utils/EntityManager.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat3.h>
#include <math/norm.h>

#include <filamentapp/IBL.h>
#include <filamentapp/NativeWindowHelper.h>
#include <filament/IndirectLight.h>

#include <sdl2/SDL.h>

#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#define IBL_DIR "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/lightroom_14b"

using namespace filament;

static size_t fileSize(int fd) {
    size_t filesize;
    filesize = (size_t) lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return filesize;
}

Material* load_material_from_file(filament::Engine* engine, const utils::Path& path) {

    Material* material = nullptr;

    int fd = open(path.c_str(), O_RDONLY);

    size_t size = fileSize(fd);
    char* data = (char*) malloc(size);
    read(fd, data, size);

    if (data) {
        material = Material::Builder().package(data, size).build(*engine);
        free(data);
    }
    else {
        std::cout << "Error with allocating data\n";
    }
    close(fd);

    return material;
}

IBL* load_IBL(const utils::Path& iblDirectory, Engine* engine) {
    utils::Path iblPath(iblDirectory);

    if (!iblPath.exists()) {
        std::cerr << "The specified IBL path does not exist: " << iblPath << std::endl;
        return nullptr;
    }

    if (!iblPath.isDirectory()) {
        std::cerr << "The specified IBL path is not a directory: " << iblPath << std::endl;
        return nullptr;
    }

    IBL* ibl= new IBL(*engine);
    if (!ibl->loadFromDirectory(iblPath)) {
        std::cerr << "Could not load the specified IBL: " << iblPath << std::endl;
        delete ibl;
        return nullptr;
    }

    return ibl;
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_EVENTS);
    Engine *engine = Engine::create(Engine::Backend::OPENGL);
    SDL_Window* sdl_window = SDL_CreateWindow("test", 0, 0, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
        | SDL_WINDOW_RESIZABLE);
    
    SwapChain* swapChain = engine->createSwapChain(getNativeWindow(sdl_window));
    Renderer* renderer = engine->createRenderer();

    auto& entity_m = utils::EntityManager::get();
    auto& trans_m = engine->getTransformManager();

    utils::Entity camera_entity = entity_m.create();
    Camera* camera = engine->createCamera(camera_entity);
    View* view = engine->createView();
    view->setCamera(camera);
    Scene* scene = engine->createScene();
    view->setScene(scene);

    auto ibl = load_IBL(IBL_DIR, engine);
    if (ibl) {
        ibl->getIndirectLight()->setIntensity(10000);
        scene->setIndirectLight(ibl->getIndirectLight());
        scene->setSkybox(ibl->getSkybox());
    }
    
    int width, height;
    double fov = 60.0, near_plane = 0.1, far_plane = 50.0;
    SDL_GL_GetDrawableSize(sdl_window, &width, &height);
    view->setViewport({0, 0, uint32_t(width), uint32_t(height)});
    camera->setProjection(fov, double(width) / double(height), near_plane, far_plane);
    math::float3 lookat_center = {0.0f, 0.0f, 0.0f};
    math::float3 camera_up = {0.0f, 1.0f, 0.0f};
    float camera_dist = 4.0f;
    camera->lookAt({0.0f, 0.0f, camera_dist}, lookat_center, camera_up);

    filamesh::MeshReader::MaterialRegistry material_registry;
    
    // filamat::Package pkg = builder.build(engine->getJobSystem());

    filamat::Package pkg(const void* src, size_t size);

    auto material = load_material_from_file(engine, utils::Path("/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/sandboxLit.filamat"));
    
    const utils::CString defaultMaterialName("DefaultMaterial");
    auto mat_i = material->createInstance();
    mat_i->setParameter("baseColor", RgbType::sRGB,
                                     {0.60f, 0.60f, 0.80f});
    mat_i->setParameter("roughness", 0.00f);
    mat_i->setParameter("metallic", 1.00f);
    mat_i->setParameter("reflectance", 1.00f);
    mat_i->setParameter("sheenColor", 0.00f);
    mat_i->setParameter("clearCoat", 0.00f);
    mat_i->setParameter("clearCoatRoughness", 0.00f);
    
    material_registry.registerMaterialInstance(defaultMaterialName, mat_i);

    // read filamesh file from path
    auto mesh = filamesh::MeshReader::loadMeshFromFile(
        engine,
        utils::Path("/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/suzanne.filamesh"),
        material_registry);
    
    scene->addEntity(mesh.renderable);

    // auto trans_i = trans_m.getInstance(mesh.renderable);

    SDL_Event event;
    bool window_should_close = false;
    while (!window_should_close) {
                
        // beginFrame() returns false if we need to skip a frame
        if (renderer->beginFrame(swapChain)) {
            // for each View
            renderer->render(view);
            renderer->endFrame();
        }

        if (!UTILS_HAS_THREADING) {
            engine->execute();
        }
        
        double theta = SDL_GetPerformanceCounter() * 10e-8 * 3.141592653589793 / 180.0;
        math::float3 eye = {camera_dist * std::sin(theta),
                            0.0f,
                            camera_dist * std::cos(theta)};
        camera->lookAt(eye, lookat_center, camera_up);
        
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                window_should_close = true;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
                        SDL_GL_GetDrawableSize(sdl_window, &width, &height);
                        view->setViewport({0, 0, uint32_t(width), uint32_t(height)});
                        camera->setProjection(fov, double(width) / double(height), near_plane, far_plane);
                        break;
                    }
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    window_should_close = true;
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
        
        SDL_Delay(16);
    }
    
    return 0;
}
