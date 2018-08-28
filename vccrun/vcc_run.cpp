//**************************************************************************
//**
//**  ##   ##    ##    ##   ##   ####     ####   ###     ###
//**  ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**   ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**   ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**    ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**     #    ##    ##    #      ####     ####   ##       ##
//**
//**  $Id$
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
//**  Copyright (C) 2018 Ketmar Dark
//**
//**  This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**  This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************
#include <signal.h>
#include <time.h>

#include "vcc_run.h"

#include "modules/mod_sound/sound.h"
#include "modules/mod_console.h"

#ifdef SERIALIZER_USE_LIBHA
# include "filesys/halib/libha.h"
#endif


// ////////////////////////////////////////////////////////////////////////// //
//#define DEBUG_OBJECT_LOADER


// ////////////////////////////////////////////////////////////////////////// //
VObject *mainObject = nullptr;
VStr appName;
bool compileOnly = false;
bool writeToConsole = true; //FIXME
bool dumpProfile = false;


// ////////////////////////////////////////////////////////////////////////// //
static VStr buildConfigName (const VStr &optfile) {
  for (int f = 0; f < optfile.length(); ++f) {
    char ch = optfile[f];
    if (ch >= '0' && ch <= '9') continue;
    if (ch >= 'A' && ch <= 'Z') continue;
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch == '_' || ch == ' ' || ch == '.') continue;
    return VStr();
  }
  if (optfile.length()) {
    return VStr(".")+optfile+".cfg";
  } else {
    return VStr(".options.cfg");
  }
}


// ////////////////////////////////////////////////////////////////////////// //
#include "vcc_run_serializer.cpp"
#include "vcc_run_net.cpp"


// ////////////////////////////////////////////////////////////////////////// //
#if defined(WIN32)
# include <windows.h>
#endif

__attribute__((noreturn, format(printf, 1, 2))) void Host_Error (const char *error, ...) {
#if defined(VCC_STANDALONE_EXECUTOR) && defined(WIN32)
  static char workString[1024];
  va_list argPtr;
  va_start(argPtr, error);
  vsnprintf(workString, sizeof(workString), error, argPtr);
  va_end(argPtr);
  MessageBox(NULL, workString, "VaVoom/C Runner Fatal Error", MB_OK);
#else
  fprintf(stderr, "FATAL: ");
  va_list argPtr;
  va_start(argPtr, error);
  vfprintf(stderr, error, argPtr);
  va_end(argPtr);
  fprintf(stderr, "\n");
#endif
  exit(1);
}


static const char *comatoze (vuint32 n) {
  static char buf[128];
  int bpos = (int)sizeof(buf);
  buf[--bpos] = 0;
  int xcount = 0;
  do {
    if (xcount == 3) { buf[--bpos] = ','; xcount = 0; }
    buf[--bpos] = '0'+n%10;
    ++xcount;
  } while ((n /= 10) != 0);
  return &buf[bpos];
}


// ////////////////////////////////////////////////////////////////////////// //
void PR_WriteOne (const VFieldType &type) {
  char buf[128];
  size_t blen = 0;

  switch (type.Type) {
    case TYPE_Int: blen = snprintf(buf, sizeof(buf), "%d", PR_Pop()); break;
    case TYPE_Byte: blen = snprintf(buf, sizeof(buf), "%d", PR_Pop()); break;
    case TYPE_Bool: blen = snprintf(buf, sizeof(buf), "%s", (PR_Pop() ? "true" : "false")); break;
    case TYPE_Float: blen = snprintf(buf, sizeof(buf), "%f", PR_Popf()); break;
    case TYPE_Name: blen = snprintf(buf, sizeof(buf), "%s", *PR_PopName()); break;
    case TYPE_String:
      {
        VStr s = PR_PopStr();
        if (writeToConsole) VConsole::WriteStr(s); else printf("%s", *s);
      }
      return;
    case TYPE_Vector: { TVec v = PR_Popv(); blen = snprintf(buf, sizeof(buf), "(%f,%f,%f)", v.x, v.y, v.z); } break;
    case TYPE_Pointer: blen = snprintf(buf, sizeof(buf), "<%s>(%p)", *type.GetName(), PR_PopPtr()); break;
    case TYPE_Class: if (PR_PopPtr()) blen = snprintf(buf, sizeof(buf), "<%s>", *type.GetName()); else blen = snprintf(buf, sizeof(buf), "<none>"); break;
    case TYPE_State:
      {
        VState *st = (VState *)PR_PopPtr();
        if (st) {
          blen = snprintf(buf, sizeof(buf), "<state:%s %d %f>", *st->SpriteName, st->Frame, st->Time);
        } else {
          blen = snprintf(buf, sizeof(buf), "<state>");
        }
      }
      break;
    case TYPE_Reference: blen = snprintf(buf, sizeof(buf), "<%s>", (type.Class ? *type.Class->Name : "none")); break;
    case TYPE_Delegate: blen = snprintf(buf, sizeof(buf), "<%s:%p:%p>", *type.GetName(), PR_PopPtr(), PR_PopPtr()); break;
    case TYPE_Struct: PR_PopPtr(); blen = snprintf(buf, sizeof(buf), "<%s>", *type.Struct->Name); break;
    case TYPE_Array: PR_PopPtr(); blen = snprintf(buf, sizeof(buf), "<%s>", *type.GetName()); break;
    case TYPE_SliceArray: blen = snprintf(buf, sizeof(buf), "<%s:%d>", *type.GetName(), PR_Pop()); PR_PopPtr(); break;
    case TYPE_DynamicArray:
      {
        VScriptArray *a = (VScriptArray *)PR_PopPtr();
        blen = snprintf(buf, sizeof(buf), "%s(%d)", *type.GetName(), a->Num());
      }
      break;
    default: Sys_Error(va("Tried to print something strange: `%s`", *type.GetName()));
  }

  if (blen) {
    if (writeToConsole) VConsole::WriteStr(buf, blen); else printf("%s", buf);
  }
}


