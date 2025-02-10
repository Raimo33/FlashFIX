/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/10 17:50:26 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 18:00:01 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <flashfix.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define TEST_BUFFER_SIZE 1024
#define ASSERT(condition) \
  do { \
    if (!(condition)) { \
      printf("FAIL at %s:%d\n", __FILE__, __LINE__); \
      return false; \
    } \
  } while (0)

#define RUN_TEST(test_fn) \
  do { \
    printf("Running %-30s", #test_fn); \
    if (test_fn()) \
      printf("[OK]\n"); \
    else \
      printf("[FAIL]\n"); \
  } while (0)

typedef struct {
  fix_message_t message;
  const char* expected;
  size_t expected_len;
  size_t buffer_size;
  ff_error_t expected_error;
} test_case_t;

static bool test_standard_message(void);
static bool test_buffer_too_small(void);
static bool test_one_field_message(void);
static bool test_no_error_param(void);

static void test_serializer(void) {
  RUN_TEST(test_standard_message);
  RUN_TEST(test_buffer_too_small);
  RUN_TEST(test_one_field_message);
  RUN_TEST(test_no_error_param);
}

static bool run_serializer_test(const test_case_t *test)
{
  char buffer[TEST_BUFFER_SIZE] = {0};
  ff_error_t error;
  int32_t ret;
  
  const size_t buffer_size = test->buffer_size ? 
                            test->buffer_size : 
                            sizeof(buffer);
  
  ret = ff_serialize(buffer, buffer_size, &test->message, &error);
  
  if (test->expected_error != FF_OK)
  {
    ASSERT(ret == 0);
    ASSERT(error == test->expected_error);
    return true;
  }
  
  ASSERT(ret == (int32_t)test->expected_len);
  ASSERT(memcmp(buffer, test->expected, test->expected_len) == 0);
  ASSERT(error == test->expected_error);
  
  return true;
}

static bool test_standard_message(void)
{
  const fix_message_t standard_msg = {
    .fields = {
      { "35", "D", 2, 1 },
      { "49", "BROKER", 2, 6 },
      { "56", "EXCHANGE", 2, 8 },
      { "34", "1", 2, 1 },
      { "52", "20250210-15:15:24.000", 2, 23 },
      { "11", "123456", 2, 6 },
      { "21", "1", 2, 1 },
      { "55", "AAPL", 2, 4 },
      { "40", "2", 2, 1 },
      { "54", "1", 2, 1 },
      { "38", "100", 2, 3 },
    },
    .n_fields = 11
  };

  const char expected[] = 
    "35=D\x01"
    "49=BROKER\x01"
    "56=EXCHANGE\x01"
    "34=1\x01"
    "52=20250210-15:15:24.000\x01"
    "11=123456\x01"
    "21=1\x01"
    "55=AAPL\x01"
    "40=2\x01"
    "54=1\x01"
    "38=100\x01";

  const test_case_t test = {
    .message = standard_msg,
    .expected = expected,
    .expected_len = sizeof(expected) - 1,
    .expected_error = FF_OK
  };

  return run_serializer_test(&test);
}

static bool test_one_field_message(void)
{
  const fix_message_t one_field_msg = {
    .fields = {
      { "55", "BTCUSDT", 2, 7 }
    },
    .n_fields = 1
  };

  const char expected[] = 
    "55=BTCUSDT\x01";

  const test_case_t test = {
    .message = one_field_msg,
    .expected = expected,
    .expected_len = sizeof(expected) - 1,
    .expected_error = FF_OK
  };

  return run_serializer_test(&test);
}

static bool test_buffer_too_small(void)
{
  const fix_message_t dummy_msg = {
    .fields = {
      { "55", "AAPL", 2, 4 }
    },
    .n_fields = 1
  };
  const test_case_t test = {
    .message = dummy_msg,
    .buffer_size = 0,
    .expected_error = FF_BUFFER_TOO_SMALL
  };
  return run_serializer_test(&test);
}

static bool test_no_error_param(void)
{
  char buffer[TEST_BUFFER_SIZE];
  const fix_message_t dummy_msg = {
    .fields = {
      { "55", "AAPL", 2, 4 }
    },
    .n_fields = 1
  };
  const int32_t ret = ff_serialize(buffer, sizeof(buffer), &dummy_msg, NULL);
  ASSERT(ret > 0);
  return true;
}

int main(void)
{
  printf("Running tests...\n");
  printf("Testing serializer...\n");
  test_serializer();
  printf("Testing deserializer...\n");
  // test_deserializer();
  printf("Tests completed.\n");
  return 0;
}