#include "error_handler.h"

void __attribute__((weak)) ENC28j60_Error_Handler(enum ENC28j60_Error error)
{
    while(1) {}
}