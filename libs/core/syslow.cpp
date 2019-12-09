//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2010 Jānis Legzdiņš
//**  Copyright (C) 2018-2019 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, version 3 of the License ONLY.
//**
//**  This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**  You should have received a copy of the GNU General Public License
//**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//**
//**************************************************************************
//**
//**  Low-level OS-dependent functions
//**
//**************************************************************************
#define SHITDOZE_USE_WINMM

#include "core.h"


/*
  static void *operator new (size_t size);
  static void *operator new[] (size_t size);
  static void operator delete (void *p);
  static void operator delete[] (void *p);
*/
#include <new>

#if 0
moved to zone
void *operator new [] (std::size_t s) noexcept(false) /*throw(std::bad_alloc)*/ {
  //fprintf(stderr, "new[]\n");
  return Z_Calloc(s);
}


void operator delete [] (void *p) throw () {
  //fprintf(stderr, "delete[]\n");
  Z_Free(p);
}
#endif


static VStr sys_NormalizeUserName (const char *s) {
  if (!s) return VStr("PLAYER").makeImmutableRetSelf();
  while (*s && (unsigned char)(*s) <= ' ') ++s;
  if (!s[0]) return VStr("PLAYER").makeImmutableRetSelf();
  VStr res;
  while (*s) {
    char ch = *s++;
    if ((unsigned char)ch <= ' ' || ch >= 127) ch = '_';
    res += ch;
  }
  while (res.length() && (unsigned char)(res[res.length()-1]) <= ' ') res.chopRight(1);
  if (res.length() == 0) return VStr("PLAYER").makeImmutableRetSelf();
  return res;
}


#ifndef WIN32
#define CANON_PATH_DELIM  '/'
static inline bool isPathDelimiter (char ch) noexcept { return (ch == '/'); }
static inline bool isRootPath (const VStr &s) noexcept { return (s.length() > 0 && s[0] == '/'); }
static inline int getRootPathLength (const VStr &s) noexcept { return (s.length() > 0 && s[0] == '/' ? 1 : 0); }
#else
//#define CANON_PATH_DELIM  '\\'
#define CANON_PATH_DELIM  '/'
static inline bool isPathDelimiter (char ch) noexcept { return (ch == '/' || ch == '\\'); }
static inline bool isRootPath (const VStr &s) noexcept {
  return
   (s.length() > 0 && (s[0] == '/' || s[0] == '\\')) ||
   (s.length() > 2 && s[1] == ':' && (s[2] == '/' || s[2] == '\\'));
}
static inline int getRootPathLength (const VStr &s) noexcept {
  return
   s.length() > 0 && (s[0] == '/' || s[0] == '\\') ? 1 :
   s.length() > 2 && s[1] == ':' && (s[2] == '/' || s[2] == '\\') ? 3 : 0;
}
#endif


//#define SFFCI_FULL_SCAN_DEBUG

#ifdef SFFCI_FULL_SCAN
# undef SFFCI_FULL_SCAN
#endif
#ifdef WIN32
# define SFFCI_FULL_SCAN
#endif
/*
#ifndef SFFCI_FULL_SCAN
# define SFFCI_FULL_SCAN
#endif
*/

