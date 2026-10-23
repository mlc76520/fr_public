// This file is distributed under a BSD license. See LICENSE.txt for details.

// GCC versions of the FRIED MMX inline assembly, written with MMX intrinsics
// that map 1:1 to the original instructions (the C fallbacks in the sources
// do not round exactly like the assembly the released players used).

#ifndef __FRIED_MMX_LINUX_HPP__
#define __FRIED_MMX_LINUX_HPP__

#include <mmintrin.h>

namespace FRIED
{
  static inline __m64 mmLoad(const void *p)   { __m64 r; sCopyMem(&r,p,8); return r; }
  static inline void mmStore(void *p,__m64 v) { sCopyMem(p,&v,8); }

  // transpose 4x4 words in r0..r3, using r4,r5 as temporaries (same register dance as MM_TRANSPOSE)
  #define MMI_TRANSPOSE(r0,r1,r2,r3,r4,r5) \
    r4 = r0; \
    r5 = r2; \
    r0 = _mm_unpacklo_pi16(r0,r1); \
    r4 = _mm_unpackhi_pi16(r4,r1); \
    r2 = _mm_unpacklo_pi16(r2,r3); \
    r5 = _mm_unpackhi_pi16(r5,r3); \
    r1 = r0; \
    r3 = r4; \
    r0 = _mm_unpacklo_pi32(r0,r2); \
    r1 = _mm_unpackhi_pi32(r1,r2); \
    r4 = _mm_unpacklo_pi32(r4,r5); \
    r3 = _mm_unpackhi_pi32(r3,r5);
}

#endif
