#include <unistd.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

// Need to use clang and libc++, because filament is build with clang.
#define CXX "clang++"
#define CPPFLAGS "-std=c++17", "-stdlib=libc++", "-Wall", "-Wextra", "-Wno-return-type-c-linkage", "-g"

#define PROJECT_NAME "TinyDronesSim-EnvironmentBackend"

#define BUILD_FOLDER "build/"
#define OBJ_FOLDER "obj/"
#define BIN_FOLDER "bin/"
#define LIB_FOLDER "lib/"
#define SRC_FOLDER "src/"
#define TESTS_FOLDER "tests/"
#define INCLUDE_FOLDER "include/"

#define FILAMENT_INCLUDE_PATH               "./filament/filament/include/"
#define FILAMENT_BACKEND_INCLUDE_PATH       "./filament/filament/backend/include/"
#define FILAMENT_FILAMESH_INCLUDE_PATH      "./filament/libs/filameshio/include/"
#define FILAMENT_FILAMAT_INCLUDE_PATH       "./filament/libs/filamat/include/"
#define FILAMENT_UTILS_INCLUDE_PATH         "./filament/libs/utils/include/"
#define FILAMENT_MATH_INCLUDE_PATH          "./filament/libs/math/include/"
#define FILAMENT_FILABRIDGE_INCLUDE_PATH    "./filament/libs/filabridge/include/"
#define FILAMENT_IBLPREFILTER_INCLUDE_PATH  "./filament/libs/iblprefilter/include/"
#define FILAMENT_GLTFIO_INCLUDE_PATH        "./filament/libs/gltfio/include/"
#define FILAMENT_SDL2_INCLUDE_PATH          "./filament/third_party/libsdl2/include/"
#define FILAMENT_STB_INCLUDE_PATH           "./filament/third_party/stb/"
#define FILAMENT_ROBIN_MAP_INCLUDE_PATH     "./filament/third_party/robin-map/"

#define FILAMENT_LIBS        "./filament/libs/"
#define FILAMENT_THIRD_PARTY "./filament/third_party/"

#define FILAMENT_LIB_PATH         "./filament/filament/"
#define FILAMENT_BACKEND_LIB_PATH "./filament/filament/backend/"
#define FILAMENT_SHADERS_LIB_PATH "./filament/shaders/"

#define FILAMENT_BLUEGL_LIB_PATH        FILAMENT_LIBS "bluegl/"
#define FILAMENT_BLUEVK_LIB_PATH        FILAMENT_LIBS "bluevk/"
#define FILAMENT_UTILS_LIB_PATH         FILAMENT_LIBS "utils/"
#define FILAMENT_FILABRIDGE_LIB_PATH    FILAMENT_LIBS "filabridge/"
#define FILAMENT_FILAFLAT_LIB_PATH      FILAMENT_LIBS "filaflat/"
#define FILAMENT_GEOMETRY_LIB_PATH      FILAMENT_LIBS "geometry/"
#define FILAMENT_GLTFIO_LIB_PATH        FILAMENT_LIBS "gltfio/"
#define FILAMENT_FILAMESHIO_LIB_PATH    FILAMENT_LIBS "filameshio/"
#define FILAMENT_IBL_PATH               FILAMENT_LIBS "ibl/"
#define FILAMENT_IBL_PREFILTER_LIB_PATH FILAMENT_LIBS "iblprefilter/"
#define FILAMENT_FILAMAT_LIB_PATH       FILAMENT_LIBS "filamat/"

#define FILAMENT_SMOLV_LIB_PATH           FILAMENT_THIRD_PARTY "smol-v/tnt/"
#define FILAMENT_GLSLANG_LIB_PATH         FILAMENT_THIRD_PARTY "glslang/tnt/glslang/"
#define FILAMENT_DRACO_DEC_LIB_PATH       FILAMENT_THIRD_PARTY "draco/tnt/"
#define FILAMENT_MIKKTSPACE_LIB_PATH      FILAMENT_THIRD_PARTY "mikktspace/"
#define FILAMENT_SPIRV_LIB_PATH           FILAMENT_THIRD_PARTY "glslang/tnt/SPIRV/"
#define FILAMENT_SPIRV_CROSS_LIB_PATH     FILAMENT_THIRD_PARTY "spirv-cross/tnt/"
#define FILAMENT_SPIRV_TOOLS_LIB_PATH     FILAMENT_THIRD_PARTY "spirv-tools/source/"
#define FILAMENT_SPIRV_TOOLS_OPT_LIB_PATH FILAMENT_THIRD_PARTY "spirv-tools/source/opt/"
#define FILAMENT_MESH_OPTIMIZER_LIB_PATH  FILAMENT_THIRD_PARTY "meshoptimizer/tnt/"
#define FILAMENT_SDL2_LIB_PATH            FILAMENT_THIRD_PARTY "libsdl2/tnt/"

