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
# include <sys/uio.h>

# ifndef FIX_MAX_FIELDS
#  define FIX_MAX_FIELDS 256
# endif 

typedef struct
{
  const char *tags[FIX_MAX_FIELDS] __attribute__((aligned(64)));
  const char *values[FIX_MAX_FIELDS] __attribute__((aligned(64)));
  uint16_t tag_lens[FIX_MAX_FIELDS] __attribute__((aligned(64)));
  uint16_t value_lens[FIX_MAX_FIELDS] __attribute__((aligned(64)));
  uint16_t n_fields;
} ff_message_t;

#endif