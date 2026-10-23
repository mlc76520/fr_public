// This file is distributed under a BSD license. See LICENSE.txt for details.

// GCC versions of the MMX inline assembly in genbitmap.cpp, written with MMX
// intrinsics that map 1:1 to the original instructions so the generated
// textures stay bit-exact. Included from genbitmap.cpp only.

static inline __m64 M64(sU64 v)             { __m64 r; sCopyMem(&r,&v,8); return r; }
static inline __m64 Ld64(const void *p)     { __m64 r; sCopyMem(&r,p,8); return r; }
static inline void St64(void *p,__m64 v)    { sCopyMem(p,&v,8); }

void Fade64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  __m64 mm0,mm1,mm2,mm3;

  mm2 = Ld64(&c0);
  mm3 = Ld64(&c1);

  mm0 = _mm_cvtsi32_si64(fade);
  mm0 = _mm_unpacklo_pi32(mm0,mm0);
  mm0 = _mm_srai_pi32(mm0,1);
  mm0 = _mm_xor_si64(mm0,M64(0xffffffff00000000ULL));
  mm0 = _mm_add_pi32(mm0,M64(0x0000800100000000ULL));
  mm0 = _mm_packs_pi32(mm0,mm0);
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm1 = mm0;
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm1 = _mm_unpackhi_pi16(mm1,mm1);

  mm3 = _mm_mulhi_pi16(mm3,mm0);
  mm2 = _mm_mulhi_pi16(mm2,mm1);
  mm2 = _mm_add_pi16(mm2,mm3);
  mm2 = _mm_slli_pi16(mm2,1);
  St64(&r,mm2);

  _mm_empty();
}

void AddScale64(sU64 &r,sU64 &c0,sU64 &c1,sInt fade)
{
  __m64 mm0,mm2,mm3;

  mm2 = Ld64(&c0);
  mm3 = Ld64(&c1);

  mm0 = _mm_cvtsi32_si64(fade);
  mm0 = _mm_srai_pi32(mm0,1);
  mm0 = _mm_packs_pi32(mm0,mm0);
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm0 = _mm_unpacklo_pi16(mm0,mm0);

  mm3 = _mm_mulhi_pi16(mm3,mm0);
  mm3 = _mm_slli_pi16(mm3,1);
  mm2 = _mm_adds_pi16(mm2,mm3);
  St64(&r,mm2);

  _mm_empty();
}

void BilinearFilter(BilinearContext *ctx,sU64 *r,sInt u,sInt v)
{
  const __m64 adjust = M64(0x8000800080008000ULL);
  __m64 mm0,mm1,mm2,mm3,mm4,mm5;
  sU32 eax,ebx,edx,ecx;
  const sU8 *s0,*s1;

  // clamp u, v
  eax = u & ctx->XAm;
  ebx = v & ctx->YAm;
  if(eax > (sU32) ctx->XMax1)
    eax = (~(sU32)((sInt)eax>>31)) & ctx->XMax1;
  if(ebx > (sU32) ctx->YMax1)
    ebx = (~(sU32)((sInt)ebx>>31)) & ctx->YMax1;

  mm0 = _mm_cvtsi32_si64(eax);
  mm1 = _mm_cvtsi32_si64(ebx);

  // calc s0, s1, u0, u1
  eax = (eax >> 13) & ~7u;                          // u0 (pixel byte offset)
  ebx >>= 16;                                       // v0 (line)
  edx = (ebx+1) & ctx->YSize1;
  s0 = (const sU8 *) ctx->Src + (ebx << ctx->XShift);
  s1 = (const sU8 *) ctx->Src + (edx << ctx->XShift);
  ecx = (eax+8) & ctx->XSize1;                      // u1

  // load pixels
  mm2 = Ld64(s0+eax);
  mm3 = Ld64(s0+ecx);
  mm4 = Ld64(s1+eax);
  mm5 = Ld64(s1+ecx);

  // actual sample
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm1 = _mm_unpacklo_pi16(mm1,mm1);
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm1 = _mm_unpacklo_pi16(mm1,mm1);
  mm0 = _mm_srli_pi16(mm0,1);
  mm1 = _mm_srli_pi16(mm1,1);

  // horiz
  mm3 = _mm_sub_pi16(mm3,mm2);
  mm5 = _mm_sub_pi16(mm5,mm4);
  mm3 = _mm_mulhi_pi16(mm3,mm0);
  mm5 = _mm_mulhi_pi16(mm5,mm0);
  mm3 = _mm_slli_pi16(mm3,1);
  mm5 = _mm_slli_pi16(mm5,1);
  mm2 = _mm_add_pi16(mm2,mm3);
  mm4 = _mm_add_pi16(mm4,mm5);

  // vert+write
  mm4 = _mm_sub_pi16(mm4,mm2);
  mm2 = _mm_add_pi16(mm2,adjust);
  mm4 = _mm_mulhi_pi16(mm4,mm1);
  mm4 = _mm_slli_pi16(mm4,1);
  mm2 = _mm_add_pi16(mm2,mm4);
  mm2 = _mm_subs_pu16(mm2,adjust);
  St64(r,mm2);

  _mm_empty();
}