//==========================================================================
//
//  Sys_FindFileCI
//
//==========================================================================
VStr Sys_FindFileCI (VStr path, bool lastIsDir) {
  VStr realpath;

  #ifdef SFFCI_FULL_SCAN_DEBUG
  GLog.Logf(NAME_Debug, "Sys_FindFileCI: path=<%s> (lastIsDir=%d)", *path, (int)lastIsDir);
  #endif
  if (path.length() == 0) return realpath;
  if (isRootPath(path)) {
    realpath = path.left(getRootPathLength(path));
    path.chopLeft(getRootPathLength(path));
    #ifdef WIN32
    // hack for UNC
    if (realpath.length() == 1 && path.length() && isPathDelimiter(path[0])) {
      realpath += path[0];
      path.chopLeft(1);
    } else {
      for (int f = 0; f < realpath.length(); ++f) if (isPathDelimiter(realpath[f])) realpath.getMutableCStr()[f] = CANON_PATH_DELIM;
    }
    #endif
    while (path.length() && isPathDelimiter(path[0])) path.chopLeft(1);
  }

  while (path.length() && isPathDelimiter(path[path.length()-1])) path.chopRight(1);
  if (path.length() == 0) return realpath;

  #ifdef SFFCI_FULL_SCAN_DEBUG
  GLog.Logf(NAME_Debug, " realpath=<%s>; path=<%s>", *realpath, *path);
  #endif
  while (path.length()) {
    if (isPathDelimiter(path[0])) {
      //if (realpath.length() && !isPathDelimiter(realpath[0])) realpath += path[0];
      path.chopLeft(1);
      continue;
    }
    // get path component
    int pos = 0;
    while (pos < path.length() && !isPathDelimiter(path[pos])) ++pos;
    VStr ppart = path.left(pos);
    path.chopLeft(pos+1);
    #ifdef SFFCI_FULL_SCAN_DEBUG
    GLog.Logf(NAME_Debug, "  realpath=<%s>; path=<%s>; ppart=<%s>", *realpath, *path, *ppart);
    #endif
    // ignore "current dir"
    if (ppart == ".") continue;
    // process "upper dir" in-place
    if (ppart == "..") {
      int start = (isRootPath(realpath) ? getRootPathLength(realpath) : 0);
      int end = realpath.length();
      while (end > start && !isPathDelimiter(realpath[end])) --end;
      if (start == 0 && end > 0 && isPathDelimiter(realpath[end-1])) --end;
      realpath = realpath.left(end);
      #ifdef SFFCI_FULL_SCAN_DEBUG
      GLog.Logf(NAME_Debug, "    updir: realpath=<%s>", *realpath);
      #endif
      continue;
    }
    // add to real path
    if (realpath.length() && !isPathDelimiter(realpath[realpath.length()-1])) realpath += CANON_PATH_DELIM;
    //VStr prevpath = realpath;
    //realpath += ppart;
    const bool wantDir = (lastIsDir || path.length() != 0);
    // check "as-is"
    #ifndef SFFCI_FULL_SCAN
    VStr newpath = realpath+ppart;
    //if (Sys_FileTime(newpath) != -1) { realpath = newpath; continue; } // i found her!
    const bool okname = (wantDir ? Sys_DirExists(newpath) : Sys_FileExists(newpath));
    if (okname) { realpath = newpath; continue; } // i found her!
    #endif
    // scan directory
    void *dh = Sys_OpenDir((realpath.length() ? realpath : VStr(".")), wantDir);
    if (!dh) return VStr::EmptyString; // alas
    #ifdef SFFCI_FULL_SCAN
    VStr inexactName;
    vassert(inexactName.length() == 0);
    #endif
    for (;;) {
      VStr fname = Sys_ReadDir(dh);
      if (fname.length() == 0) {
        #ifdef SFFCI_FULL_SCAN
        if (inexactName.length()) break;
        #endif
        // alas
        Sys_CloseDir(dh);
        return VStr::EmptyString;
      }
      #ifdef SFFCI_FULL_SCAN_DEBUG
      GLog.Logf(NAME_Debug, "   fname=<%s>; ppart=<%s>; wantDir=%d", *fname, *ppart, (int)wantDir);
      #endif
      VStr diskName;
      if (wantDir) {
        if (!fname.endsWith("/")) continue;
        diskName = fname.left(fname.length()-1);
      } else {
        if (fname.endsWith("/")) continue; // just in case
        diskName = fname;
      }
      #ifdef SFFCI_FULL_SCAN
      if (ppart == diskName) { inexactName.clear(); break; }
      if (inexactName.length() == 0 && ppart.strEquCI(diskName)) inexactName = diskName;
      #else
      if (ppart.strEquCI(diskName)) { realpath += diskName; break; }
      #endif
    }
    Sys_CloseDir(dh);
    #ifdef SFFCI_FULL_SCAN
      #ifdef SFFCI_FULL_SCAN_DEBUG
      GLog.Logf(NAME_Debug, "   inexactName: <%s>", *inexactName);
      #endif
    if (inexactName.length() != 0) realpath += inexactName; else realpath += ppart;
    #endif
    #ifdef SFFCI_FULL_SCAN_DEBUG
    GLog.Logf(NAME_Debug, "  sce: realpath=<%s>; path=<%s>; ppart=<%s>", *realpath, *path, *ppart);
    #endif
  }
  vassert(path.length() == 0);
  #ifdef SFFCI_FULL_SCAN_DEBUG
  GLog.Logf(NAME_Debug, " done: realpath=<%s>", *realpath);
  #endif
  if (lastIsDir) {
    if (!Sys_DirExists(realpath)) return VStr::EmptyString; // oops
  } else {
    if (!Sys_FileExists(realpath)) return VStr::EmptyString; // oops
  }
  #ifdef SFFCI_FULL_SCAN_DEBUG
  GLog.Logf(NAME_Debug, " exit: realpath=<%s>", *realpath);
  #endif
  return realpath;
}


