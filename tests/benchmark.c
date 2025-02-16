/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-16 22:54:38                                                

================================================================================*/

//TODO FLAMEGRAPH
#define _GNU_SOURCE
#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <immintrin.h>

#define N_ITERATIONS 100'000
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (FIX_MAX_FIELDS + 3)
#define ALIGNMENT 64
#define static_assert _Static_assert

static void init_random_tags(char **tags, uint16_t *tag_lens);
static void init_random_values(char **values, uint16_t *value_lens);
static bool fits_buffer(uint16_t *tag_lens, uint16_t *value_lens);
static void init_message_buffers(char **buffers);
static void fill_message_structs(ff_message_t *messages, char **tags, char **values, uint16_t *tag_lens, uint16_t *value_lens);
static void serialize(char **buffers, ff_message_t *messages);
static void serialize_and_finalize(char **buffers, ff_message_t *messages, uint16_t *message_lens);
//TODO static void fits_in_buffer_and_serialize_and_finalize(char **buffers, ff_message_t *messages, uint16_t *message_lens);
static void is_full_message_and_deserialize(char **buffers, ff_message_t *messages, uint16_t *message_lenghts);
static void deserialize(char **buffers, ff_message_t *messages);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static void *aligned_calloc_p(const size_t nmemb, const size_t size);
static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode);
static void free_strings(char **strings, const uint16_t n_fields);

int32_t main(void)
{
  //TODO detect and print cpu info (architecture, clock_speed, and available avx intructions)

  static_assert(BUFFER_SIZE < UINT16_MAX, "MAX_BUFFER_SIZE must be less than UINT16_MAX");

  char **tags = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(char *));
  char **values = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(char *));
  uint16_t *tag_lens = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(uint16_t));
  uint16_t *value_lens = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(uint16_t));
  ff_message_t *message_structs = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(ff_message_t));
  char **message_buffers = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(char *));
  uint16_t *message_lens = aligned_calloc_p(FIX_MAX_FIELDS, sizeof(uint16_t));

  do {
    bzero(tags, FIX_MAX_FIELDS * sizeof(char *));
    bzero(tag_lens, FIX_MAX_FIELDS * sizeof(uint16_t));
    bzero(values, FIX_MAX_FIELDS * sizeof(char *));
    bzero(value_lens, FIX_MAX_FIELDS * sizeof(uint16_t));

    init_random_tags(tags, tag_lens);
    init_random_values(values, value_lens);
  } while (!fits_buffer(tag_lens, value_lens));
  
  init_message_buffers(message_buffers);

  fill_message_structs(message_structs, tags, values, tag_lens, value_lens);

  free(tag_lens);
  free(value_lens);

  printf("Benchmarking FlashFIX with %d iterations per test...\n", N_ITERATIONS);

  serialize(message_buffers, message_structs);
  serialize_and_finalize(message_buffers, message_structs, message_lens);
  //TODO fits_in_buffer_and_serialize_and_finalize(message_buffers, message_structs, message_lens);
  is_full_message_and_deserialize(message_buffers, message_structs, message_lens);
  deserialize(message_buffers, message_structs);

  free_strings(tags, FIX_MAX_FIELDS);
  free_strings(values, FIX_MAX_FIELDS);
  free_strings(message_buffers, FIX_MAX_FIELDS);
  free(tags);
  free(values);
  free(message_structs);
  free(message_buffers);
  free(message_lens);
}

static void init_random_tags(char **tags, uint16_t *tag_lens)
{
  constexpr char charset[] = "0123456789";

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    tags[i] = generate_random_string(charset, sizeof(charset) - 1, MEAN_TAG_LEN, MAX_TAG_LEN, &tag_lens[i]);
}

static void init_random_values(char **values, uint16_t *value_lens)
{
  constexpr char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  
  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    values[i] = generate_random_string(charset, sizeof(charset) - 1, MEAN_VALUE_LEN, MAX_VALUE_LEN, &value_lens[i]);
}

static bool fits_buffer(uint16_t *tag_lens, uint16_t *value_lens)
{
  uint16_t total_len = 0;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    total_len += tag_lens[i] + value_lens[i] + 2;
  total_len += strlen("8=FIX.4.4|9=000000000|10=000|");

  return total_len < BUFFER_SIZE;
}

static void init_message_buffers(char **buffers)
{
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
    buffers[i] = aligned_calloc_p(BUFFER_SIZE, sizeof(char));
}

static void fill_message_structs(ff_message_t *messages, char **tags, char **values, uint16_t *tag_lens, uint16_t *value_lens)
{
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    messages[i].n_fields = i + 1;

    for (uint16_t j = 0; j <= i; j++)
    {
      messages[i].fields[j].tag = tags[j];
      messages[i].fields[j].tag_len = tag_lens[j];
      messages[i].fields[j].value = values[j];
      messages[i].fields[j].value_len = value_lens[j];
    }
  }
}

static void serialize(char **buffers, ff_message_t *messages)
{
  constexpr char filename[] = "benchmark_serialize.csv";
  printf("Running serialization benchmark, saving to %s...\n", filename);
  const int32_t fd = open_p(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_serialize(buffers[i], &messages[i], NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void serialize_and_finalize(char **buffers, ff_message_t *messages, uint16_t *message_lens)
{
  constexpr char filename[] = "benchmark_serialize_and_finalize.csv";
  printf("Running serialization and finalization benchmark, saving to %s...\n", filename);
  const int32_t fd = open_p(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      message_lens[i] = ff_finalize(buffers[i], ff_serialize(buffers[i], &messages[i], NULL), NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void is_full_message_and_deserialize(char **buffers, ff_message_t *messages, uint16_t *message_lenghts)
{
  constexpr char filename[] = "benchmark_is_full_message_and_deserialize.csv";
  printf("Running is_full and deserialization benchmark, saving to %s...\n", filename);
  const int32_t fd = open_p(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      if (ff_is_full_message(buffers[i], BUFFER_SIZE, message_lenghts[i], NULL))
        ff_deserialize(buffers[i], BUFFER_SIZE, &messages[i], NULL);
    }
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void deserialize(char **buffers, ff_message_t *messages)
{
  constexpr char filename[] = "benchmark_deserialize.csv";
  printf("Running deserialization benchmark, saving to %s...\n", filename);
  const int32_t fd = open_p(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_deserialize(buffers[i], BUFFER_SIZE, &messages[i], NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length)
{
  uint16_t len = gaussian_rand(median_len, 1);
  len = clamp(len, 1, max_len);
  char *str = aligned_calloc_p(len, sizeof(char));

  for (uint16_t i = 0; i < len; i++)
    str[i] = charset[rand() % charset_len];

  if (string_length)
    *string_length = len;
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

static void *aligned_calloc_p(const size_t nmemb, const size_t size)
{
  void *ptr = aligned_alloc(ALIGNMENT, nmemb * size);
  if (!ptr)
  {
    perror("aligned_alloc");
    exit(EXIT_FAILURE);
  }
  bzero(ptr, nmemb * size);
  return ptr;
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

static void free_strings(char **strings, const uint16_t n_fields)
{
  for (uint32_t i = 0; i < n_fields; i++)
    free(strings[i]);
}