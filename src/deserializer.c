/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   deserializer.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:11:25 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:57:16 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "internal/common.h"
#include <flashfix/deserializer.h>
#include <string.h>

static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size);
HOT static inline bool is_checksum_sequence(const char *buffer);
static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error);
static inline uint16_t check_content_length_tag(const char *buffer, ff_error_t *restrict error);
static inline uint16_t deserialize_content_length(const char *buffer, uint16_t *content_length, ff_error_t *restrict error);
static inline uint16_t validate_checksum(char *buffer, const uint16_t buffer_size, const uint16_t content_length, char **body_start, ff_error_t *restrict error);
static void tokenize_message(char *restrict buffer, const uint16_t buffer_size);
static void fill_message_fields(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message, ff_error_t *restrict error);

bool is_full_fix_message(const char *buffer, UNUSED const uint16_t buffer_size, const uint16_t message_len, ff_error_t *restrict error)
{
  static const uint8_t checksum_len = STR_LEN(FIX_CHECKSUM "=000\x01");
  static const uint8_t begin_string_len = STR_LEN(FIX_BEGINSTRING "=" FIX_VERSION "\x01");
  static const uint8_t body_length_len = STR_LEN(FIX_BODYLENGTH "=");

  if (message_len < (checksum_len + begin_string_len + body_length_len))
    return false;

  //TODO potenziale loop infinito se content len e' piu grante del buffer_size. fare un assert

  return (!!get_checksum_start(buffer, message_len));
}

uint16_t deserialize_fix_message(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message, ff_error_t *restrict error)
{
  if (!buffer || !message)
    return (*error = FF_NULL_POINTER, 0);
  
  const char *buffer_start = buffer;
  char *body_start;
  uint16_t content_length;

  ff_error_t local_error = FF_OK;

  buffer += check_begin_string(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += check_content_length_tag(buffer, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += deserialize_content_length(buffer, &content_length, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  buffer += validate_checksum(buffer, buffer_size, content_length, &body_start, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  tokenize_message(body_start, content_length);
  fill_message_fields(body_start, content_length, message, &local_error);
  if (UNLIKELY(local_error != FF_OK)) goto error;

  return buffer - buffer_start;

error:
  return (*error = local_error, 0);
}

static const char *get_checksum_start(const char *buffer, const uint16_t buffer_size)
{
  const char *end = buffer + buffer_size - STR_LEN(FIX_CHECKSUM "=000\x01");

#ifdef __AVX512F__
  while (UNLIKELY(buffer + 64 < end))
  {
    PREFETCHR(buffer + 128, 3);
    __m512i chunk = _mm512_loadu_si512((const void*)(buffer));

    __mmask64 m1 = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8('1'));
    __mmask64 m2 = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8('0'));
    __mmask64 m3 = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8('='));
    __mmask64 m4 = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8(0x01));

    __mmask64 candidates = _kand_mask64(m1, _kshiftri_mask64(m2, 1));
    candidates = _kand_mask64(candidates, _kshiftri_mask64(m3, 2));
    candidates = _kand_mask64(candidates, _kshiftri_mask64(m4, 6));

    if (candidates)
    {
      int32_t index = __builtin_ctzll(candidates);
      return buffer + index;
    }

    buffer += 64;
  }
#endif

#ifdef __AVX2__
  while (UNLIKELY(buffer + 32 < end))
  {
    PREFETCHR(buffer + 64, 3);
    __m256i chunk = _mm256_loadu_si256((const void*)(buffer));

    __m256i m1 = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('1'));
    __m256i m2 = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('0'));
    __m256i m3 = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('='));
    __m256i m4 = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8(0x01));

    __m256i shifted_m2 = _mm256_srli_epi64(m2, 1);
    __m256i shifted_m3 = _mm256_srli_epi64(m3, 2);
    __m256i shifted_m4 = _mm256_srli_epi64(m4, 6);

    __m256i candidates = _mm256_and_si256(m1, shifted_m2);
    candidates = _mm256_and_si256(candidates, shifted_m3);
    candidates = _mm256_and_si256(candidates, shifted_m4);

    int mask = _mm256_movemask_epi8(candidates);
    if (mask)
    {
      int index = __builtin_ctz(mask);
      return buffer + index;
    }

    buffer += 32;
  }
#endif

#ifdef __SSE2__
  while (UNLIKELY(buffer + 16 < end))
  {
    PREFETCHR(buffer + 32, 3);
    __m128i chunk = _mm_loadu_si128((const __m128i*)(buffer));

    __m128i m1 = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('1'));
    __m128i m2 = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('0'));
    __m128i m3 = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('='));
    __m128i m4 = _mm_cmpeq_epi8(chunk, _mm_set1_epi8(0x01));

    __m128i shifted_m2 = _mm_srli_epi64(m2, 1);
    __m128i shifted_m3 = _mm_srli_epi64(m3, 2);
    __m128i shifted_m4 = _mm_srli_epi64(m4, 6);

    __m128i candidates = _mm_and_si128(m1, shifted_m2);
    candidates = _mm_and_si128(candidates, shifted_m3);
    candidates = _mm_and_si128(candidates, shifted_m4);

    int32_t mask = _mm_movemask_epi8(candidates);
    if (mask)
    {
      int16_t index = __builtin_ctz(mask);
      return buffer + index;
    }

    buffer += 16;
  }