void PR_WriteFlush () {
  if (writeToConsole) VConsole::PutChar('\n'); else printf("\n");
}


// ////////////////////////////////////////////////////////////////////////// //
class VVccLog : public VLogListener {
public:
  virtual void Serialise (const char* text, EName event) override {
    dprintf("%s", text);
  }
};


// ////////////////////////////////////////////////////////////////////////// //
static VStr SourceFileName;
static TArray<VStr> scriptArgs;

static int num_dump_asm;
static const char *dump_asm_names[1024];
static bool DebugMode = false;
static FILE *DebugFile;

static VLexer Lex;
static VVccLog VccLog;


//==========================================================================
//
//  dprintf
//
//==========================================================================
__attribute__((format(printf, 1, 2))) int dprintf (const char *text, ...) {
  if (!DebugMode) return 0;

  va_list argPtr;
  FILE* fp = stderr; //(DebugFile ? DebugFile : stdout);
  va_start(argPtr, text);
  int ret = vfprintf(fp, text, argPtr);
  va_end(argPtr);
  fflush(fp);
  return ret;
}


//==========================================================================
//
//  Malloc
//
//==========================================================================
void* Malloc (size_t size) {
  if (!size) return nullptr;
  void *ptr = Z_Malloc(size);
  if (!ptr) FatalError("FATAL: couldn't alloc %d bytes", (int)size);
  memset(ptr, 0, size);
  return ptr;
}


//==========================================================================
//
//  Free
//
//==========================================================================
void Free (void* ptr) {
  if (ptr) Z_Free(ptr);
}


//==========================================================================
//
//  OpenFile
//
//==========================================================================
VStream* OpenFile (const VStr& Name) {
  return fsysOpenFile(Name);
  /*
  FILE* file = fopen(*Name, "rb");
  return (file ? new VFileReader(file) : nullptr);
  */
}


//==========================================================================
//
//  OpenDebugFile
//
//==========================================================================
static void OpenDebugFile (const VStr& name) {
  DebugFile = fopen(*name, "w");
  if (!DebugFile) FatalError("FATAL: can\'t open debug file \"%s\".", *name);
}


//==========================================================================
//
//  PC_DumpAsm
//
//==========================================================================
static void PC_DumpAsm (const char* name) {
  char buf[1024];
  char *cname;
  char *fname;

  snprintf(buf, sizeof(buf), "%s", name);

  //FIXME! PATH WITH DOTS!
  if (strstr(buf, ".")) {
    cname = buf;
    fname = strstr(buf, ".")+1;
    fname[-1] = 0;
  } else {
    dprintf("Dump ASM: Bad name %s\n", name);
    return;
  }

  //printf("<%s>.<%s>\n", cname, fname);

  for (int i = 0; i < VMemberBase::GMembers.Num(); ++i) {
    //if (VMemberBase::GMembers[i]->MemberType == MEMBER_Method) printf("O:<%s>; N:<%s>\n", *VMemberBase::GMembers[i]->Outer->Name, *VMemberBase::GMembers[i]->Name);
    if (VMemberBase::GMembers[i]->MemberType == MEMBER_Method &&
        !VStr::Cmp(cname, *VMemberBase::GMembers[i]->Outer->Name) &&
        !VStr::Cmp(fname, *VMemberBase::GMembers[i]->Name))
    {
      ((VMethod*)VMemberBase::GMembers[i])->DumpAsm();
      return;
    }
  }

  dprintf("Dump ASM: %s not found!\n", name);
}


