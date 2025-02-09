/*================================================================================

File: common.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 14:56:11                                                 
last edited: 2025-03-05 20:59:03                                                

================================================================================*/

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <immintrin.h>
//#TODO #include <stdbit.h>

# include "extensions.h"

# define STR_LEN(x) (sizeof(x) - 1)

# if defined(__AVX512F__)
  # define ALIGNMENT 64
# elif defined(__AVX2__)
  # define ALIGNMENT 32
# elif defined(__SSE2__)
  # define ALIGNMENT 16
# else
  # define ALIGNMENT sizeof(void *)
# endif

INTERNAL uint8_t compute_checksum(const char *buffer, const char *const end);
INTERNAL ALWAYS_INLINE inline uint8_t align_forward(const void *const ptr) { return -(uintptr_t)ptr & (ALIGNMENT - 1);}
INTERNAL ALWAYS_INLINE inline uint8_t memcmp8(const void *const ptr1, const void *const ptr2) { return *(uint64_t *)ptr1 == *(uint64_t *)ptr2; }
INTERNAL ALWAYS_INLINE inline uint8_t memcmp4(const void *const ptr1, const void *const ptr2) { return *(uint32_t *)ptr1 == *(uint32_t *)ptr2; }
INTERNAL ALWAYS_INLINE inline uint8_t memcmp2(const void *const ptr1, const void *const ptr2) { return *(uint16_t *)ptr1 == *(uint16_t *)ptr2; }
INTERNAL ALWAYS_INLINE inline void memcpy8(void *const dest, const void *const src) { *(uint64_t *)dest = *(uint64_t *)src; }
INTERNAL ALWAYS_INLINE inline void memcpy4(void *const dest, const void *const src) { *(uint32_t *)dest = *(uint32_t *)src; }
INTERNAL ALWAYS_INLINE inline void memcpy2(void *const dest, const void *const src) { *(uint16_t *)dest = *(uint16_t *)src; }

#endif