/*================================================================================

File: common.c                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-24 16:35:15                                                 
last edited: 2025-03-02 21:57:40                                                

================================================================================*/

#include "common.h"

#ifdef __AVX512F__
  static __m512i _512_vec_zeros;
#endif

#ifdef __AVX2__
  static __m256i _256_vec_zeros;
#endif

#ifdef __SSE2__
  static __m128i _128_vec_zeros;
#endif

CONSTRUCTOR void ff_common_init(void)
{
#ifdef __AVX512F__
  _512_vec_zeros = _mm512_setzero_si512();
#endif

#ifdef __AVX2__
  _256_vec_zeros = _mm256_setzero_si256();
#endif

#ifdef __SSE2__
  _128_vec_zeros = _mm_setzero_si128();
#endif
}

uint8_t compute_checksum(const char *buffer,  const char *const end)
{
  uint16_t remaining = end - buffer;
  
  uint8_t misaligned_bytes = align_forward(buffer);
  misaligned_bytes -= (misaligned_bytes > remaining) * (misaligned_bytes - remaining);

  uint8_t checksum = 0;

  while (UNLIKELY(misaligned_bytes--))
  {
    checksum += *buffer++;
    remaining--;
  }

#ifdef __AVX512F__
  while (LIKELY(remaining >= 64))
  {
    const __m512i vec = _mm512_load_si512((const __m512i *)buffer);
    const __m512i sum = _mm512_sad_epu8(vec, _512_vec_zeros);
    checksum += (uint8_t)_mm512_reduce_add_epi64(sum);

    buffer += 64;
    remaining -= 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(remaining >= 32))
  {
    const __m256i vec = _mm256_load_si256((const __m256i *)buffer);
    const __m256i sum = _mm256_sad_epu8(vec, _256_vec_zeros);
    
    const __m128i sum_low = _mm256_extracti128_si256(sum, 0);
    const __m128i sum_high = _mm256_extracti128_si256(sum, 1);
    const __m128i sum_total = _mm_add_epi64(sum_low, sum_high);

    checksum += (uint8_t)(_mm_extract_epi64(sum_total, 0) + _mm_extract_epi64(sum_total, 1));

    buffer += 32;
    remaining -= 32;
  }
#endif

#ifdef __SSE4_1__
  while (LIKELY(remaining >= 16))
  {
    const __m128i vec = _mm_load_si128((const __m128i *)buffer);
    const __m128i sum = _mm_sad_epu8(vec, _128_vec_zeros);

    checksum += (uint8_t)(_mm_extract_epi64(sum, 0) + _mm_extract_epi64(sum, 1));

    buffer += 16;
    remaining -= 16;
  }
#endif

  while (LIKELY(remaining >= 8))
  {
    uint64_t chunk = *(const uint64_t *)buffer;

    chunk = (chunk & 0x00FF00FF00FF00FFULL) + ((chunk >> 8) & 0x00FF00FF00FF00FFULL);
    chunk = (chunk & 0x0000FFFF0000FFFFULL) + ((chunk >> 16) & 0x0000FFFF0000FFFFULL);
    chunk = (chunk & 0x00000000FFFFFFFFULL) + (chunk >> 32);

    checksum += (uint8_t)chunk;
    
    buffer += 8;
    remaining -= 8;
  }

  while (LIKELY(remaining--))
    checksum += *buffer++;

  return checksum;
}