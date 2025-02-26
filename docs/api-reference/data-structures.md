# Data Structures

## Messages

Included in the `flashfix/structs.h` header file.

```c
typedef struct
{
  uint16_t tag_len;
  uint16_t value_len;
  const char *tag;
  const char *value;
} ff_field_t;

typedef struct
{
  ff_field_t fields[FIX_MAX_FIELDS];
  uint16_t n_fields;
} ff_message_t;
```