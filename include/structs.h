/*================================================================================

File: structs.h                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-13 13:38:07                                                 
last edited: 2025-03-04 12:18:32                                                

================================================================================*/

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

typedef struct
{
  uint16_t tag_len;
  uint16_t value_len;
  char *tag;
  char *value;
} fix_field_t;

typedef struct
{
  fix_field_t *fields;
  uint16_t field_count;
} fix_message_t;

#endif