//==========================================================================
//
//  DumpAsm
//
//==========================================================================
static void DumpAsm () {
  for (int i = 0; i < num_dump_asm; ++i) PC_DumpAsm(dump_asm_names[i]);
}


//==========================================================================
//
//  DisplayUsage
//
//==========================================================================
static void DisplayUsage () {
  printf("\n");
  printf("VCC Version 1.%d. Copyright (c) 2000-2001 by JL, 2018 by Ketmar Dark. (" __DATE__ " " __TIME__ "; opcodes: %d)\n", PROG_VERSION, NUM_OPCODES);
  printf("Usage: vcc [options] source[.c] [object[.dat]]\n");
  printf("    -d<file>     Output debugging information into specified file\n");
  printf("    -a<function> Output function's ASM statements into debug file\n");
  printf("    -D<name>           Define macro\n");
  printf("    -I<directory>      Include files directory\n");
  printf("    -P<directory>      Package import files directory\n");
  printf("    -base <directory>  Set base directory\n");
  printf("    -file <name>       Add pak file\n");
  exit(1);
}


//==========================================================================
//
//  ProcessArgs
//
//==========================================================================
static void ProcessArgs (int ArgCount, char **ArgVector) {
  int count = 0; // number of file arguments
  bool nomore = false;

  TArray<VStr> paklist;

  for (int i = 1; i < ArgCount; ++i) {
    const char *text = ArgVector[i];
    if (count == 0 && !nomore && *text == '-') {
      ++text;
      if (*text == 0) DisplayUsage();
      if (text[0] == '-' && text[1] == 0) { nomore = true; continue; }
      const char option = *text++;
      switch (option) {
        case 'd': DebugMode = true; if (*text) OpenDebugFile(text); break;
        case 'c': compileOnly = true; break;
        case 'a': /*if (!*text) DisplayUsage(); dump_asm_names[num_dump_asm++] = text;*/ VMemberBase::doAsmDump = true; break;
        case 'I': VMemberBase::StaticAddIncludePath(text); break;
        case 'D': VMemberBase::StaticAddDefine(text); break;
        case 'P': VMemberBase::StaticAddPackagePath(text); break;
        default:
          --text;
          if (VStr::Cmp(text, "nocol") == 0) {
            vcErrorIncludeCol = false;
          } else if (VStr::Cmp(text, "profile") == 0) {
            dumpProfile = true;
          } else if (VStr::Cmp(text, "base") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            fsysBaseDir = VStr(ArgVector[i]);
          } else if (VStr::Cmp(text, "file") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            paklist.append(VStr(":")+VStr(ArgVector[i]));
            //fprintf(stderr, "<%s>\n", ArgVector[i]);
          } else if (VStr::Cmp(text, "pakdir") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            paklist.append(VStr("/")+VStr(ArgVector[i]));
            //fprintf(stderr, "<%s>\n", ArgVector[i]);
          } else {
            //fprintf(stderr, "*<%s>\n", text);
            DisplayUsage();
          }
          break;
      }
      continue;
    }
    ++count;
    switch (count) {
      case 1: SourceFileName = VStr(text).DefaultExtension(".vc"); break;
      default: scriptArgs.Append(VStr(text)); break;
    }
  }

  if (count == 0) DisplayUsage();

  /*
  if (!DebugFile) {
    VStr DbgFileName;
    DbgFileName = ObjectFileName.StripExtension()+".txt";
    OpenDebugFile(DbgFileName);
    DebugMode = true;
  }
  */

  fsysInit();
  for (int f = 0; f < paklist.length(); ++f) {
    VStr pname = paklist[f];
    if (pname.length() < 2) continue;
    char type = pname[0];
    pname.chopLeft(1);
    if (type == ':') {
      if (fsysAppendPak(pname)) {
        dprintf("added pak file '%s'...\n", *pname);
      } else {
        fprintf(stderr, "CAN'T add pak file '%s'!\n", *pname);
      }
    } else if (type == '/') {
      if (fsysAppendDir(pname)) {
        dprintf("added pak directory '%s'...\n", *pname);
      } else {
        fprintf(stderr, "CAN'T add pak directory '%s'!\n", *pname);
      }
    }
  }

  SourceFileName = SourceFileName.fixSlashes();
  dprintf("Main source file: %s\n", *SourceFileName);
}


