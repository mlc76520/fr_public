// This file is distributed under a BSD license. See LICENSE.txt for details.

// Linux/SDL2 replacement for the Win32 parts of _start.cpp: entry point,
// window and message loop, keyboard, timing, sound (in place of DirectSound),
// files and image decoding. Included from _start.cpp; Direct3D 9 itself is
// provided by DXVK-native, which renders into the SDL window.
//
// The small Win32 look-alikes below (timeGetTime, GetAsyncKeyState, ...) let
// the platform independent parts of _start.cpp compile unchanged.

#include <cpuid.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************/
/***                                                                      ***/
/***   Win32 look-alikes                                                  ***/
/***                                                                      ***/
/****************************************************************************/

#define VK_SPACE    0x20
#define VK_ESCAPE   0x1b
#define VK_PAUSE    0x13

#define GWL_STYLE   (-16)
#define WS_POPUP    0x80000000
#define WS_VISIBLE  0x10000000
#define SW_RESTORE  9
#define SW_MAXIMIZE 3
#define SW_MINIMIZE 6

#define MB_OK                 0
#define MB_TASKMODAL          0
#define MB_SETFOREGROUND      0
#define MB_TOPMOST            0
#define MB_ICONERROR          0

#define _CrtSetDbgFlag(x)     0
#define _CrtCheckMemory()

static volatile sBool WQuit;              // message loop is done
static sU8 KeyPressedSinceQuery[256];     // for GetAsyncKeyState() bit 0

static sU32 timeGetTime()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (sU32)(ts.tv_sec*1000ULL + ts.tv_nsec/1000000);
}

static void Sleep(sU32 ms)                { SDL_Delay(ms); }
static void ExitProcess(sU32 code)        { SDL_Quit(); _exit(code); }
static void OutputDebugString(const sChar *s) { fputs(s,stderr); }

static sInt MessageBox(void *,const sChar *text,const sChar *caption,sU32)
{
  fprintf(stderr,"%s: %s\n",caption,text);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,caption,text,SDLWindow);
  return 0;
}

static SDL_Scancode VKToScancode(sInt vk)
{
  switch(vk)
  {
  case VK_SPACE:  return SDL_SCANCODE_SPACE;
  case VK_ESCAPE: return SDL_SCANCODE_ESCAPE;
  case VK_PAUSE:  return SDL_SCANCODE_PAUSE;
  }
  return SDL_SCANCODE_UNKNOWN;
}

// bit 15: key is down, bit 0: key was pressed since the last call
static sInt GetAsyncKeyState(sInt vk)
{
  SDL_Scancode sc = VKToScancode(vk);
  sInt result = 0;

  SDL_PumpEvents();
  if(sc!=SDL_SCANCODE_UNKNOWN && SDL_GetKeyboardState(0)[sc])
    result |= 0x8000;
  if(KeyPressedSinceQuery[vk&255])
    result |= 1;
  KeyPressedSinceQuery[vk&255] = 0;

  return result;
}

static BOOL GetClientRect(HWND win,RECT *r)
{
  int w,h;
  SDL_GetWindowSize((SDL_Window *) win,&w,&h);
  r->left = r->top = 0;
  r->right = w;
  r->bottom = h;
  return TRUE;
}

static BOOL GetWindowRect(HWND win,RECT *r)
{
  int x,y,w,h;
  SDL_GetWindowPosition((SDL_Window *) win,&x,&y);
  SDL_GetWindowSize((SDL_Window *) win,&w,&h);
  r->left = x;
  r->top = y;
  r->right = x+w;
  r->bottom = y+h;
  return TRUE;
}

static BOOL MoveWindow(HWND win,sInt x,sInt y,sInt w,sInt h,BOOL)
{
  SDL_SetWindowPosition((SDL_Window *) win,x,y);
  SDL_SetWindowSize((SDL_Window *) win,w,h);
  return TRUE;
}

static LONG SetWindowLong(HWND,sInt,LONG)  { return 0; }
static BOOL ShowWindow(HWND,sInt)          { return TRUE; }
static BOOL ClipCursor(const RECT *)       { return TRUE; }
static sBool IsFullscreen();
static sInt ShowCursor(BOOL show)          { SDL_ShowCursor(show && !IsFullscreen() ? SDL_ENABLE : SDL_DISABLE); return show ? 0 : -1; }
static BOOL DestroyWindow(HWND)            { WQuit = sTRUE; return TRUE; }
static BOOL SetWindowText(HWND win,const sChar *text) { SDL_SetWindowTitle((SDL_Window *) win,text); return TRUE; }
static BOOL IsIconic(HWND win)             { return (SDL_GetWindowFlags((SDL_Window *) win) & SDL_WINDOW_MINIMIZED) != 0; }
static BOOL IsZoomed(HWND win)             { return (SDL_GetWindowFlags((SDL_Window *) win) & SDL_WINDOW_MAXIMIZED) != 0; }

static BOOL GetCursorPos(POINT *pt)
{
  int x,y;
  SDL_GetGlobalMouseState(&x,&y);
  pt->x = x;
  pt->y = y;
  return TRUE;
}

static BOOL SetCursorPos(sInt x,sInt y)
{
  SDL_WarpMouseGlobal(x,y);
  return TRUE;
}

