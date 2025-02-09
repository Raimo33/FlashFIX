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
} fix_field_t;

typedef struct
{
  fix_field_t fields[FIX_MAX_FIELDS];
  uint16_t field_count;
} fix_message_t;
```