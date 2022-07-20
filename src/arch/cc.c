#include <arch/cc.h>
#include <lwip/sys.h>
#include <time.h>

u32_t sys_now(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return 1000 * tp.tv_sec + tp.tv_nsec / 1000000;
}