#define ENVLIB_TARGET_NAME "libenvironment.so"

#define FILAMENT_MATC_EXECUTABLE_PATH "./filament/tools/matc/matc"

#define cmd_append_static_array(cmd, array)          \
    do {                                             \
        for (int i = 0; i < ARRAY_LEN(array); ++i) { \
            cmd_append(cmd, array[i]);               \
        }                                            \
    } while (0);

const char *get_file_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

bool has_file_ext(const char *filename, const char *ext)
{
    return !strcmp(get_file_ext(filename), ext);
}

void cmd_append_files_with_ext(Cmd *cmd, File_Paths file_paths, const char *ext)
{
    for (int i = 0; i < file_paths.count; ++i) {
        if (has_file_ext(file_paths.items[i], ext)) {
            cmd_append(cmd, file_paths.items[i]);
        }
    }
}

// folder must end with '/' for example 'build/release/'
void move_local_file_to_folder(const char* file_name, const char* folder)
{
    const char* current_dir = get_current_dir_temp();
    String_Builder sb_from = {0};
    sb_append_cstr(&sb_from, current_dir);
    sb_append_cstr(&sb_from, "/");
    sb_append_cstr(&sb_from, file_name);
    sb_append_null(&sb_from);
    String_Builder sb_to = {0};
    sb_append_cstr(&sb_to, current_dir);
    sb_append_cstr(&sb_to, "/");
    sb_append_cstr(&sb_to, folder);
    sb_append_cstr(&sb_to, file_name);
    sb_append_null(&sb_to);
    rename(sb_from.items, sb_to.items);
    sb_free(sb_from);
    sb_free(sb_to);
}

void move_obj_files_to_bin()
{
    Nob_File_Paths file_paths = {0};
    nob_read_entire_dir(".", &file_paths);
    for (int i = 0; i < file_paths.count; ++i) {
        if (has_file_ext(file_paths.items[i], "o")) {
            move_local_file_to_folder(file_paths.items[i], BUILD_FOLDER OBJ_FOLDER);
        }
    }
}

const char* enter_folder(const char *folder)
{
    const char* current_dir = nob_get_current_dir_temp();
    String_Builder sb = {0};
    sb_append_cstr(&sb, current_dir);
    sb_append_cstr(&sb, "/");
    sb_append_cstr(&sb, folder);
    sb_append_null(&sb);
    set_current_dir(sb.items);
    sb_free(sb);
    return current_dir;
}

void build_success(const char* target_name)
{
    printf("\nSuccessfully built '%s'!\n\n", target_name);
}

bool build_google_filament(Cmd *cmd)
{
    const char* prev_dir = enter_folder("filament");

    bool result = true;

    cmd_append(cmd, "cmake", "-G", "Ninja",
               "-DCMAKE_CXX_COMPILER=clang++", "-DCMAKE_C_COMPILER=clang",
               "-DCMAKE_BUILD_TYPE=Release",
               "-DCMAKE_INSTALL_PREFIX=../release/filament",
               "-DCMAKE_CXX_FLAGS=\"-stdlib=libc++ -fPIC\"",
               "-DCMAKE_C_FLAGS=\"-fPIC\"");
    if (!cmd_run_sync_and_reset(cmd)) {
        result = false;
        goto exit;
    }

    cmd_append(cmd, "ninja");
    if (!cmd_run_sync_and_reset(cmd)) {
        result = false;
        goto exit;
    }
    
exit:
    set_current_dir(prev_dir);
    if (result) build_success("Goolge-Filament");
    return result;
}

