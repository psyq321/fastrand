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

#include "fastrand.h"
#include <cstdio>
#include <ctime>
#include <chrono>

/*******************************************************************************
  Performance Measurement
*******************************************************************************/

fastrand::mwc1616 fr_mwc1616;
fastrand::xorshift128plus fr_xorshift128plus;

const size_t NUM_RUNS = 1000000000;

/*******************************************************************************
  measure_mwc1616
*******************************************************************************/

void measure_mwc1616() {
  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < NUM_RUNS; i++) {
    fr_mwc1616.Generate();
  }

  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> elapsed = end - start;

  /////////////
  // Results //
  /////////////

  double bandwidth = 4. /* sizeof(uint32_t) */ *
                     (double)fastrand::SIMD_LEN_MWC1616 * (double)NUM_RUNS /
                     elapsed.count() * .000001;

  //
  // print results, so compiler will not optimize out the loop

  for (int i = 0; i < fastrand::SIMD_LEN_MWC1616; i++) {
    printf("%08X ", fr_mwc1616.res_[i]);
  }
  printf("\n");

  printf("Elapsed time: %f ms\n", elapsed.count());
  printf("Bandwidth: %f GB/s, %f GNumbers/s, %f ns per number\n\n", bandwidth,
         bandwidth * .25, 1. / (bandwidth * .125));
}

/*******************************************************************************
  measure_xorshift128p
*******************************************************************************/

void measure_xorshift128p() {
  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < NUM_RUNS; i++) {
    fr_xorshift128plus.Generate();
  }

  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> elapsed = end - start;

  /////////////
  // Results //
  /////////////

  double bandwidth = 8. /* sizeof(uint64_t) */ *
                     (double)fastrand::SIMD_LEN_XORSHIFT128p *
                     (double)NUM_RUNS /
                     elapsed.count() * .000001;

  //
  // print results, so compiler will not optimize out the loop

  for (int i = 0; i < fastrand::SIMD_LEN_XORSHIFT128p; i++) {
    printf("%08llX ", fr_xorshift128plus.res_[i]);
  }
  printf("\n");

  printf("Elapsed time: %f ms\n", elapsed.count());
  printf("Bandwidth: %f GB/s, %f GNumbers/s, %f ns per number\n", bandwidth,
         bandwidth * 0.125, 1. / (bandwidth * .125));
}

/*******************************************************************************
  main
*******************************************************************************/

int main()
{ 

  ///////////////
  // Benchmark //
  ///////////////

  measure_mwc1616();
  measure_xorshift128p();

#if 0
  //
  // Generate files for randomness testing

  FILE *fp = fopen("xorshift128p.bin", "wb");

  if(fp) {
    for(size_t i = 0; i < 10000000; i++) {
      fr_xorshift128plus.Generate();
      fwrite(fr_xorshift128plus.res_, sizeof(uint64_t), fastrand::SIMD_LEN_XORSHIFT128p, fp);
    }
    fclose(fp);
  }

  FILE* fp2 = fopen("mwc1616.bin", "wb");

  if(fp2) {
    for(size_t i = 0; i < 10000000; i++) {
      fr_mwc1616.Generate();
      fwrite(fr_mwc1616.res_, sizeof(uint32_t), fastrand::SIMD_LEN_MWC1616, fp2);
    }
    fclose(fp2);
  }

#endif
}

