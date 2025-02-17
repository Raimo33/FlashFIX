/*================================================================================

File: common.c                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-17 15:08:46                                                

================================================================================*/

#include "common.h"

uint8_t compute_checksum(const char *buffer, const uint16_t len)
{
  const char *const end = buffer + len;
  uint8_t checksum = 0;
  const char *aligned_buffer = align_forward(buffer, 64);

  while (UNLIKELY(buffer < aligned_buffer))
    checksum += *buffer++;

#ifdef __AVX512F__
  while (LIKELY(buffer + 64 <= end))
  {
    PREFETCHR(buffer + 128, 3);

    const __m512i vec = _mm512_load_si512((const __m512i *)buffer);
    const __m512i sum = _mm512_sad_epu8(vec, _mm512_setzero_si512());
    checksum += (uint8_t)_mm512_reduce_add_epi64(sum);

    buffer += 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(buffer + 32 <= end))
  {
    PREFETCHR(buffer + 64, 3);

    const __m256i vec = _mm256_load_si256((const __m256i *)buffer);
    const __m256i sum = _mm256_sad_epu8(vec, _mm256_setzero_si256());
    
    __m128i sum_low = _mm256_castsi256_si128(sum);
    const __m128i sum_high = _mm256_extracti128_si256(sum, 1);
    sum_low = _mm_add_epi64(sum_low, sum_high);

    checksum += (uint8_t)(_mm_extract_epi64(sum_low, 0) + _mm_extract_epi64(sum_low, 1));

    buffer += 32;
  }
#endif

#ifdef __SSE4_1__
  while (LIKELY(buffer + 16 <= end))
  {
    PREFETCHR(buffer + 32, 3);

    const __m128i vec = _mm_load_si128((const __m128i *)buffer);
    const __m128i sum = _mm_sad_epu8(vec, _mm_setzero_si128());

    checksum += (uint8_t)(_mm_extract_epi64(sum, 0) + _mm_extract_epi64(sum, 1));

    buffer += 16;
  }
#endif

  while (LIKELY(buffer + 8 <= end))
  {
    uint64_t chunk = *(const uint64_t *)buffer;

    chunk = (chunk & 0x00FF00FF00FF00FFULL) + ((chunk >> 8) & 0x00FF00FF00FF00FFULL);
    chunk = (chunk & 0x0000FFFF0000FFFFULL) + ((chunk >> 16) & 0x0000FFFF0000FFFFULL);
    chunk = (chunk & 0x00000000FFFFFFFFULL) + (chunk >> 32);

    checksum += (uint8_t)chunk;
    
    buffer += 8;
  }

  while (LIKELY(buffer < end))
    checksum += *buffer++;

  return checksum;
}
