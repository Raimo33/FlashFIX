/*================================================================================

File: common.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 14:56:11                                                 
last edited: 2025-02-15 00:17:29                                                

================================================================================*/

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <immintrin.h>
//#TODO #include <stdbit.h>

# include "extensions.h"
# include "tags.h"

# define STR_LEN(x) (sizeof(x) - 1)

INTERNAL uint8_t compute_checksum(const char *buffer, const uint16_t len); 

#endif