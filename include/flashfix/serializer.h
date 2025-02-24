/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-24 16:35:15                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"
# include "errors.h"

bool ff_message_fits_in_buffer(const ff_message_t *restrict message, const uint16_t buffer_size, ff_error_t *restrict error);
uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message, ff_error_t *restrict error);
int32_t ff_serialize_and_write(const int32_t fd, const ff_message_t *msg, ff_write_state_t *state, ff_error_t *error);

#endif