#pragma once

#include <cstdlib>

#define ENV_ERR_MEM_ALLOC "Out of memory. By more RAM lol"

enum class Env_Log_Type {HARD_ERROR, SOFT_ERROR, WARNING, INFO};

void env_log(Env_Log_Type type, const char* user_msg, ...);

template <typename... ARGS>
void env_info(const char* info_msg, ARGS... args) { env_log(Env_Log_Type::INFO, (info_msg), args...); }

template <typename... ARGS>
void env_warning(const char* warning_msg, ARGS... args) { env_log(Env_Log_Type::WARNING, (warning_msg), args...); }

template <typename... ARGS>
void env_soft_error(const char* error_msg, ARGS... args) { env_log(Env_Log_Type::SOFT_ERROR, (error_msg), args...); }

template <typename... ARGS>
void env_hard_error(const char* error_msg, ARGS... args) { env_log(Env_Log_Type::HARD_ERROR, (error_msg), args...); std::exit(1); }

#define ENV_OBJ_TYPE_ARGUMENT_ERROR(got_type, expected_type)            \
    do {                                                                \
        env_soft_error("The function %s expected object of type '%s' but got '%s'", \
                       __FUNCTION__, env_object_type_name[(expected_type)], \
                       env_object_type_name[(got_type)]);               \
    } while (false);