#endif

  while (LIKELY(buffer < end))
  {
    if (is_checksum_sequence(buffer))
      return buffer;
    buffer++;
  }

  return NULL;
}

static inline bool is_checksum_sequence(const char *buffer)
{
  //TODO potential off by one error. i dereference 8 bytes, but the buffer is guaranteed to be 7 before end
  uint64_t word = *(const uint64_t *)buffer;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  const uint64_t expected = ('1' <<  0) | ('0' <<  8) | ('=' << 16) | (0x01ULL << 55);
  const uint64_t mask = (0xFFULL <<  0) | (0xFFULL <<  8) | (0xFFULL << 16) | (0xFFULL << 55);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const uint64_t expected = ('1' << 56) | ('0' << 48) | ('=' << 40) | (0x01ULL << 8);
  const uint64_t mask = (0xFFULL << 56) | (0xFFULL << 48) | (0xFFULL << 40) | (0xFFULL << 8);
#else
  # error "Unknown byte order"
#endif

  return (word & mask) == expected;
}

static inline uint16_t check_begin_string(const char *buffer, ff_error_t *restrict error)
{
  static const char begin_string_tag[] = FIX_BEGINSTRING "=" FIX_VERSION "\x01";

  if (UNLIKELY(*(uint16_t *)buffer != *(const uint16_t *)begin_string_tag))
    return (*error = FF_INVALID_MESSAGE, 0);

  return STR_LEN(begin_string_tag);
}

static inline uint16_t check_content_length_tag(const char *buffer, ff_error_t *restrict error)
{
  static const char body_length_tag[] = FIX_BODYLENGTH "=";

  if (UNLIKELY(memcmp(buffer, body_length_tag, STR_LEN(body_length_tag))))
    return (*error = FF_INVALID_MESSAGE, 0);

  return STR_LEN(body_length_tag);
}

static inline uint16_t deserialize_content_length(const char *buffer, uint16_t *content_length, ff_error_t *restrict error)
{
  const char *buffer_start = buffer;

  *content_length = (uint16_t)strtoul(buffer, (char **)&buffer, 10);
  if (UNLIKELY(*buffer++ != '\x01'))
    return (*error = FF_INVALID_MESSAGE, 0);

  return buffer - buffer_start;
}

static inline uint16_t validate_checksum(char *buffer, const uint16_t buffer_size, const uint16_t content_length, char **body_start, ff_error_t *restrict error)
{
  *body_start = buffer;

  const char *body_end = get_checksum_start(buffer, buffer_size);
  const char *checksum_start = body_end + STR_LEN(FIX_CHECKSUM "=");
  if (UNLIKELY(body_end - *body_start != content_length))
    return (*error = FF_CONTENT_LENGTH_MISMATCH, 0);

  const uint8_t expected_checksum = compute_checksum(*body_start, content_length);
  const uint8_t provided_checksum = (uint8_t)strtoul(checksum_start, (char **)&buffer, 10);
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
    __m512i vec_soh = _mm512_set1_epi8(0x01);
    __m512i vec_equals = _mm512_set1_epi8(0x3D);

    while (LIKELY(buffer + 64 <= end))
    {
      PREFETCHR(buffer + 128, 3);
      __m512i chunk = _mm512_loadu_si512((__m512i*)buffer);
      __m512i cmp_soh = _mm512_cmpeq_epi8(chunk, vec_soh);
      __m512i cmp_equals = _mm512_cmpeq_epi8(chunk, vec_equals);
      __m512i cmp = _mm512_or_si512(cmp_soh, cmp_equals);
      __m512i result = _mm512_andnot_si512(cmp, chunk);
      _mm512_storeu_si512((__m512i*)buffer, result);
      buffer += 64;
    }
  }
#endif

#ifdef __AVX2__
  {
    __m128i vec_soh = _mm_set1_epi8(0x01);
    __m128i vec_equals = _mm_set1_epi8(0x3D);
  
    while (LIKELY(buffer + 16 <= end))
    {
      PREFETCHR(buffer + 32, 3);
      __m128i chunk = _mm_loadu_si128((__m128i*)buffer);
      
      __m128i cmp_soh = _mm_cmpeq_epi8(chunk, vec_soh);
      __m128i cmp_equals = _mm_cmpeq_epi8(chunk, vec_equals);
      __m128i cmp = _mm_or_si128(cmp_soh, cmp_equals);
      
      __m128i result = _mm_andnot_si128(cmp, chunk);
      _mm_storeu_si128((__m128i*)buffer, result);
      
      buffer += 16;
    }
  }
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
    *buffer = (c != '\x01' || c != '=') * c;
    buffer++;
  }
}

static void fill_message_fields(char *restrict buffer, const uint16_t buffer_size, fix_message_t *restrict message, ff_error_t *restrict error)
{
  (void)buffer;
  (void)buffer_size;
  (void)message;
  (void)error;
  //TODO implement
}


