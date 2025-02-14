/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-02-14 18:07:32                                                

================================================================================*/

//TODO FLAMEGRAPH

# define FIX_MAX_FIELDS 1024
# define N_ITERATIONS 1'000'000
# define MEDIAN_TAG_LEN 2
# define MEDIAN_VALUE_LEN 6

#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(void)
{
  //TODO detect and print cpu info (architecture, clock_speed, and available avx intructions)

  benchmark_serialize();
  benchmark_deserialize();
  //TODO benchmark serialize+finalize
  //TODO benchmark is_full+deserialize

  return 0;
}

static void benchmark_serialize(void)
{
  //TODO two benchmarks, one aligned and one unaligned

  const ff_message_t *messages = generate_message_structs();
  struct timespec start, end;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    uint32_t n = N_ITERATIONS;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (n--)
      flashfix_serialize(*messages++);
    clock_gettime(CLOCK_MONOTONIC, &end);
  
    const double elapsed_seconds = nanoseconds(start, end) / 1e9;
    printf("Serialize %d fields: %lld ns, throughput: %f messages/s\n", i + 1, nanoseconds(start, end), N_ITERATIONS / elapsed_seconds);
  }
}

static void benchmark_deserialize(void)
{
  //TODO two benchmarks, one aligned and one unaligned

  const char *messages = generate_message_buffers();
  struct timespec start, end;

  for (uint32_t i = 0; i < FIX_MAX_FIELDS; i++)
  {
    uint32_t n = N_ITERATIONS;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (n--)
      flashfix_deserialize(*messages++);
    clock_gettime(CLOCK_MONOTONIC, &end);
  
    const double elapsed_seconds = nanoseconds(start, end) / 1e9;
    printf("Deserialize %d fields: %lld ns, throughput: %f messages/s\n", i + 1, nanoseconds(start, end), N_ITERATIONS / elapsed_seconds);
  }
}

static const ff_message_t *generate_message_structs(void)
{
  static ff_message_t messages[FIX_MAX_FIELDS];

  for (int i = 0; i < FIX_MAX_FIELDS; i++)
  {
    messages[i].n_fields = i + 1;
    for (int j = 0; j <= i; j++)
    {
      //TODO generate random tags and values with random lengths with a gaussian distribution
    }
  }
  return messages;
}

static const char *generate_message_buffers(void)
{
  //TODO return an array of message buffers, first with one field, then two, then three, etc.
  //TODO real body length, checksum, and begin string
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

static char *generate_random_tag(void)
{
  constexpr char charset[] = "0123456789";


}

static char *generate_random_value(void)
{
  constexpr char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
}

static double gaussian_rand(double mean, double stddev)
{
  const double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
  return z0 * stddev + mean;
}