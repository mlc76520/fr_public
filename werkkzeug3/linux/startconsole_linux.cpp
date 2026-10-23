// This file is distributed under a BSD license. See LICENSE.txt for details.

// Linux version of base_console/_startconsole.cpp (command line tools).

#include "_types.hpp"
#include "_startconsole.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

sSystem_ *sSystem;
class sBroker_ *sBroker;

sInt sFatality = 0;

/****************************************************************************/

int main(int argc,char **argv)
{
  sInt ret;

  sBroker = new sBroker_;
  sSystem = new sSystem_;
  ret = sAppMain(argc,argv);
  delete sSystem;
  delete sBroker;

  return ret;
}

/****************************************************************************/

void sSystem_::Log(sChar *s)
{
  fputs(s,stderr);
}

sNORETURN void sSystem_::Abort(sChar *msg)
{
  if(msg)
    PrintF("fatal error: %s\n",msg);
  exit(1);
}

void sSystem_::Tag()
{
}

void sSystem_::PrintF(const sChar *format,...)
{
  sChar buffer[2048];

  sFormatString(buffer,2048,format,&format);
  fputs(buffer,stdout);
  fflush(stdout);
}

/****************************************************************************/

sU8 *sSystem_::LoadFile(const sChar *name,sInt &size)
{
  FILE *f = fopen(name,"rb");
  sU8 *mem;

  if(!f)
    return 0;

  fseek(f,0,SEEK_END);
  size = ftell(f);
  fseek(f,0,SEEK_SET);
  mem = new sU8[size+1];
  if(fread(mem,1,size,f)!=(size_t)size)
  {
    delete[] mem;
    mem = 0;
  }
  fclose(f);

  return mem;
}

sU8 *sSystem_::LoadFile(const sChar *name)
{
  sInt dummy;
  return LoadFile(name,dummy);
}

sChar *sSystem_::LoadText(const sChar *name)
{
  sInt size;
  sU8 *mem = LoadFile(name,size);

  if(mem)
    mem[size] = 0;

  return (sChar *) mem;
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

sInt sSystem_::GetTime()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (sInt)(ts.tv_sec*1000 + ts.tv_nsec/1000000);
}
