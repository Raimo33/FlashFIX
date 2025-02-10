/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   common.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: craimond <claudio.raimondi@pm.me>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/09 19:14:53 by craimond          #+#    #+#             */
/*   Updated: 2025/02/10 14:59:12 by craimond         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "common.h"

uint8_t compute_checksum(const char *buffer, const uint16_t len)
{
  uint8_t checksum = 0;
  
  const char *end = buffer + len;

#ifdef __AVX512F__
  while (LIKELY(buffer + 64 <= end))
  {
    PREFETCHR(buffer + 128, 3);
    __m512i vec = _mm512_loadu_si512((const __m512i *)buffer);
    __m512i sum = _mm512_sad_epu8(vec, _mm512_setzero_si512());
    
    checksum += (uint8_t)_mm512_reduce_add_epi64(sum);
    buffer += 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(buffer + 32 <= end))
  {
    PREFETCHR(buffer + 64, 3);
    __m256i vec = _mm256_loadu_si256((const __m256i *)buffer);
    __m256i sum = _mm256_sad_epu8(vec, _mm256_setzero_si256());
    
    __m128i sum_low = _mm256_castsi256_si128(sum);
    __m128i sum_high = _mm256_extracti128_si256(sum, 1);
    sum_low = _mm_add_epi64(sum_low, sum_high);

    checksum += (uint8_t)(_mm_extract_epi64(sum_low, 0) + _mm_extract_epi64(sum_low, 1));
    buffer += 32;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(buffer + 16 <= end))
  {
    PREFETCHR(buffer + 32, 3);
    __m128i vec = _mm_loadu_si128((const __m128i *)buffer);
    __m128i sum = _mm_sad_epu8(vec, _mm_setzero_si128());

    checksum += (uint8_t)(_mm_extract_epi64(sum, 0) + _mm_extract_epi64(sum, 1));
    buffer += 16;
  }
#endif

//TODO ultimo loop SWAR per blocchetti di 8 byte (uint64_t)

  while (LIKELY(buffer < end))
    checksum += *buffer++;

  return checksum;
}