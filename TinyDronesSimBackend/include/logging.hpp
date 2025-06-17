#pragma once

#include <cstdlib>

#define TDS_DEBUG_BREAK __builtin_trap()

#define tds_assert(statement)                   \
    do {                                        \
        if (!(statement)) {                     \
            TDS_DEBUG_BREAK;                    \
            std::exit(1);                       \
        }                                       \
    } while (0)

#define tds_unreachable() tds_assert(false)

#define ENV_ERR_MEM_ALLOC "Out of memory. By more RAM lol"

enum class Tds_Log_Type {HARD_ERROR, SOFT_ERROR, WARNING, INFO, TODO};

void tds_log(Tds_Log_Type type, const char* user_msg, ...);

template <typename... ARGS>
void tds_info(const char* info_msg, ARGS... args) { tds_log(Tds_Log_Type::INFO, (info_msg), args...); }

template <typename... ARGS>
void tds_warning(const char* warning_msg, ARGS... args) { tds_log(Tds_Log_Type::WARNING, (warning_msg), args...); }

template <typename... ARGS>
void tds_soft_error(const char* error_msg, ARGS... args) { tds_log(Tds_Log_Type::SOFT_ERROR, (error_msg), args...); }

template <typename... ARGS>
void tds_hard_error(const char* error_msg, ARGS... args)
{
    tds_log(Tds_Log_Type::HARD_ERROR, (error_msg), args...);
    std::exit(1);
}

template <typename... ARGS>
void tds_todo(const char* todo_msg, ARGS... args)
{
    tds_log(Tds_Log_Type::TODO, (todo_msg), args...);
    std::exit(0);
}
