#ifndef __CC_H__
#define __CC_H__

#include <stdint.h>

typedef uint32_t u32_t;
#define LWIP_RAND() ((u32_t)rand())

u32_t sys_now(void);

#endif
