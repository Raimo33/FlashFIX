#ifndef COMMON_H
# define COMMON_H

# include <stdint.h>
# include <stdbool.h>
# include <immintrin.h>

# include "extensions.h"
# include "tags.h"

# define STR_LEN(x) (sizeof(x) - 1)

uint8_t compute_checksum(const char *buffer, const uint16_t len); 

#endif