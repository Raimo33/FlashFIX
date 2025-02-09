# Serialization

The following function prototypes can be found in the `serialization.h` header file.

```c
#include <flashfix/serialization.h>
```

These functions **don't check the validity of messages**, they assume that the message struct is correctly filled with the right values.

## ff_serialize

```c
uint16_t ff_serialize(char *restrict buffer, const fix_message_t *restrict message);
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
- `field_count` is different from the actual number of fields in the message
- `message` with `NULL` fields
- `message` with empty `{}` fields array
- `message` with empty `""` field strings 
- `message` with `value_len == 0` or `tag_len == 0`
- `message` with `field_count == 0`

## ff_serialize_raw

```c
uint16_t ff_serialize_raw(char *restrict buffer, const fix_message_t *restrict message);
```

### Description
serializes a fix message into a buffer by concatenating the fields with '=' and '\x01' delimiters.

### Parameters

- `buffer` - the buffer where to store the serialized message
- `message` - the message struct containing the fields to serialize.

### Returns

- length of the serialized message in bytes

### Undefined Behavior

- `buffer` is `NULL`
- `message` is `NULL`
- `message` doesn't fit in the buffer
- `message->fields` is `NULL`
- `value_len` or `tag_len` are different from the actual lengths of the strings
- `value` or `tag` is `NULL`
- `field_count` is different from the actual number of fields in the message