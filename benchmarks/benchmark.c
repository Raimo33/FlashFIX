/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-03-05 14:58:30                                                

================================================================================*/

#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <immintrin.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FIELDS 64
#define N_ITERATIONS 1'000'000
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define ALIGNMENT 64
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (MAX_FIELDS + 3)
#define static_assert _Static_assert
#define STR_LEN(str) sizeof(str) - 1
#define ALIGNED(n) __attribute__((aligned(n)))

static void init_random_tags(char **tags);
static void init_random_values(char **values);
static void fill_message_structs(fix_message_t *messages, char **tags, char **values);
static void fill_message_buffers(char **buffers, char **tags, char **values);
static void fill_message_lengths(uint16_t *message_lengths, char **message_buffers);
static void serialize(fix_message_t *messages);
static void serialize_raw(fix_message_t *messages);
static void deserialize(char **buffers);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static uint8_t compute_checksum(const char *buffer, const uint16_t len);
static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode);
static void *calloc_p(const size_t n, const size_t size);
static void free_strings(char **strings, const uint16_t n);
static void free_message_structs(fix_message_t *messages);

int32_t main(void)
{
  static_assert(BUFFER_SIZE < UINT16_MAX, "BUFFER_SIZE must be less than UINT16_MAX");

  char **tags = calloc_p(MAX_FIELDS, sizeof(char *));
  char **values = calloc_p(MAX_FIELDS, sizeof(char *));

  init_random_tags(tags);
  init_random_values(values);
  
  {
    fix_message_t *message_structs = calloc_p(MAX_FIELDS, sizeof(fix_message_t));
    
    fill_message_structs(message_structs, tags, values);
    
    serialize(message_structs);
    serialize_raw(message_structs);
    
    free_message_structs(message_structs);
    free(message_structs);
  }
  
  {
    char **message_buffers = calloc_p(MAX_FIELDS, sizeof(char *));
    uint16_t *message_lengths = calloc_p(MAX_FIELDS, sizeof(uint16_t));
    
    fill_message_buffers(message_buffers, tags, values);
    fill_message_lengths(message_lengths, message_buffers);
    
    deserialize(message_buffers);
    free_strings(message_buffers, MAX_FIELDS);
    free(message_buffers);
    free(message_lengths);
  }
  
  free_strings(tags, MAX_FIELDS);
  free_strings(values, MAX_FIELDS);
  free(tags);
  free(values);
}


static void init_random_tags(char **tags)
{
  constexpr char charset[] = "0123456789";
  constexpr uint8_t charset_len = sizeof(charset) - 1;

  for (uint32_t i = 0; i < MAX_FIELDS; i++)
    tags[i] = generate_random_string(charset, charset_len, MEAN_TAG_LEN, MAX_TAG_LEN);
}

static void init_random_values(char **values)
{
  constexpr char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  constexpr uint8_t charset_len = sizeof(charset) - 1;
  
  for (uint32_t i = 0; i < MAX_FIELDS; i++)
    values[i] = generate_random_string(charset, charset_len, MEAN_VALUE_LEN, MAX_VALUE_LEN);
}

static void fill_message_structs(fix_message_t *messages, char **tags, char **values)
{
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
  {
    messages[i].field_count = i + 1;
    messages[i].fields = calloc_p(MAX_FIELDS, sizeof(fix_field_t));

    for (uint16_t j = 0; j <= i; j++)
    {
      messages[i].fields[j] = (fix_field_t){
        .tag_len = strlen(tags[j]),
        .value_len = strlen(values[j]),
        .tag = tags[j],
        .value = values[j]
      };
    }
  }
}

static void fill_message_buffers(char **buffers, char **tags, char **values)
{
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
  {
    char temp_body[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};
    char *buffer = buffers[i] = calloc_p(BUFFER_SIZE, sizeof(char));
    int total = 0;

    total += sprintf(buffer + total, "8=FIX.4.4\x01");

    int body_len = 0;
    for (uint16_t j = 0; j <= i; j++)
      body_len += sprintf(temp_body + body_len, "%s=%s\x01", tags[j], values[j]);

    total += sprintf(buffer + total, "9=%d\x01", body_len);

    memcpy(buffer + total, temp_body, body_len);
    total += body_len;

    uint8_t checksum = compute_checksum(buffer, total);

    total += sprintf(buffer + total, "10=%03d\x01", checksum);
  }
}

static void fill_message_lengths(uint16_t *message_lengths, char **message_buffers)
{
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
    message_lengths[i] = strlen(message_buffers[i]);
}

static void serialize(fix_message_t *messages)
{
  const int32_t fd = open_p("benchmark_serialize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
  {
    fix_message_t message = messages[i];
    uint64_t total_cycles = 0;

    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      start = __rdtscp(&aux);
      ff_serialize(buffer, &message);
      end = __rdtscp(&aux);

      total_cycles += (end - start);
    }
  
    const uint64_t avg_cpu_cycles = total_cycles / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void serialize_raw(fix_message_t *messages)
{
  const int32_t fd = open_p("benchmark_serialize_raw.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
  {
    fix_message_t message = messages[i];
    uint64_t total_cycles = 0;
    
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {      
      start = __rdtscp(&aux);
      ff_serialize_raw(buffer, &message);
      end = __rdtscp(&aux);
    
      total_cycles += (end - start);
    }
  
    const uint64_t avg_cpu_cycles = total_cycles / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void deserialize(char **buffers)
{
  const int32_t fd = open_p("benchmark_deserialize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;
  
  fix_message_t message ALIGNED(ALIGNMENT) = {0};
  message.fields = calloc_p(MAX_FIELDS, sizeof(fix_field_t));
  message.field_count = MAX_FIELDS;
  
  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
  {
    char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT);
    uint64_t total_cycles = 0;
    
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      memcpy(buffer, buffers[i], BUFFER_SIZE);

      start = __rdtscp(&aux);
      ff_deserialize(buffer, BUFFER_SIZE, &message);
      end = __rdtscp(&aux);

      total_cycles += (end - start);
    }
  
    const uint64_t avg_cpu_cycles = total_cycles / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  free(message.fields);
  close(fd);
}

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len)
{
  uint16_t len = gaussian_rand(median_len, 1);
  len = clamp(len, 1, max_len);
  char *str = calloc_p(len + 1, sizeof(char));

  for (uint16_t i = 0; i < len; i++)
    str[i] = charset[rand() % charset_len];
  str[len] = '\0';

  return str;
}

static double gaussian_rand(const double mean, const double stddev)
{
  const double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
  return z0 * stddev + mean;
}

static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max)
{
  return n < min ? min : n > max ? max : n;
}

static uint8_t compute_checksum(const char *buffer, const uint16_t len)
{
  uint8_t checksum = 0;
  for (uint16_t i = 0; i < len; i++)
    checksum += buffer[i];
  return checksum;
}

static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode)
{
  const int32_t fd = open(pathname, flags, mode);
  if (fd == -1)
  {
    perror("open");
    exit(EXIT_FAILURE);
  }
  return fd;
}

static void *calloc_p(const size_t n, const size_t size)
{
  void *ptr = calloc(n, size);
  if (!ptr)
  {
    perror(strerror(errno));
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static void free_strings(char **strings, const uint16_t n)
{
  for (uint16_t i = 0; i < n; i++)
    free(strings[i]);
}

static void free_message_structs(fix_message_t *messages)
{
  for (uint16_t i = 0; i < MAX_FIELDS; i++)
    free(messages[i].fields);
}