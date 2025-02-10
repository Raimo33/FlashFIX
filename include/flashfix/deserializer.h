/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deserializer.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 18:54:25 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 17:26:26 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>
# include <stdbool.h>

# include "structs.h"
# include "errors.h"

/***********************************************************************************************

description:
  - checks if the buffer contains a full FIX message (aka full checksum is present) 

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

it doesn't check:
  - if the message is correct
  - if there are duplicate tags
  - if the message contains non printable characters
  - if tags are in the correct order
  - if tags are part of the FIX standard

it does check:
  - the presence of the full checksum tag ("10=XXX|")

***********************************************************************************************/
bool ff_is_full(const char *restrict buffer, const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error);

/***********************************************************************************************

description:
  - deserializes a fix message by splitting the fields with '=' and '\x01' delimiters

inputs:
  - the buffer which contains the serialized message (comprehensive of beginstring, bodylength and checksum)
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
  - buffer_size is different from the actual size of the buffer
  - buffer does not contain the full message

it doesn't check:
  - if there are duplicate tags
  - if tags are in the correct order
  - if tags are part of the FIX standard 

it does check:
  - the presence and correctness of the beginstring, bodylength and checksum tags
  - if there are non printable characters
  - if the buffer is too small
  - if there are adiacent separators '|' or '='

***********************************************************************************************/
uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message, ff_error_t *restrict error);

#endif