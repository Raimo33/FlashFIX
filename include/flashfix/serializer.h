/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-16 22:54:38                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"
# include "errors.h"

/*

description:
  - checks if the finalized message derived from the message struct fits in the buffer

inputs:
  - the message struct to check
  - the size of the buffer

outputs:
  - true if the message fits in the buffer, false otherwise

undefined behaviour:
  - message is NULL
  - message n_fields is different from the actual number of fields in the message

it doesn't check:
  - if the message already contains beginstring, bodylength and checksum

*/

bool ff_message_fits_in_buffer(const ff_message_t *restrict message, const uint16_t buffer_size, ff_error_t *restrict error);

/*

description:
  - serializes a fix message by concatenating the fields with '=' and '\x01' delimiters

inputs:
  - the buffer where to store the serialized message
  - the message struct containing the fields to serialize
  - an optional pointer to retrieve error information

outputs:
  - length of the serialized message
  - the serialized message in the buffer
  - the error code if any (unless error is NULL)

undefined behaviour:
  - buffer is NULL
  - message is NULL
  - message doesn't fit in the buffer
  - value_len and tag_len are different from the actual length of the value and tag
  - n_fields is different from the actual number of fields in the message
  - the same message pointer is used with different data
  - non printable characters
  - message with NULL fields
  - message with empty {} fields array
  - message with empty "" field strings 
  - message with value_len == 0 or tag_len == 0
  - message with n_fields == 0

it doesn't check:
  - if the buffer is empty
  - if the buffer is big enough
  - if the message is correct
  - if there are duplicate tags
  - if the output buffer will be null terminated

*/
uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message, ff_error_t *restrict error);

/*

description:
  - computes and adds the final beginstring, bodylength and checksum tags to the serialized message in place

inputs:
  - the buffer which contains a serialized message minus beginstring, bodylength and checksum
  - the length of the serialized message (portion of the buffer that is full)
  - an optional pointer to retrieve error information

outputs:
  - the length of the serialized message
  - the finalized message in the buffer
  - the error code if any (unless error is NULL)

undefined behaviour:
  - buffer is NULL
  - buffer is nut big enough to contain the final message

it doesn't check:
  - if the buffer is empty
  - if the buffer is big enough
  - if beginstring, bodylength and checksum are already present
  - if the message is correct
  - if there are duplicate tags

*/
uint16_t ff_finalize(char *restrict buffer, const uint16_t len, ff_error_t *restrict error);

#endif