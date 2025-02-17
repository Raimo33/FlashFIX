/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-17 13:36:59                                                

================================================================================*/

#define _GNU_SOURCE
#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <immintrin.h>

#define N_ITERATIONS 1'000'000
#define MEAN_TAG_LEN 2
#define MEAN_VALUE_LEN 6
#define MAX_TAG_LEN 5
#define MAX_VALUE_LEN 1024
#define ALIGNMENT 64
#define BUFFER_SIZE (MEAN_TAG_LEN + MEAN_VALUE_LEN + 2) * (FIX_MAX_FIELDS + 3)
#define static_assert _Static_assert
#define STR_LEN(str) sizeof(str) - 1
#define ALIGNED(n) __attribute__((aligned(n)))

static void init_random_tags(char *tags[FIX_MAX_FIELDS]);
static void init_random_values(char *values[FIX_MAX_FIELDS]);
static bool fits_buffer(char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS]);
static void fill_message_structs(ff_message_t *messages, char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS]);
static void fill_message_buffers(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE], char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS]);
static void fill_message_lengths(uint16_t message_lengths[FIX_MAX_FIELDS], char message_buffers[FIX_MAX_FIELDS][BUFFER_SIZE]);
static void serialize(ff_message_t messages[FIX_MAX_FIELDS]);
static void serialize_and_finalize(ff_message_t messages[FIX_MAX_FIELDS]);
static void fits_in_buffer_and_serialize_and_finalize(ff_message_t messages[FIX_MAX_FIELDS]);
static void deserialize(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE]);
static void is_full_message_and_deserialize(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE], uint16_t message_lenghts[FIX_MAX_FIELDS]);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static uint8_t compute_checksum(const char *buffer, const uint16_t len);
static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode);
static void *aligned_calloc_p(const size_t n, const size_t size);
static void free_strings(char *strings[FIX_MAX_FIELDS], const uint16_t n);

int32_t main(void)
{
  //TODO detect and print cpu info (architecture, clock_speed, and available avx intructions)

  static_assert(BUFFER_SIZE < UINT16_MAX, "BUFFER_SIZE must be less than UINT16_MAX");

  char *tags[FIX_MAX_FIELDS] ALIGNED(ALIGNMENT) = {0};
  char *values[FIX_MAX_FIELDS] ALIGNED(ALIGNMENT) = {0};

  do {
    bzero(tags, FIX_MAX_FIELDS * sizeof(char *));
    bzero(values, FIX_MAX_FIELDS * sizeof(char *));

    init_random_tags(tags);
    init_random_values(values);
  } while (!fits_buffer(tags, values));
  
  {
    ff_message_t message_structs[FIX_MAX_FIELDS] ALIGNED(ALIGNMENT) = {0};
    
    fill_message_structs(message_structs, tags, values);
    
    serialize(message_structs);
    serialize_and_finalize(message_structs);
    fits_in_buffer_and_serialize_and_finalize(message_structs);
  }

  {
    char message_buffers[FIX_MAX_FIELDS][BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};
    uint16_t message_lengths[FIX_MAX_FIELDS] ALIGNED(ALIGNMENT) = {0};

    fill_message_buffers(message_buffers, tags, values);
    fill_message_lengths(message_lengths, message_buffers);

    deserialize(message_buffers);
    is_full_message_and_deserialize(message_buffers, message_lengths);
  }
  
  free_strings(tags, FIX_MAX_FIELDS);
  free_strings(values, FIX_MAX_FIELDS);
}

static void init_random_tags(char *tags[FIX_MAX_FIELDS])
{
  constexpr char charset[] = "0123456789";
  constexpr uint8_t charset_len = sizeof(charset) - 1;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    tags[i] = generate_random_string(charset, charset_len, MEAN_TAG_LEN, MAX_TAG_LEN);
}

static void init_random_values(char *values[FIX_MAX_FIELDS])
{
  constexpr char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  constexpr uint8_t charset_len = sizeof(charset) - 1;
  
  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    values[i] = generate_random_string(charset, charset_len, MEAN_VALUE_LEN, MAX_VALUE_LEN);
}

