# Serialization

The following function prototypes can be found in the `serialization.h` header file.

```c
#include <flashfix/serialization.h>
```

These functions **don't check the validity of messages**, they assume that the message struct is correctly filled with the right values.

## ff_message_fits_in_buffer

```c
bool ff_message_fits_in_buffer(const ff_message_t *restrict message, const uint16_t buffer_size, ff_error_t *restrict error);
```

### Description
checks if the finalized message derived from the message struct fits in the provided buffer size.

### Parameters
  - `message` - the message struct to be serialized
  - `buffer_size` - the size of the buffer in bytes
  - `error` - an optional pointer to retrieve error information

### Returns
  - `true` if the message fits in the buffer
  - `false` if the message does not fit in the buffer
  - `false` in case of error

### Undefined Behavior
  - `message` is NULL
  - message field lengths add up to more than `UINT16_MAX`

### Errors
  __this function does not set any errors__

## ff_serialize

```c
uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message, ff_error_t *restrict error);
```

### Description
serializes a fix message into a buffer by concatenating the fields with '=' and '\x01' delimiters

### Parameters
  - `buffer` - the buffer where to store the serialized message
  - `message` - the message struct containing the fields to serialize
  - `error` - an optional pointer to retrieve error information

### Returns
  - length of the serialized message in bytes
  - `0` in case of error

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

### Errors
  __this function does not set any errors__

## ff_finalize

```c
uint16_t ff_finalize(char *restrict buffer, const uint16_t len, ff_error_t *restrict error);
```

### Description
computes and adds the final beginstring, bodylength and checksum tags to the serialized message in place

### Parameters
  - `buffer` - the buffer which contains a serialized message minus beginstring, bodylength and checksum
  - `len` - the length of the serialized message (portion of the buffer that is full) in bytes
  - `error` - an optional pointer to retrieve error information

### Returns
  - length of the finalized message in bytes
  - `0` in case of error

### Undefined Behavior
  - `buffer` is `NULL`
  - `buffer` is nut big enough to contain the final message