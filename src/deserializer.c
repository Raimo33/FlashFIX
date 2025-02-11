/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-11 12:37:26                                                

================================================================================*/

#include "internal/common.h"
#include <flashfix/deserializer.h>
#include <string.h>

static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size);
HOT ALWAYS_INLINE static inline bool is_checksum_sequence(const char *buffer);
static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error);
static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error);
static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error);
static inline uint16_t validate_checksum(char *buffer, const uint16_t buffer_size, const uint16_t body_length, char **body_start, ff_error_t *restrict error);
static void tokenize_message(char *restrict buffer, const uint16_t buffer_size);
static void fill_message_fields(char *restrict buffer, const uint16_t buffer_size, ff_message_t *restrict message, ff_error_t *restrict error);

bool ff_is_full(const char *buffer, UNUSED const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error)
{
  static const uint8_t checksum_len = STR_LEN(FIX_CHECKSUM "=000\x01");
  static const uint8_t begin_string_len = STR_LEN(FIX_BEGINSTRING "=" FIX_VERSION "\x01");
  static const uint8_t body_length_len = STR_LEN(FIX_BODYLENGTH "=") + 2;
  static const uint8_t total_minimum_len = checksum_len + begin_string_len + body_length_len;

  if (message_len < total_minimum_len)
    return false;

  const bool has_checksum = !!get_checksum_start(buffer, message_len);
  (void)(error && (*error = FF_BUFFER_TOO_SMALL * (!has_checksum && message_len >= buffer_size)));

  return has_checksum;
}

/*
TODO correctly handle all the possible edge cases coming from the network.
multiple separators adjacent, multiple = adjacent, etc.
*/
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
  const char *end = buffer + buffer_size - STR_LEN(FIX_CHECKSUM "=000\x01");

#ifdef __AVX512F__
  {
    //TODO load only once, static
    const __mm512i vec_ones   = _mm512_set1_epi8('1');
    const __mm512i vec_zeros  = _mm512_set1_epi8('0');
    const __mm512i vec_equals = _mm512_set1_epi8('=');
    const __mm512i vec_soh    = _mm512_set1_epi8('\x01');

    while (UNLIKELY(buffer + 64 <= end))
    {
      PREFETCHR(buffer + 128, 3);

      const __m512i chunk = _mm512_loadu_si512((const void*)(buffer));

      const __mmask64 m1 = _mm512_cmpeq_epi8_mask(chunk, vec_ones);
      const __mmask64 m2 = _mm512_cmpeq_epi8_mask(chunk, vec_zeros);
      const __mmask64 m3 = _mm512_cmpeq_epi8_mask(chunk, vec_equals);
      const __mmask64 m4 = _mm512_cmpeq_epi8_mask(chunk, vec_soh);

      __mmask64 candidates = _kand_mask64(m1, _kshiftri_mask64(m2, 1));
      candidates = _kand_mask64(candidates, _kshiftri_mask64(m3, 2));
      candidates = _kand_mask64(candidates, _kshiftri_mask64(m4, 6));

      if (candidates)
        return buffer + __builtin_ctzll(candidates);

      buffer += 64;
    }
  }
#endif

#ifdef __AVX2__
  {
    //TODO load only once, static
    const __m256i vec_ones   = _mm256_set1_epi8('1');
    const __m256i vec_zeros  = _mm256_set1_epi8('0');
    const __m256i vec_equals = _mm256_set1_epi8('=');
    const __m256i vec_soh    = _mm256_set1_epi8('\x01');

    while (UNLIKELY(buffer + 32 <= end))
    {
      PREFETCHR(buffer + 64, 3);
  
      const __m256i chunk = _mm256_loadu_si256((const void*)(buffer));
  
      const __m256i m1 = _mm256_cmpeq_epi8(chunk, vec_ones);
      const __m256i m2 = _mm256_cmpeq_epi8(chunk, vec_zeros);
      const __m256i m3 = _mm256_cmpeq_epi8(chunk, vec_equals);
      const __m256i m4 = _mm256_cmpeq_epi8(chunk, vec_soh);
  
      const __m256i shifted_m2 = _mm256_srli_epi64(m2, 1);
      const __m256i shifted_m3 = _mm256_srli_epi64(m3, 2);
      const __m256i shifted_m4 = _mm256_srli_epi64(m4, 6);
  
      __m256i candidates = _mm256_and_si256(m1, shifted_m2);
      candidates = _mm256_and_si256(candidates, shifted_m3);
      candidates = _mm256_and_si256(candidates, shifted_m4);
  
      int mask = _mm256_movemask_epi8(candidates);
      if (mask)
        return buffer + __builtin_ctz(mask);
  
      buffer += 32;
    }
  }
#endif