#ifndef WIN32
// normal OS
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>

struct DirInfo {
  DIR *dh;
  VStr path; // with slash
  bool wantDirs;
};


//==========================================================================
//
//  isRegularFile
//
//==========================================================================
static bool isRegularFile (VStr filename) {
  struct stat st;
  if (filename.length() == 0) return false;
  if (stat(*filename, &st) == -1) return false;
  return (S_ISREG(st.st_mode) != 0);
}


//==========================================================================
//
//  isDirectory
//
//==========================================================================
static bool isDirectory (VStr filename) {
  struct stat st;
  if (filename.length() == 0) return false;
  if (stat(*filename, &st) == -1) return false;
  return (S_ISDIR(st.st_mode) != 0);
}


//==========================================================================
//
//  Sys_FileExists
//
//==========================================================================
bool Sys_FileExists (VStr filename) {
  return (!filename.isEmpty() && access(*filename, R_OK) == 0 && isRegularFile(filename));
}


//==========================================================================
//
//  Sys_FileDelete
//
//==========================================================================
void Sys_FileDelete (VStr filename) {
  if (filename.length()) unlink(*filename);
}


//==========================================================================
//
//  Sys_FileTime
//
//  Returns -1 if not present
//
//==========================================================================
int Sys_FileTime (VStr path) {
  if (path.isEmpty()) return -1;
  struct stat buf;
  if (stat(*path, &buf) == -1) return -1;
  return (S_ISREG(buf.st_mode) ? buf.st_mtime : -1);
}


//==========================================================================
//
//  Sys_Touch
//
//==========================================================================
bool Sys_Touch (VStr path) {
  if (path.isEmpty()) return -1;
  utimbuf tv;
  tv.actime = tv.modtime = time(NULL);
  return (utime(*path, &tv) == 0);
}


//==========================================================================
//
//  Sys_CurrFileTime
//
//==========================================================================
int Sys_CurrFileTime () {
  return (int)time(NULL);
}


//==========================================================================
//
//  Sys_CreateDirectory
//
//==========================================================================
bool Sys_CreateDirectory (VStr path) {
  if (path.isEmpty()) return false;
  return (mkdir(*path, 0777) == 0);
}


//==========================================================================
//
//  Sys_OpenDir
//
//==========================================================================
void *Sys_OpenDir (VStr path, bool wantDirs) {
  if (path.isEmpty()) return nullptr;
  path = path.removeTrailingSlash();
  DIR *dh = opendir(*path);
  if (!dh) return nullptr;
  auto res = (DirInfo *)Z_Malloc(sizeof(DirInfo));
  if (!res) { closedir(dh); return nullptr; }
  memset((void *)res, 0, sizeof(DirInfo));
  res->dh = dh;
  res->path = path;
  res->wantDirs = wantDirs;
  if (res->path.length() == 0) res->path = "./";
  if (res->path[res->path.length()-1] != '/') res->path += "/";
  return (void *)res;
}


