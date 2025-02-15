/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-15 17:33:44                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"
# include "errors.h"

/*

description:
  - checks if the buffer contains a full FIX message (aka full checksum is present) 
  very important: if
  the buffer is too small (aka the partial message that arrived has filled the whole buffer)
  there is potential for an infinite loop if you keep repeating the call to ff_is_full
  with the same buffer and buffer_size, the function will yield a FF_BUFFER_TOO_SMALL error
  in that case, you should only avoid ff_is_full with a NULL error pointer if you have
  other ways to detect that the buffer is full

inputs:
  - the buffer which contains a full or partial serialized message
  - the exact size of the buffer
  - the length of the serialized message (portion of the buffer that is full)
  - an optional pointer to retrieve error information

outputs:
  - true if the buffer contains a full FIX message, false otherwise
  - the error code if any (unless error is NULL)

undefined behaviour:
  - buffer is NULL
  - buffer_size is different from the actual size of the buffer
  - message_len is different from the actual length of the message
  - message_len is bigger than buffer_size

it doesn't check:
  - if the buffer is empty
  - if the message is correct
  - if there are duplicate tags
  - if the message contains non printable characters
  - if tags are in the correct order
  - if tags are part of the FIX standard

it does check:
  - the presence of the full checksum tag ("10=XXX|")

*/
bool ff_is_full(const char *restrict buffer, const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error);

/*

description:
  - deserializes a fix message by tokenizing the buffer in place:
    replacing '=' and '\x01' delimiters with '\0' and store pointers
    to the beginning of each field in the message struct

inputs:
  - the buffer which contains the full serialized message
  - the exact size of the buffer
  - the message struct where to store the deserialized fields
  - an optional pointer to retrieve error information

outputs:
  - the number of bytes read from the buffer
  - the error code if any (unless error is NULL)
  - the message struct filled with the fields (apart from beginstring, bodylength and checksum)

undefined behaviour:
  - buffer is NULL
  - message is NULL
  - message->n_fields > FIX_MAX_FIELDS
  - buffer_size is different from the actual size of the buffer
  - buffer does not contain the full message
  - message is not full (missing checksum)

it doesn't check:
  - if the buffer is empty
  - if message->n_fields is different from the actual number of fields
  - if there are duplicate tags
  - if tags are in the correct order
  - if tags are part of the FIX standard 
  - if there are non printable characters

it does check:
  - the presence and correctness of the beginstring, bodylength tags
  - if the buffer is too small
  - if there are adjacent separators '|' or '='

*/
uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);

#endif