static bool fits_buffer(char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS])
{
  uint16_t total_len = STR_LEN("8=FIX.4.4|9=000000000|10=000|");

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
    total_len += strlen(tags[i]) + strlen(values[i]) + 2;

  return total_len < BUFFER_SIZE;
}

static void fill_message_structs(ff_message_t *messages, char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS])
{
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    messages[i].n_fields = i + 1;

    for (uint16_t j = 0; j <= i; j++)
    {
      messages[i].fields[j].tag = tags[j];
      messages[i].fields[j].tag_len = strlen(tags[j]);
      messages[i].fields[j].value = values[j];
      messages[i].fields[j].value_len = strlen(values[j]);
    }
  }
}

static void fill_message_buffers(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE], char *tags[FIX_MAX_FIELDS], char *values[FIX_MAX_FIELDS])
{
  char temp_body[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    char *buffer = buffers[i];
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

static void fill_message_lengths(uint16_t message_lengths[FIX_MAX_FIELDS], char message_buffers[FIX_MAX_FIELDS][BUFFER_SIZE])
{
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
    message_lengths[i] = strlen(message_buffers[i]);
}

static void serialize(ff_message_t messages[FIX_MAX_FIELDS])
{
  const int32_t fd = open_p("benchmark_serialize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_serialize(buffer, &messages[i], NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void serialize_and_finalize(ff_message_t messages[FIX_MAX_FIELDS])
{
  const int32_t fd = open_p("benchmark_serialize_and_finalize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_finalize(buffer, ff_serialize(buffer, &messages[i], NULL), NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void fits_in_buffer_and_serialize_and_finalize(ff_message_t messages[FIX_MAX_FIELDS])
{
  const int32_t fd = open_p("benchmark_fits_in_buffer_and_serialize_and_finalize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      if (ff_message_fits_in_buffer(&messages[i], BUFFER_SIZE, NULL))
        ff_finalize(buffer, ff_serialize(buffer, &messages[i], NULL), NULL);
    }
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void deserialize(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE])
{
  const int32_t fd = open_p("benchmark_deserialize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  ff_message_t message ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      ff_deserialize(buffers[i], BUFFER_SIZE, &message, NULL);
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static void is_full_message_and_deserialize(char buffers[FIX_MAX_FIELDS][BUFFER_SIZE], uint16_t message_lenghts[FIX_MAX_FIELDS])
{
  const int32_t fd = open_p("benchmark_is_full_message_and_deserialize.csv", O_TRUNC | O_CREAT | O_WRONLY, 0644);
  
  uint64_t start, end;
  uint32_t aux;

  ff_message_t message ALIGNED(ALIGNMENT) = {0};

  dprintf(fd, "# of fields, # of cpu cycles\n");
  for (uint16_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      if (ff_is_full_message(buffers[i], BUFFER_SIZE, message_lenghts[i], NULL))
        ff_deserialize(buffers[i], BUFFER_SIZE, &message, NULL);
    }
    end = __rdtscp(&aux);
  
    const uint64_t avg_cpu_cycles = (end - start) / N_ITERATIONS;
    dprintf(fd, "%d, %lu\n", i + 1, avg_cpu_cycles);
  }

  close(fd);
}

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint16_t median_len, const uint16_t max_len)
{
  uint16_t len = gaussian_rand(median_len, 1);
  len = clamp(len, 1, max_len);
  char *str = aligned_calloc_p(len + 1, sizeof(char));

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

static void *aligned_calloc_p(const size_t n, const size_t size)
{
  void *ptr = aligned_alloc(ALIGNMENT, n * size);
  if (!ptr)
  {
    perror("aligned_alloc");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static void free_strings(char *strings[FIX_MAX_FIELDS], const uint16_t n)
{
  for (uint16_t i = 0; i < n; i++)
    free(strings[i]);
}