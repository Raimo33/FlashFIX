/*================================================================================

File: serializer.c                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-12 13:35:28                                                

================================================================================*/

#include "common.h"
#include <flashfix/serializer.h>
#include <string.h>

static uint8_t ultoa(uint64_t num, char *buffer);
static bool message_fits_in_buffer(const ff_message_t *restrict message, const uint16_t buffer_size);

#ifdef __AVX512F__
  //TODO declare the AVX512F vars
#endif

#ifdef __AVX2__
  //TODO declare the AVX2 vars
#endif

#ifdef __SSE2__
  //TODO declare the SSE2 vars
#endif

CONSTRUCTOR void ff_serializer_init(void)
{
#ifdef __AVX512F__
  //TODO init the AVX512F vars
#endif

#ifdef __AVX2__
  //TODO init the AVX2 vars
#endif

#ifdef __SSE2__
  //TODO init the SSE2 vars
#endif
}

uint16_t ff_serialize(char *buffer, const uint16_t buffer_size, const ff_message_t *message, ff_error_t *restrict error)
{
  ff_error_t local_error = FF_OK;

  const char *buffer_start = buffer;

  if (UNLIKELY(!message_fits_in_buffer(message, buffer_size)))
  {
    local_error = FF_BUFFER_TOO_SMALL;
    goto error;
  }

  const ff_field_t *fields = message->fields;
  const uint16_t n_fields = message->n_fields;

  for (uint16_t i = 0; LIKELY(i < n_fields); i++)
  {
    memcpy(buffer, fields[i].tag, fields[i].tag_len);
    buffer += fields[i].tag_len;
    *buffer++ = '=';

    memcpy(buffer, fields[i].value, fields[i].value_len);
    buffer += fields[i].value_len;
    *buffer++ = '\x01';
  }

  return buffer - buffer_start;

error:
  (void)(error && (*error = local_error));
  return 0;
}

