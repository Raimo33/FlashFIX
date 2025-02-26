# Data Structures

## Messages

Included in the `flashfix/structs.h` header file.

```c
typedef struct
{
  char *tags[FIX_MAX_FIELDS];
  char *values[FIX_MAX_FIELDS];
  uint16_t tag_lens[FIX_MAX_FIELDS];
  uint16_t value_lens[FIX_MAX_FIELDS];
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
} ff_error_t;
```