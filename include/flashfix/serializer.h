/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-11 12:37:26                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>
# include <stdbool.h>

# include "structs.h"
# include "errors.h"

/*

description:
  - serializes a fix message by concatenating the fields with '=' and '\x01' delimiters

inputs:
  - the buffer where to store the serialized message
  - the exact size of the buffer
  - the message struct containing the fields to serialize
  - an optional pointer to retrieve error information

outputs:
  - length of the serialized message
  - the serialized message in the buffer
  - the error code if any (unless error is NULL)

undefined behaviour:
  - buffer is NULL
  - message is NULL
  - value_len and tag_len are different from the actual length of the value and tag
  - n_fields is different from the actual number of fields in the message
  - non printable characters
  - buffer size is different from the actual size of the buffer
  - message with NULL fields
  - message with empty {} fields array
  - message with empty "" field strings 
  - message with value_len == 0 or tag_len == 0
  - message with n_fields == 0

it doesn't check:
  - if the message is correct
  - if there are duplicate tags
  - if the output buffer will be null terminated

it does check:
  - if the buffer is big enough

*/
uint16_t ff_serialize(char *restrict buffer, const uint16_t buffer_size, const ff_message_t *restrict message, ff_error_t *restrict error);

/*

description:
  - computes and adds the final beginstring, bodylength and checksum tags to the serialized message in place

inputs:
  - the buffer which contains a serialized message minus beginstring, bodylength and checksum
  - the exact size of the buffer
  - the length of the serialized message (portion of the buffer that is full)
  - an optional pointer to retrieve error information

outputs:
  - the length of the serialized message
  - the finalized message in the buffer
  - the error code if any (unless error is NULL)

undefined behaviour:
  - buffer is NULL
  - buffer_size is different from the actual size of the buffer
  - len > buffer_size

it doesn't check:
  - if beginstring, bodylength and checksum are already present
  - if the message is correct
  - if there are duplicate tags

it does check:
  - if the buffer is big enough

*/
uint16_t ff_finalize(char *restrict buffer, const uint16_t buffer_size, const uint16_t len, ff_error_t *restrict error);

#endif