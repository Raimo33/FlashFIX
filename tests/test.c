/*================================================================================

File: test.c                                                                    
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-10 21:08:13                                                 
last edited: 2025-02-24 16:35:15                                                

================================================================================*/

#include <flashfix.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <fcntl.h>

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)
#define static_assert _Static_assert

int tests_run = 0;

static bool compare_messages(const ff_message_t *a, const ff_message_t *b)
{
  if (a->n_fields != b->n_fields)
    return false;

  for (uint8_t i = 0; i < a->n_fields; i++)
  {
    const bool tag_lens_equal = a->fields[i].tag_len == b->fields[i].tag_len;
    const bool value_lens_equal = a->fields[i].value_len == b->fields[i].value_len;
    const bool tags_equal = memcmp(a->fields[i].tag, b->fields[i].tag, a->fields[i].tag_len) == 0;
    const bool values_equal = memcmp(a->fields[i].value, b->fields[i].value, a->fields[i].value_len) == 0;

    const bool are_equal = tag_lens_equal && value_lens_equal && tags_equal && values_equal;

    if (!are_equal)
      return false;
  }

  return true;
}

static bool compare_files(int fd, const char *expected_output)
{
  const size_t expected_len = strlen(expected_output);
  size_t total_read = 0;
  char buffer[4096];
  ssize_t bytes_read;

  while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
  {
    if (total_read + (size_t)bytes_read > expected_len)
      return false;
    if (memcmp(buffer, expected_output + total_read, bytes_read) != 0)
      return false;
    total_read += bytes_read;
  }

  if (bytes_read == -1)
    return false;
  if (total_read != expected_len)
    return false;

  return true;
}

void flood_fd(int fd)
{
  char buf[4096] = {0};
  while (write(fd, buf, sizeof(buf)) > 0);
}

static char *all_tests(void);
static char *test_message_fits_in_buffer_positive(void);
static char *test_message_fits_in_buffer_negative(void);
static char *test_serialize_normal_message(void);
static char *test_serialize_one_field_message(void);
static char *test_serialize_no_error_param(void);
static char *test_serialize_write_normal_message(void);
static char *test_serialize_write_one_field_message(void);
static char *test_serialize_write_no_error_param(void);
static char *test_serialize_write_non_blocking(void);
static char *test_is_full_normal_message_positive(void);
static char *test_is_full_normal_message_negative(void);
static char *test_is_full_message_too_big(void);
static char *test_is_full_no_error_param(void);
static char *test_deserialize_normal_message(void);
static char *test_deserialize_too_many_fields(void);
static char *test_deserialize_no_error_param(void);
static char *test_deserialize_no_beginstr(void);
static char *test_deserialize_no_body_length(void);
static char *test_deserialize_wrong_beginstr(void);
static char *test_deserialize_wrong_body_length1(void);
static char *test_deserialize_wrong_body_length2(void);
static char *test_deserialize_checksum_mismatch(void);

int main(void)
{
  char *result = all_tests();

  if (result != 0)
    printf("%s\n", result);
  else
    printf("all tests passed\n");

  printf("Tests run: %d\n", tests_run);

  return !!result;
}

static char *all_tests(void)
{
  mu_run_test(test_message_fits_in_buffer_positive);
  mu_run_test(test_message_fits_in_buffer_negative);

  mu_run_test(test_serialize_normal_message);
  mu_run_test(test_serialize_one_field_message);
  mu_run_test(test_serialize_no_error_param);

  mu_run_test(test_serialize_write_normal_message);
  mu_run_test(test_serialize_write_one_field_message);
  mu_run_test(test_serialize_write_no_error_param);
  mu_run_test(test_serialize_write_non_blocking);

  mu_run_test(test_is_full_normal_message_positive);
  mu_run_test(test_is_full_normal_message_negative);
  mu_run_test(test_is_full_message_too_big);
  mu_run_test(test_is_full_no_error_param);

  mu_run_test(test_deserialize_normal_message);
  mu_run_test(test_deserialize_too_many_fields);
  mu_run_test(test_deserialize_no_error_param);
  mu_run_test(test_deserialize_no_beginstr);
  mu_run_test(test_deserialize_no_body_length);
  mu_run_test(test_deserialize_wrong_beginstr);
  mu_run_test(test_deserialize_wrong_body_length1);
  mu_run_test(test_deserialize_wrong_body_length2);
  mu_run_test(test_deserialize_checksum_mismatch);

  return 0;
}

