/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-15 17:33:44                                                

================================================================================*/

//TODO FLAMEGRAPH
//TODO check for file errors
//TODO check for malloc errors

#define FIX_MAX_FIELDS 1024
#define N_ITERATIONS 1
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (FIX_MAX_FIELDS + 3)
#define ALIGNMENT 64
#define static_assert _Static_assert

#define _GNU_SOURCE
#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static void init_unaligned_random_tags(char **tags, uint16_t *tag_lens);
static void init_unaligned_random_values(char **values, uint16_t *value_lens);
// static void init_aligned_random_tags(char **tags, uint16_t *tag_lens);
// static void init_aligned_random_values(char **values, uint16_t *value_lens);
static void unaligned_serialize(ff_message_t *messages);
// static void aligned_serialize(ff_message_t *messages);
// static void unaligned_deserialize(char **buffers);
// static void aligned_deserialize(char **buffers);
// static void unaligned_serialize_and_finalize(ff_message_t *messages);
// static void aligned_serialize_and_finalize(ff_message_t *messages);
// static void unaligned_is_full_and_deserialize(char **buffers);
// static void aligned_is_full_and_deserialize(char **buffers);
static void fill_message_structs(ff_message_t *messages, char **tags, char **values, uint16_t *tag_lens, uint16_t *value_lens);
// static void fill_message_buffers();
static inline long long nanoseconds(struct timespec start, struct timespec end);
static char *generate_unaligned_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length);
// static char *generate_aligned_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static void free_strings(char **arr, const uint32_t len);

int32_t main(void)
{
  //TODO detect and print32_t cpu info (architecture, clock_speed, and available avx intructions)

  static_assert(BUFFER_SIZE < UINT16_MAX, "MAX_BUFFER_SIZE must be less than UINT16_MAX");

  char **unaligned_tags = malloc(FIX_MAX_FIELDS * sizeof(char *));
  char **unaligned_values = malloc(FIX_MAX_FIELDS * sizeof(char *));
  // char **aligned_tags = malloc(FIX_MAX_FIELDS * sizeof(char *));
  // char **aligned_values = malloc(FIX_MAX_FIELDS * sizeof(char *));
  uint16_t *tag_lens = malloc(FIX_MAX_FIELDS * sizeof(uint16_t));
  uint16_t *value_lens = malloc(FIX_MAX_FIELDS * sizeof(uint16_t));

  init_unaligned_random_tags(unaligned_tags, tag_lens);
  init_unaligned_random_values(unaligned_values, value_lens);
  // init_aligned_random_tags(aligned_tags, tag_lens);
  // init_aligned_random_values(aligned_values, value_lens);

  ff_message_t *unaligned_message_structs = malloc(FIX_MAX_FIELDS * sizeof(ff_message_t));
  // ff_message_t *aligned_message_structs = malloc(FIX_MAX_FIELDS * sizeof(ff_message_t));
  // char **unaligned_message_buffers = malloc(FIX_MAX_FIELDS * sizeof(char *));
  // char **aligned_message_buffers = malloc(FIX_MAX_FIELDS * sizeof(char *));

  fill_message_structs(unaligned_message_structs, unaligned_tags, unaligned_values, tag_lens, value_lens);
  // fill_message_structs(aligned_message_structs, aligned_tags, aligned_values, tag_lens, value_lens);
  // fill_message_buffers(unaligned_message_buffers, unaligned_message_structs);
  // fill_message_buffers(aligned_message_buffers, aligned_message_structs);

  free(tag_lens);
  free(value_lens);

  printf("Benchmarking FlashFIX with %d iterations per test...\n", N_ITERATIONS);

  unaligned_serialize(unaligned_message_structs);
  //aligned_serialize(aligned_message_structs);
  //unaligned_deserialize(unaligned_message_buffers);
  //aligned_deserialize(aligned_message_buffers);
  //unaligned_serialize_and_finalize(unaligned_message_structs);
  //aligned_serialize_and_finalize(aligned_message_structs);
  //unaligned_is_full_and_deserialize(unaligned_message_buffers);
  //aligned_is_full_and_deserialize(aligned_message_buffers);

  free_strings(unaligned_tags, FIX_MAX_FIELDS);
  free_strings(unaligned_values, FIX_MAX_FIELDS);
  // free_arr(aligned_tags, FIX_MAX_FIELDS);
  // free_arr(aligned_values, FIX_MAX_FIELDS);
  free(unaligned_tags);
  free(unaligned_values);
  // free(aligned_tags);
  // free(aligned_values);
  free(unaligned_message_structs);
  // free(aligned_message_structs);
  // free(unaligned_message_buffers);
  // free(aligned_message_buffers);
}