//==========================================================================
//
//  Sys_ReadDir
//
//==========================================================================
VStr Sys_ReadDir (void *adir) {
  if (!adir) return VStr();
  DirInfo *dh = (DirInfo *)adir;
  if (!dh->dh) return VStr();
  for (;;) {
    struct dirent *de = readdir(dh->dh);
    if (!de) break;
    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
    VStr diskName = dh->path+de->d_name;
    bool isRegFile = isRegularFile(diskName);
    if (!isRegFile && !dh->wantDirs) continue; // skip directories
    if (!isRegFile && !isDirectory(diskName)) continue; // something strange
    VStr res = VStr(de->d_name);
    if (!isRegFile) res += "/";
    return res;
  }
  closedir(dh->dh);
  dh->dh = nullptr;
  dh->path.clear();
  return VStr();
}


//==========================================================================
//
//  Sys_CloseDir
//
//==========================================================================
void Sys_CloseDir (void *adir) {
  if (adir) {
    DirInfo *dh = (DirInfo *)adir;
    if (dh->dh) closedir(dh->dh);
    dh->path.clear();
    Z_Free((void *)dh);
  }
}


//==========================================================================
//
//  Sys_DirExists
//
//==========================================================================
bool Sys_DirExists (VStr path) {
  if (path.isEmpty()) return false;
  path = path.removeTrailingSlash();
  struct stat s;
  if (stat(*path, &s) == -1) return false;
  return !!S_ISDIR(s.st_mode);
}


//==========================================================================
//
//  Sys_Time
//
//  return valud should not be zero
//
//==========================================================================
double Sys_Time () {
  static bool initialized = false;
#ifdef __linux__
  static time_t secbase = 0;
  struct timespec ts;
  if (clock_gettime(/*CLOCK_MONOTONIC*/CLOCK_MONOTONIC_RAW, &ts) != 0) Sys_Error("clock_gettime failed");
  if (!initialized) {
    initialized = true;
    secbase = ts.tv_sec;
  }
  return (ts.tv_sec-secbase)+ts.tv_nsec/1000000000.0+1.0;
#else
  struct timeval tp;
  struct timezone tzp;
  static int secbase = 0;
  gettimeofday(&tp, &tzp);
  if (!initialized) {
    initialized = true;
    secbase = tp.tv_sec;
  }
  return (tp.tv_sec-secbase)+tp.tv_usec/1000000.0+1.0;
#endif
}


//==========================================================================
//
//  Sys_Time
//
// get time (start point is arbitrary) in nanoseconds
//
//==========================================================================
vuint64 Sys_GetTimeNano () {
#ifdef __linux__
  static bool initialized = false;
  static time_t secbase = 0;
  struct timespec ts;
  if (clock_gettime(/*CLOCK_MONOTONIC*/CLOCK_MONOTONIC_RAW, &ts) != 0) Sys_Error("clock_gettime failed");
  if (!initialized) {
    initialized = true;
    secbase = ts.tv_sec;
  }
  return (ts.tv_sec-secbase+1)*1000000000ULL+ts.tv_nsec;
#else
  return (vuint64)(Sys_Time()*1000000000.0);
#endif
}


//==========================================================================
//
//  Sys_Time_CPU
//
//  return valud should not be zero
//
//==========================================================================
double Sys_Time_CPU () {
#ifdef __linux__
  static bool initialized = false;
  static time_t secbase = 0;
  struct timespec ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) Sys_Error("clock_gettime failed");
  if (!initialized) {
    initialized = true;
    secbase = ts.tv_sec;
  }
  return (ts.tv_sec-secbase)+ts.tv_nsec/1000000000.0+1.0;
#else
  return Sys_Time();
#endif
}


//==========================================================================
//
//  Sys_GetTimeCPUNano
//
//==========================================================================
vuint64 Sys_GetTimeCPUNano () {
#ifdef __linux__
  static bool initialized = false;
  static time_t secbase = 0;
  struct timespec ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) Sys_Error("clock_gettime failed");
  if (!initialized) {
    initialized = true;
    secbase = ts.tv_sec;
  }
  return (ts.tv_sec-secbase+1)*1000000000ULL+ts.tv_nsec;
#else
  return Sys_GetTimeNano();
#endif
}


//==========================================================================
//
//  Sys_Yield
//
//==========================================================================
void Sys_Yield () {
  //usleep(1);
  /*static*/ const struct timespec sleepTime = {0, 28500000};
  nanosleep(&sleepTime, nullptr);
}


