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