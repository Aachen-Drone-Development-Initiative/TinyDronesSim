#include <logging.hpp>
#include <cstdarg>
#include <cstdio>


void env_log(Env_Log_Type type, const char* user_msg, ...)
{
    switch (type) {
    case Env_Log_Type::INFO:
        printf("INFO: ");
        break;
    case Env_Log_Type::WARNING:
        printf("WARNING: ");
        break;
    case Env_Log_Type::SOFT_ERROR:
        printf("ERROR: ");
        break;
    case Env_Log_Type::HARD_ERROR:
        printf("FATAL ERROR: ");
        break;
    };
    va_list args;
    va_start(args, user_msg);
    vprintf(user_msg, args);
    printf("\n");
    va_end(args);
}
