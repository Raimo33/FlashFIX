/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-15 00:17:29                                                

================================================================================*/

//TODO FLAMEGRAPH

#define FIX_MAX_FIELDS 1024
#define N_ITERATIONS 1'000'000
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (FIX_MAX_FIELDS + 3)
#define static_assert _Static_assert

#define _GNU_SOURCE
#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

static void benchmark_serialize(void);
static void benchmark_deserialize(void);
static void generate_message_structs(ff_message_t *messages);
static void generate_message_buffers(char **buffers);
static inline long long nanoseconds(struct timespec start, struct timespec end);
static uint8_t compute_checksum(const char *buffer, const uint16_t len);
static char *generate_random_body(const uint16_t n_fields, uint16_t *body_len);
static char *generate_random_field(void);
static char *generate_random_tag(uint16_t *len);
static char *generate_random_value(uint16_t *len);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);

int32_t main(void)
{
  //TODO detect and print32_t cpu info (architecture, clock_speed, and available avx intructions)

  static_assert(BUFFER_SIZE < UINT16_MAX, "MAX_BUFFER_SIZE must be less than UINT16_MAX");

  benchmark_serialize();
  benchmark_deserialize();
  //TODO benchmark serialize+finalize
  //TODO benchmark is_full+deserialize

  return 0;
}

static void benchmark_serialize(void)
{
  //TODO two benchmarks, one aligned and one unaligned
  ff_message_t *messages = calloc(FIX_MAX_FIELDS, sizeof(ff_message_t));
  generate_message_structs(messages);
  char buffer[BUFFER_SIZE] = {0};
  struct timespec start, end;

  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_serialize(buffer, sizeof(buffer), &messages[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
  
    const double elapsed_seconds = nanoseconds(start, end) / 1e9;
    const uint64_t throughput = (uint64_t)(N_ITERATIONS / elapsed_seconds);
    printf("Serialize (unaligned) %d fields in %f seconds, throughput: %lu messages/s\n", i + 1, elapsed_seconds, throughput);
  }
}

static void benchmark_deserialize(void)
{
  //TODO two benchmarks, one aligned and one unaligned
  char **messages = calloc(FIX_MAX_FIELDS * BUFFER_SIZE, sizeof(char *));
  generate_message_buffers(messages);
  ff_message_t message = {0};
  struct timespec start, end;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_deserialize(messages[i], sizeof(messages[i]), &message, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
  
    const double elapsed_seconds = nanoseconds(start, end) / 1e9;
    const uint64_t throughput = (uint64_t)(N_ITERATIONS / elapsed_seconds);
    printf("Deserialize (unaligned) %d fields in %f seconds, throughput: %lu messages/s\n", i + 1, elapsed_seconds, throughput);
  }
}

static void generate_message_structs(ff_message_t *messages)
{
  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    messages[i].n_fields = i + 1;
    for (uint32_t j = 0; j <= i; j++)
    {
      messages[i].fields[j].tag = generate_random_tag(&messages[i].fields[j].tag_len);
      messages[i].fields[j].value = generate_random_value(&messages[i].fields[j].value_len);
    }
  }
}

static void generate_message_buffers(char **buffers)
{
  uint16_t body_length;
  char *body = NULL;
  char *body_and_header = NULL;
  char *full_message = NULL;
  uint16_t len;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {

    do {
      free(body);
      free(body_and_header);
      free(full_message);
      body = generate_random_body(i + 1, &body_length);
      len = asprintf(&body_and_header, "8=FIX.4.4%c9=%d%c%s%c", '\x01', body_length, '\x01', body, '\x01');
      const uint8_t checksum = compute_checksum(body_and_header, len);
      len = asprintf(&full_message, "%s10=%03d%c", body_and_header, checksum, '\x01');
    } while (len > BUFFER_SIZE);

    sprintf(buffers[i], "%s", full_message);
  }

  free(body);
  free(body_and_header);
  free(full_message);
}

static inline long long nanoseconds(struct timespec start, struct timespec end)
{
  return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

static uint8_t compute_checksum(const char *buffer, const uint16_t len)
{
  uint8_t checksum = 0;
  for (uint16_t i = 0; i < len; i++)
    checksum += buffer[i];
  return checksum;
}

static char *generate_random_body(const uint16_t n_fields, uint16_t *body_len)
{
  char *body;
  uint16_t len = 0;

  for (uint16_t i = 0; i < n_fields; i++)
  {
    char *field = generate_random_field();
    len += asprintf(&body, "%s%s", body, field);
    free(field);
  }

  *body_len = len;
  return body;
}

static char *generate_random_field(void)
{
  char *tag = generate_random_tag(NULL);
  char *value = generate_random_value(NULL);
  char *field;

  asprintf(&field, "%s=%s%c", tag, value, '\x01');

  free(tag);
  free(value);
  return field;
}

static char *generate_random_tag(uint16_t *len)
{
  constexpr char charset[] = "0123456789";

  return generate_random_string(charset, sizeof(charset) - 1, MEAN_TAG_LEN, MAX_TAG_LEN, len);
}

static char *generate_random_value(uint16_t *len)
{
  constexpr char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  return generate_random_string(charset, sizeof(charset) - 1, MEAN_VALUE_LEN, MAX_VALUE_LEN, len);
}

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len, uint16_t *string_length)
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