/*================================================================================

File: common.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 14:56:11                                                 
last edited: 2025-03-01 10:59:00                                                

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

INTERNAL uint8_t compute_checksum(const char *buffer, const char *end);
INTERNAL ALWAYS_INLINE inline void *align_forward(const void *ptr)
{
  return (void *)(((uintptr_t)ptr + ALIGNMENT - 1) & -ALIGNMENT);
}

#endif