struct SYSTEMTIME { sU16 wHour,wMinute,wSecond; };

static void GetLocalTime(SYSTEMTIME *st)
{
  time_t t = time(0);
  struct tm tm;
  localtime_r(&t,&tm);
  st->wHour = tm.tm_hour;
  st->wMinute = tm.tm_min;
  st->wSecond = tm.tm_sec;
}

static BOOL QueryPerformanceFrequency(LARGE_INTEGER *f)
{
  f->QuadPart = 1000000000LL;
  return TRUE;
}

static BOOL QueryPerformanceCounter(LARGE_INTEGER *c)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  c->QuadPart = ts.tv_sec*1000000000LL + ts.tv_nsec;
  return TRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Startup                                                            ***/
/***                                                                      ***/
/****************************************************************************/

static void MakeCpuMask()
{
  unsigned int a,b,c,d;

  sSystem->CpuMask = 0;
  if(!__get_cpuid(1,&a,&b,&c,&d))
    return;

  if(d&(1<< 4)) sSystem->CpuMask |= sCPU_RDTSC;
  if(d&(1<<15)) sSystem->CpuMask |= sCPU_CMOVE;
  if(d&(1<<23)) sSystem->CpuMask |= sCPU_MMX;
  if(d&(1<<25)) sSystem->CpuMask |= sCPU_SSE|sCPU_MMX2;
  if(d&(1<<26)) sSystem->CpuMask |= sCPU_SSE2;
}

static sChar LinuxCmdLine[2048];
static sU32 LinuxConfigFlags = sSF_DIRECT3D|sSF_FULLSCREEN|sSF_WAITVSYNC;
static sInt LinuxConfigX = 1280;
static sInt LinuxConfigY = 720;

static void Usage(const char *exe)
{
  fprintf(stderr,
    "usage: %s [options] [file.kx]\n"
    "  -w, --window        run in a window (default: fullscreen)\n"
    "  -s, --size WxH      window size (default: 1280x720)\n"
    "  -l, --loop          loop the demo\n"
    "      --no-vsync      don't wait for vertical sync\n"
    "  -h, --help          show this help\n"
    "keys: Esc quits, F11 toggles fullscreen.\n"
    "Without a file name, <executable name>.kx next to the executable is used.\n",exe);
}

static void ParseCommandLine(int argc,char **argv)
{
  LinuxCmdLine[0] = 0;
  for(sInt i=1;i<argc;i++)
  {
    const char *a = argv[i];
    if(!strcmp(a,"-w") || !strcmp(a,"--window"))
      LinuxConfigFlags &= ~sSF_FULLSCREEN;
    else if((!strcmp(a,"-s") || !strcmp(a,"--size")) && i+1<argc)
    {
      if(sscanf(argv[++i],"%dx%d",&LinuxConfigX,&LinuxConfigY)!=2 || LinuxConfigX<64 || LinuxConfigY<64)
      {
        Usage(argv[0]);
        exit(1);
      }
    }
    else if(!strcmp(a,"-l") || !strcmp(a,"--loop"))
      IntroLoop = 1;
    else if(!strcmp(a,"--no-vsync"))
      LinuxConfigFlags &= ~sSF_WAITVSYNC;
    else if(!strcmp(a,"-h") || !strcmp(a,"--help"))
    {
      Usage(argv[0]);
      exit(0);
    }
    else if(a[0]=='-')
    {
      Usage(argv[0]);
      exit(1);
    }
    else
      sCopyString(LinuxCmdLine,a,sizeof(LinuxCmdLine));
  }
}

#ifndef sLINUX_NO_MAIN                    // the .kx exporter has its own main()
int main(int argc,char **argv)
{
  ParseCommandLine(argc,argv);
  WCmdLine = LinuxCmdLine;

  // same quality settings the release config dialog used
  IntroTexQuality = 2;
  IntroShadows = sFALSE;

  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)!=0)
  {
    fprintf(stderr,"SDL_Init failed: %s\n",SDL_GetError());
    return 1;
  }

  setenv("DXVK_WSI_DRIVER","SDL2",0);      // DXVK-native renders into our SDL window

  sSystem = new sSystem_;
  sSetMem(sSystem,0,sizeof(sSystem_));
  sSystem->ConfigFlags = LinuxConfigFlags;
  sSystem->ConfigX = LinuxConfigX;
  sSystem->ConfigY = LinuxConfigY;

  sSystem->DXD = Direct3DCreate9(D3D_SDK_VERSION);
  if(!sSystem->DXD)
    sFatal("could not initialize Direct3D 9 (DXVK): is Vulkan available?");

  if(sAppHandler(sAPPCODE_CONFIG,0))
    sSystem->InitX();

  sRelease(sSystem->DXD);
  delete sSystem;
  SDL_Quit();

  return 0;
}

#endif

/****************************************************************************/

void sSetConfig(sU32 flags,sInt xs,sInt ys)
{
  sSystem->ConfigFlags = flags;
  sSystem->ConfigX = xs;
  sSystem->ConfigY = ys;
}

/****************************************************************************/

static void PushKey(sU32 key)
{
  if(sSystem->KeyIndex < MAX_KEYBUFFER)
    sSystem->KeyBuffer[sSystem->KeyIndex++] = key;
  else
    sSystem->KeyBuffer[MAX_KEYBUFFER-1] = key;
}