#ifdef __SSE2__
  //TODO load only once, static
  const __m128i vec_oness  = _mm_set1_epi8('1');
  const __m128i vec_zeros  = _mm_set1_epi8('0');
  const __m128i vec_equals = _mm_set1_epi8('=');
  const __m128i vec_soh    = _mm_set1_epi8('\x01');

  while (UNLIKELY(buffer + 16 <= end))
  {
    PREFETCHR(buffer + 32, 3);

    const __m128i chunk = _mm_loadu_si128((const __m128i*)(buffer));

    const __m128i m1 = _mm_cmpeq_epi8(chunk, vec_oness);
    const __m128i m2 = _mm_cmpeq_epi8(chunk, vec_zeros);
    const __m128i m3 = _mm_cmpeq_epi8(chunk, vec_equals);
    const __m128i m4 = _mm_cmpeq_epi8(chunk, vec_soh);

    const __m128i shifted_m2 = _mm_srli_epi64(m2, 1);
    const __m128i shifted_m3 = _mm_srli_epi64(m3, 2);
    const __m128i shifted_m4 = _mm_srli_epi64(m4, 6);

    __m128i candidates = _mm_and_si128(m1, shifted_m2);
    candidates = _mm_and_si128(candidates, shifted_m3);
    candidates = _mm_and_si128(candidates, shifted_m4);

    const int32_t mask = _mm_movemask_epi8(candidates);
    if (mask)
      return buffer + __builtin_ctz(mask);

    buffer += 16;
  }
#endif

  while (LIKELY(buffer <= end))
  {
    //TODO find a way to compare all 7 bytes at once without potential UB. the buffer is guaranteed to have 7 bytes left, not 8.
    //or at least combine the first 4 comparisons in a single 32 bit integer (with masks to mask out the last one)
    //or at least combine the first 2 comparisons in a single 16 bit integer (without masks, straight up !=)
    if (buffer[0] == '1' && buffer[1] == '0' && buffer[2] == '=' && buffer[6] == '\x01')
      return buffer;
    buffer++;
  }

  return NULL;
}

static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error)
{
  static const char begin_string_tag[] = FIX_BEGINSTRING "=" FIX_VERSION "\x01";

  *error = FF_INVALID_MESSAGE * (*(uint16_t *)buffer != *(const uint16_t *)begin_string_tag);

  return STR_LEN(begin_string_tag);
}

static inline uint16_t check_body_length_tag(const char *buffer, ff_error_t *restrict error)
{
  static const char body_length_tag[] = FIX_BODYLENGTH "=";

  *error = FF_INVALID_MESSAGE * memcmp(buffer, body_length_tag, STR_LEN(body_length_tag));

  return STR_LEN(body_length_tag);
}

static inline uint16_t deserialize_body_length(const char *buffer, uint16_t *body_length, ff_error_t *restrict error)
{
  const char *buffer_start = buffer;

  *body_length = (uint16_t)strtoul(buffer, (char **)&buffer, 10); //TODO custom strtouint16?, non-printable safe
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
  const uint8_t provided_checksum = (uint8_t)strtoul(checksum_start, (char **)&buffer, 10); //TODO custom strtouint8?, non-printable safe
  if (UNLIKELY(expected_checksum != provided_checksum))
    return (*error = FF_CHECKSUM_MISMATCH, 0);
  buffer += STR_LEN("\x01");

  return buffer - *body_start;  
}

static void tokenize_message(char *restrict buffer, const uint16_t buffer_size)
{
  const char *end = buffer + buffer_size;

#ifdef __AVX512F__
  {
    const __m512i vec_soh    = _mm512_set1_epi8('\x01');
    const __m512i vec_equals = _mm512_set1_epi8('=');

    while (LIKELY(buffer + 64 <= end))
    {
      PREFETCHR(buffer + 128, 3);
    
      const __m512i chunk = _mm512_loadu_si512((__m512i*)buffer);

      const __m512i cmp_soh     = _mm512_cmpeq_epi8(chunk, vec_soh);
      const __m512i cmp_equals  = _mm512_cmpeq_epi8(chunk, vec_equals);
  
      const __m512i cmp  = _mm512_or_si512(cmp_soh, cmp_equals);
      const __m512i result = _mm512_andnot_si512(cmp, chunk);
      _mm512_storeu_si512((__m512i*)buffer, result);
    
      buffer += 64;
    }
  }
#endif

//TODO intialize these constant simd variables only once, static with the first call (EVERYWHERE in the project, not just here)
#ifdef __AVX2__
  {
    const __m256i vec_soh    = _mm256_set1_epi8('\x01');
    const __m256i vec_equals = _mm256_set1_epi8('=');

    while (LIKELY(buffer + 32 <= end))
    {
      PREFETCHR(buffer + 64, 3);
    
      const __m256i chunk = _mm256_loadu_si256((__m256i*)buffer);

      const __m256i cmp_soh     = _mm256_cmpeq_epi8(chunk, vec_soh);
      const __m256i cmp_equals  = _mm256_cmpeq_epi8(chunk, vec_equals);

      const __m256i cmp = _mm256_or_si256(cmp_soh, cmp_equals);
      const __m256i result = _mm256_andnot_si256(cmp, chunk);
      _mm256_storeu_si256((__m256i*)buffer, result);
    
      buffer += 32;
    }
  } 
#endif

#ifdef __SSE2__
  //TODO implement the same thing for SSE2
#endif

  static const uint64_t mask_soh = 0x0101010101010101ULL;
  static const uint64_t mask_equals = 0x3D3D3D3D3D3D3D3DULL;
  static const uint64_t ones = 0x0101010101010101ULL;
  static const uint64_t high_mask = 0x8080808080808080ULL;
  
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
  (void)buffer;
  (void)buffer_size;
  (void)message;
  (void)error;
}


