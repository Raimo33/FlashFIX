#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

# ifndef FIX_MAX_FIELDS
#   define FIX_MAX_FIELDS 64
# endif

typedef struct __attribute__((aligned(64)))
{
  const char *tag;
  const char *value;
  uint16_t tag_len;
  uint16_t value_len;
} ff_field_t;

typedef struct __attribute__((aligned(64)))
{
  ff_field_t fields[FIX_MAX_FIELDS];
  uint8_t n_fields;
} ff_message_t;

#endif