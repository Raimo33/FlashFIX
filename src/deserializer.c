/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-13 13:38:07                                                

================================================================================*/

#include "common.h"
#include <flashfix/deserializer.h>
#include <string.h>

HOT static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size);
ALWAYS_INLINE static inline bool check_zero_equal_soh(const char *candidate);
static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error);
static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error);
static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error);
static inline uint16_t validate_checksum(char *buffer, const uint16_t buffer_size, const uint16_t body_length, char **body_start, ff_error_t *restrict error);
static void tokenize_message(char *restrict buffer, const uint16_t buffer_size);
static void fill_message_fields(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);
static uint32_t ff_atoui(const char *str, const char **endptr);

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

bool ff_is_full(const char *buffer, UNUSED const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error)
{
  constexpr uint8_t checksum_len = STR_LEN(FIX_CHECKSUM "=000\x01");
  constexpr uint8_t begin_string_len = STR_LEN(FIX_BEGINSTRING "=" FIX_VERSION "\x01");
  constexpr uint8_t body_length_len = STR_LEN(FIX_BODYLENGTH "=") + 2;
  constexpr uint8_t total_minimum_len = checksum_len + begin_string_len + body_length_len;

  if (message_len < total_minimum_len)
    return false;

  const bool has_checksum = !!get_checksum_start(buffer, message_len);
  (void)(error && (*error = FF_BUFFER_TOO_SMALL * (!has_checksum && message_len >= buffer_size)));

  return has_checksum;
}

uint16_t ff_deserialize(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error)
{
  ff_error_t local_error = FF_OK;
  
  const char *buffer_start = buffer;
  char *body_start;
  uint16_t body_length;

  buffer += check_begin_string(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += check_body_length_tag(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += deserialize_body_length(buffer, &body_length, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += validate_checksum(buffer, buffer_size, body_length, &body_start, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  tokenize_message(body_start, body_length);

  fill_message_fields(body_start, body_length, message, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  return buffer - buffer_start;

error:
  (void)(error && (*error = local_error));
  return 0;
}

static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size)
{
  const char *last = buffer + buffer_size - STR_LEN(FIX_CHECKSUM "=000\x01");
  constexpr uint8_t alignment = 64;

  uint8_t displacement = (const uintptr_t)buffer % alignment;
  while (UNLIKELY(displacement-- && buffer < last))
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
      const char *candidate = buffer + offset;

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
      const int32_t offset = __builtin_ctz(mask);
      const char *candidate = buffer + offset;

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
      const int32_t offset = __builtin_ctz(mask);
      const char *candidate = buffer + offset;

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
      const int32_t byte_offset = __builtin_ctzll(mask) >> 3;
      const char *candidate = buffer + byte_offset;

      if (UNLIKELY(check_zero_equal_soh(candidate + 1)))
        return candidate;

      mask &= mask - 1;
    }

    buffer += 8;
  }


  while (LIKELY(buffer < last))
  {
    if (buffer[0] == '1' && check_zero_equal_soh(buffer + 1))
      return buffer;
    buffer++;
  }

  return NULL;
}

static inline bool check_zero_equal_soh(const char *buffer)
{
  return *(const uint16_t *)(buffer) == *(const uint16_t *)"0=" && buffer[6] == '\x01';
}

static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error)
{
  constexpr char begin_string_tag[] = FIX_BEGINSTRING "=" FIX_VERSION "\x01";

  *error = FF_INVALID_MESSAGE * (*(uint16_t *)buffer != *(const uint16_t *)begin_string_tag);

  return STR_LEN(begin_string_tag);
}

static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error)
{
  constexpr char body_length_tag[] = FIX_BODYLENGTH "=";

  *error = FF_INVALID_MESSAGE * memcmp(buffer, body_length_tag, STR_LEN(body_length_tag));

  return STR_LEN(body_length_tag);
}

static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error)
{
  const char *buffer_start = buffer;

  *body_length = (uint16_t)ff_atoui(buffer, (const char **)&buffer);
  *error = FF_INVALID_MESSAGE * (*buffer++ != '\x01');

  return buffer - buffer_start;
}

static inline uint16_t validate_checksum(char *buffer, const uint16_t buffer_size, const uint16_t body_length, char **body_start, ff_error_t *restrict error)
{
  *body_start = buffer;

  const char *body_end = get_checksum_start(buffer, buffer_size);
  const char *checksum_start = body_end + STR_LEN(FIX_CHECKSUM "=");
  if (UNLIKELY(body_end - *body_start != body_length))
    return (*error = FF_BODY_LENGTH_MISMATCH, 0);

  const uint8_t expected_checksum = compute_checksum(*body_start, body_length);
  const uint8_t provided_checksum = (uint8_t)ff_atoui(checksum_start, (const char **)&buffer);
  if (UNLIKELY(expected_checksum != provided_checksum))
    return (*error = FF_CHECKSUM_MISMATCH, 0);
  buffer += STR_LEN("\x01");

  return buffer - *body_start;  
}

