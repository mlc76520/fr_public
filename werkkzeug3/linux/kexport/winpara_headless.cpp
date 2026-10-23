// This file is distributed under a BSD license. See LICENSE.txt for details.

// The parameter window of the editor, reduced to what the exporter needs:
// operators set their default parameters by running their edit handler with
// WerkOp::SetDefaults set, which makes these calls write the defaults instead
// of building controls (see the "else" branches in ../../winpara.cpp).

#include "werkkzeug.hpp"
#include "winpara.hpp"
#include "genblobspline.hpp"

void WinPara::EditOp(WerkOp *op)
{
  sVERIFY(op->SetDefaults);
  Line = 0;
  Op = op;
  InfoCount = 0;
  GrayLabels = 0;
  NextGroup = 0;
}

void WinPara::Label(const sChar *label)                               {}
void WinPara::AddSpecial(const sChar *name,sInt nr)                   {}
sControl *WinPara::AddBox(sU32 cmd,sInt pos,sInt offset,sChar *name) { return 0; }
void WinPara::EditGroup(const sChar *name)                            { NextGroup = name; }
void WinPara::EditSpacer(const sChar *name)                           {}
void WinPara::EditSetGray(sBool g)                                    { GrayLabels = g; }

void WinPara::DefaultF(sInt offset,sControl *con,sF32 x)
{
  *Op->Op.GetEditPtrF(offset+0) = x;
}

void WinPara::DefaultF2(sInt offset,sControl *con,sF32 x,sF32 y)
{
  *Op->Op.GetEditPtrF(offset+0) = x;
  *Op->Op.GetEditPtrF(offset+1) = y;
}

void WinPara::DefaultF3(sInt offset,sControl *con,sF32 x,sF32 y,sF32 z)
{
  *Op->Op.GetEditPtrF(offset+0) = x;
  *Op->Op.GetEditPtrF(offset+1) = y;
  *Op->Op.GetEditPtrF(offset+2) = z;
}

sControl *WinPara::EditString(const sChar *name,sInt index)
{
  Op->Op.SetString(index,"");
  return 0;
}

sControl *WinPara::EditFileName(const sChar *name,sInt index,const sChar *ext)
{
  Op->Op.SetString(index,"");
  return 0;
}

sTextControl *WinPara::EditText(const sChar *name,sInt index)
{
  Op->Op.SetString(index,"");
  return 0;
}

sControl *WinPara::EditSpline(const sChar *name,sInt index)                 { return 0; }
sControl *WinPara::EditLink(const sChar *name,sInt offset)                  { return 0; }
sControl *WinPara::EditLinkI(const sChar *name,sInt offset,sInt offsetflags) { return 0; }

sControl *WinPara::EditInt(const sChar *name,sInt offset,sInt zones,sInt def,sInt min,sInt max,sF32 step)
{
  for(sInt i=0;i<zones;i++)
    *Op->Op.GetEditPtrS(offset+i) = def;
  return 0;
}

sControl *WinPara::EditFloat(const sChar *name,sInt offset,sInt zones,sF32 def,sF32 min,sF32 max,sF32 step)
{
  for(sInt i=0;i<zones;i++)
    *Op->Op.GetEditPtrF(offset+i) = def;
  return 0;
}

sControl *WinPara::EditAngles(const sChar *name,sInt offset,sInt zones,sF32 def,sF32 min,sF32 max,sF32 step)
{
  return EditFloat(name,offset,zones,def,min,max,step);
}

sControl *WinPara::EditPos2D(const sChar *name,sInt offset,sInt zones,sF32 def,sF32 min,sF32 max,sF32 step)
{
  return EditFloat(name,offset,zones,def,min,max,step);
}

sControl *WinPara::EditRGBA(const sChar *name,sInt offset,sU32 def)
{
  *Op->Op.GetEditPtrU(offset) = def;
  return 0;
}

sControl *WinPara::EditCycle(const sChar *name,sInt offset,sInt def,sChar *cycle)
{
  *Op->Op.GetEditPtrU(offset) = def;
  return 0;
}

sControl *WinPara::EditBool(const sChar *name,sInt offset,sInt def)
{
  *Op->Op.GetEditPtrU(offset) = def;
  return 0;
}

sControl *WinPara::EditMask(const sChar *name,sInt offset,sInt def,sInt zones,sChar *picks)
{
  *Op->Op.GetEditPtrU(offset) = def;
  return 0;
}

sControl *WinPara::EditFlags(const sChar *name,sInt offset,sInt def,sChar *cycle)
{
  *Op->Op.GetEditPtrU(offset) = def;
  return 0;
}

BlobSpline *WinPara::GetBlobSpline()
{
  if(!Op) return 0;
  return (BlobSpline *) Op->Op.GetBlobData();
}

BlobPipe *WinPara::GetBlobPipe()
{
  if(!Op) return 0;
  return (BlobPipe *) Op->Op.GetBlobData();
}

void WinPara::EditBlobSpline()
{
  if(!Op) return;
  BlobSpline *spline = GetBlobSpline();
  if(!spline)
  {
    sInt bytes;
    spline = MakeDummyBlobSpline(&bytes);
    Op->Op.SetBlob((sU8 *)spline,bytes);
  }
  spline->SetSelect(spline->Select);
}

void WinPara::EditBlobPipe()
{
  if(!Op) return;
  if(!GetBlobPipe())
  {
    sInt bytes;
    BlobPipe *pipe = MakeDummyBlobPipe(&bytes);
    Op->Op.SetBlob((sU8 *)pipe,bytes);
  }
}

// the animation op table, cut out of ../../winpara.cpp at build time
#include "winpara_animops.inc"
