/*================================================================================

File: common.h                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 14:56:11                                                 
last edited: 2025-02-12 13:35:28                                                

================================================================================*/

#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <immintrin.h>

# include "extensions.h"
# include "tags.h"

# define STR_LEN(x) (sizeof(x) - 1)

INTERNAL uint8_t compute_checksum(const char *buffer, const uint16_t len); 

#endif