// gray value of the 3 color channels (shared by "alpha" and "gray color")
static inline __m64 BitmapInnerGray(__m64 mm0)
{
  __m64 mm6 = _mm_srli_si64(mm0,16);
  __m64 mm7 = _mm_srli_si64(mm0,32);
  mm0 = _mm_add_pi16(mm0,mm6);
  mm7 = _mm_add_pi16(mm7,mm6);
  mm0 = _mm_srli_pi16(mm0,1);
  mm7 = _mm_srli_pi16(mm7,1);
  return _mm_add_pi16(mm0,mm7);
}

void __stdcall Bitmap_Inner(sU64 *d,sU64 *s,sInt count,sInt mode,sU64 *x=0)
{
  const __m64 mm3c = M64(0x7fff7fff7fff7fffULL);
  const __m64 mm4c = M64(0x00007fff7fff7fffULL);
  const __m64 mm5c = M64(0x7fff000000000000ULL);
  const __m64 mm6c = _mm_setzero_si64();
  const __m64 mm7c = M64(0x3fff3fff3fff3fffULL);
  const __m64 color = Ld64(&s[0]);                // mm2 = data[0]
  __m64 mm0,mm1,mm2,mm4,mm5,mm7;
  __m64 range = mm7c,zero = mm6c;
  sInt i;

  if(mode<0)
    mode = 0;
  if(mode>=0x15)                                  // color range setup
  {
    range = _mm_subs_pi16(Ld64(&s[1]),color);
    zero = _mm_setzero_si64();
    mode = 0x15;
  }

  for(i=0;i<count;i++)
  {
    switch(mode)
    {
    case 0x00:                    // add
      mm0 = _mm_adds_pi16(Ld64(&x[i]),Ld64(&s[i]));
      break;
    case 0x01:                    // sub
      mm0 = _mm_subs_pu16(Ld64(&x[i]),Ld64(&s[i]));
      break;
    case 0x02:                    // mul
      mm0 = _mm_slli_pi16(_mm_mulhi_pi16(Ld64(&x[i]),Ld64(&s[i])),1);
      break;
    case 0x03:                    // diff
      mm0 = _mm_sub_pi16(Ld64(&x[i]),Ld64(&s[i]));
      mm0 = _mm_add_pi16(mm0,mm3c);
      mm0 = _mm_srli_pi16(mm0,1);
      break;
    case 0x04:                    // alpha
      mm0 = BitmapInnerGray(Ld64(&s[i]));
      mm1 = _mm_and_si64(Ld64(&x[i]),mm4c);
      mm0 = _mm_slli_si64(mm0,47);
      mm0 = _mm_and_si64(mm0,mm5c);
      mm0 = _mm_or_si64(mm0,mm1);
      break;
    case 0x05:                    // mul color
      mm0 = _mm_slli_pi16(_mm_mulhi_pi16(Ld64(&x[i]),color),1);
      break;
    case 0x06:                    // add color
      mm0 = _mm_adds_pi16(Ld64(&x[i]),color);
      break;
    case 0x07:                    // sub color
      mm0 = _mm_subs_pu16(Ld64(&x[i]),color);
      break;
    case 0x08:                    // gray color
      mm0 = BitmapInnerGray(Ld64(&x[i]));
      mm0 = _mm_srli_pi16(mm0,1);
      mm0 = _mm_unpacklo_pi16(mm0,mm0);
      mm0 = _mm_unpacklo_pi16(mm0,mm0);
      mm0 = _mm_or_si64(mm0,mm5c);
      break;
    case 0x09:                    // invert color
      mm0 = _mm_xor_si64(Ld64(&x[i]),mm3c);
      break;
    case 0x0a:                    // scale color
      mm0 = Ld64(&x[i]);
      mm1 = mm0;
      mm0 = _mm_mullo_pi16(mm0,color);
      mm1 = _mm_mulhi_pi16(mm1,color);
      mm7 = mm0;
      mm0 = _mm_unpacklo_pi16(mm0,mm1);
      mm7 = _mm_unpackhi_pi16(mm7,mm1);
      mm0 = _mm_srli_pi32(mm0,11);
      mm7 = _mm_srli_pi32(mm7,11);
      mm0 = _mm_packs_pi32(mm0,mm7);
      break;
    case 0x0b:                    // merge
      mm0 = Ld64(&d[i]);
      mm4 = Ld64(&s[i]);
      mm5 = Ld64(&x[i]);
      mm1 = _mm_subs_pi16(mm3c,mm0);
      mm0 = _mm_mulhi_pi16(mm0,mm4);
      mm1 = _mm_mulhi_pi16(mm1,mm5);
      mm0 = _mm_adds_pi16(mm0,mm1);
      mm0 = _mm_slli_pi16(mm0,1);
      break;
    case 0x0c:                    // brightness
      mm0 = Ld64(&s[i]);
      mm1 = Ld64(&x[i]);
      mm2 = _mm_cmpgt_pi16(mm0,mm7c);
      mm2 = _mm_srli_pi16(mm2,1);
      mm0 = _mm_xor_si64(mm0,mm2);
      mm1 = _mm_xor_si64(mm1,mm2);
      mm0 = _mm_mulhi_pi16(mm0,mm1);
      mm0 = _mm_slli_pi16(mm0,2);
      mm0 = _mm_xor_si64(mm0,mm2);
      break;
    case 0x0d:                    // subr
      mm0 = _mm_subs_pu16(Ld64(&s[i]),Ld64(&x[i]));
      break;
    case 0x0e:                    // mulmerge
      mm0 = Ld64(&d[i]);
      mm4 = Ld64(&s[i]);
      mm5 = Ld64(&x[i]);
      mm4 = _mm_mulhi_pi16(mm4,mm5);
      mm4 = _mm_slli_pi16(mm4,1);
      mm1 = _mm_subs_pi16(mm3c,mm0);
      mm0 = _mm_mulhi_pi16(mm0,mm4);
      mm1 = _mm_mulhi_pi16(mm1,mm5);
      mm0 = _mm_adds_pi16(mm0,mm1);
      mm0 = _mm_slli_pi16(mm0,1);
      break;
    case 0x0f:                    // sharpen
      {
        __m64 pix = Ld64(&x[i]);
        mm0 = _mm_subs_pu16(pix,Ld64(&d[i]));
        mm1 = mm0;
        mm0 = _mm_mullo_pi16(mm0,color);
        mm1 = _mm_mulhi_pi16(mm1,color);
        mm7 = mm0;
        mm0 = _mm_unpacklo_pi16(mm0,mm1);
        mm7 = _mm_unpackhi_pi16(mm7,mm1);
        mm0 = _mm_srai_pi32(mm0,11);
        mm7 = _mm_srai_pi32(mm7,11);
        mm0 = _mm_packs_pi32(mm0,mm7);
        mm0 = _mm_adds_pi16(mm0,pix);
        mm0 = _mm_max_pi16(mm0,mm6c);
      }
      break;
    case 0x10:                    // hardlight
      mm1 = _mm_slli_pi16(Ld64(&s[i]),1);
      mm0 = Ld64(&x[i]);
      mm2 = mm1;
      mm1 = _mm_and_si64(mm1,mm3c);
      mm2 = _mm_srai_pi16(mm2,15);
      mm4 = mm0;
      mm0 = _mm_mulhi_pi16(mm0,mm1);
      mm0 = _mm_slli_pi16(mm0,1);
      mm1 = _mm_add_pi16(mm1,mm4);
      mm0 = _mm_xor_si64(mm0,mm2);
      mm1 = _mm_and_si64(mm1,mm2);
      mm0 = _mm_add_pi16(mm0,mm1);
      break;
    case 0x11:                    // over
      mm0 = Ld64(&x[i]);
      mm1 = Ld64(&s[i]);
      mm2 = mm1;
      mm1 = _mm_subs_pi16(mm1,mm0);
      mm2 = _mm_unpackhi_pi16(mm2,mm2);
      mm2 = _mm_unpackhi_pi16(mm2,mm2);
      mm1 = _mm_mulhi_pi16(mm1,mm2);
      mm1 = _mm_slli_pi16(mm1,1);
      mm0 = _mm_adds_pi16(mm0,mm1);
      mm0 = _mm_max_pi16(mm0,mm6c);
      break;
    case 0x12:                    // addsmooth (screen)
      mm0 = _mm_xor_si64(Ld64(&x[i]),mm3c);
      mm1 = _mm_xor_si64(Ld64(&s[i]),mm3c);
      mm0 = _mm_mulhi_pi16(mm0,mm1);
      mm0 = _mm_slli_pi16(mm0,1);
      mm0 = _mm_xor_si64(mm0,mm3c);
      break;
    case 0x13:                    // min
      mm0 = _mm_min_pi16(Ld64(&x[i]),Ld64(&s[i]));
      break;
    case 0x14:                    // max
      mm0 = _mm_max_pi16(Ld64(&x[i]),Ld64(&s[i]));
      break;
    default:                      // color range
      mm0 = _mm_mulhi_pi16(Ld64(&x[i]),range);
      mm0 = _mm_slli_pi16(mm0,1);
      mm0 = _mm_adds_pi16(mm0,color);
      mm0 = _mm_max_pi16(mm0,zero);
      break;
    }
    St64(&d[i],mm0);
  }

  _mm_empty();
}

