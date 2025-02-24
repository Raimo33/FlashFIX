/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-24 19:19:48                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"
# include "errors.h"

uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message);
int32_t ff_serialize_write(const int32_t fd, const ff_message_t *msg, ff_write_state_t *state);

#endif