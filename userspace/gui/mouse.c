#include "mouse.h"

int register_mouse_event_handler(ps2_mouse_event_handler_t handler)
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x02), "D"(handler) : "memory");
    return result;
}