static void tokenize_message(char *restrict buffer, const uint16_t buffer_size)
{
  const char *end = buffer + buffer_size;
  constexpr uint8_t alignment = 64;

  uint8_t displacement = (const uintptr_t)buffer % alignment;
  while (UNLIKELY(displacement-- && buffer < end))
  {
    const char c = *buffer;
    *buffer = (c != '\x01' && c != '=') * c;
    buffer++;
  }

#ifdef __AVX512F__
  while (LIKELY(buffer + 64 <= end))
  {
    PREFETCHR(buffer + 128, 3);
  
    const __m512i chunk = _mm512_load_si512((__m512i*)buffer);

    const __m512i cmp_soh     = _mm512_cmpeq_epi8(chunk, _512_vec_soh);
    const __m512i cmp_equals  = _mm512_cmpeq_epi8(chunk, _512_vec_equals);

    const __m512i cmp  = _mm512_or_si512(cmp_soh, cmp_equals);
    const __m512i result = _mm512_andnot_si512(cmp, chunk);
    _mm512_storeu_si512((__m512i*)buffer, result);
  
    buffer += 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(buffer + 32 <= end))
  {
    PREFETCHR(buffer + 64, 3);
  
    const __m256i chunk = _mm256_load_si256((__m256i *)buffer);

    const __m256i cmp_soh     = _mm256_cmpeq_epi8(chunk, _256_vec_soh);
    const __m256i cmp_equals  = _mm256_cmpeq_epi8(chunk, _256_vec_equals);

    const __m256i cmp = _mm256_or_si256(cmp_soh, cmp_equals);
    const __m256i result = _mm256_andnot_si256(cmp, chunk);
    _mm256_storeu_si256((__m256i *)buffer, result);
  
    buffer += 32;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(buffer + 16 <= end))
  {
    PREFETCHR(buffer + 32, 3);
  
    const __m128i chunk = _mm_load_si128((__m128i *)buffer);

    const __m128i cmp_soh     = _mm_cmpeq_epi8(chunk, _128_vec_soh);
    const __m128i cmp_equals  = _mm_cmpeq_epi8(chunk, _128_vec_equals);

    const __m128i cmp = _mm_or_si128(cmp_soh, cmp_equals);
    const __m128i result = _mm_andnot_si128(cmp, chunk);
    _mm_storeu_si128((__m128i *)buffer, result);
  
    buffer += 16;
  }
#endif

  constexpr uint64_t mask_soh = 0x0101010101010101ULL;
  constexpr uint64_t mask_equals = 0x3D3D3D3D3D3D3D3DULL;
  constexpr uint64_t ones = 0x0101010101010101ULL;
  constexpr uint64_t high_mask = 0x8080808080808080ULL;
  
  while (LIKELY(buffer + 8 <= end))
  {
    PREFETCHR(buffer + 16, 3);
    const uint64_t chunk = *(const uint64_t *)buffer;

    const uint64_t tmp_soh = ((chunk ^ mask_soh) - ones) & ~(chunk ^ mask_soh) & high_mask;
    const uint64_t tmp_equals = ((chunk ^ mask_equals) - ones) & ~(chunk ^ mask_equals) & high_mask;

    const uint64_t matching_tokens = tmp_soh | tmp_equals;
    const uint64_t byte_mask = ((matching_tokens >> 7) & ones) * 0xFF;

    const uint64_t new_chunk = chunk & ~byte_mask;
    *((uint64_t *)buffer) = new_chunk;

    buffer += 8;
  }

  while (LIKELY(buffer < end))
  {
    const char c = *buffer;
    *buffer = (c != '\x01' && c != '=') * c;
    buffer++;
  }
}

static void fill_message_fields(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error)
{
  //TODO raise buffer too small error if message fields > MAX_FIELDS
  //TODO handle adiacent | and =
  (void)buffer;
  (void)buffer_size;
  (void)message;
  (void)error;
}

static uint32_t ff_atoui(const char *str, const char **endptr)
{
  uint32_t result = 0;
  
  while (UNLIKELY(*str == ' '))
    str++;

  while (LIKELY(*str >= '0' && *str <= '9'))
  {
    result = (result << 3) + (result << 1) + (uint32_t)(*str - '0');
    str++;
  }

  (void)(endptr && (*endptr = str));
  return result;
}