bool build_libenvironment_shared(Cmd *cmd)
{
    // First move all the 
    
    // -fPIC - 'position independet code' necessary for shared libraries
    // -fvisibility=hidden - hides all symbols by default, we have the macro ENV_API to exclude only the api functions from this rule
    cmd_append(cmd, CXX, CPPFLAGS, "-fPIC", "-fvisibility=hidden", "-c");
    cmd_append(cmd, "-I", INCLUDE_FOLDER,
               "-I", FILAMENT_INCLUDE_PATH,
               "-I", FILAMENT_BACKEND_INCLUDE_PATH,
               "-I", FILAMENT_UTILS_INCLUDE_PATH,
               "-I", FILAMENT_MATH_INCLUDE_PATH,
               "-I", FILAMENT_FILAMESH_INCLUDE_PATH,
               "-I", FILAMENT_FILAMAT_INCLUDE_PATH,
               "-I", FILAMENT_FILABRIDGE_INCLUDE_PATH,
               "-I", FILAMENT_IBLPREFILTER_INCLUDE_PATH,
               "-I", FILAMENT_GLTFIO_INCLUDE_PATH,
               "-I", FILAMENT_ROBIN_MAP_INCLUDE_PATH,
               "-I", FILAMENT_SDL2_INCLUDE_PATH,
               "-I", FILAMENT_STB_INCLUDE_PATH);

    const char* source_files[] = {
        SRC_FOLDER "camera.cpp",
        SRC_FOLDER "environment.cpp",
        SRC_FOLDER "filament_entity.cpp",
        SRC_FOLDER "filament_object_wrappers.cpp",
        SRC_FOLDER "frame.cpp",
        SRC_FOLDER "logging.cpp",
        SRC_FOLDER "math.cpp",
        SRC_FOLDER "mesh.cpp",
        SRC_FOLDER "object_manager.cpp",
        SRC_FOLDER "stb_image.cpp",
        SRC_FOLDER "window.cpp"
    };

    cmd_append_static_array(cmd, source_files);

    if (!cmd_run_sync_and_reset(cmd)) return false;

    cmd_append(cmd, CXX, "-fPIC", "-fuse-ld=lld", "-shared", "-o", ENVLIB_TARGET_NAME);
    
    File_Paths obj_file_paths = {0};
    nob_read_entire_dir(".", &obj_file_paths);
    cmd_append_files_with_ext(cmd, obj_file_paths, "o");

    cmd_append(cmd, "-L", FILAMENT_LIB_PATH,
               "-L", FILAMENT_BACKEND_LIB_PATH,
               "-L", FILAMENT_SHADERS_LIB_PATH,
               "-L", FILAMENT_BLUEGL_LIB_PATH,
               "-L", FILAMENT_BLUEVK_LIB_PATH,
               "-L", FILAMENT_UTILS_LIB_PATH,
               "-L", FILAMENT_FILABRIDGE_LIB_PATH,
               "-L", FILAMENT_FILAFLAT_LIB_PATH,
               "-L", FILAMENT_GEOMETRY_LIB_PATH,
               "-L", FILAMENT_GLTFIO_LIB_PATH,
               "-L", FILAMENT_FILAMESHIO_LIB_PATH,
               "-L", FILAMENT_IBL_PATH,
               "-L", FILAMENT_IBL_PREFILTER_LIB_PATH,
               "-L", FILAMENT_FILAMAT_LIB_PATH,
               "-L", FILAMENT_SMOLV_LIB_PATH,
               "-L", FILAMENT_GLSLANG_LIB_PATH,
               "-L", FILAMENT_DRACO_DEC_LIB_PATH,
               "-L", FILAMENT_MIKKTSPACE_LIB_PATH,
               "-L", FILAMENT_SPIRV_LIB_PATH,
               "-L", FILAMENT_SPIRV_CROSS_LIB_PATH,
               "-L", FILAMENT_SPIRV_TOOLS_LIB_PATH,
               "-L", FILAMENT_SPIRV_TOOLS_OPT_LIB_PATH,
               "-L", FILAMENT_MESH_OPTIMIZER_LIB_PATH,
               "-L", FILAMENT_SDL2_LIB_PATH);

    const char *filament_libs[] = {
        "-lfilament",
        "-lbackend",
        "-lbluegl",
        "-lbluevk",
        "-lutils",
        "-lfilabridge",
        "-lfilaflat",
        "-lgeometry",
        "-lsmol-v",

        "-lgltfio",
        "-lgltfio_core",
        "-lglslang",
        "-lshaders",
        "-ldracodec",
        "-lmikktspace",
        "-lSPIRV",
        "-lspirv-cross-core",
        "-lspirv-cross-glsl",
        "-lspirv-cross-msl",
        "-lSPIRV-Tools",
        "-lSPIRV-Tools-opt",
    
        "-lfilameshio",
        "-lmeshoptimizer",
        "-lfilament-iblprefilter",
        "-lfilamat",
        "-libl",
        "-lfilament",
        "-lutils",
        "-lsdl2",
    };
    
    cmd_append_static_array(cmd, filament_libs);

    const char* external_libs[] = {
        "-lpthread",
        "-lc++",
        "-ldl"
    };
    cmd_append_static_array(cmd, external_libs);

    if (!cmd_run_sync_and_reset(cmd)) return false;

    move_obj_files_to_bin();
    move_local_file_to_folder(ENVLIB_TARGET_NAME, BUILD_FOLDER LIB_FOLDER);

    build_success(ENVLIB_TARGET_NAME);
    
    return true;
}

