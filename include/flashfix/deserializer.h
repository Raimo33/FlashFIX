/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-24 17:33:11                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"
# include "errors.h"

uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);
//TODO ff_deserialize_read, which reads with readv directly from the buffer and zerocopies the message into the struct

#endif