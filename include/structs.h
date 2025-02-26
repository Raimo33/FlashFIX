/*================================================================================

File: structs.h                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-13 13:38:07                                                 
last edited: 2025-02-25 14:58:53                                                

================================================================================*/

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

# ifndef FIX_MAX_FIELDS
#  define FIX_MAX_FIELDS 256
# endif 

typedef struct
{
  uint16_t tag_len;
  uint16_t value_len;
  const char *tag;
  const char *value;
} fix_field_t;

typedef struct
{
  fix_field_t fields[FIX_MAX_FIELDS] __attribute__((aligned(64)));
  uint16_t n_fields;
} fix_message_t;

#endif