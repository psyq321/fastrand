/*******************************************************************************

  Super-Fast MWC1616 [1] and xorshift128+ [2] Pseudo-Random Number Generator
  for x86 Architecture, using SSE4, AVX2 and AVX-512 instructions
  
  Copyright (c) 2012-2023, Ivan Dimkovic (www.digicortex.net)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation 
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

********************************************************************************
  REFERENCES:

  [1]  G. Marsaglia, The Marsaglia Random Number CDROM, with the DIEHARD 
  Battery of Tests of Randomness. Department of Statistics, Florida State 
  University, (1996) http://stat.fsu.edu/~geo/diehard.html

  [2] Marsaglia, G. (2003). Xorshift RNGs. Journal of Statistical Software, 
  8(14), 1–6. https://doi.org/10.18637/jss.v008.i14

********************************************************************************

 USAGE NOTICE:

 Please DO NOT use these pseudo-random number generators for cryptographic
 or security purposes. They are designed for speed and quality of randomness
 only, nothing else. Also, please do not use the MWC1616 generator for
 scientific purposes, as it is not a statistically robust generator.

*******************************************************************************/

#pragma once

#include <immintrin.h>
#include <smmintrin.h>
#include <cstdint>
#include <random>

namespace fastrand {
#if defined(__AVX512F__)
constexpr auto SIMD_LEN_MWC1616 = 16;
constexpr auto SIMD_LEN_XORSHIFT128p = 8;
constexpr auto ALIGN = 64;
using ivec_t = __m512i;

#define _simd_xor_si _mm512_xor_si512
#define _simd_load_si _mm512_load_si512
#define _simd_store_si _mm512_store_si512
#define _simd_and_si _mm512_and_si512
#define _simd_srli_epi32 _mm512_srli_epi32
#define _simd_srli_epi64 _mm512_srli_epi64
#define _simd_slli_epi32 _mm512_slli_epi32
#define _simd_slli_epi64 _mm512_slli_epi64
#define _simd_add_epi32 _mm512_add_epi32
#define _simd_add_epi64 _mm512_add_epi64
#define _simd_mullo_epi32 _mm512_mullo_epi32
#elif defined(__AVX2__)
constexpr auto SIMD_LEN_MWC1616 = 8;
constexpr auto SIMD_LEN_XORSHIFT128p = 4;
constexpr auto ALIGN = 32;
using ivec_t = __m256i;

#define _simd_xor_si _mm256_xor_si256
#define _simd_load_si _mm256_load_si256
#define _simd_store_si _mm256_store_si256
#define _simd_and_si _mm256_and_si256
#define _simd_srli_epi32 _mm256_srli_epi32
#define _simd_srli_epi64 _mm256_srli_epi64
#define _simd_slli_epi32 _mm256_slli_epi32
#define _simd_slli_epi64 _mm256_slli_epi64
#define _simd_add_epi32 _mm256_add_epi32
#define _simd_add_epi64 _mm256_add_epi64
#define _simd_mullo_epi32 _mm256_mullo_epi32
#elif defined(__SSE__)
constexpr auto SIMD_LEN_MWC1616 = 4;
constexpr auto SIMD_LEN_XORSHIFT128p = 2;
constexpr auto ALIGN = 16;
using ivec_t = __m128i;
#define _simd_xor_si _mm_xor_si128
#define _simd_load_si _mm_load_si128
#define _simd_store_si _mm_store_si128
#define _simd_and_si _mm_and_si128
#define _simd_srli_epi32 _mm_srli_epi32
#define _simd_srli_epi64 _mm_srli_epi64
#define _simd_slli_epi32 _mm_slli_epi32
#define _simd_slli_epi64 _mm_slli_epi64
#define _simd_add_epi32 _mm_add_epi32
#define _simd_add_epi64 _mm_add_epi64
#define _simd_mullo_epi32 _mm_mullo_epi32
#else
#error "Unsupported SIMD instruction set, at least SSE4 is required"
#endif

/*******************************************************************************
  MWC1616
*******************************************************************************/

struct alignas(ALIGN) mwc1616 {

  //
  // MWC1616 PRNG data