static sU32 TranslateKey(const SDL_Keysym &ks)
{
  SDL_Keycode k = ks.sym;

  switch(k)
  {
  case SDLK_ESCAPE:     return sKEY_ESCAPE;
  case SDLK_RETURN:
  case SDLK_KP_ENTER:   return sKEY_ENTER;
  case SDLK_SPACE:      return sKEY_SPACE;
  case SDLK_BACKSPACE:  return sKEY_BACKSPACE;
  case SDLK_TAB:        return sKEY_TAB;
  case SDLK_LEFT:       return sKEY_LEFT;
  case SDLK_RIGHT:      return sKEY_RIGHT;
  case SDLK_UP:         return sKEY_UP;
  case SDLK_DOWN:       return sKEY_DOWN;
  }
  if(k>='a' && k<='z')                    // like the Win32 virtual key codes
    return k-'a'+'A';
  if(k>='0' && k<='9')
    return k;
  return 0;
}

static sBool IsFullscreen()
{
  return SDLWindow && (SDL_GetWindowFlags(SDLWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP)==SDL_WINDOW_FULLSCREEN_DESKTOP;
}

static void ToggleFullscreen()
{
  sBool full = !IsFullscreen();
  SDL_SetWindowFullscreen(SDLWindow,full ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  SDL_ShowCursor(full ? SDL_DISABLE : SDL_ENABLE);
}

static void HandleEvent(const SDL_Event &ev)
{
  sSystem_ *sys = sSystem;
  sU32 key;

  switch(ev.type)
  {
  case SDL_QUIT:
    PushKey(sKEY_CLOSE);
    break;

  case SDL_WINDOWEVENT:
    switch(ev.window.event)
    {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
      if(!sys->WMinimized)
      {
        sys->InitScreens();
        PushKey(sKEY_MODECHANGE);
      }
      break;
    case SDL_WINDOWEVENT_MINIMIZED:
      sys->WMinimized = 1;
      break;
    case SDL_WINDOWEVENT_RESTORED:
    case SDL_WINDOWEVENT_MAXIMIZED:
      if(sys->WMinimized)
      {
        sys->WMinimized = 0;
        sys->InitScreens();
        PushKey(sKEY_MODECHANGE);
      }
      break;
    case SDL_WINDOWEVENT_FOCUS_GAINED:
      sys->WFocus = 1;
      break;
    case SDL_WINDOWEVENT_FOCUS_LOST:
      sys->WFocus = 0;
      break;
    case SDL_WINDOWEVENT_CLOSE:
      PushKey(sKEY_CLOSE);
      break;
    }
    break;

  case SDL_KEYDOWN:
  case SDL_KEYUP:
    if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F11 && !ev.key.repeat)
    {
      ToggleFullscreen();
      break;
    }
    if(ev.type==SDL_KEYDOWN)
    {
      if(ev.key.keysym.sym==SDLK_ESCAPE) KeyPressedSinceQuery[VK_ESCAPE] = 1;
      if(ev.key.keysym.sym==SDLK_SPACE)  KeyPressedSinceQuery[VK_SPACE] = 1;
      if(ev.key.keysym.sym==SDLK_PAUSE)  KeyPressedSinceQuery[VK_PAUSE] = 1;
    }
    key = TranslateKey(ev.key.keysym);
    if(key)
      PushKey(ev.type==SDL_KEYDOWN ? key : key|sKEYQ_BREAK);
    break;

  case SDL_MOUSEMOTION:
    sys->WMouseX = ev.motion.x;
    sys->WMouseY = ev.motion.y;
    break;

  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    {
      sU32 bit = 0,qual = 0,code = 0;
      switch(ev.button.button)
      {
      case SDL_BUTTON_LEFT:   bit = 1; qual = sKEYQ_MOUSEL; code = sKEY_MOUSEL; break;
      case SDL_BUTTON_RIGHT:  bit = 2; qual = sKEYQ_MOUSER; code = sKEY_MOUSER; break;
      case SDL_BUTTON_MIDDLE: bit = 4; qual = sKEYQ_MOUSEM; code = sKEY_MOUSEM; break;
      }
      if(!bit)
        break;
      if(ev.type==SDL_MOUSEBUTTONDOWN)
      {
        sys->MouseButtons |= bit;
        sys->MouseButtonsSave |= bit;
        sys->KeyQual |= qual;
        PushKey(code|sys->KeyQual);
      }
      else
      {
        sys->MouseButtons &= ~bit;
        sys->KeyQual &= ~qual;
        PushKey(code|sKEYQ_BREAK|sys->KeyQual);
      }
    }
    break;

  case SDL_MOUSEWHEEL:
    if(ev.wheel.y>0) PushKey(sKEY_WHEELUP);
    if(ev.wheel.y<0) PushKey(sKEY_WHEELDOWN);
    break;
  }
}

static sBool MessagePump()
{
  SDL_Event ev;

  while(SDL_PollEvent(&ev))
    HandleEvent(ev);

  return !WQuit;
}

/****************************************************************************/

void sSystem_::InitX()
{
  sInt i;
  Uint32 winflags;

  MakeCpuMask();
  sInitTypes();
#if sLINK_RYGDXT
  sInitDXT();
#endif

  SingleCore = SDL_GetCPUCount()==1;

  WStartTime = timeGetTime();
  sSetRndSeed(timeGetTime()&0x7fffffff);
  sBroker = new sBroker_;
#if sLINK_UTIL
  sPerfMon = new sPerfMon_;
  sBroker->AddRoot(sPerfMon);
#endif

// init

  WDeviceLost = 0;
  WActiveCount = 0;
  WActiveMsg = 1;
  WContinuous = 1;
  WSinglestep = 0;
  WFullscreen = 0;
  WWindowedStyle = 0;
  WMinimized = 0;
  WMaximized = 0;
  WAborting = sFALSE;
  WResponse = 0;
  WConstantUpdate = sTRUE;
  WAbortKeyFlag = 0;
  WFocus = 1;

  CmdLowQuality = 0;
  CmdLowRes = 0;
  CmdShaderLevel = sPS_20;
  CmdNoRt = sFALSE;
  CmdWindowed = 0;
  CmdFullscreen = 0;

// window (D3D always runs "windowed" here, fullscreen is a borderless desktop-sized window)

  winflags = SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN;
  if(ConfigFlags & sSF_FULLSCREEN)
    winflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  SDLWindow = SDL_CreateWindow(sWindowTitle,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,ConfigX,ConfigY,winflags);
  if(!SDLWindow)
    sFatal("could not create window: %s",SDL_GetError());
  if(ConfigFlags & sSF_FULLSCREEN)
    SDL_ShowCursor(SDL_DISABLE);
  ConfigFlags &= ~(sSF_FULLSCREEN|sSF_MULTISCREEN);   // InitScreens(): windowed swapchain

  Screen[0].Window = (sDInt) SDLWindow;
  WScreenCount = 1;

  sSetMem(GeoBuffer,0,sizeof(GeoBuffer));
  sSetMem(GeoHandle,0,sizeof(GeoHandle));

  sSetMem(&PerfThis,0,sizeof(PerfThis));
  sSetMem(&PerfLast,0,sizeof(PerfLast));
  PerfThis.Time = sSystem->PerfTime();

  ZBufXSize = ZBufYSize = 0;
  ZBufFormat = 0;
  ZBuffer = 0;
  RTBuffer = 0;
  CurrentRT = 0;
  SyncQuery = 0;

  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags = 0;

  sSetMem(Setups,0,sizeof(Setups));
  sSetMem(Shaders,0,sizeof(Shaders));
  MtrlClearCaches();
  InitScreens();
  if(CmdShaderLevel>GetShaderLevel())
    CmdShaderLevel = GetShaderLevel();

  sFloatFix();                            // _control87(_PC_24|_RC_NEAR,MCW_PC|MCW_RC)

  if(!InitDS())
    sFatal("could not initialize sound: %s",SDL_GetError());

  // main loop

  DXDev->BeginScene();
  sAppHandler(sAPPCODE_INIT,0);
  DXDev->EndScene();

  while(MessagePump())
  {
    if(!WMinimized)
    {
      for(i=0;i<KeyIndex;i++)
        sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
      KeyIndex = 0;

      Render();

      PerfLast.TimeFiltered = (PerfLast.TimeFiltered*7+PerfLast.Time)/8;
      PerfLast.Line     = PerfThis.Line;
      PerfLast.Triangle = PerfThis.Triangle;
      PerfLast.Vertex   = PerfThis.Vertex;
      PerfLast.Material = PerfThis.Material;
      PerfThis.Line     = 0;
      PerfThis.Triangle = 0;
      PerfThis.Vertex   = 0;
      PerfThis.Material = 0;
#if sLINK_UTIL
      sPerfMon->Flip();
#else
      sSystem->PerfKalib();
#endif
    }
    else
    {
      for(i=0;i<KeyIndex;i++)
        sAppHandler(sAPPCODE_KEY,KeyBuffer[i]);
      KeyIndex = 0;
      Sleep(20);
    }

    MarkDS();
  }

  sAppHandler(sAPPCODE_EXIT,0);

// cleanup

#if sLINK_UTIL
  sBroker->RemRoot(sPerfMon);
#endif

  ExitDS();
  sBroker->Free();      // some objects may still hold resources, like Geometries
  sBroker->Dump();
  ExitScreens();

  for(i=0;i<MAX_TEXTURE;i++)
    Textures[i].Flags=0;

  sExitTypes();

  sBroker->Free();
  sBroker->Dump();
  delete sBroker;
  sBroker = 0;

  SDL_DestroyWindow(SDLWindow);
  SDLWindow = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Sound (SDL audio in place of the DirectSound streaming thread)     ***/
/***                                                                      ***/
/****************************************************************************/

static sInt DXSDeviceSamples;             // samples per SDL audio callback

static void SDLAudioCallback(void *,Uint8 *stream,int len)
{
  sInt samples = len/4;

  if(DXSHandler)
    (*DXSHandler)((sS16 *)stream,samples,(void *)DXSHandlerUser);
  else
    sSetMem(stream,0,len);

  DXSTotalSamples += samples;
  DXSTime = timeGetTime();
}

sBool sSystem_::InitDS()
{
  SDL_AudioSpec want,have;

  sSetMem(&want,0,sizeof(want));
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = 1024;
  want.callback = SDLAudioCallback;

  DXSHandler = 0;
  DXSDevice = SDL_OpenAudioDevice(0,0,&want,&have,0);
  if(!DXSDevice)
    return sFALSE;
  DXSDeviceSamples = have.samples;

  DXSTotalSamples = 0;
  DXSLastTotalSample = 0;
  DXSTime = timeGetTime();
  SoundTime = 1;
  SDL_PauseAudioDevice(DXSDevice,0);

  return sTRUE;
}

void sSystem_::ExitDS()
{
  if(DXSDevice)
  {
    SDL_CloseAudioDevice(DXSDevice);
    DXSDevice = 0;
  }
}

// estimate the sample that is currently audible: the audio callback runs
// about one buffer ahead of the playback position.
void sSystem_::MarkDS()
{
  sInt pos;

  SDL_LockAudioDevice(DXSDevice);
  pos = DXSTotalSamples - 2*DXSDeviceSamples + sMulDiv(timeGetTime()-DXSTime,44100,1000);
  pos = sMin<sInt>(pos,DXSTotalSamples-DXSDeviceSamples);
  if(pos>DXSLastTotalSample)
    DXSLastTotalSample = pos;
  SDL_UnlockAudioDevice(DXSDevice);
}

void sSystem_::SetSoundHandler(sSoundHandler hnd,sInt align,void *user)
{
  SDL_LockAudioDevice(DXSDevice);

  DXSHandler = hnd;
  if(align<0)
    align=64;
  DXSHandlerAlign = align;
  DXSHandlerSample = 0;
  DXSHandlerUser = user;

  DXSLastTotalSample = 0;
  DXSTotalSamples = 0;
  DXSTime = timeGetTime();

  SDL_UnlockAudioDevice(DXSDevice);
}

sInt sSystem_::GetCurrentSample()
{
  return DXSLastTotalSample;
}

/****************************************************************************/
/***                                                                      ***/
/***   File                                                               ***/
/***                                                                      ***/
/****************************************************************************/

sU8 *sSystem_::LoadFile(const sChar *name,sInt &size)
{
  FILE *f;
  sU8 *mem;
  long len;

  if(!name || !name[0])
    return 0;
  f = fopen(name,"rb");
  if(!f)
    return 0;

  fseek(f,0,SEEK_END);
  len = ftell(f);
  fseek(f,0,SEEK_SET);
  mem = new sU8[len+1];
  if(fread(mem,1,len,f)!=(size_t)len)
  {
    delete[] mem;
    mem = 0;
  }
  fclose(f);
  size = len;

  return mem;
}

sU8 *sSystem_::LoadFile(const sChar *name)
{
  sInt dummy;
  return LoadFile(name,dummy);
}

void sSystem_::GetModuleBaseName(sChar *buffer,sInt size)
{
  sChar path[2048];
  ssize_t len = readlink("/proc/self/exe",path,sizeof(path)-1);

  if(len<=0)
  {
    sCopyString(buffer,"theta",size);
    return;
  }
  path[len] = 0;

  // strip extension (if any), keep the directory: "<dir>/<name>"
  sChar *slash = strrchr(path,'/');
  sChar *dot = strrchr(path,'.');
  if(dot && (!slash || dot>slash))
    *dot = 0;
  sCopyString(buffer,path,size);
}

sU8 *sSystem_::LoadFileIfNewerThan(const sChar *name,const sChar *other,sInt &size)
{
  struct stat s1,s2;

  if(stat(name,&s1)!=0 || stat(other,&s2)!=0 || s1.st_mtime < s2.st_mtime)
    return 0;
  return LoadFile(name,size);
}

sChar *sSystem_::LoadText(const sChar *name)
{
  sInt size;
  sU8 *mem = LoadFile(name,size);

  if(mem)
    mem[size] = 0;

  return (sChar *)mem;
}

sBool sSystem_::SaveFile(const sChar *name,const sU8 *data,sInt size)
{
  FILE *f = fopen(name,"wb");
  sBool ok;

  if(!f)
    return sFALSE;
  ok = fwrite(data,1,size,f)==(size_t)size;
  ok = (fclose(f)==0) && ok;

  return ok;
}

sU64 sSystem_::GetFileStamp(const sChar *name)
{
  struct stat s;

  if(stat(name,&s)!=0)
    return 0;
  return (sU64) s.st_mtime;
}

sDirEntry *sSystem_::LoadDir(const sChar *name)
{
  DIR *dir;
  struct dirent *de;
  struct stat s;
  sDirEntry *list;
  sInt count,max;
  sChar path[2048];

  dir = opendir(name);
  if(!dir)
    return 0;

  max = 256;
  count = 0;
  list = new sDirEntry[max];
  while((de = readdir(dir)))
  {
    if(!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;
    if(count+1>=max)
    {
      sDirEntry *nl = new sDirEntry[max*2];
      sCopyMem(nl,list,sizeof(sDirEntry)*count);
      delete[] list;
      list = nl;
      max *= 2;
    }
    snprintf(path,sizeof(path),"%s/%s",name,de->d_name);
    sSetMem(&list[count],0,sizeof(sDirEntry));
    sCopyString(list[count].Name,de->d_name,sizeof(list[count].Name));
    if(stat(path,&s)==0)
    {
      list[count].IsDir = S_ISDIR(s.st_mode);
      list[count].Size = list[count].IsDir ? 0 : (sInt) s.st_size;
    }
    count++;
  }
  closedir(dir);
  sSetMem(&list[count],0,sizeof(sDirEntry));

  return list;
}

sBool sSystem_::MakeDir(const sChar *name)
{
  return mkdir(name,0755)==0;
}

sBool sSystem_::CheckDir(const sChar *name)
{
  struct stat s;
  return stat(name,&s)==0 && S_ISDIR(s.st_mode);
}

sBool sSystem_::CheckFile(const sChar *name)
{
  struct stat s;
  return stat(name,&s)==0 && S_ISREG(s.st_mode);
}

sBool sSystem_::RenameFile(const sChar *oldname,const sChar *newname)
{
  return rename(oldname,newname)==0;
}

sBool sSystem_::DeleteFile(const sChar *name)
{
  return unlink(name)==0;
}

void sSystem_::GetCurrentDir(sChar *buffer,sInt size)
{
  if(!getcwd(buffer,size))
    buffer[0] = 0;
}

sU32 sSystem_::GetDriveMask()
{
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Image decoding (stb_image in place of OleLoadPicture/libpng)       ***/
/***                                                                      ***/
/****************************************************************************/

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_GIF
#include "third_party/stb_image.h"

sBitmap *sSystem_::LoadBitmap(const sU8 *data,sInt size)
{
  sInt x,y;
  sU8 *pic;
  sBitmap *bm;

  if(LoadBitmapCore(data,size,x,y,pic))
  {
    bm = new sBitmap;
    bm->Init(x,y);

    sCopyMem(bm->Data,pic,x*y*4);
    delete[] pic;

    return bm;
  }
  else
  {
    return 0;
  }
}

// Decodes to 32-bit BGRA, stretched to power-of-two dimensions like the
// original GDI StretchBlt path (nearest neighbour, COLORONCOLOR style).
sBool sSystem_::LoadBitmapCore(const sU8 *data,sInt size,sInt &xout,sInt &yout,sU8 *&dataout)
{
  int w,h,n;
  sU8 *img;
  sInt xs,ys;

  xout = 0;
  yout = 0;
  dataout = 0;

  img = stbi_load_from_memory(data,size,&w,&h,&n,4);
  if(!img)
    return sFALSE;

  xout = xs = sMakePower2(w);
  yout = ys = sMakePower2(h);
  dataout = new sU8[xs*ys*4];

  for(sInt y=0;y<ys;y++)
  {
    const sU8 *srow = img + (y*h/ys)*w*4;
    sU8 *d = dataout + y*xs*4;
    for(sInt x=0;x<xs;x++)
    {
      const sU8 *s = srow + (x*w/xs)*4;
      d[0] = s[2];
      d[1] = s[1];
      d[2] = s[0];
      d[3] = 0xff;
      d += 4;
    }
  }

  stbi_image_free(img);
  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Host Font Interface                                                ***/
/***                                                                      ***/
/****************************************************************************/

// GDI text output, rebuilt on stb_truetype with the GDI conventions the
// generators rely on: the requested height is the em height, metrics come from
// the OS/2 table like tmAscent/tmDescent, a non-zero width sets the average
// character width, text is cp1252 (symbol fonts use their 0xf0xx range), the
// y position is the top of the cell and advances are whole pixels.
//
// Fonts are looked up in <executable dir>/fonts/<name>.ttf (Windows file names
// such as arial.ttf, arialbd.ttf, webdings.ttf), then through fontconfig.

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

struct LinuxFont
{
  sChar Name[64];
  sInt Style;
  sU8 *File;
  stbtt_fontinfo Info;
  sBool Symbol;
  sInt WinAscent,WinDescent,AvgCharWidth;   // OS/2 table, font units
};

static LinuxFont Fonts[8];
static LinuxFont *CurFont;
static sF32 FontScaleX,FontScaleY;
static sInt FontAscent;
static sInt FontPageX,FontPageY;

static const sU16 CP1252[32] =
{
  0x20ac,0x0081,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,0x02c6,0x2030,0x0160,0x2039,0x0152,0x008d,0x017d,0x008f,
  0x0090,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,0x02dc,0x2122,0x0161,0x203a,0x0153,0x009d,0x017e,0x0178,
};

static const sU8 *FindFontTable(const sU8 *ttf,const char *tag)
{
  sInt count = (ttf[4]<<8)|ttf[5];
  for(sInt i=0;i<count;i++)
  {
    const sU8 *rec = ttf+12+16*i;
    if(!memcmp(rec,tag,4))
      return ttf + ((rec[8]<<24)|(rec[9]<<16)|(rec[10]<<8)|rec[11]);
  }
  return 0;
}

static sU8 *LoadFontFile(const sChar *name,sInt style,sChar *found,sInt foundsize)
{
  static const char *suffix[4] = { "","i","bd","bi" };
  char lower[64],path[2048],dir[2048];
  sU8 *data;
  sInt size,i;

  for(i=0;name[i] && i<63;i++)
    lower[i] = (name[i]>='A' && name[i]<='Z') ? name[i]-'A'+'a' : name[i];
  lower[i] = 0;

  // next to the executable
  sSystem->GetModuleBaseName(dir,sizeof(dir));
  char *slash = strrchr(dir,'/');
  if(slash) *slash = 0;
  snprintf(path,sizeof(path),"%s/fonts/%s%s.ttf",dir,lower,suffix[style&3]);
  sCopyString(found,path,foundsize);
  if((data = sSystem->LoadFile(path,size)))
    return data;
  snprintf(path,sizeof(path),"%s/fonts/%s.ttf",dir,lower);
  sCopyString(found,path,foundsize);
  if((data = sSystem->LoadFile(path,size)))
    return data;

  // fontconfig
  for(i=0;lower[i];i++)
    if(!((lower[i]>='a' && lower[i]<='z') || (lower[i]>='0' && lower[i]<='9') || lower[i]==' ' || lower[i]=='-'))
      lower[i] = ' ';
  snprintf(path,sizeof(path),"fc-match -f '%%{file}' '%s:weight=%d:slant=%d' 2>/dev/null",
    lower,(style&2) ? 200 : 80,(style&1) ? 100 : 0);
  FILE *p = popen(path,"r");
  if(!p)
    return 0;
  if(!fgets(path,sizeof(path),p))
    path[0] = 0;
  pclose(p);
  sCopyString(found,path,foundsize);
  return path[0] ? sSystem->LoadFile(path,size) : 0;
}

static LinuxFont *GetFont(const sChar *name,sInt style)
{
  sInt i;

  for(i=0;i<8 && Fonts[i].File;i++)
    if(Fonts[i].Style==style && !sCmpStringI(Fonts[i].Name,name))
      return &Fonts[i];
  if(i==8)
    i = 7;

  LinuxFont *f = &Fonts[i];
  delete[] f->File;
  sSetMem(f,0,sizeof(*f));
  sCopyString(f->Name,name,sizeof(f->Name));
  f->Style = style;
  sChar found[2048];
  f->File = LoadFontFile(name,style,found,sizeof(found));
  if(!f->File || !stbtt_InitFont(&f->Info,f->File,stbtt_GetFontOffsetForIndex(f->File,0)))
  {
    fprintf(stderr,"warning: font '%s' not found\n",name);
    delete[] f->File;
    f->File = 0;
    return 0;
  }

  f->Symbol = stbtt_FindGlyphIndex(&f->Info,'A')==0 && stbtt_FindGlyphIndex(&f->Info,0xf041)!=0;
  fprintf(stderr,"font '%s': %s%s\n",name,found,f->Symbol ? " (symbol)" : "");

  const sU8 *os2 = FindFontTable(f->File,"OS/2");
  if(os2)
  {
    f->AvgCharWidth = (sS16)((os2[2]<<8)|os2[3]);
    f->WinAscent = (os2[74]<<8)|os2[75];
    f->WinDescent = (os2[76]<<8)|os2[77];
  }
  else
  {
    sInt asc,desc,gap;
    stbtt_GetFontVMetrics(&f->Info,&asc,&desc,&gap);
    f->WinAscent = asc;
    f->WinDescent = -desc;
  }

  return f;
}

static sInt FontGlyph(sInt ch,sBool wide)
{
  sInt cp = ch;

  if(!wide)
  {
    ch &= 255;
    cp = (ch>=0x80 && ch<0xa0) ? CP1252[ch-0x80] : ch;
  }
  if(CurFont->Symbol)
  {
    sInt g = stbtt_FindGlyphIndex(&CurFont->Info,0xf000|(ch&0xff));
    if(g)
      return g;
  }
  return stbtt_FindGlyphIndex(&CurFont->Info,cp);
}

static sInt FontAdvance(sInt glyph)
{
  sInt adv,lsb;
  stbtt_GetGlyphHMetrics(&CurFont->Info,glyph,&adv,&lsb);
  return (sInt) floorf(adv*FontScaleX+0.5f);
}

sInt sSystem_::FontBegin(sInt pagex,sInt pagey,const sChar *name,sInt xs,sInt ys,sInt style)
{
  FontPageX = pagex;
  FontPageY = pagey;
  FontMem = new sU32[pagex*pagey];
  sSetMem(FontMem,0,pagex*pagey*4);

  if(name[0]==0) name = "arial";
  CurFont = GetFont(name,style&3);
  if(!CurFont)
    return ys;

  if(ys<=0) ys = 16;
  FontScaleY = stbtt_ScaleForMappingEmToPixels(&CurFont->Info,ys);
  FontScaleX = FontScaleY;
  if(xs>0 && CurFont->AvgCharWidth>0)
    FontScaleX = (sF32) xs / CurFont->AvgCharWidth;

  FontAscent = (sInt) floorf(CurFont->WinAscent*FontScaleY+0.5f);
  return FontAscent + (sInt) floorf(CurFont->WinDescent*FontScaleY+0.5f);
}

sInt sSystem_::FontWidth(const sChar *string,sInt len)
{
  sInt w = 0;

  if(len==-1)
    len = sGetStringLen(string);
  if(CurFont)
    for(sInt i=0;i<len;i++)
      w += FontAdvance(FontGlyph(string[i],sFALSE));
  return w;
}

void sSystem_::FontCharWidth(sInt ch,sInt *widths)
{
  sInt x0,y0,x1,y1,glyph,adv;

  widths[0] = widths[1] = widths[2] = 0;
  if(!CurFont)
    return;

  glyph = FontGlyph(ch,sFALSE);
  adv = FontAdvance(glyph);
  stbtt_GetGlyphBitmapBox(&CurFont->Info,glyph,FontScaleX,FontScaleY,&x0,&y0,&x1,&y1);
  if(x1<=x0)                              // blank glyph: all advance (GDI reports a 1 pixel black box)
  {
    x0 = 0;
    x1 = sMax(adv,1);
  }
  widths[0] = x0;
  widths[1] = x1-x0;
  widths[2] = adv-x1;
}

static void FontDraw(sInt x,sInt y,sInt glyph)
{
  sInt x0,y0,x1,y1,w,h;

  stbtt_GetGlyphBitmapBox(&CurFont->Info,glyph,FontScaleX,FontScaleY,&x0,&y0,&x1,&y1);
  w = x1-x0;
  h = y1-y0;
  if(w<=0 || h<=0)
    return;

  sU8 *cov = new sU8[w*h];
  stbtt_MakeGlyphBitmap(&CurFont->Info,cov,w,h,w,FontScaleX,FontScaleY,glyph);

  for(sInt j=0;j<h;j++)
  {
    sInt py = y+FontAscent+y0+j;
    if(py<0 || py>=FontPageY)
      continue;
    for(sInt i=0;i<w;i++)
    {
      sInt px = x+x0+i;
      if(px<0 || px>=FontPageX)
        continue;
      sU32 &d = sSystem->FontMem[py*FontPageX+px];
      sU32 c = cov[j*w+i];
      if(c>(d&255))
        d = c*0x010101;                   // white text (GDI: SetTextColor 0xffffff)
    }
  }
  delete[] cov;
}

void sSystem_::FontPrint(sInt x,sInt y,const sChar *string,sInt len)
{
  if(len==-1)
    len = sGetStringLen(string);
  if(CurFont)
    for(sInt i=0;i<len;i++)
    {
      sInt glyph = FontGlyph(string[i],sFALSE);
      FontDraw(x,y,glyph);
      x += FontAdvance(glyph);
    }
}

void sSystem_::FontPrint(sInt x,sInt y,const sWChar *string,sInt len)
{
  if(len==-1)
    for(len=0;string[len];len++);
  if(CurFont)
    for(sInt i=0;i<len;i++)
    {
      sInt glyph = FontGlyph(string[i],sTRUE);
      FontDraw(x,y,glyph);
      x += FontAdvance(glyph);
    }
}

void sSystem_::FontEnd()
{
  delete[] FontMem;
  FontMem = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Screenshots for testing: THETA_SHOTS="t1,t2,..." (seconds of music)  ***/
/***   writes shot_<t>.png into THETA_SHOTDIR (default: current dir)      ***/
/***                                                                      ***/
/****************************************************************************/

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

static sF32 ShotTimes[256];
static sInt ShotCount = -1;
static sInt ShotNext;

static int CompareF32(const void *a,const void *b)
{
  sF32 x = *(const sF32 *)a, y = *(const sF32 *)b;
  return (x>y) - (x<y);
}

static void LinuxCaptureFrame(IDirect3DDevice9 *dev)
{
  if(ShotCount<0)
  {
    const char *env = getenv("THETA_SHOTS");
    ShotCount = 0;
    while(env && *env && ShotCount<256)
    {
      char *end;
      ShotTimes[ShotCount++] = strtof(env,&end);
      env = (*end==',') ? end+1 : 0;
    }
    qsort(ShotTimes,ShotCount,sizeof(sF32),CompareF32);
  }
  if(ShotNext>=ShotCount)
    return;

  sF32 t = sSystem->GetCurrentSample()/44100.0f;
  if(t<ShotTimes[ShotNext])
    return;
  while(ShotNext<ShotCount && ShotTimes[ShotNext]<=t)
    ShotNext++;

  IDirect3DSurface9 *bb = 0,*sys = 0;
  D3DSURFACE_DESC desc;
  D3DLOCKED_RECT lr;

  if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb)))
    return;
  bb->GetDesc(&desc);
  if(SUCCEEDED(dev->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&sys,0))
    && SUCCEEDED(dev->GetRenderTargetData(bb,sys))
    && SUCCEEDED(sys->LockRect(&lr,0,D3DLOCK_READONLY)))
  {
    sU8 *rgb = new sU8[desc.Width*desc.Height*3];
    for(sU32 y=0;y<desc.Height;y++)
    {
      const sU8 *s = (const sU8 *)lr.pBits + y*lr.Pitch;
      sU8 *d = rgb + y*desc.Width*3;
      for(sU32 x=0;x<desc.Width;x++,s+=4,d+=3)
      {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
      }
    }
    sys->UnlockRect();

    char name[1024];
    const char *dir = getenv("THETA_SHOTDIR");
    snprintf(name,sizeof(name),"%s/shot_%07.2f.png",dir ? dir : ".",t);
    stbi_write_png(name,desc.Width,desc.Height,3,rgb,desc.Width*3);
    fprintf(stderr,"screenshot %s\n",name);
    delete[] rgb;
  }
  sRelease(sys);
  sRelease(bb);
}