//==========================================================================
//
//  Sys_GetCPUCount
//
//==========================================================================
int Sys_GetCPUCount () {
  //nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
#ifdef NO_SYSCONF
# ifdef __SWITCH__
  int res = 4;
# else
  int res = 1;
# endif
#else
  int res = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  if (res < 1) return 1;
  if (res > 64) return 64; // arbitrary limit
  return res;
}


//==========================================================================
//
//  Sys_GetCurrentTID
//
//  let's hope that 32 bits are enough for thread ids on all OSes, lol
//
//==========================================================================
/*
vuint32 Sys_GetCurrentTID () {
  static __thread pid_t cachedTID = (pid_t)-1;
  if (__builtin_expect(cachedTID == (pid_t)-1, 0)) {
#ifdef __linux__
    cachedTID = syscall(__NR_gettid);
#else
// WARNING TO IMPLEMENTORS: this must be *FAST*!
// this function is used (or will be used) to protect several things
// like `VStr`, and it should not incur significant overhead, or your
// port will be dog-slow.
# error "Please, implement `Sys_GetCurrentTID()` for your OS!"
#endif
  }
  return (vuint32)cachedTID;
}
*/


//==========================================================================
//
//  Sys_GetUserName
//
//==========================================================================
VStr Sys_GetUserName () {
#ifndef __SWITCH__
  uid_t uid = geteuid();
  struct passwd *pw = getpwuid(uid);
  if (pw) return sys_NormalizeUserName(pw->pw_name);
#endif
  return sys_NormalizeUserName(nullptr);
}


#else

//==========================================================================
//
//  shitdoze
//
//==========================================================================

#include <signal.h>
#include <fcntl.h>
#define ftime fucked_ftime
#include <io.h>
#undef ftime
#include <direct.h>
#include <conio.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <windows.h>

#ifndef R_OK
# define R_OK  (4)
#endif


struct ShitdozeDir {
  HANDLE dir_handle;
  WIN32_FIND_DATA dir_buf;
  VStr path;
  bool gotName;
  bool wantDirs;
};


//==========================================================================
//
//  isRegularFile
//
//==========================================================================
static bool isRegularFile (VStr filename) {
  if (filename.length() == 0) return false;
  DWORD attrs = GetFileAttributes(*filename);
  if (attrs == INVALID_FILE_ATTRIBUTES) return false;
  return ((attrs&(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_DIRECTORY)) == 0);
}


//==========================================================================
//
//  isDirectory
//
//==========================================================================
static bool isDirectory (VStr filename) {
  if (filename.length() == 0) return false;
  DWORD attrs = GetFileAttributes(*filename);
  if (attrs == INVALID_FILE_ATTRIBUTES) return false;
  return ((attrs&FILE_ATTRIBUTE_DIRECTORY) != 0);
}


//==========================================================================
//
//  Sys_FileExists
//
//==========================================================================
bool Sys_FileExists (VStr filename) {
  return (!filename.isEmpty() && access(*filename, R_OK) == 0 && isRegularFile(filename));
}


//==========================================================================
//
//  Sys_FileDelete
//
//==========================================================================
void Sys_FileDelete (VStr filename) {
  if (filename.length()) DeleteFile(*filename);
}


//==========================================================================
//
//  Sys_FileTime
//
//  Returns -1 if not present
//
//==========================================================================
int Sys_FileTime (VStr path) {
  if (path.isEmpty()) return -1;
  if (!isRegularFile(path)) return -1;
  struct stat buf;
  if (stat(*path, &buf) == -1) return -1;
  return buf.st_mtime;
}


//==========================================================================
//
//  Sys_Touch
//
//==========================================================================
bool Sys_Touch (VStr path) {
  if (path.isEmpty()) return -1;
  utimbuf tv;
  tv.actime = tv.modtime = time(NULL);
  return (utime(*path, &tv) == 0);
}


//==========================================================================
//
//  Sys_CurrFileTime
//
//==========================================================================
int Sys_CurrFileTime () {
  return (int)time(NULL);
}


//==========================================================================
//
//  Sys_CreateDirectory
//
//==========================================================================
bool Sys_CreateDirectory (VStr path) {
  if (path.isEmpty()) return false;
  return (mkdir(*path) == 0);
}


