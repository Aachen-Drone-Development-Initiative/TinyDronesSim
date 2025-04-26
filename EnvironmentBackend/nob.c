#include "nob_config.h"

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

// Need to use clang, because filament is build with clang so the libraries expect libc++
#define CXX "clang++"
#define CPPFLAGS "-std=c++17", "-stdlib=libc++", "-Wall", "-Wextra", "-g"

#define PROJECT_NAME "TinyDronesSim-EnvironmentBackend"

#define BUILD_FOLDER "build/"
#define OBJ_FOLDER "obj/"
#define BIN_FOLDER "bin/"
#define LIB_FOLDER "lib/"
#define SRC_FOLDER "src/"
#define TESTS_FOLDER "tests/"
#define INCLUDE_FOLDER "include/"

#define FILAMENT_LIB_PATH "filament/lib/"

#define FILAMENT_INCLUDE_PATH              FILAMENT_PATH "filament/include/"
#define FILAMENT_BACKEND_INCLUDE_PATH      FILAMENT_PATH "filament/backend/include/"
#define FILAMENT_FILAMESH_INCLUDE_PATH     FILAMENT_PATH "libs/filameshio/include/"
#define FILAMENT_FILAMAT_INCLUDE_PATH      FILAMENT_PATH "libs/filamat/include/"
#define FILAMENT_UTILS_INCLUDE_PATH        FILAMENT_PATH "libs/utils/include/"
#define FILAMENT_MATH_INCLUDE_PATH         FILAMENT_PATH "libs/math/include/"
#define FILAMENT_FILABRIDGE_INCLUDE_PATH   FILAMENT_PATH "libs/filabridge/include/"
#define FILAMENT_IBLPREFILTER_INCLUDE_PATH FILAMENT_PATH "libs/iblprefilter/include/"
#define FILAMENT_GLTFIO_INCLUDE_PATH       FILAMENT_PATH "libs/gltfio/include/"
#define FILAMENT_SDL2_INCLUDE_PATH         FILAMENT_PATH "third_party/libsdl2/include/"
#define FILAMENT_STB_INCLUDE_PATH          FILAMENT_PATH "third_party/stb/"
#define FILAMENT_ROBIN_MAP_INCLUDE_PATH    FILAMENT_PATH "third_party/robin-map/"

#define ENVLIB_TARGET_NAME "libenvironment.so"

#define FILAMENT_MATC_EXECUTABLE_PATH FILAMENT_PATH "tools/matc/matc"

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

bool build_libenvironment_shared(Cmd *cmd)
{
    // -fPIC - 'position independet code' necessary for shared libraries
    // -fvisibility=hidden - hides all symbols by default, we have the macro ENV_API to exclude the api functions from this rule
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
        SRC_FOLDER "frame.cpp",
        SRC_FOLDER "logging.cpp",
        SRC_FOLDER "math.cpp",
        SRC_FOLDER "mesh.cpp",
        SRC_FOLDER "object_manager.cpp",
        SRC_FOLDER "stb_image.cpp",
        SRC_FOLDER "window.cpp",
    };

    cmd_append_static_array(cmd, source_files);

    if (!cmd_run_sync_and_reset(cmd)) return false;

    cmd_append(cmd, CXX, "-fPIC", "-fuse-ld=lld", "-shared", "-o", ENVLIB_TARGET_NAME);
    
    File_Paths obj_file_paths = {0};
    nob_read_entire_dir(".", &obj_file_paths);
    cmd_append_files_with_ext(cmd, obj_file_paths, "o");

    cmd_append(cmd, "-L", FILAMENT_LIB_PATH);

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
        "-lvkshaders",

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
        "-lstb",
        "-limage",
        "-limageio",
        "-lktxreader",
        "-lfilamat",
        "-libl",
        "-lfilament",
        "-lutils",
        "-lpng",
        "-ltinyexr",
        "-lz",
        "-limage",
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
        "GET THE DEPENDENCIES:\n"
        "  - ..\n"
        "BUILD THIS PROJECT:\n"
        "  Run without arguments to build " PROJECT_NAME ".\n"
        "  You can call 'nob' from any directory.\n"
        "  ARGUMENTS:\n"
        "    'help'/'--help' Print this help message.\n"
        "    'clean'         Clean the build.\n"
        "    'tests'         Also build the tests.\n"
        "    'materials'     Also compile the materials (.mat to .filamat).\n";
    
    printf("%s", help_message);
}

bool clean_build(Cmd *cmd)
{
    cmd_append(cmd, "rm", "-r", BUILD_FOLDER);
    return cmd_run_sync_and_reset(cmd);
}

void handle_execution_from_different_directory(int argc, char** argv, Cmd* cmd)
{
    // If we are called from another directory, the first argument 'argv[0]' would be for example
    // something like '../../nob'. In that case we enter that directory '../..' and change 'argv[0]' to "./nob"
    int i = strlen(argv[0]) - 1;
    while (*(argv[0] + i) != '/' && i >= 0) --i;
    if (i != 0) {
        char* relative_path = malloc(sizeof(char) * (i + 1)); // leaked
        strncpy(relative_path, argv[0], i + 1);
        enter_folder(relative_path);
    }
    String_Builder new_argv0 = {0}; // leaked
    sb_append_cstr(&new_argv0, "./");
    sb_append_cstr(&new_argv0, argv[0] + i + 1);
    argv[0] = new_argv0.items;
}

int main(int argc, char **argv)
{
    Cmd cmd = {0};

    handle_execution_from_different_directory(argc, argv, &cmd);

    // Nob can detect, if the build script nob.c or nob_config.h have been changed and rebuilds nob automatically.
    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "nob_config.h");

    const char *program_name = shift_args(&argc, &argv);
    bool build_tests = false;
    bool compile_materials = false;

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
        else if (!strcmp(nob_cmd, "tests")) {
            build_tests = true; 
        }
        else if (!strcmp(nob_cmd, "materials")) {
            compile_materials = true; 
        }
    }

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER OBJ_FOLDER)) return 1; 
    if (!mkdir_if_not_exists(BUILD_FOLDER BIN_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER LIB_FOLDER)) return 1;

    if (!build_libenvironment_shared(&cmd)) return 1;

    if (compile_materials) {
        if (!compile_filament_materials(&cmd)) return 1;
    }

    if (build_tests) {
        if (!build_libenvironment_shared_test(&cmd)) return 1;
    }

    return 0;
}
