/*================================================================================

File: common.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 14:56:11                                                 
last edited: 2025-02-16 22:54:38                                                

================================================================================*/

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <immintrin.h>
//#TODO #include <stdbit.h>

# include "extensions.h"

# define STR_LEN(x) (sizeof(x) - 1)

INTERNAL uint8_t compute_checksum(const char *buffer, const uint16_t len);
INTERNAL ALWAYS_INLINE inline void *align_forward(const void *ptr, const uint8_t alignment)
{
  uintptr_t addr = (uintptr_t)ptr;
  return (void *)((addr + (alignment - 1)) & ~(uintptr_t)(alignment - 1));
}

#endif