//==========================================================================
//
//  Sys_OpenDir
//
//==========================================================================
void *Sys_OpenDir (VStr dirname, bool wantDirs) {
  if (dirname.isEmpty()) return nullptr;
  auto sd = (ShitdozeDir *)Z_Malloc(sizeof(ShitdozeDir));
  if (!sd) return nullptr;
  memset((void *)sd, 0, sizeof(ShitdozeDir));
  VStr path = dirname;
  if (path.length() == 0) path = "./";
  if (path[path.length()-1] != '/' && path[path.length()-1] != '\\' && path[path.length()-1] != ':') path += "/";
  sd->dir_handle = FindFirstFile(va("%s*.*", *path), &sd->dir_buf);
  if (sd->dir_handle == INVALID_HANDLE_VALUE) { Z_Free(sd); return nullptr; }
  sd->gotName = true;
  sd->path = path;
  sd->wantDirs = wantDirs;
  return (void *)sd;
}


//==========================================================================
//
//  Sys_ReadDir
//
//==========================================================================
VStr Sys_ReadDir (void *adir) {
  if (!adir) return VStr();
  auto sd = (ShitdozeDir *)adir;
  for (;;) {
    if (!sd->gotName) {
      if (FindNextFile(sd->dir_handle, &sd->dir_buf) != TRUE) return VStr();
    }
    sd->gotName = false;
    auto res = VStr(sd->dir_buf.cFileName);
    if (res == "." || res == "..") continue;
    VStr diskName = sd->path+res;
    bool isRegFile = isRegularFile(diskName);
    if (!isRegFile && !sd->wantDirs) continue; // skip directories
    if (!isRegFile && !isDirectory(diskName)) continue; // something strange
    if (!isRegFile) res += "/";
    return res;
  }
}


//==========================================================================
//
//  Sys_CloseDir
//
//==========================================================================
void Sys_CloseDir (void *adir) {
  if (adir) {
    auto sd = (ShitdozeDir *)adir;
    FindClose(sd->dir_handle);
    sd->path.clear();
    Z_Free(sd);
  }
}


//==========================================================================
//
//  Sys_DirExists
//
//==========================================================================
bool Sys_DirExists (VStr path) {
  if (path.isEmpty()) return false;
  path = path.removeTrailingSlash();
  struct stat s;
  if (stat(*path, &s) == -1) return false;
  return !!(s.st_mode & S_IFDIR);
}


#ifdef SHITDOZE_USE_WINMM
static bool shitdozeTimerInited = false;
static vuint32 shitdozeLastTime = 0;
static vuint64 shitdozeCurrTime = 0;

struct ShitdozeTimerInit {
  ShitdozeTimerInit () { timeBeginPeriod(1); shitdozeTimerInited = true; shitdozeLastTime = (vuint32)timeGetTime(); }
  ~ShitdozeTimerInit () { timeEndPeriod(1); shitdozeTimerInited = false; }
};

ShitdozeTimerInit thisIsFuckinShitdozeTimerInitializer;


double Sys_Time () {
  if (!shitdozeTimerInited) Sys_Error("shitdoze shits itself");
  vuint32 currtime = (vuint32)timeGetTime();
  if (currtime > shitdozeLastTime) {
    shitdozeCurrTime += currtime-shitdozeLastTime;
  } else {
    // meh; do nothing on wraparound, this should only happen once
  }
  shitdozeLastTime = currtime;
  return shitdozeCurrTime/1000.0+1.0;
}


//==========================================================================
//
//  Sys_GetTimeNano
//
//==========================================================================
vuint64 Sys_GetTimeNano () {
  if (!shitdozeTimerInited) Sys_Error("shitdoze shits itself");
  vuint32 currtime = (vuint32)timeGetTime();
  if (currtime > shitdozeLastTime) {
    shitdozeCurrTime += currtime-shitdozeLastTime;
  } else {
    // meh; do nothing on wraparound, this should only happen once
  }
  shitdozeLastTime = currtime;
  return (vuint64)(shitdozeCurrTime+1000)*1000000ULL; // msecs -> nsecs
}


