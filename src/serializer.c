/*================================================================================

File: serializer.c                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-25 14:58:53                                                

================================================================================*/

#include "common.h"
#include "serializer.h"
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

static inline uint16_t compute_body_length(const uint16_t *tag_lens, const uint16_t *value_lens, uint16_t n_fields);
ALWAYS_INLINE static inline int32_t horizontal_sum128(__m128i vec);
static uint8_t utoa(uint16_t num, char *buffer);
ALWAYS_INLINE static inline uint16_t div100(uint16_t n);
ALWAYS_INLINE static inline uint16_t mul100(uint16_t n);

#ifdef __AVX512F__
  //TODO define constants
#endif

#ifdef __AVX2__
  //TODO define constants
#endif

#ifdef __SSE2__
  //TODO define constants
#endif

CONSTRUCTOR void ff_serializer_init(void)
{
  //TODO initialize constants
}

uint16_t ff_serialize(char *restrict buffer, const ff_message_t *restrict message)
{
  const char *const buffer_start = buffer;

  const uint16_t n_fields = message->n_fields;
  const uint16_t *tag_lens = message->tag_lens;
  const uint16_t *value_lens = message->value_lens;
  const char *const *tags = message->tags;
  const char *const *values = message->values;

  *(uint64_t *)buffer = *(uint64_t *)"8=FIX.4.";
  buffer += 8;
  *(uint32_t *)buffer = *(uint32_t *)"4\x01""9=";
  buffer += 4;

  const uint16_t body_length = compute_body_length(message->tag_lens, message->value_lens, n_fields);
  const uint8_t body_length_len = utoa(body_length, buffer);
  buffer += body_length_len;
  *buffer++ = '\x01';

  //TODO parallelize the copies, by precalculating the offsets
  for (uint16_t i = 0; LIKELY(i < n_fields); i++)
  {
    const uint16_t tag_len = tag_lens[i];
    const uint16_t value_len = value_lens[i];
    const char *tag = tags[i];
    const char *value = values[i];

    memcpy(buffer, tag, tag_len);
    buffer += tag_len;
    *buffer++ = '=';
    memcpy(buffer, value, value_len);
    buffer += value_len;
    *buffer++ = '\x01';
  }

  constexpr char checksum_table[256][4] = {
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

  const uint8_t checksum = compute_checksum(buffer_start, buffer);
  *(uint32_t *)buffer = *(uint32_t *)"10=";
  buffer += 3;

  *(uint32_t *)buffer = *(uint32_t *)checksum_table[checksum];
  buffer += 4;

  return buffer - buffer_start;
}

uint16_t ff_serialize_raw(char *restrict buffer, const ff_message_t *restrict message)
{
  const char *const buffer_start = buffer;

  const uint16_t n_fields = message->n_fields;
  const uint16_t *tag_lens = message->tag_lens;
  const uint16_t *value_lens = message->value_lens;
  const char *const *tags = message->tags;
  const char *const *values = message->values;

  for (uint16_t i = 0; LIKELY(i < n_fields); i++)
  {
    const uint16_t tag_len = tag_lens[i];
    const uint16_t value_len = value_lens[i];
    const char *tag = tags[i];
    const char *value = values[i];

    memcpy(buffer, tag, tag_len);
    buffer += tag_len;
    *buffer++ = '=';
    memcpy(buffer, value, value_len);
    buffer += value_len;
    *buffer++ = '\x01';
  }

  return buffer - buffer_start;
}

static inline uint16_t compute_body_length(const uint16_t *tag_lens, const uint16_t *value_lens, uint16_t n_fields)
{
  uint16_t total_len = (n_fields << 1);
  uint16_t i = 0;

#ifdef __AVX512F__
  while (LIKELY(i + 16 <= n_fields))
  {
    PREFETCHR(tag_lens + i + 16, 3);
    PREFETCHR(value_lens + i + 16, 3);

    const __m512i tag_lens_chunk = _mm512_load_si512((__m512i *)(tag_lens + i));
    const __m512i value_lens_chunk = _mm512_load_si512((__m512i *)(value_lens + i));
    const __m512i sum = _mm512_add_epi16(tag_lens_chunk, value_lens_chunk);

    total_len += _mm512_reduce_add_epi16(sum);
    i += 16;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(i + 8 <= n_fields))
  {
    PREFETCHR(tag_lens + i + 8, 3);
    PREFETCHR(value_lens + i + 8, 3);

    const __m128i tag_vec   = _mm_loadu_si128((const __m128i *)(tag_lens + i));
    const __m128i value_vec = _mm_loadu_si128((const __m128i *)(value_lens + i));
    const __m128i sum16     = _mm_add_epi16(tag_vec, value_vec);

    const __m256i sum32 = _mm256_cvtepu16_epi32(sum16);
    const __m128i low  = _mm256_castsi256_si128(sum32);
    const __m128i high = _mm256_extracti128_si256(sum32, 1);

    total_len += horizontal_sum128(low) + horizontal_sum128(high);
    i += 8;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(i + 8 <= n_fields))
  {
    PREFETCHR(tag_lens + i + 8, 3);
    PREFETCHR(value_lens + i + 8, 3);

    __m128i tag_vec   = _mm_loadu_si128((const __m128i *)(tag_lens + i));
    __m128i value_vec = _mm_loadu_si128((const __m128i *)(value_lens + i));
    __m128i sum16     = _mm_add_epi16(tag_vec, value_vec);

    __m128i low  = _mm_unpacklo_epi16(sum16, _mm_setzero_si128());
    __m128i high = _mm_unpackhi_epi16(sum16, _mm_setzero_si128());

    total_len += horizontal_sum128(low) + horizontal_sum128(high);
    i += 8;
  }
#endif

  while (LIKELY(i < n_fields))
  {
    total_len += tag_lens[i] + value_lens[i];
    i++;
  }

  return total_len;
}

static inline int32_t horizontal_sum128(__m128i vec)
{
#ifdef __SSE3__
  vec = _mm_hadd_epi32(vec, vec);
  vec = _mm_hadd_epi32(vec, vec);
#else
  vec = _mm_add_epi32(vec, _mm_shuffle_epi32(vec, _MM_SHUFFLE(2, 3, 0, 1)));
  vec = _mm_add_epi32(vec, _mm_shuffle_epi32(vec, _MM_SHUFFLE(1, 0, 3, 2)));
#endif
  return _mm_cvtsi128_si32(vec);
}

static uint8_t utoa(uint16_t num, char *buffer)
{
  constexpr char digit_pairs_reverse[] =
    "00" "10" "20" "30" "40" "50" "60" "70" "80" "90"
    "01" "11" "21" "31" "41" "51" "61" "71" "81" "91"
    "02" "12" "22" "32" "42" "52" "62" "72" "82" "92"
    "03" "13" "23" "33" "43" "53" "63" "73" "83" "93"
    "04" "14" "24" "34" "44" "54" "64" "74" "84" "94"
    "05" "15" "25" "35" "45" "55" "65" "75" "85" "95"
    "06" "16" "26" "36" "46" "56" "66" "76" "86" "96"
    "07" "17" "27" "37" "47" "57" "67" "77" "87" "97"
    "08" "18" "28" "38" "48" "58" "68" "78" "88" "98"
    "09" "19" "29" "39" "49" "59" "69" "79" "89" "99";

  char tmp[sizeof(__m128i)] ALIGNED(16) = {0};
  uint8_t len = 0;

  if (UNLIKELY(num < 10))
  {
    *buffer = '0' + num;
    return 1;
  }

  while (LIKELY(num >= 100))
  {
    const uint16_t q = div100(num);
    const uint8_t r = num - mul100(q);
    
    *(uint16_t *)(tmp + len) = *(uint16_t *)(digit_pairs_reverse + (r << 1));
    num = q;
  }

  const bool is_single_digit = num < 10;
  len += is_single_digit;
  *(uint16_t *)(tmp + len - is_single_digit) = *(uint16_t *)(digit_pairs_reverse + (num << 1));
  len += 2 - is_single_digit;

  uint8_t i = len;
  while (LIKELY(i--))
    *buffer++ = tmp[i];

  return len;
}

static inline uint16_t div100(uint16_t n)
{ 
  return ((uint32_t)n * 5243U) >> 19;
}

static inline uint16_t mul100(uint16_t n)
{
  return (n << 6) + (n << 5) + (n << 2);
}
