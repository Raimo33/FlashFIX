# Examples

## Serialization

```c
#include <flashfix/serialization.h>

#define STR_LEN(s) (sizeof(s) - 1)

/*...*/

fix_field_t data_fields[5] = {
  {.tag = "35",  .value = "A",           .tag_len = 2, .value_len = 1},
  {.tag = "49",  .value = "CLIENT123",   .tag_len = 2, .value_len = 9},
  {.tag = "56",  .value = "SPOT",        .tag_len = 2, .value_len = 4},
  {.tag = "34",  .value = "1",           .tag_len = 2, .value_len = 1},
  {.tag = "52",  .value = timestamp_str, .tag_len = 2, .value_len = 21},
};

char raw_serialized_data[1024];
const uint16_t data_len = ff_serialize_raw(raw_serialized_data, (const fix_message_t *){fields, 5});

fix_field_t logon_fields[6] = {
  {.tag = "35",  .value = "A",           .tag_len = 2, .value_len = 1},
  {.tag = "49",  .value = "CLIENT123",   .tag_len = 2, .value_len = 9},
  {.tag = "56",  .value = "SPOT",        .tag_len = 2, .value_len = 4},
  {.tag = "34",  .value = "1",           .tag_len = 2, .value_len = 1},
  {.tag = "52",  .value = timestamp_str, .tag_len = 2, .value_len = 21},
  {.tag = "98",  .value = "0",           .tag_len = 2, .value_len = 1},
};

char serialized_logon[1024];
const uint16_t logon_len = ff_serialize(serialized_logon, (const fix_message_t *){logon_fields, 6});

write(sockfd, raw_serialized_data, data_len);

/*...*/
```

## Deserialization

```c
#include <flashfix/deserialization.h>

/*...*/

char read_buffer[4096];
uint16_t bytes_read = 0;

do {
  bytes_read += read(sockfd, read_buffer, 1024);
} while (ff_is_complete(read_buffer, bytes_read) == false);

fix_field_t fields[16];
fix_message_t message = {fields, 16};
ff_deserialize(&message, read_buffer, bytes_read);

/*...*/
```