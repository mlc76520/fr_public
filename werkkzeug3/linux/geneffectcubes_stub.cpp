// This file is distributed under a BSD license. See LICENSE.txt for details.

// Stand-in for werkkzeug3/geneffectcubes.cpp, whose shaders need the Windows-only
// "asc" HLSL compiler. These 2006 effects are not used by theta (2004).

#include "geneffectcubes.hpp"

sVector __declspec(align(16)) CubeBuffer[0x10100*2];
sInt CubeCount;

KObject * __stdcall Init_Effect_Cubes(KOp *op) { return 0; }
void __stdcall Exec_Effect_Cubes(KOp *op,KEnvironment *kenv) {}

KObject * __stdcall Init_Effect_BounceWorm(KOp *op) { return 0; }
void __stdcall Exec_Effect_BounceWorm(KOp *op,KEnvironment *kenv,sInt,sF32,sF32,sF32,sF32,sInt,sInt) {}

KObject * __stdcall Init_Effect_Wirbeln(KOp *op) { return 0; }
void __stdcall Exec_Effect_Wirbeln(KOp *op,KEnvironment *kenv,sF32,sF32,sF32,sF32,sF32,sF32,sInt,sInt,sF32,sF32,sF32,sInt) {}

KObject * __stdcall Init_Effect_XSI2Cube(KOp *op) { return 0; }
void __stdcall Exec_Effect_XSI2Cube(KOp *op,KEnvironment *kenv) {}

KObject * __stdcall Init_Effect_CubeMorpher(KOp *op) { return 0; }
void __stdcall Exec_Effect_CubeMorpher(KOp *op,KEnvironment *kenv) {}
