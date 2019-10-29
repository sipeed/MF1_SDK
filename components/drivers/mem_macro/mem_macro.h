#ifndef __MEM_MACRO_H
#define __MEM_MACRO_H

#include <stdint.h>

/* clang-format off */
///////////////////////////////////////////////////////////////////////////////
#define _IS_IOMEM(x)                            (!((uint64_t)(&(x))&0x80000000))
#define _IS_CACHEMEM(x)                         ((uint64_t)(&(x))&0x80000000)
#define _IS_IOMEMP(x)                           (!((uint64_t)(x)&0x80000000))
#define _IS_CACHEMEMP(x)                        ((uint64_t)(x)&0x80000000)

#define _IOMEM(x, type)                         (*(type *)(((uint64_t)&(x))-0x40000000))
#define _IOMEM_UINT8(x)                         (*(uint8_t *)(((uint64_t)&(x))-0x40000000))
#define _IOMEM_ADDR(x)                          (_IS_IOMEM(x)?(uint64_t)(x):(((uint64_t)&(x))-0x40000000))
#define _IOMEM_PADDR(p)                         (_IS_IOMEMP(p)?(uint64_t)(p):(((uint64_t)(p))-0x40000000))
#define _ADDR(x)	                            ((uint64_t)(x))
#define _IN_BUF(x, buf)	                        (_ADDR(x)>=_ADDR(buf) && _ADDR(x)<_ADDR(buf)+sizeof(buf))
#define CHECK_IOMEM(x)                          configASSERT(_IS_IOMEM(x))
#define CHECK_IOMEMP(x)                         configASSERT(_IS_IOMEMP(x))

#define _MODULE_ADDR(x)                         (x)

#define _CACHE_ADDR(x)                          (_IS_CACHEMEM(x)?(uint64_t)(x):(((uint64_t)&(x))+0x40000000))
#define _CACHE_PADDR(p)                         (_IS_CACHEMEMP(p)?(uint64_t)(p):(((uint64_t)(p))+0x40000000))

///////////////////////////////////////////////////////////////////////////////

#endif /* __MEM_MARCO_H */
