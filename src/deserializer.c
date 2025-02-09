/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-05 20:59:03                                                

================================================================================*/

#include "common.h"
#include "deserializer.h"
#include <string.h>

static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size);
static inline bool check_zero_equal_soh(const char *buffer);
static bool tokenize(char *buffer, const char *const end, fix_message_t *const restrict message);
static uint32_t atoui(const char *str, const char **endptr);
static inline uint32_t mul10(uint32_t n);

//TODO find a way to make them const, forcing prevention of thread safety issues, constexpr??
#ifdef __AVX512F__
  static __m512i _512_vec_ones;
  static __m512i _512_vec_zeros;
  static __m512i _512_vec_equals; 
  static __m512i _512_vec_soh;
#endif

#ifdef __AVX2__
  static __m256i _256_vec_ones;
  static __m256i _256_vec_zeros;
  static __m256i _256_vec_equals;
  static __m256i _256_vec_soh;
#endif

#ifdef __SSE2__
  static __m128i _128_vec_ones;
  static __m128i _128_vec_zeros;
  static __m128i _128_vec_equals;
  static __m128i _128_vec_soh;
#endif

CONSTRUCTOR void ff_deserializer_init(void)
{
#ifdef __AVX512F__
  _512_vec_ones   = _mm512_set1_epi8('1');
  _512_vec_zeros  = _mm512_set1_epi8('0');
  _512_vec_equals = _mm512_set1_epi8('=');
  _512_vec_soh    = _mm512_set1_epi8('\x01');
#endif

#ifdef __AVX2__
  _256_vec_ones   = _mm256_set1_epi8('1');
  _256_vec_zeros  = _mm256_set1_epi8('0');
  _256_vec_equals = _mm256_set1_epi8('=');
  _256_vec_soh    = _mm256_set1_epi8('\x01');
#endif

#ifdef __SSE2__
  _128_vec_ones   = _mm_set1_epi8('1');
  _128_vec_zeros  = _mm_set1_epi8('0');
  _128_vec_equals = _mm_set1_epi8('=');
  _128_vec_soh    = _mm_set1_epi8('\x01');
#endif
}

uint16_t ff_deserialize(char *buffer, const uint16_t buffer_size, fix_message_t *restrict message)
{
  const char *const buffer_start = buffer;

  bool valid = buffer_size >= STR_LEN("8=FIX.4.4\x019=0\x01""10=000\x01");
  valid &= memcmp8(buffer, "8=FIX.4.4");
  buffer += 8;
  valid &= memcmp4(buffer, "4\x01""9=");
  buffer += 4;

  if (UNLIKELY(!valid))
    return 0;  

  const uint16_t body_length = (uint16_t)atoui(buffer, (const char **)&buffer);
  if (UNLIKELY(*buffer++ != '\x01'))
    return 0;

  const uint16_t remaining = buffer_size - (buffer - buffer_start);
  const char *const body_start = buffer;
  const char *const checksum_start = get_checksum_start(buffer, remaining);

  valid = (checksum_start != NULL) & (body_length == checksum_start - body_start);
  if (UNLIKELY(!valid))
    return 0;

  buffer = (char *)checksum_start + STR_LEN("10=");

  const uint8_t expected_checksum = compute_checksum(buffer_start, checksum_start);
  const uint8_t provided_checksum = (uint8_t)atoui(buffer, (const char **)&buffer);
  buffer += STR_LEN("\x01");

  valid = (expected_checksum == provided_checksum);
  valid &= tokenize((char *)body_start, checksum_start, message);
  return (buffer - buffer_start) * valid;
}

bool ff_is_complete(const char *buffer, const uint16_t len)
{
  return !!get_checksum_start(buffer, len);
}

