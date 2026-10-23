// Linux/GCC compatibility shim for the werkkzeug3 codebase (MSVC, x86-32).
// Force-included before every translation unit (-include linux/compat.hpp).

#pragma once

#if !defined(__i386__)
#error "werkkzeug3 assumes a 32-bit x86 target: build with -m32"
#endif

#define sLINUX 1

#define __w64
#define __int64 long long
#define __cdecl __attribute__((cdecl))
#define __stdcall __attribute__((stdcall))
#define _stdcall __attribute__((stdcall))
#define __fastcall __attribute__((fastcall))
#define __forceinline inline __attribute__((always_inline))
#define __declspec(x) __declspec_##x
#define __declspec_noreturn __attribute__((noreturn))
#define __declspec_naked __attribute__((naked))
#define __declspec_align(n) __attribute__((aligned(n)))
#define __declspec_novtable
#define __declspec_selectany __attribute__((weak))
#define __declspec_dllimport
#define __declspec_dllexport

#define _alloca __builtin_alloca

#include <stddef.h>
#include <string.h>
#include <mmintrin.h>
#include <xmmintrin.h>

#include <stdlib.h>
#include <malloc.h>
static inline void *_aligned_malloc(size_t size,size_t align) { return memalign(align,size); }
static inline void _aligned_free(void *p) { free(p); }
#include <stdint.h>
#define __int32 int32_t
#define __int16 int16_t
#define __int8 int8_t
#define __assume(x) ((void)0)
#define _ecvt ecvt
#define _fcvt fcvt
