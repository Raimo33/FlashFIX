# Deserialization

The following function prototypes can be found in the `deserialization.h` header file.

```c
#include <flashfix/deserialization.h>
```

These functions **only verify the structural integrity** of messages in terms of format, checksum, and body length. They do not validate the correctness of the messages (e.g. duplicate tags, invalid values, etc.).

## ff_deserialize

```c
uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);
```

### Description
deserializes a fix message by tokenizing the buffer **in place**: replacing `'='` and `'\x01'` delimiters with `'\0'` and store pointers to the beginning of each field in the message struct

### Parameters
  - `buffer` - the buffer which contains the full serialized message
  - `buffer_size` - the size of the buffer in bytes
  - `message` - the message struct where to store the deserialized fields
  - `error` - an optional pointer to retrieve error information

### Returns
  - length of the deserialized message in bytes
  - `0` in case of error

### Undefined Behavior
  - `buffer` is `NULL`
  - `message` is `NULL`
  - `buffer` does not contain a full message
  - `message->n_fields` is greater than `FIX_MAX_FIELDS`
  - `buffer_size` is different from the actual size of the buffer
  - `buffer` does not contain a full message
  - `buffer` contains non printable characters

### Errors
  - `FF_INVALID_MESSAGE` - the message does not contain mandatory tags
  - `FF_BODY_LENGTH_MISMATCH` - the body length tag does not match the actual body length
  - `FF_CHECKSUM_MISMATCH` - the checksum tag does not match the actual checksum