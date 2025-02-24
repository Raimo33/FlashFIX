/*================================================================================

File: structs.h                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-13 13:38:07                                                 
last edited: 2025-02-24 19:19:48                                                

================================================================================*/

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>
# include <sys/uio.h>

# ifndef FIX_MAX_FIELDS
#   define FIX_MAX_FIELDS 256
# endif

typedef struct
{
  uint16_t tag_len;
  uint16_t value_len;
  char *tag;
  char *value;
} ff_field_t;

//TODO SOA (Struct of Arrays): https://en.wikipedia.org/wiki/AoS_and_SoA

typedef struct
{
  ff_field_t fields[FIX_MAX_FIELDS];
  uint16_t n_fields;
} ff_message_t;

typedef struct
{
  struct iovec iov[(FIX_MAX_FIELDS + 3) << 2];
  char body_length_str[5];
  uint16_t iovcnt;
  uint16_t bytes_written;
  uint16_t total_bytes;
} ff_write_state_t;

#endif