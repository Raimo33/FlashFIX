/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-16 19:07:50                                                

================================================================================*/

//TODO FLAMEGRAPH
//TODO check for file errors
//TODO check for malloc errors

#define N_ITERATIONS 1'000'000
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (FIX_MAX_FIELDS + 3) //TODO could be surpassed by a lot in rare random cases
#define ALIGNMENT 64
#define static_assert _Static_assert

#define _GNU_SOURCE
#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <immintrin.h>

static void init_random_tags(char **tags, uint16_t *tag_lens);
static void init_random_values(char **values, uint16_t *value_lens);
static void init_message_buffers(char **buffers);
static void serialize(char **buffers, ff_message_t *messages);
static void deserialize(char **buffers, ff_message_t *messages);
static void serialize_and_finalize(char **buffers, ff_message_t *messages, uint16_t *message_lens);
static void is_full_and_deserialize(char **buffers, ff_message_t *messages, uint16_t *message_lenghts);
static void fill_message_structs(ff_message_t *messages, char **tags, char **values, uint16_t *tag_lens, uint16_t *value_lens);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static void free_strings(char **strings, const uint16_t n_fields);

int32_t main(void)
{
  //TODO detect and print32_t cpu info (architecture, clock_speed, and available avx intructions)

  static_assert(BUFFER_SIZE < UINT16_MAX, "MAX_BUFFER_SIZE must be less than UINT16_MAX");

  char **tags = calloc(FIX_MAX_FIELDS, sizeof(char *));
  char **values = calloc(FIX_MAX_FIELDS, sizeof(char *));
  uint16_t *tag_lens = calloc(FIX_MAX_FIELDS, sizeof(uint16_t));
  uint16_t *value_lens = calloc(FIX_MAX_FIELDS, sizeof(uint16_t));
  ff_message_t *message_structs = calloc(FIX_MAX_FIELDS, sizeof(ff_message_t));
  char **message_buffers = calloc(FIX_MAX_FIELDS, sizeof(char *));
  uint16_t *message_lens = calloc(FIX_MAX_FIELDS, sizeof(uint16_t));

  init_random_tags(tags, tag_lens);
  init_random_values(values, value_lens);
  init_message_buffers(message_buffers);

  fill_message_structs(message_structs, tags, values, tag_lens, value_lens);

  free(tag_lens);
  free(value_lens);

  printf("Benchmarking FlashFIX with %d iterations per test...\n", N_ITERATIONS);

  serialize(message_buffers, message_structs);
  serialize_and_finalize(message_buffers, message_structs, message_lens);
  is_full_and_deserialize(message_buffers, message_structs, message_lens);
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

static void init_message_buffers(char **buffers)
{
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
    buffers[i] = aligned_alloc(ALIGNMENT, BUFFER_SIZE);
}

static void serialize(char **buffers, ff_message_t *messages)
{
  constexpr char filename[] = "benchmark_serialize.csv";
  printf("Running serialization benchmark, saving to %s...\n", filename);
  const int fd = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_serialize(buffers[i], BUFFER_SIZE, &messages[i], NULL);
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
  const int fd = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      message_lens[i] = ff_finalize(buffers[i], BUFFER_SIZE, ff_serialize(buffers[i], BUFFER_SIZE, &messages[i], NULL), NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void is_full_and_deserialize(char **buffers, ff_message_t *messages, uint16_t *message_lenghts)
{
  constexpr char filename[] = "benchmark_is_full_and_deserialize.csv";
  printf("Running is_full and deserialization benchmark, saving to %s...\n", filename);
  const int fd = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      if (ff_is_full(buffers[i], BUFFER_SIZE, message_lenghts[i], NULL))
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
  const int fd = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
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

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length)
{
  uint16_t len = gaussian_rand(median_len, 1);
  len = clamp(len, 1, max_len);
  char *str = aligned_alloc(ALIGNMENT, len);

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

static void free_strings(char **strings, const uint16_t n_fields)
{
  for (uint32_t i = 0; i < n_fields; i++)
    free(strings[i]);
}