//==========================================================================
//
//  initialize
//
//==========================================================================
static void initialize () {
  DebugMode = false;
  DebugFile = nullptr;
  num_dump_asm = 0;
  VName::StaticInit();
  //VMemberBase::StaticInit();
  VObject::StaticInit();
  VMemberBase::StaticAddDefine("VCC_STANDALONE_EXECUTOR");
#ifdef VCCRUN_HAS_SDL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_SDL");
#endif
#ifdef VCCRUN_HAS_OPENGL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_OPENGL");
#endif
#ifdef VCCRUN_HAS_OPENAL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_OPENAL");
#endif
  VMemberBase::StaticAddDefine("VCCRUN_HAS_IMAGO");
  VObject::onExecuteNetMethodCB = &onExecuteNetMethod;
}


// ////////////////////////////////////////////////////////////////////////// //
// <0: error; bit 0: has arg; bit 1: returns int
static int checkArg (VMethod *mmain) {
  if (!mmain) return -1;
  if ((mmain->Flags&FUNC_VarArgs) != 0) return -1;
  //if (mmain->NumParams > 0 && mmain->ParamFlags[0] != 0) return -1;
  int res = 0;
  if (mmain->ReturnType.Type != TYPE_Void && mmain->ReturnType.Type != TYPE_Int) return -1;
  if (mmain->ReturnType.Type == TYPE_Int) res |= 0x02;
  if (mmain->NumParams != 0) {
    if (mmain->NumParams != 1) return -1;
    if (mmain->ParamFlags[0] == 0) {
      VFieldType atp = mmain->ParamTypes[0];
      dprintf("  ptype0: %s\n", *atp.GetName());
      if (atp.Type != TYPE_Pointer) return -1;
      atp = atp.GetPointerInnerType();
      if (atp.Type != TYPE_DynamicArray) return -1;
      atp = atp.GetArrayInnerType();
      if (atp.Type != TYPE_String) return -1;
      res |= 0x01;
    } else if ((mmain->ParamFlags[0]&(FPARM_Out|FPARM_Ref)) != 0) {
      VFieldType atp = mmain->ParamTypes[0];
      dprintf("  ptype1: %s\n", *atp.GetName());
      if (atp.Type != TYPE_DynamicArray) return -1;
      atp = atp.GetArrayInnerType();
      if (atp.Type != TYPE_String) return -1;
      res |= 0x01;
    }
  }
  return res;
}


// ////////////////////////////////////////////////////////////////////////// //
/*
#include <SDL.h>
#include <GL/gl.h>

void boo () {
  int width = 800;
  int height = 600;

  Uint32 flags = SDL_WINDOW_OPENGL;

#if 0
  //k8: require OpenGL 1.5
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  //SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#else
  //k8: require OpenGL 2.1, sorry; non-shader renderer was removed anyway
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  //SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

  glGetError();

  auto hw_window = SDL_CreateWindow("tesT", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
  if (!hw_window) {
#ifndef WIN32
    fprintf(stderr, "ALAS: cannot create SDL2 window.\n");
#endif
    return;
  }

  auto hw_glctx = SDL_GL_CreateContext(hw_window);
  if (!hw_glctx) {
    SDL_DestroyWindow(hw_window);
    hw_window = nullptr;
#ifndef WIN32
    fprintf(stderr, "ALAS: cannot create SDL2 OpenGL context.\n");
#endif
    return;
  }

  SDL_GL_MakeCurrent(hw_window, hw_glctx);
  glGetError();

#if !defined(WIN32)
  {
    int ghi, glo;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &ghi);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &glo);
    fprintf(stderr, "OpenGL version: %d.%d\n", ghi, glo);

    int ltmp = 666;
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &ltmp); fprintf(stderr, "STENCIL BUFFER BITS: %d\n", ltmp);
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &ltmp); fprintf(stderr, "RED BITS: %d\n", ltmp);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &ltmp); fprintf(stderr, "GREEN BITS: %d\n", ltmp);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &ltmp); fprintf(stderr, "BLUE BITS: %d\n", ltmp);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &ltmp); fprintf(stderr, "DEPTH BITS: %d\n", ltmp);
  }
#endif

  //SDL_GL_MakeCurrent(hw_window, hw_glctx);
  SDL_GL_MakeCurrent(hw_window, nullptr);
  SDL_GL_DeleteContext(hw_glctx);
  SDL_DestroyWindow(hw_window);
}
*/