  uint32_t x_[SIMD_LEN_MWC1616];
  uint32_t y_[SIMD_LEN_MWC1616];
  uint32_t mask_[SIMD_LEN_MWC1616];
  uint32_t mul1_[SIMD_LEN_MWC1616];
  uint32_t mul2_[SIMD_LEN_MWC1616];

  //
  // Result (32-bit pseudorandom random values)

  uint32_t res_[SIMD_LEN_MWC1616];

  ////////////////////
  // Implementation //
  ////////////////////

  mwc1616() {

    //
    // Initialize masks and multipliers

    for (int i = 0; i < SIMD_LEN_MWC1616; i++) {
      mask_[i] = 0xFFFFu;
      mul1_[i] = 0x4650u;
      mul2_[i] = 0x78B7u;

      x_[i] = std::random_device{}();
      y_[i] = std::random_device{}();
    }
  };

  //
  // Generates N 32-bit pseudorandom unsigned integers
  // N=4 (SSE4), N=8 (AVX2), N=16 (AVX-512)

  inline void Generate() {

    const ivec_t x = _simd_load_si((ivec_t *)x_);
    const ivec_t y = _simd_load_si((ivec_t *)y_);

    const ivec_t mask = _simd_load_si((ivec_t *)mask_);
    const ivec_t mul1 = _simd_load_si((ivec_t *)mul1_);
    const ivec_t mul2 = _simd_load_si((ivec_t *)mul2_);

    ivec_t xmask = _simd_and_si(x, mask);
    ivec_t xshift = _simd_srli_epi32(x, 0x10);
    ivec_t xmul = _simd_mullo_epi32(xmask, mul1);
    ivec_t xnew = _simd_add_epi32(xmul, xshift);
    _simd_store_si((ivec_t *)x_, xnew);

    ivec_t ymask = _simd_and_si(y, mask);
    ivec_t yshift = _simd_srli_epi32(y, 0x10);
    ivec_t ymul = _simd_mullo_epi32(ymask, mul2);
    ivec_t ynew = _simd_add_epi32(ymul, yshift);
    _simd_store_si((ivec_t *)y_, ynew);

    ivec_t ymasknew = _simd_and_si(ynew, mask);
    ivec_t xshiftnew = _simd_slli_epi32(xnew, 0x10);
    ivec_t res = _simd_add_epi32(xshiftnew, ymasknew);
    _simd_store_si((ivec_t *)res_, res);
  }
};

/*******************************************************************************
  xorshift128+
*******************************************************************************/

struct alignas(ALIGN) xorshift128plus {

  //
  // xorshift128+ PRNG data

  uint64_t s0_[SIMD_LEN_XORSHIFT128p];
  uint64_t s1_[SIMD_LEN_XORSHIFT128p];

  //
  // Result (64-bit pseudorandom random values)

  uint64_t res_[SIMD_LEN_XORSHIFT128p];

  ////////////////////
  // Implementation //
  ////////////////////

  xorshift128plus() {
    for (uint32_t i = 0; i < SIMD_LEN_XORSHIFT128p; i++) {
      s0_[i] = (uint64_t)std::random_device{}() << 32u;
      s0_[i] |= std::random_device{}();

      s1_[i] = (uint64_t)std::random_device{}() << 32u;
      s1_[i] |= std::random_device{}();
    }
  };

  //
  // Generates N 64-bit pseudorandom unsigned integers
  // N=2 (SSE4), N=4 (AVX2), N=8 (AVX-512)

  void Generate() {
    ivec_t s1 = _simd_load_si((ivec_t *)s0_);
    ivec_t s0 = _simd_load_si((ivec_t *)s1_);

    _simd_store_si((ivec_t *)s0_, s0);

    const ivec_t s1_l = _simd_slli_epi64(s1, 23u);

    s1 = _simd_xor_si(s1, s1_l);

    const ivec_t s0_r = _simd_srli_epi64(s0, 5u);
    const ivec_t s1_r = _simd_srli_epi64(s1, 18u);
    const ivec_t s10x = _simd_xor_si(s1, s0);

    ivec_t s1_new = _simd_xor_si(_simd_xor_si(s10x, s1_r), s0_r);

    _simd_store_si((ivec_t *)s1_, s1_new);
    _simd_store_si((ivec_t *)res_, _simd_add_epi64(s1_new, s0));
  };
};

}