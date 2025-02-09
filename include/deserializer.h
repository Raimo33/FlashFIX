/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-28 19:14:47                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message);
bool ff_is_complete(const char *buffer, const uint16_t len);

#endif