// ////////////////////////////////////////////////////////////////////////// //
int main (int argc, char **argv) {
  //boo();

  VStack ret;
  ret.i = 0;

  srand(time(nullptr));

#if 0
  {
    TArray<VStr> pts;
    VStr s = "/this/is/path/";
    s.SplitPath(pts);
    for (int f = 0; f < pts.length(); ++f) printf("  %d: <%s>:%d\n", f, *pts[f], pts[f].length());
    printf("<%s>\n", *(s.ToUpper()));
    printf("<%s>\n", *(s.ToLower()));
    printf("<%s>\n", *(s.ToUpper().ToLower()));
    printf("<%s>\n", *s);
  }
#endif

  try {
    GLog.AddListener(&VccLog);

    int starttime;
    int endtime;

    starttime = time(0);

    initialize();

    ProcessArgs(argc, argv);

    /*{
      auto fl = fsysOpenFile("text/model");
      auto sz = fl->TotalSize();
      printf("SIZE: %d (%d)\n", sz, fl->Tell());
      auto buf = new char[sz];
      fl->Serialize(buf, sz);
      delete fl;
      auto fo = fsysOpenDiskFileWrite("_000.ini");
      fo->Serialize(buf, sz);
      delete fo;
    }*/

    /*{
      VStr s = fsysFileFindAnyExt("a.boo");
      fprintf(stderr, "%s\n", *s);
    }*/

    PR_Init();

    VMemberBase::StaticLoadPackage(VName("engine"), TLocation());
    //VMemberBase::StaticLoadPackage(VName("ui"), TLocation());

    VPackage *CurrentPackage = new VPackage(VName("vccrun"));

    dprintf("Compiling '%s'...\n", *SourceFileName);

    VStream *strm = OpenFile(SourceFileName);
    if (!strm) {
      FatalError("FATAL: cannot open file '%s'", *SourceFileName);
    }

    CurrentPackage->LoadSourceObject(strm, SourceFileName, TLocation());
    dprintf("Total memory used: %u\n", VExpression::TotalMemoryUsed);
    DumpAsm();
    endtime = time(0);
    dprintf("Time elapsed: %02d:%02d\n", (endtime-starttime)/60, (endtime-starttime)%60);
    // free compiler memory
    VMemberBase::StaticCompilerShutdown();
    dprintf("Peak compiler memory usage: %s bytes.\n", comatoze(VExpression::PeakMemoryUsed));
    dprintf("Released compiler memory  : %s bytes.\n", comatoze(VExpression::TotalMemoryFreed));
    if (VExpression::CurrMemoryUsed != 0) {
      dprintf("Compiler leaks %s bytes (this is harmless).\n", comatoze(VExpression::CurrMemoryUsed));
    }

    VScriptArray scargs(scriptArgs);
    VClass *mklass = VClass::FindClass("Main");
    if (mklass && !compileOnly) {
      dprintf("Found class 'Main'\n");
      VMethod *mmain = mklass->FindAccessibleMethod("main");
      if (mmain) {
        dprintf(" Found method 'main()' (return type: %u:%s)\n", mmain->ReturnType.Type, *mmain->ReturnType.GetName());
        int atp = checkArg(mmain);
        if (atp < 0) FatalError("Main::main() should be either arg-less, or have one `array!string*` argument, and should be either `void`, or return `int`!");
        auto sss = pr_stackPtr;
        mainObject = VObject::StaticSpawnObject(mklass, false); // replacement
        if ((mmain->Flags&FUNC_Static) == 0) {
          //auto imain = Spawn<VLevel>();
          P_PASS_REF((VObject *)mainObject);
        }
        if (atp&0x01) P_PASS_REF(&scargs);
        ret = VObject::ExecuteFunction(mmain);
        if ((atp&0x02) == 0) ret.i = 0;
        if (sss != pr_stackPtr) FatalError("FATAL: stack imbalance!");
      }
    }

    if (dumpProfile) VObject::DumpProfile();
    VSoundManager::StaticShutdown();
    VObject::StaticExit();
    VName::StaticExit();
  } catch (VException& e) {
    ret.i = -1;
#ifndef WIN32
    FatalError("FATAL: %s", e.What());
#else
    FatalError("%s", e.What());
#endif
  }

  return ret.i;
}


#include "vcc_run_vobj.cpp"
