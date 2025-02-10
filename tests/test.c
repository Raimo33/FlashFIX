/*================================================================================

File: test.c                                                                    
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-10 21:04:36                                                 
last edited: 2025-02-10 21:04:36                                                

================================================================================*/

/*================================================================================

File: test.c                                                                    
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-10 20:25:54                                                 
last edited: 2025-02-10 20:25:54                                                

================================================================================*/

#include <flashfix/serializer.h>
#include <stdio.h>
#include <string.h>

# define mu_assert(message, test) do { if (!(test)) return message; } while (0)
# define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)

int tests_run = 0;

static char *all_tests(void);
static char *test_serializer_normal_message(void);
static char *test_serializer_one_field_message(void);
static char *test_serializer_buffer_too_small(void);
static char *test_serializer_no_error_param(void);
//TODO test finalize
//TODO test is_full
//TODO test deserialize

int main(void)
{
  char *result = all_tests();

  if (result != 0)
    printf("%s\n", result);
  else
    printf("all tests passed\n");

  printf("Tests run: %d\n", tests_run);

  return result != 0;
}

static char *all_tests(void)
{
  mu_run_test(test_serializer_normal_message);
  mu_run_test(test_serializer_one_field_message);
  mu_run_test(test_serializer_buffer_too_small);
  mu_run_test(test_serializer_no_error_param);
  return 0;
}

static char *test_serializer_normal_message(void)
{
  const fix_message_t message = {
    .fields = {
      { .tag = "9", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 22 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  const char expected_buffer[] =
    "9=123\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01";
  const uint16_t expected_len = sizeof(expected_buffer) - 1;
  const ff_error_t expected_error = FF_OK;

  char buffer[sizeof(expected_buffer)];
  ff_error_t error = FF_OK;
  uint16_t len = ff_serialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: serialize normal message: wrong length", len == expected_len);
  mu_assert("error: serialize normal message: wrong error", error == expected_error);
  mu_assert("error: serialize normal message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);
}

static char *test_serializer_one_field_message(void)
{
  const fix_message_t message = {
    .fields = {
      { .tag = "9", .tag_len = 1, .value = "123", .value_len = 3 }
    },
    .n_fields = 1
  };
  const char expected_buffer[] = "9=123\x01";
  const uint16_t expected_len = sizeof(expected_buffer) - 1;
  const ff_error_t expected_error = FF_OK;

  char buffer[sizeof(expected_buffer)];
  ff_error_t error = FF_OK;
  uint16_t len = ff_serialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: serialize one field message: wrong length", len == expected_len);
  mu_assert("error: serialize one field message: wrong error", error == expected_error);
  mu_assert("error: serialize one field message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);
}

static char *test_serializer_buffer_too_small(void)
{
  const fix_message_t message = {
    .fields = {
      { .tag = "9", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 22 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  const char expected_buffer[] = "";
  const uint16_t expected_len = 0;
  const ff_error_t expected_error = FF_BUFFER_TOO_SMALL;

  char buffer[sizeof(expected_buffer)] = {0};
  ff_error_t error = FF_OK;
  uint16_t len = ff_serialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: serialize buffer too small: wrong length", len == expected_len);
  mu_assert("error: serialize buffer too small: wrong error", error == expected_error);
  mu_assert("error: serialize buffer too small: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);
}

static char *test_serializer_no_error_param(void)
{
  const fix_message_t message = {
    .fields = {
      { .tag = "9", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 22 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  const char expected_buffer[] =
    "9=123\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01";
  const uint16_t expected_len = sizeof(expected_buffer) - 1;

  char buffer[sizeof(expected_buffer)];
  uint16_t len = ff_serialize(buffer, sizeof(buffer), &message, NULL);

  mu_assert("error: serialize no error param: wrong length", len == expected_len);
  mu_assert("error: serialize no error param: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);
}