uint16_t ff_finalize(char *buffer, const uint16_t buffer_size, const uint16_t len, ff_error_t *restrict error)
{
  ff_error_t local_error = FF_OK;

  const char *buffer_start = buffer;

  static const ff_field_t begin_string = {
    .tag = FIX_BEGINSTRING,
    .tag_len = STR_LEN(FIX_BEGINSTRING),
    .value = FIX_VERSION,
    .value_len = STR_LEN(FIX_VERSION)
  };

  char body_length_str[16];
  const ff_field_t body_length = {
    .tag = FIX_BODYLENGTH,
    .tag_len = STR_LEN(FIX_BODYLENGTH),
    .value = body_length_str,
    .value_len = ultoa(len, body_length_str)
  };

  const uint8_t added_len = begin_string.tag_len + 1 + begin_string.value_len + 1 + body_length.tag_len + 1 + body_length.value_len + 1;
  const uint8_t checksum_len = STR_LEN(FIX_CHECKSUM) + 1 + 3 + 1;
  if (UNLIKELY(len + added_len + checksum_len > buffer_size))
  {
    local_error = FF_BUFFER_TOO_SMALL;
    goto error;
  }

  memmove(buffer + added_len, buffer, len);

  const ff_message_t message = {
    .fields = {
      begin_string,
      body_length
    },
    .n_fields = 2
  };
  ff_serialize(buffer, added_len, &message, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += added_len + len;

  constexpr char checksum_table[256][sizeof(uint32_t)] = {
    {"000\x01"}, {"001\x01"}, {"002\x01"}, {"003\x01"}, {"004\x01"}, {"005\x01"}, {"006\x01"}, {"007\x01"}, {"008\x01"}, {"009\x01"},
    {"010\x01"}, {"011\x01"}, {"012\x01"}, {"013\x01"}, {"014\x01"}, {"015\x01"}, {"016\x01"}, {"017\x01"}, {"018\x01"}, {"019\x01"},
    {"020\x01"}, {"021\x01"}, {"022\x01"}, {"023\x01"}, {"024\x01"}, {"025\x01"}, {"026\x01"}, {"027\x01"}, {"028\x01"}, {"029\x01"},
    {"030\x01"}, {"031\x01"}, {"032\x01"}, {"033\x01"}, {"034\x01"}, {"035\x01"}, {"036\x01"}, {"037\x01"}, {"038\x01"}, {"039\x01"},
    {"040\x01"}, {"041\x01"}, {"042\x01"}, {"043\x01"}, {"044\x01"}, {"045\x01"}, {"046\x01"}, {"047\x01"}, {"048\x01"}, {"049\x01"},
    {"050\x01"}, {"051\x01"}, {"052\x01"}, {"053\x01"}, {"054\x01"}, {"055\x01"}, {"056\x01"}, {"057\x01"}, {"058\x01"}, {"059\x01"},
    {"060\x01"}, {"061\x01"}, {"062\x01"}, {"063\x01"}, {"064\x01"}, {"065\x01"}, {"066\x01"}, {"067\x01"}, {"068\x01"}, {"069\x01"},
    {"070\x01"}, {"071\x01"}, {"072\x01"}, {"073\x01"}, {"074\x01"}, {"075\x01"}, {"076\x01"}, {"077\x01"}, {"078\x01"}, {"079\x01"},
    {"080\x01"}, {"081\x01"}, {"082\x01"}, {"083\x01"}, {"084\x01"}, {"085\x01"}, {"086\x01"}, {"087\x01"}, {"088\x01"}, {"089\x01"},
    {"090\x01"}, {"091\x01"}, {"092\x01"}, {"093\x01"}, {"094\x01"}, {"095\x01"}, {"096\x01"}, {"097\x01"}, {"098\x01"}, {"099\x01"},
    {"100\x01"}, {"101\x01"}, {"102\x01"}, {"103\x01"}, {"104\x01"}, {"105\x01"}, {"106\x01"}, {"107\x01"}, {"108\x01"}, {"109\x01"},
    {"110\x01"}, {"111\x01"}, {"112\x01"}, {"113\x01"}, {"114\x01"}, {"115\x01"}, {"116\x01"}, {"117\x01"}, {"118\x01"}, {"119\x01"},
    {"120\x01"}, {"121\x01"}, {"122\x01"}, {"123\x01"}, {"124\x01"}, {"125\x01"}, {"126\x01"}, {"127\x01"}, {"128\x01"}, {"129\x01"},
    {"130\x01"}, {"131\x01"}, {"132\x01"}, {"133\x01"}, {"134\x01"}, {"135\x01"}, {"136\x01"}, {"137\x01"}, {"138\x01"}, {"139\x01"},
    {"140\x01"}, {"141\x01"}, {"142\x01"}, {"143\x01"}, {"144\x01"}, {"145\x01"}, {"146\x01"}, {"147\x01"}, {"148\x01"}, {"149\x01"},
    {"150\x01"}, {"151\x01"}, {"152\x01"}, {"153\x01"}, {"154\x01"}, {"155\x01"}, {"156\x01"}, {"157\x01"}, {"158\x01"}, {"159\x01"},
    {"160\x01"}, {"161\x01"}, {"162\x01"}, {"163\x01"}, {"164\x01"}, {"165\x01"}, {"166\x01"}, {"167\x01"}, {"168\x01"}, {"169\x01"},
    {"170\x01"}, {"171\x01"}, {"172\x01"}, {"173\x01"}, {"174\x01"}, {"175\x01"}, {"176\x01"}, {"177\x01"}, {"178\x01"}, {"179\x01"},
    {"180\x01"}, {"181\x01"}, {"182\x01"}, {"183\x01"}, {"184\x01"}, {"185\x01"}, {"186\x01"}, {"187\x01"}, {"188\x01"}, {"189\x01"},
    {"190\x01"}, {"191\x01"}, {"192\x01"}, {"193\x01"}, {"194\x01"}, {"195\x01"}, {"196\x01"}, {"197\x01"}, {"198\x01"}, {"199\x01"},
    {"200\x01"}, {"201\x01"}, {"202\x01"}, {"203\x01"}, {"204\x01"}, {"205\x01"}, {"206\x01"}, {"207\x01"}, {"208\x01"}, {"209\x01"},
    {"210\x01"}, {"211\x01"}, {"212\x01"}, {"213\x01"}, {"214\x01"}, {"215\x01"}, {"216\x01"}, {"217\x01"}, {"218\x01"}, {"219\x01"},
    {"220\x01"}, {"221\x01"}, {"222\x01"}, {"223\x01"}, {"224\x01"}, {"225\x01"}, {"226\x01"}, {"227\x01"}, {"228\x01"}, {"229\x01"},
    {"230\x01"}, {"231\x01"}, {"232\x01"}, {"233\x01"}, {"234\x01"}, {"235\x01"}, {"236\x01"}, {"237\x01"}, {"238\x01"}, {"239\x01"},
    {"240\x01"}, {"241\x01"}, {"242\x01"}, {"243\x01"}, {"244\x01"}, {"245\x01"}, {"246\x01"}, {"247\x01"}, {"248\x01"}, {"249\x01"},
    {"250\x01"}, {"251\x01"}, {"252\x01"}, {"253\x01"}, {"254\x01"}, {"255\x01"}
  };

  const uint8_t checksum = compute_checksum(buffer_start, buffer - buffer_start);

  memcpy(buffer, FIX_CHECKSUM, STR_LEN(FIX_CHECKSUM));
  buffer += STR_LEN(FIX_CHECKSUM);
  *buffer++ = '=';
  *(uint32_t *)buffer = *(const uint32_t *)checksum_table[checksum];
  buffer += 4;

  return buffer - buffer_start;

error:
  (void)(error && (*error = local_error));
  return 0;
}

//TODO simd, adesso sono accanto le len dei tag e dei valori
static bool message_fits_in_buffer(const ff_message_t *restrict message, const uint16_t buffer_size)
{
  uint16_t total_len = (message->n_fields << 1);

  for (uint8_t i = 0; LIKELY(i < message->n_fields); i++)
    total_len += message->fields[i].tag_len + message->fields[i].value_len;

  return total_len <= buffer_size;
}

//TODO simd?? swar??
static uint8_t ultoa(uint64_t num, char *buffer)
{
  if (num == 0)
  {
    buffer[0] = '0';
    return 1;
  }

  const uint8_t digits = 1 +
    (num >= 10UL) +
    (num >= 100UL) +
    (num >= 1000UL) +
    (num >= 10000UL) +
    (num >= 100000UL) +
    (num >= 1000000UL) +
    (num >= 10000000UL) +
    (num >= 100000000UL) +
    (num >= 1000000000UL) +
    (num >= 10000000000UL) +
    (num >= 100000000000UL) +
    (num >= 1000000000000UL) +
    (num >= 10000000000000UL) +
    (num >= 100000000000000UL) +
    (num >= 1000000000000000UL) +
    (num >= 10000000000000000UL) +
    (num >= 100000000000000000UL) +
    (num >= 1000000000000000000UL);

  constexpr uint64_t power10[] = {
    1UL,
    10UL,
    100UL,
    1000UL,
    10000UL,
    100000UL,
    1000000UL,
    10000000UL,
    100000000UL,
    1000000000UL,
    10000000000UL,
    100000000000UL,
    1000000000000UL,
    10000000000000UL,
    100000000000000UL,
    1000000000000000UL,
    10000000000000000UL,
    100000000000000000UL,
    1000000000000000000UL,
  };

  uint64_t power = power10[digits - 1];
  char *p = buffer;
  uint64_t quotient;
  for (uint8_t i = 0; i < digits; i++)
  {
    quotient = num / power;
    *p++ = (char)('0' + quotient);
    num -= quotient * power;
    power /= 10;
  }

  return digits;
}