static void init_unaligned_random_tags(char **tags, uint16_t *tag_lens)
{
  constexpr char charset[] = "0123456789";

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    tags[i] = generate_unaligned_random_string(charset, sizeof(charset) - 1, MEAN_TAG_LEN, MAX_TAG_LEN, &tag_lens[i]);
}

static void init_unaligned_random_values(char **values, uint16_t *value_lens)
{
  constexpr char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  
  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    values[i] = generate_unaligned_random_string(charset, sizeof(charset) - 1, MEAN_VALUE_LEN, MAX_VALUE_LEN, &value_lens[i]);
}

static void unaligned_serialize(ff_message_t *messages)
{
  char buffer[BUFFER_SIZE + 1] = {0};
  char *unaligned_buffer = buffer + 1;
  const uint16_t buffer_size = sizeof(buffer) - 1;

  constexpr char filename[] = "benchmark_serialize_unaligned.csv";
  printf("Running unaligned serialization benchmark, saving to %s...\n", filename);
  const int fd = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  struct timespec start, end;

  dprintf(fd, "# of fields, avg time (ns), throughput (msg/s)\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_serialize(unaligned_buffer, buffer_size, &messages[i], NULL);
    printf("message.n_fields: %d\n", messages[i].n_fields);
    clock_gettime(CLOCK_MONOTONIC, &end);
  
    const double average_elapsed_ns = nanoseconds(start, end) / (double)N_ITERATIONS;
    const uint64_t throughput = 1e9 / average_elapsed_ns;
    dprintf(fd, "%d, %.2f, %lu\n", i + 1, average_elapsed_ns, throughput);
  }

  close(fd);
}

static void fill_message_structs(ff_message_t *messages, char **tags, char **values, uint16_t *tag_lens, uint16_t *value_lens)
{
  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    for (uint32_t j = 0; j <= i; j++)
    {
      messages[i].fields[j].tag = tags[j];
      messages[i].fields[j].tag_len = tag_lens[j];
      messages[i].fields[j].value = values[j];
      messages[i].fields[j].value_len = value_lens[j];
    }
    messages[i].n_fields = i + 1;
  }
}

// static void fill_message_buffers()
// {
//   //TODO, no malloc, use the values and tags arrays
// }

static inline long long nanoseconds(struct timespec start, struct timespec end)
{
  return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

static char *generate_unaligned_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length)
{
  uint16_t len = gaussian_rand(median_len, 1);
  len = clamp(len, 1, max_len);
  char *str = malloc(len + 1);

  for (uint16_t i = 0; i < len; i++)
    str[i] = charset[rand() % charset_len];
  str[len] = '\0';

  if (string_length)
    *string_length = len;
  return str;
}

// static char *generate_aligned_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length)
// {
//   uint16_t len = gaussian_rand(median_len, 1);
//   len = clamp(len, 1, max_len);
//   char *str = aligned_alloc(ALIGNMENT, len + 1);

//   for (uint16_t i = 0; i < len; i++)
//     str[i] = charset[rand() % charset_len];
//   str[len] = '\0';

//   if (string_length)
//     *string_length = len;
//   return str;
// }

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

static void free_strings(char **arr, const uint32_t len)
{
  for (uint32_t i = 0; i < len; i++)
    free(arr[i]);
}