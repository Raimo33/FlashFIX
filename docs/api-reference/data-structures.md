# Data Structures

## Messages

Included in the `flashfix/structs.h` header file.

```c
typedef struct
{
  uint16_t tag_len;
  uint16_t value_len;
  char *tag;
  char *value;
} ff_field_t;

typedef struct
{
  ff_field_t fields[FIX_MAX_FIELDS];
  uint16_t n_fields;
} ff_message_t;
```

## Errors

Included in the `flashfix/errors.h` header file.

```c
typedef enum
{
  FF_OK = 0,
  FF_INVALID_MESSAGE,
  FF_BODY_LENGTH_MISMATCH,
  FF_CHECKSUM_MISMATCH,
  FF_TOO_MANY_FIELDS,
  FF_MESSAGE_TOO_BIG
} ff_error_t;
```
