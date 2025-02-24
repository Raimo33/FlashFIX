# Serialization

The following function prototypes can be found in the `serialization.h` header file.

```c
#include <flashfix/serialization.h>
```

These functions **don't check the validity of messages**, they assume that the message struct is correctly filled with the right values.

## ff_serialize

```c
uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message);
```

### Description
serializes a fix message into a buffer by concatenating the fields with '=' and '\x01' delimiters and adding the mandatory beginstring, bodylength, and checksum fields.

### Parameters
  - `buffer` - the buffer where to store the serialized message
  - `message` - the message struct containing the fields to serialize

### Returns
  - length of the serialized message in bytes

### Undefined Behavior
  - `buffer` is `NULL`
  - `message` is `NULL`
  - `message` doesn't fit in the buffer
  - value_len and `tag_len` are different from the actual length of the value and tag
  - `n_fields` is different from the actual number of fields in the message
  - non printable characters
  - `message` with `NULL` fields
  - `message` with empty `{}` fields array
  - `message` with empty `""` field strings 
  - `message` with `value_len == 0` or `tag_len == 0`
  - `message` with `n_fields == 0`

## ff_serialize_write

```c
int32_t ff_serialize_write(const int32_t fd, const ff_message_t *msg, ff_write_state_t *state);
```

### Description
serializes and directly writes a fix message into an fd by concatenating the fields with '=' and '\x01' delimiters and adding the mandatory beginstring, bodylength, and checksum fields.

### Parameters
  - `fd` - the file descriptor where to write the serialized message
  - `msg` - the message struct containing the fields to serialize
  - `state` - the write state struct to keep track of the writing process

### Returns
  - number of bytes written in case of success
  - `-1` in case of error (check the errno for more information)

Note: if the fd is in non-blocking mode, and the write operation could not be completed, the function will return -1. Check the ERRNO for more information and call the function again with the same state to continue the writing process from where it left off.

### Undefined Behavior
  - `fd` is `NULL`
  - `msg` is `NULL`
  - `state` is `NULL`
  - value_len and `tag_len` are different from the actual length of the value and tag
  - `n_fields` is different from the actual number of fields in the message
  - non printable characters
  - `msg` with `NULL` fields
  - `msg` with empty `{}` fields array
  - `msg` with empty `""` field strings 
  - `msg` with `value_len == 0` or `tag_len == 0`
  - `msg` with `n_fields == 0`