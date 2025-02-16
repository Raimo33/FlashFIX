/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-16 22:54:38                                                

================================================================================*/

#include "common.h"
#include <flashfix/deserializer.h>
#include <string.h>

HOT static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size);
ALWAYS_INLINE static inline bool check_zero_equal_soh(const char *candidate);
static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error);
static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error);
static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error);
static inline uint16_t validate_message(const char *buffer_start, const uint16_t buffer_size, const char *body_start, const uint16_t body_length, ff_error_t *restrict error);
static void tokenize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);
static uint32_t ff_atoui(const char *str, const char **endptr);
ALWAYS_INLINE static inline uint32_t mul10(uint32_t n);

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

bool ff_is_full_message(const char *buffer, UNUSED const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error)
{
  constexpr uint8_t begin_string_len = STR_LEN("8=FIX.4.4\x01");
  constexpr uint8_t min_body_length_len = STR_LEN("9=0\x01");
  constexpr uint8_t checksum_len = STR_LEN("10=000\x01");
  constexpr uint8_t total_minimum_len = checksum_len + begin_string_len + min_body_length_len;

  if (message_len < total_minimum_len)
    return false;

  const bool has_checksum = !!get_checksum_start(buffer, message_len);
  (void)(error && (*error = FF_MESSAGE_TOO_BIG * (!has_checksum && message_len >= buffer_size)));

  return has_checksum;
}

//TODO reduce branching
uint16_t ff_deserialize(char *buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error)
{
  ff_error_t local_error = FF_OK;
  bzero(message, sizeof(ff_message_t));
  const char *const buffer_start = buffer;
  
  buffer += check_begin_string(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;
  
  buffer += check_body_length_tag(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;
  
  uint16_t body_length;
  buffer += deserialize_body_length(buffer, &body_length, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  char *const body_start = buffer;

  buffer += validate_message(buffer_start, buffer_size, body_start, body_length, &local_error); 
  if (UNLIKELY(local_error != FF_OK)) goto error;

  tokenize(body_start, body_length, message, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  return buffer - buffer_start;

error:
  (void)(error && (*error = local_error));
  return 0;
}

//TODO optimize, bottleneck
static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size)
{
  const char *const last = buffer + buffer_size - STR_LEN("10=000\x01");
  const char *const aligned_buffer = align_forward(buffer, 64);

  while (UNLIKELY(buffer < aligned_buffer))
  {
    if (buffer[0] == '1' && check_zero_equal_soh(buffer + 1))
      return buffer;

    buffer++;
  }

#ifdef __AVX512F__
  while (LIKELY(buffer + 64 <= last))
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
  }

#endif

#ifdef __AVX2__
  while (LIKELY(buffer + 32 <= last))
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
  }
#endif

#ifdef __SSE2__
  while (LIKELY(buffer + 16 <= last))
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
  }
#endif

  constexpr uint64_t repeated = 0x3131313131313131ULL; 
  while (LIKELY(buffer + 8 <= last))
  {
    const uint64_t chunk = *(const uint64_t *)buffer;
    const uint64_t cmp = chunk ^ repeated;
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
  }

  while (LIKELY(buffer <= last))
  {
    if (buffer[0] == '1' && check_zero_equal_soh(buffer + 1))
      return buffer;
    buffer++;
  }

  return NULL;
}

static inline bool check_zero_equal_soh(const char *buffer)
{
  return *(uint16_t *)(buffer) == *(uint16_t *)"0=" && buffer[5] == '\x01';
}

static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error)
{
  constexpr char begin_string_tag[] = "8=FIX.4.4\x01";

  *error = FF_INVALID_MESSAGE * !!memcmp(buffer, begin_string_tag, STR_LEN(begin_string_tag));

  return STR_LEN(begin_string_tag);
}

static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error)
{
  constexpr char body_length_tag[] = "9=";

  *error = FF_INVALID_MESSAGE * (*(uint16_t *)buffer != *(uint16_t *)body_length_tag);

  return STR_LEN(body_length_tag);
}

static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error)
{
  const char *const buffer_start = buffer;

  *body_length = (uint16_t)ff_atoui(buffer, (const char **)&buffer);
  *error = FF_INVALID_MESSAGE * (*buffer++ != '\x01');

  return buffer - buffer_start;
}

//TODO split in two: validate body length, validate checksum
static inline uint16_t validate_message(const char *buffer_start, const uint16_t buffer_size, const char *body_start, const uint16_t body_length, ff_error_t *restrict error)
{
  const uint16_t remaining = buffer_size - (body_start - buffer_start);

  const char *buffer = get_checksum_start(body_start, remaining);
  if (UNLIKELY(buffer - body_start != body_length))
    return (*error = FF_BODY_LENGTH_MISMATCH, 0);
  buffer += STR_LEN("10=");

  const uint16_t header_length = body_start - buffer_start;

  const uint8_t expected_checksum = compute_checksum(buffer_start, body_length + header_length);
  const uint8_t provided_checksum = (uint8_t)ff_atoui(buffer, (const char **)&buffer);

  if (UNLIKELY(expected_checksum != provided_checksum))
    return (*error = FF_CHECKSUM_MISMATCH, 0);
  buffer += STR_LEN("\x01");

  return buffer - body_start;  
}

static void tokenize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error)
{
  const char *const end = buffer + buffer_size;

  while (LIKELY(buffer < end))
  {
    char *delim = memchr(buffer, '=', end - buffer);
    if (UNLIKELY(!delim))
    {
      *error = FF_INVALID_MESSAGE;
      return;
    }
    const uint16_t tag_len = delim - buffer;
    *delim++ = '\0';

    char *soh = memchr(delim, '\x01', end - delim);
    const uint16_t value_len = soh - delim;
    *soh++ = '\0';

    if (UNLIKELY(message->n_fields == FIX_MAX_FIELDS))
    {
      *error = FF_TOO_MANY_FIELDS;
      return;
    }

    message->fields[message->n_fields++] = (ff_field_t) {
      .tag = buffer,
      .tag_len = tag_len,
      .value = delim,
      .value_len = value_len
    };

    buffer = soh;
  }
}

static uint32_t ff_atoui(const char *str, const char **endptr)
{
  uint32_t result = 0;
  
  while (UNLIKELY(*str == ' '))
    str++;

  while (LIKELY(*str >= '0' && *str <= '9'))
  {
    result = mul10(result) + (*str - '0');
    str++;
  }

  (void)(endptr && (*endptr = str));
  return result;
}

static inline uint32_t mul10(uint32_t n)
{
  return (n << 3) + (n << 1);
}


