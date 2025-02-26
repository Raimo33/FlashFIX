/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-25 14:58:53                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint16_t ff_serialize(char *restrict buffer, const fix_message_t *restrict message);
uint16_t ff_serialize_raw(char *restrict buffer, const fix_message_t *restrict message);

#endif