#else
//==========================================================================
//
//  Sys_Time
//
//==========================================================================
double Sys_Time () {
  static double pfreq;
  static double curtime = 0.0;
  static double lastcurtime = 0.0;
  static vuint32 oldtime;
  static int lowshift;
  static bool initialized = false;

  if (!initialized) {
    LARGE_INTEGER PerformanceFreq;
    LARGE_INTEGER PerformanceCount;
    vuint32 lowpart, highpart;

    if (!QueryPerformanceFrequency(&PerformanceFreq)) Sys_Error("No hardware timer available");
    // get 32 out of the 64 time bits such that we have around
    // 1 microsecond resolution
    lowpart = (vuint32)PerformanceFreq.u.LowPart;
    highpart = (vuint32)PerformanceFreq.u.HighPart;
    lowshift = 0;
    while (highpart || lowpart > 2000000.0) {
      ++lowshift;
      lowpart >>= 1;
      lowpart |= (highpart&1)<<31;
      highpart >>= 1;
    }

    pfreq = 1.0 / (double)lowpart;

    // read current time and set old time
    QueryPerformanceCounter(&PerformanceCount);

    oldtime = ((vuint32)PerformanceCount.u.LowPart>>lowshift)|((vuint32)PerformanceCount.u.HighPart<<(32-lowshift));

    // set start time
    /*
    const char *p = GArgs.CheckValue("-starttime");
    if (p) {
      curtime = (double)atof(p);
    } else {
      curtime = 0.0;
    }
    */
    curtime = 0.0;
    lastcurtime = curtime;
    initialized = true;
  }

  static int sametimecount;
  LARGE_INTEGER PerformanceCount;
  vuint32 temp, t2;
  double time;

  QueryPerformanceCounter(&PerformanceCount);

  temp = ((unsigned int)PerformanceCount.u.LowPart>>lowshift)|((unsigned int)PerformanceCount.u.HighPart<<(32-lowshift));

  // check for turnover or backward time
  if (temp <= oldtime && oldtime-temp < 0x10000000) {
    oldtime = temp; // so we can't get stuck
  } else {
    t2 = temp-oldtime;

    time = (double)t2*pfreq;
    oldtime = temp;

    curtime += time;

    if (curtime == lastcurtime) {
      ++sametimecount;
      if (sametimecount > 100000) {
        curtime += 1.0;
        sametimecount = 0;
      }
    } else {
      sametimecount = 0;
    }
    lastcurtime = curtime;
  }

  return curtime+1.0;
}


//==========================================================================
//
//  Sys_GetTimeNano
//
//==========================================================================
vuint64 Sys_GetTimeNano () {
  return (vuint64)(Sys_Time()*1000000000.0);
}
#endif


//==========================================================================
//
//  Sys_Time_CPU
//
//==========================================================================
double Sys_Time_CPU () {
  return Sys_Time();
}


//==========================================================================
//
//  Sys_GetTimeCPUNano
//
//==========================================================================
vuint64 Sys_GetTimeCPUNano () {
  return Sys_GetTimeNano();
}


//==========================================================================
//
//  Sys_Yield
//
//==========================================================================
void Sys_Yield () {
  Sleep(1);
}


//==========================================================================
//
//  Sys_GetCPUCount
//
//==========================================================================
int Sys_GetCPUCount () {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  int res = (int)info.dwNumberOfProcessors;
  if (res < 1) return 1;
  if (res > 64) return 64; // arbitrary limit
  return res;
}


//==========================================================================
//
//  Sys_GetCurrentTID
//
//  let's hope that 32 bits are enough for thread ids on all OSes, lol
//
//==========================================================================
/*
vuint32 Sys_GetCurrentTID () {
  return (vuint32)GetCurrentThreadId();
}
*/


//==========================================================================
//
//  Sys_GetUserName
//
//==========================================================================
VStr Sys_GetUserName () {
  char buf[32];
  DWORD len;
  memset(buf, 0, sizeof(buf));
  if (!GetUserNameA(buf, &len)) buf[0] = 0;
  buf[sizeof(buf)-1] = 0; // just in case
  return sys_NormalizeUserName(buf);
}
#endif
