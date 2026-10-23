// This file is distributed under a BSD license. See LICENSE.txt for details.

// kexport: converts a werkkzeug3 project (.k) into the .kx file the demo
// player loads, like "File/Export" in the editor, without the editor's GUI.

#include "werkkzeug.hpp"
#include "winpara.hpp"
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************/
/***                                                                      ***/
/***   Editor functions the document model calls, without a GUI           ***/
/***                                                                      ***/
/****************************************************************************/

static sInt BugCount;

void WinFindResults::Clear()                    { BugCount = 0; }
void WinFindResults::Update()                   {}
sInt WinFindResults::Count()                    { return BugCount; }

void WinFindResults::Add(WerkOp *op,sChar *text)
{
  BugCount++;
  fprintf(stderr,"  %s: %s\n",op ? op->Name : "?",text ? text : "");
}

void WinPagelist::UpdateList()                  {}
void WinPara::SetOp(WerkOp *op)                 {}

void WerkkzeugApp::FindResults(sBool show)      {}
void WerkkzeugApp::MusicStart(sInt mode)        {}
void WerkkzeugApp::MusicLoad()                  {}

/****************************************************************************/
/***                                                                      ***/
/***   Main                                                               ***/
/***                                                                      ***/
/****************************************************************************/

static void Usage()
{
  fprintf(stderr,
    "usage: kexport [--song file] input.k output.kx\n"
    "  --song file   music to embed (default: the song file named in the project,\n"
    "                looked up next to the project file)\n");
  exit(1);
}

// project files store Windows paths: try the name as is, then its file
// part next to the project file
static void FindSong(WerkDoc *doc,const sChar *project)
{
  sChar path[1024];
  sChar *name;
  const sChar *slash;

  if(!doc->SongName[0] || sSystem->CheckFile(doc->SongName))
    return;

  name = doc->SongName;
  for(sChar *p=doc->SongName;*p;p++)
    if(*p=='\\' || *p=='/')
      name = p+1;

  slash = strrchr(project,'/');
  if(slash)
    snprintf(path,sizeof(path),"%.*s/%s",(int)(slash-project),project,name);
  else
    snprintf(path,sizeof(path),"%s",name);

  if(sSystem->CheckFile(path))
    sCopyString(doc->SongName,path,sizeof(doc->SongName));
}

int main(int argc,char **argv)
{
  const char *song = 0,*in = 0,*out = 0;
  sU8 *file,*data,*start;
  sU32 *read;
  sInt size;

  for(sInt i=1;i<argc;i++)
  {
    if(!strcmp(argv[i],"--song") && i+1<argc)
      song = argv[++i];
    else if(argv[i][0]=='-')
      Usage();
    else if(!in)
      in = argv[i];
    else if(!out)
      out = argv[i];
    else
      Usage();
  }
  if(!in || !out)
    Usage();

  // system and editor objects (no window, no device)
  sSystem = new sSystem_;
  sSetMem(sSystem,0,sizeof(sSystem_));
  sInitTypes();
  sBroker = new sBroker_;

  WerkkzeugApp *app = (WerkkzeugApp *) calloc(1,sizeof(WerkkzeugApp));
  WinPara *para = (WinPara *) calloc(1,sizeof(WinPara));
  para->App = app;
  app->ParaWin = para;
  app->FindResultsWin = (WinFindResults *) calloc(1,sizeof(WinFindResults));
  app->PagelistWin = (WinPagelist *) calloc(1,sizeof(WinPagelist));
  app->DefaultTexSize = 0x909;
  app->TextureMode = 0;
  sCopyString(app->UserName,"kexport",sizeof(app->UserName));

  WerkDoc *doc = new WerkDoc;
  doc->App = app;
  app->Doc = doc;

  // load
  file = sSystem->LoadFile(in,size);
  if(!file)
  {
    fprintf(stderr,"kexport: can't read %s\n",in);
    return 1;
  }
  read = (sU32 *) file;
  if(*read==sMAKE4('n','o','v','a'))
    read += 2;
  if(!doc->Read(read))
  {
    fprintf(stderr,"kexport: %s is not a werkkzeug3 project (or a newer version)\n",in);
    return 1;
  }
  doc->Connect();

  if(song)
    sCopyString(doc->SongName,song,sizeof(doc->SongName));
  else
    FindSong(doc,in);
  printf("song: %s\n",doc->SongName[0] ? doc->SongName : "(none)");

  // export
  WerkExport wexport;
  start = data = new sU8[64*1024*1024];
  if(!wexport.Export(doc,data))
  {
    fprintf(stderr,"kexport: export failed\n");
    return 1;
  }
  if(!sSystem->SaveFile(out,start,(sInt)(data-start)))
  {
    fprintf(stderr,"kexport: can't write %s\n",out);
    return 1;
  }
  printf("%s: %d bytes\n",out,(sInt)(data-start));

  // operator classes the player needs (KHandlers in its op list)
  printf("operator classes:");
  for(sInt i=0;i<wexport.OpTypeCount;i++)
  {
    WerkClass *cls = FindWerkClass(wexport.OpTypes[i]);
    printf(" %02x(%s)",wexport.OpTypes[i],cls ? cls->Name : "?");
  }
  printf("\n");

  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Unused parts of the system layer                                   ***/
/***                                                                      ***/
/****************************************************************************/

sGuiManager *sGui;

sBool sAppHandler(sInt code,sDInt value)          { return sFALSE; }
sBool sSystem_::ProgressGDI(sInt value)          { return sFALSE; }

sInt sSystem_::SamplePlay(sInt handle,sF32 volume,sF32 pan,sInt freq) { return 0; }
void sSystem_::Sample3DParam(sInt handle,sInt buffer,const sVector &pos,const sVector &vel,sF32 minDist,sF32 maxDist) {}
void sSystem_::Sample3DListener(const sVector &pos,const sVector &vel,const sVector &up,const sVector &fwd,sF32 doppler,sF32 rolloff) {}

sHostFont::sHostFont()                                         { prv = 0; Height = Advance = Baseline = 0; }
sHostFont::~sHostFont()                                        {}
void sHostFont::Init(sChar *name,sInt height,sInt width,sInt style) {}
sBool sHostFont::Prepare(sBitmap *bm,sChar *letters,sHostFontLetter *met) { return sFALSE; }
sBool sHostFont::PrepareCheck(sInt xs,sInt ys,sChar *letters)  { return sFALSE; }

sBool SavePNG(const sChar *filename,const sU8 *data,sInt xSize,sInt ySize) { return sFALSE; }

// V2 songs need the synth (x86 assembler, not part of this build); .ogg songs are embedded as is
#include "_viruz2.hpp"
sU8 *sViruz2::ConvertV2M(const sU8 *data,sInt size,sInt &outSize)
{
  fprintf(stderr,"kexport: V2 songs are not supported, use an .ogg file\n");
  exit(1);
}
