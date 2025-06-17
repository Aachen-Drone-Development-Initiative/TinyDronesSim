#include <logging.hpp>
#include <cstdarg>
#include <cstdio>


void tds_log(Tds_Log_Type type, const char* user_msg, ...)
{
    switch (type) {
    case Tds_Log_Type::INFO:
        printf("INFO: ");
        break;
    case Tds_Log_Type::WARNING:
        printf("WARNING: ");
        break;
    case Tds_Log_Type::SOFT_ERROR:
        printf("ERROR: ");
        break;
    case Tds_Log_Type::HARD_ERROR:
        printf("FATAL ERROR: ");
        break;
    case Tds_Log_Type::TODO:
        printf("HIT A TODO: ");
        break;
    };
    va_list args;
    va_start(args, user_msg);
    vprintf(user_msg, args);
    printf("\n");
    va_end(args);
}