bool build_libenvironment_shared_test(Cmd *cmd)
{
    cmd_append(cmd, CXX, CPPFLAGS, "-o", "libenvironment_test");
    cmd_append(cmd, "-I", INCLUDE_FOLDER,
               "-I", FILAMENT_BACKEND_INCLUDE_PATH,
               "-I", FILAMENT_MATH_INCLUDE_PATH,
               "-I", FILAMENT_FILAMESH_INCLUDE_PATH,
               "-I", FILAMENT_UTILS_INCLUDE_PATH,
               "-I", FILAMENT_SDL2_INCLUDE_PATH);
    cmd_append(cmd, TESTS_FOLDER "libenvironment_test.cpp");
    cmd_append(cmd, BUILD_FOLDER LIB_FOLDER ENVLIB_TARGET_NAME);

    if (!cmd_run_sync_and_reset(cmd)) return false;

    move_local_file_to_folder("libenvironment_test", BUILD_FOLDER BIN_FOLDER);

    build_success("libenvironment_test");

    return true;
}

bool compile_filament_materials(Cmd *cmd)
{
    const char* materials[] = {
        "./assets/sandboxLit",
        "./assets/sandboxUnlit"
    };

    for (int i = 0; i < ARRAY_LEN(materials); ++i) {
        String_Builder in = {0};
        sb_append_cstr(&in, materials[i]);
        sb_append_cstr(&in, ".mat");
        sb_append_null(&in);
        String_Builder out = {0};
        sb_append_cstr(&out, materials[i]);
        sb_append_cstr(&out, ".filamat");
        sb_append_null(&out);
        cmd_append(cmd, FILAMENT_MATC_EXECUTABLE_PATH, "-o", out.items, in.items);
        if (!cmd_run_sync_and_reset(cmd)) return false;
        sb_free(in);
        sb_free(out);
    }
    return true;
}

void print_help()
{
    static const char* help_message =
        "--HELP MESSAGE--\n"
        "This is the integrated build-tool for '"PROJECT_NAME"' built with 'nob.h'.\n"
        "Learn more about 'nob.h' here: https://github.com/tsoding/nob.h\n\n"
        "ARGUMENTS:\n\n"
        "  'help'        Print this help message.\n"
        "  'filament'    Build Google-Filament. Make sure to install the dependencies first!!\n"
        "  'libenv'      Build 'libenvironment.so' .\n"
        "  'clean'       Clean the build.\n"
        "  'tests'       Build the tests.\n"
        "  'materials'   Compile the materials (.mat to .filamat).\n";
    
    printf("%s", help_message);
}

bool clean_build(Cmd *cmd)
{
    cmd_append(cmd, "rm", "-r", BUILD_FOLDER);
    return cmd_run_sync_and_reset(cmd);
}

int main(int argc, char **argv)
{
    Cmd cmd = {0};

    // Nob can detect, if the build script nob.c or nob_config.h have been changed and rebuilds nob automatically.
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program_name = shift_args(&argc, &argv);
    
    bool build_libenv = false;
    bool build_tests = false;
    bool compile_materials = false;
    bool build_filament = false;

    // No arguments means, nothing will happen.
    if (argc == 0) {
        print_help();
        return 0;
    }

    while (argc > 0)
    {
        const char *nob_cmd = shift_args(&argc, &argv);

        if (!strcmp(nob_cmd, "help") || !strcmp(nob_cmd, "--help")) { 
            print_help();
            return 0;
        }
        else if (!strcmp(nob_cmd, "clean")) {
            if (!clean_build(&cmd)) return 1;
            return 0; 
        }
        else if (!strcmp(nob_cmd, "libenv")) {
            build_libenv = true; 
        }
        else if (!strcmp(nob_cmd, "tests")) {
            build_tests = true; 
        }
        else if (!strcmp(nob_cmd, "filament")) {
            build_filament = true;
        }
        else if (!strcmp(nob_cmd, "materials")) {
            compile_materials = true; 
        }
        else {
            nob_log(ERROR, "Unrecognized command '%s'\n", nob_cmd);
            print_help();
            return 0;
        }
    }

    if (build_filament) {
        if (!build_google_filament(&cmd)) return 1;
    }

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER OBJ_FOLDER)) return 1; 
    if (!mkdir_if_not_exists(BUILD_FOLDER BIN_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER LIB_FOLDER)) return 1;

    if (build_libenv) {
        if (!build_libenvironment_shared(&cmd)) return 1;
    }

    if (compile_materials) {
        if (!compile_filament_materials(&cmd)) return 1;
    }

    if (build_tests) {
        if (!build_libenvironment_shared_test(&cmd)) return 1;
    }

    return 0;
}