//TODO optimize, bottleneck, 93% of the time spent here
static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size)
{
  int32_t remaining = buffer_size - STR_LEN("10=000\x01") + 1;
  if (UNLIKELY(remaining <= 0))
    return NULL;

  uint8_t misaligned_bytes = align_forward(buffer);
  misaligned_bytes -= (misaligned_bytes > remaining) * (misaligned_bytes - remaining);

  while (UNLIKELY(misaligned_bytes--))
  {
    bool found = buffer[0] == '1';
    found &= check_zero_equal_soh(buffer + 1);

    if (UNLIKELY(found))
      return buffer;

    buffer++;
    remaining--;
  }

#ifdef __AVX512F__
  while (LIKELY(remaining >= 64))
  {
    const __m512i chunk = _mm512_load_si512((__m512i*)buffer);
    const __m512i cmp = _mm512_cmpeq_epi8(chunk, _512_vec_ones);
    __mmask64 mask = __mm512_movemask_epi8(cmp);

    while (UNLIKELY(mask))
    {
      const int32_t offset = __builtin_ctzll(mask);
      const char *const candidate = buffer + offset;

      if (UNLIKELY(check_zero_equal_soh(candidate + 1)))
        return candidate;

      mask &= mask - 1;
    }

    buffer += 64;
    remaining -= 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(remaining >= 32))
  {
    const __m256i chunk = _mm256_load_si256((__m256i *)buffer);
    const __m256i cmp = _mm256_cmpeq_epi8(chunk, _256_vec_ones);
    int32_t mask = _mm256_movemask_epi8(cmp);

    while (UNLIKELY(mask))
    {
      const int32_t offset = __builtin_ctz(mask); //TODO stdc_trailing_zeros(mask);
      const char *const candidate = buffer + offset;

      if (UNLIKELY(check_zero_equal_soh(candidate + 1)))
        return candidate;

      mask &= mask - 1;
    }

    buffer += 32;
    remaining -= 32;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(remaining >= 16))
  {
    const __m128i chunk = _mm_load_si128((__m128i *)buffer);
    const __m128i cmp = _mm_cmpeq_epi8(chunk, _128_vec_ones);
    int32_t mask = _mm_movemask_epi8(cmp);

    while (UNLIKELY(mask))
    {
      const int32_t offset = __builtin_ctz(mask); //TODO stdc_trailing_zeros(mask);
      const char *const candidate = buffer + offset;

      if (UNLIKELY(check_zero_equal_soh(candidate + 1)))
        return candidate;

      mask &= mask - 1;
    }

    buffer += 16;
    remaining -= 16;
  }
#endif

  while (LIKELY(remaining >= 8))
  {
    const uint64_t chunk = *(const uint64_t *)buffer;
    const uint64_t cmp = chunk ^ 0x3131313131313131ULL;
    uint64_t mask = (cmp - 0x0101010101010101ULL) & ~cmp & 0x8080808080808080ULL;

    while (UNLIKELY(mask))
    {
      const int32_t byte_offset = __builtin_ctzll(mask) >> 3; //TODO stdc_trailing_zeros(mask) >> 3;
      const char *const candidate = buffer + byte_offset;

      if (UNLIKELY(check_zero_equal_soh(candidate + 1)))
        return candidate;

      mask &= mask - 1;
    }

    buffer += 8;
    remaining -= 8;
  }

  while (LIKELY(remaining--))
  {
    bool found = (buffer[0] == '1') & check_zero_equal_soh(buffer + 1);

    if (UNLIKELY(found))
      return buffer;

    buffer++;
  }

  return NULL;
}

static inline bool check_zero_equal_soh(const char *buffer)
{
  return memcmp2(buffer, "0=") & (buffer[5] == '\x01');
}

static bool tokenize(char *buffer, const char *const end, fix_message_t *const restrict message)
{
  fix_field_t *fields = message->fields;
  const uint16_t max_fields = message->field_count;

  uint16_t field_count = 0;
  while (LIKELY(buffer < end))
  {
    char *delim = rawmemchr(buffer, '=');
    const uint16_t tag_len = delim - buffer;
    *delim++ = '\0';

    char *soh = rawmemchr(delim, '\x01');
    const uint16_t value_len = soh - delim;
    *soh++ = '\0';

    if (UNLIKELY(field_count++ >= max_fields))
      return false;
    
    *fields++ = (fix_field_t){
      .tag = buffer,
      .value = delim,
      .tag_len = tag_len,
      .value_len = value_len
    };

    buffer = soh;
  }
  message->field_count = field_count;

  return true;
}

static uint32_t atoui(const char *str, const char **endptr)
{
  uint32_t result = 0;

  if (UNLIKELY(!str))
    return 0;

  str += strspn(str, " \t\n\r\v\f");

  while (LIKELY((uint8_t)(*str - '0') < 10))
  {
    result = mul10(result) + (*str - '0');
    str++;
  }

  if (endptr) //TODO branchless
    *endptr = str;

  return result;
}

static inline uint32_t mul10(uint32_t n)
{
  return (n << 3) + (n << 1);
}