static char *test_message_fits_in_buffer_positive(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  constexpr uint16_t buffer_size = 56 + 26 + 16;

  ff_error_t error = FF_OK;
  bool fits = ff_message_fits_in_buffer(&message, buffer_size, &error);

  mu_assert("error: message fits in buffer positive: wrong outcome", fits == true);
  mu_assert("error: message fits in buffer positive: wrong error", error == FF_OK);

  return 0;
}

static char *test_message_fits_in_buffer_negative(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  constexpr uint16_t buffer_size = 56 + 26 + 16 - 1;

  ff_error_t error = FF_OK;
  bool fits = ff_message_fits_in_buffer(&message, buffer_size, &error);

  mu_assert("error: message fits in buffer negative: wrong outcome", fits == false);
  mu_assert("error: message fits in buffer negative: wrong error", error == FF_OK);

  return 0;
}

static char *test_serialize_normal_message(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  constexpr char expected_buffer[] =
    "6=123\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01";
  constexpr uint16_t expected_len = sizeof(expected_buffer) - 1;
  constexpr ff_error_t expected_error = FF_OK;

  char buffer[sizeof(expected_buffer)];
  ff_error_t error = FF_OK;
  uint16_t len = ff_serialize(buffer, &message, &error);

  mu_assert("error: serialize normal message: wrong length", len == expected_len);
  mu_assert("error: serialize normal message: wrong error", error == expected_error);
  mu_assert("error: serialize normal message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_one_field_message(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 }
    },
    .n_fields = 1
  };
  constexpr char expected_buffer[] = "6=123\x01";
  constexpr uint16_t expected_len = sizeof(expected_buffer) - 1;
  constexpr ff_error_t expected_error = FF_OK;

  char buffer[sizeof(expected_buffer)];
  ff_error_t error = FF_OK;
  uint16_t len = ff_serialize(buffer, &message, &error);

  mu_assert("error: serialize one field message: wrong length", len == expected_len);
  mu_assert("error: serialize one field message: wrong error", error == expected_error);
  mu_assert("error: serialize one field message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_no_error_param(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  constexpr char expected_buffer[] =
    "6=123\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01";
  constexpr uint16_t expected_len = sizeof(expected_buffer) - 1;

  char buffer[sizeof(expected_buffer)];
  uint16_t len = ff_serialize(buffer, &message, NULL);

  mu_assert("error: serialize no error param: wrong length", len == expected_len);
  mu_assert("error: serialize no error param: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_write_normal_message(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 },
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 8
  };
  constexpr char expected_output[] =
    "8=FIX.4.4\x01"
    "9=73\x01"
    "6=123\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=127\x01";
  constexpr int32_t expected_len = sizeof(expected_output) - 1;

  int32_t fds[2];
  mu_assert("error: serialize write normal message: pipe failed", pipe(fds) == 0);
  ff_write_state_t state = {0};
  ff_error_t error = FF_OK;
  const int32_t len = ff_serialize_and_write(fds[1], &message, &state, &error);
  close(fds[1]);

  mu_assert("error: serialize write normal message: wrong len", len == expected_len);
  mu_assert("error: serialize write normal message: wrong output", compare_files(fds[0], expected_output));
  mu_assert("error: serialize write normal message: wrong error", error == FF_OK);

  close(fds[0]);

  return 0;
}

static char *test_serialize_write_one_field_message(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "6", .tag_len = 1, .value = "123", .value_len = 3 }
    },
    .n_fields = 1
  };
  constexpr char expected_output[] =
    "8=FIX.4.4\x01"
    "9=6\x01"
    "6=123\x01"
    "10=216\x01";
  constexpr int32_t expected_len = sizeof(expected_output) - 1;

  int32_t fds[2];
  mu_assert("error: serialize write one field message: pipe failed", pipe(fds) == 0);
  ff_write_state_t state = {0};
  ff_error_t error = FF_OK;
  const int32_t len = ff_serialize_and_write(fds[1], &message, &state, &error);
  close(fds[1]);

  mu_assert("error: serialize write one field message: wrong len", len == expected_len);
  mu_assert("error: serialize write one field message: wrong output", compare_files(fds[0], expected_output));
  mu_assert("error: serialize write one field message: wrong error", error == FF_OK);

  close(fds[0]);

  return 0;
}

static char *test_serialize_write_no_error_param(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 7
  };
  constexpr char expected_output[] =
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
  constexpr int32_t expected_len = sizeof(expected_output) - 1;

  int32_t fds[2];
  mu_assert("error: serialize write no error param: pipe failed", pipe(fds) == 0);
  ff_write_state_t state = {0};
  const int32_t len = ff_serialize_and_write(fds[1], &message, &state, NULL);
  close(fds[1]);

  mu_assert("error: serialize write no error param: wrong len", len == expected_len);
  mu_assert("error: serialize write no error param: wrong output", compare_files(fds[0], expected_output));

  close(fds[0]);

  return 0;
}