static void BlurCore(sU16 *p,sU16 *q,sInt sizek,sInt xRes,sInt f0,sInt f1,sInt amp,sInt ampc)
{
  __m64 f0f1,amplo,amphi,ampclip;
  __m64 mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7;
  const sU8 *src = (const sU8 *) p;
  sU8 *dst = (sU8 *) q;
  sU32 eax,edx,mask;
  sInt ecx;

  f0f1 = _mm_unpacklo_pi16(_mm_cvtsi32_si64(f0),_mm_cvtsi32_si64(f1));
  f0f1 = _mm_unpacklo_pi32(f0f1,f0f1);

  mm0 = _mm_cvtsi32_si64(ampc);
  mm1 = _mm_cvtsi32_si64(amp);
  mm1 = _mm_unpacklo_pi16(mm1,mm1);
  ampclip = _mm_unpacklo_pi32(mm0,mm0);
  amplo = _mm_unpacklo_pi16(mm1,mm1);
  amphi = _mm_unpackhi_pi16(mm1,mm1);

  mm2 = _mm_cvtsi32_si64((sU16)(f1-f0));
  mm2 = _mm_unpacklo_pi32(mm2,mm2);

  mask = (xRes-1)<<3;
  eax = ((-((sizek+1)>>1))<<3) & mask;
  edx = eax;

  // seed first pixel
  mm0 = Ld64(src+edx);
  mm1 = mm0;
  mm0 = _mm_unpacklo_pi16(mm0,mm0);
  mm1 = _mm_unpackhi_pi16(mm1,mm1);
  mm0 = _mm_madd_pi16(mm0,mm2);
  mm1 = _mm_madd_pi16(mm1,mm2);

  // init loop
  for(ecx=sizek;ecx>0;ecx--)
  {
    mm2 = Ld64(src+eax);
    eax = (eax+8) & mask;
    mm3 = Ld64(src+eax);
    mm4 = mm2;
    mm2 = _mm_unpacklo_pi16(mm2,mm3);
    mm4 = _mm_unpackhi_pi16(mm4,mm3);
    mm2 = _mm_madd_pi16(mm2,f0f1);
    mm4 = _mm_madd_pi16(mm4,f0f1);
    mm0 = _mm_add_pi32(mm0,mm2);
    mm1 = _mm_add_pi32(mm1,mm4);
  }

  // main loop
  for(ecx=xRes;ecx>0;ecx--)
  {
    mm2 = Ld64(src+eax);
    eax = (eax+8) & mask;
    mm3 = Ld64(src+eax);
    mm4 = mm2;
    mm2 = _mm_unpacklo_pi16(mm2,mm3);     // mix1
    mm4 = _mm_unpackhi_pi16(mm4,mm3);     // mix2
    mm2 = _mm_madd_pi16(mm2,f0f1);
    mm4 = _mm_madd_pi16(mm4,f0f1);
    mm0 = _mm_add_pi32(mm0,mm2);
    mm1 = _mm_add_pi32(mm1,mm4);

    mm2 = _mm_srai_pi32(mm0,6);
    mm3 = _mm_srai_pi32(mm1,6);
    mm6 = _mm_cmpgt_pi32(mm0,ampclip);
    mm7 = _mm_cmpgt_pi32(mm1,ampclip);
    mm4 = mm2;
    mm2 = _mm_unpacklo_pi16(mm2,mm3);
    mm4 = _mm_unpackhi_pi16(mm4,mm3);
    mm3 = mm2;
    mm2 = _mm_unpacklo_pi16(mm2,mm4);     // mm2=aklo
    mm3 = _mm_unpackhi_pi16(mm3,mm4);     // mm3=akhi
    mm6 = _mm_packs_pi32(mm6,mm7);        // mm6=out
    mm7 = _mm_slli_pi16(_mm_cmpeq_pi8(mm7,mm7),15);
    mm4 = _mm_mullo_pi16(amphi,mm2);      // mm4=aklo*amphi
    mm3 = _mm_mullo_pi16(mm3,amplo);      // mm3=akhi*amplo
    mm2 = _mm_mulhi_pu16(mm2,amplo);      // mm2=aklo*amplo
    mm6 = _mm_adds_pu16(mm6,mm3);         // out+=akhi*amplo
    mm3 = Ld64(src+edx);                  // pix1
    edx = (edx+8) & mask;
    mm6 = _mm_adds_pu16(mm6,mm4);         // out+=aklo*amphi
    mm6 = _mm_adds_pu16(mm6,mm2);         // out+=aklo*amplo
    mm2 = Ld64(src+edx);                  // pix2
    mm6 = _mm_add_pi16(mm6,mm7);          // out+=add
    mm4 = mm2;
    mm6 = _mm_subs_pi16(mm6,mm7);         // out-=add
    mm2 = _mm_unpacklo_pi16(mm2,mm3);     // mix1
    mm4 = _mm_unpackhi_pi16(mm4,mm3);     // mix2
    mm2 = _mm_madd_pi16(mm2,f0f1);
    mm4 = _mm_madd_pi16(mm4,f0f1);
    St64(dst,mm6);                        // *(q+d) = out
    dst += 8;
    mm0 = _mm_sub_pi32(mm0,mm2);
    mm1 = _mm_sub_pi32(mm1,mm4);
  }

  _mm_empty();
}