static char *test_serialize_write_non_blocking(void)
{
  static const ff_message_t message = {
    .fields = {
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 7
  };
  constexpr int32_t expected_len = -1;

  int32_t fds[2];
  mu_assert("error: serialize write non-blocking: pipe failed", pipe(fds) == 0);
  mu_assert("error: serialize write non-blocking: fcntl failed", fcntl(fds[1], F_SETFL, O_NONBLOCK) != -1);
  ff_write_state_t state = {0};
  ff_error_t error = FF_OK;
  flood_fd(fds[1]);
  int32_t len = ff_serialize_and_write(fds[1], &message, &state, &error);
  close(fds[1]);

  mu_assert("error: serialize write non-blocking: wrong length", len == expected_len);
  mu_assert("error: serialize write non-blocking: wrong error", error == FF_WOULD_BLOCK);

  close(fds[0]);

  return 0;
}

static char *test_is_full_normal_message_positive(void)
{
  constexpr char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
  constexpr uint16_t len = sizeof(buffer) - 1;
  constexpr ff_error_t expected_error = FF_OK;

  ff_error_t error = FF_OK;
  bool is_full = ff_is_full_message(buffer, sizeof(buffer), len, &error);

  mu_assert("error: is_full normal message positive: wrong error", error == expected_error);
  mu_assert("error: is_full normal message positive: wrong is_full", is_full == true);

  return 0;
}

static char *test_is_full_normal_message_negative(void)
{
  constexpr char full_buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
    constexpr char buffer[sizeof(full_buffer)] =
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120";
  constexpr uint16_t len = sizeof(buffer) - 2;
  constexpr ff_error_t expected_error = FF_OK;

  ff_error_t error = FF_OK;
  bool is_full = ff_is_full_message(buffer, sizeof(buffer), len, &error);

  mu_assert("error: is_full normal message negative: wrong error", error == expected_error);
  mu_assert("error: is_full normal message negative: wrong is_full", is_full == false);

  return 0;
}

static char *test_is_full_message_too_big(void)
{
  constexpr char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120";
  constexpr uint16_t len = sizeof(buffer) - 1;
  constexpr uint16_t simulated_buffer_size = sizeof(buffer) - 1;

  constexpr ff_error_t expected_error = FF_MESSAGE_TOO_BIG;

  ff_error_t error = FF_OK;
  bool is_full = ff_is_full_message(buffer, simulated_buffer_size, len, &error);

  mu_assert("error: is_full message too big: wrong error", error == expected_error);
  mu_assert("error: is_full message too big: wrong is_full", is_full == false);

  return 0;
}

static char *test_is_full_no_error_param(void)
{
  constexpr char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
  constexpr uint16_t len = sizeof(buffer) - 1;

  bool is_full = ff_is_full_message(buffer, sizeof(buffer), len, NULL);

  mu_assert("error: is_full no error param: wrong is_full", is_full == true);

  return 0;
}

static char *test_deserialize_normal_message(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
  static const ff_message_t expected_message = {
    .fields = {
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 7
  };
  constexpr uint16_t expected_len = sizeof(buffer) - 1;
  constexpr ff_error_t expected_error = FF_OK;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize normal message: wrong length", len == expected_len);
  mu_assert("error: deserialize normal message: wrong error", error == expected_error);
  mu_assert("error: deserialize normal message: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_too_many_fields(void)
{
  static_assert(FIX_MAX_FIELDS == 64, "FIX_MAX_FIELDS must be 64 for this test to work");

  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=697\x01"
    "1=field1\x01"
    "2=field2\x01"
    "3=field3\x01"
    "4=field4\x01"
    "5=field5\x01"
    "6=field6\x01"
    "7=field7\x01"
    "8=field8\x01"
    "9=field9\x01"
    "10=field10\x01"
    "11=field11\x01"
    "12=field12\x01"
    "13=field13\x01"
    "14=field14\x01"
    "15=field15\x01"
    "16=field16\x01"
    "17=field17\x01"
    "18=field18\x01"
    "19=field19\x01"
    "20=field20\x01"
    "21=field21\x01"
    "22=field22\x01"
    "23=field23\x01"
    "24=field24\x01"
    "25=field25\x01"
    "26=field26\x01"
    "27=field27\x01"
    "28=field28\x01"
    "29=field29\x01"
    "30=field30\x01"
    "31=field31\x01"
    "32=field32\x01"
    "33=field33\x01"
    "34=field34\x01"
    "35=field35\x01"
    "36=field36\x01"
    "37=field37\x01"
    "38=field38\x01"
    "39=field39\x01"
    "40=field40\x01"
    "41=field41\x01"
    "42=field42\x01"
    "43=field43\x01"
    "44=field44\x01"
    "45=field45\x01"
    "46=field46\x01"
    "47=field47\x01"
    "48=field48\x01"
    "49=field49\x01"
    "50=field50\x01"
    "51=field51\x01"
    "52=field52\x01"
    "53=field53\x01"
    "54=field54\x01"
    "55=field55\x01"
    "56=field56\x01"
    "57=field57\x01"
    "58=field58\x01"
    "59=field59\x01"
    "60=field60\x01"
    "61=field61\x01"
    "62=field62\x01"
    "63=field63\x01"
    "64=field64\x01"
    "65=field65\x01"
    "10=014\x01";
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_TOO_MANY_FIELDS;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize too many fields: wrong length", len == expected_len);
  mu_assert("error: deserialize too many fields: wrong error", error == expected_error);

  return 0;
}
static char *test_deserialize_no_error_param(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=120\x01";
  static const ff_message_t expected_message = {
    .fields = {
      { .tag = "35", .tag_len = 2, .value = "D", .value_len = 1 },
      { .tag = "49", .tag_len = 2, .value = "BROKER", .value_len = 6 },
      { .tag = "56", .tag_len = 2, .value = "CLIENT", .value_len = 6 },
      { .tag = "34", .tag_len = 2, .value = "1", .value_len = 1 },
      { .tag = "52", .tag_len = 2, .value = "20250210-18:52:11.000", .value_len = 21 },
      { .tag = "98", .tag_len = 2, .value = "0", .value_len = 1 },
      { .tag = "108", .tag_len = 3, .value = "30", .value_len = 2 }
    },
    .n_fields = 7
  };
  constexpr uint16_t expected_len = sizeof(buffer) - 1;

  ff_message_t message;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, NULL);

  mu_assert("error: deserialize no error param: wrong length", len == expected_len);
  mu_assert("error: deserialize no error param: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_no_beginstr(void)
{
  char buffer[] = 
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=87\x01";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_INVALID_MESSAGE;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize no begin string: wrong length", len == expected_len);
  mu_assert("error: deserialize no begin string: wrong error", error == expected_error);
  mu_assert("error: deserialize no begin string: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_no_body_length(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=148";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_INVALID_MESSAGE;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize no body length: wrong length", len == expected_len);
  mu_assert("error: deserialize no body length: wrong error", error == expected_error);
  mu_assert("error: deserialize no body length: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_wrong_beginstr(void)
{
  char buffer[] = 
    "8=FIX.4.2\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=118\x01";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_INVALID_MESSAGE;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize wrong beginstr: wrong length", len == expected_len);
  mu_assert("error: deserialize wrong beginstr: wrong error", error == expected_error);
  mu_assert("error: deserialize wrong beginstr: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_wrong_body_length1(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=68\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=121\x01";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_BODY_LENGTH_MISMATCH;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize wrong bodylength 1: wrong length", len == expected_len);
  mu_assert("error: deserialize wrong bodylength 1: wrong error", error == expected_error);
  mu_assert("error: deserialize wrong bodylength 1: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_wrong_body_length2(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=66\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=30\x01"
    "10=119\x01";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_BODY_LENGTH_MISMATCH;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize wrong bodylength 2: wrong length", len == expected_len);
  mu_assert("error: deserialize wrong bodylength 2: wrong error", error == expected_error);
  mu_assert("error: deserialize wrong bodylength 2: wrong message", compare_messages(&message, &expected_message));

  return 0;
}

static char *test_deserialize_checksum_mismatch(void)
{
  char buffer[] = 
    "8=FIX.4.4\x01"
    "9=67\x01"
    "35=D\x01"
    "49=BROKER\x01"
    "56=CLIENT\x01"
    "34=1\x01"
    "52=20250210-18:52:11.000\x01"
    "98=0\x01"
    "108=31\x01"
    "10=255\x01";
  constexpr ff_message_t expected_message = {0};
  constexpr uint16_t expected_len = 0;
  constexpr ff_error_t expected_error = FF_CHECKSUM_MISMATCH;

  ff_message_t message;
  ff_error_t error = FF_OK;
  uint16_t len = ff_deserialize(buffer, sizeof(buffer), &message, &error);

  mu_assert("error: deserialize checksum mismatch: wrong length", len == expected_len);
  mu_assert("error: deserialize checksum mismatch: wrong error", error == expected_error);
  mu_assert("error: deserialize checksum mismatch: wrong message", compare_messages(&message, &expected_message));

  return 0;
}