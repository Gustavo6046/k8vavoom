//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
//**  Copyright (C) 2018-2019 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, either version 3 of the License, or
//**  (at your option) any later version.
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
#include "gamedefs.h"
#include "net/network.h"
#include "cl_local.h"
#include "ui/ui.h"
#include "neoui/neoui.h"
#include <time.h>
#ifdef SERVER
# include "render/r_local.h"
#endif

extern int fsys_warp_n0;
extern int fsys_warp_n1;
extern VStr fsys_warp_cmd;

static const double max_fps_cap = 0.004; // no more than 250 FPS
extern bool sv_skipOneTitlemap;


// state updates, number of tics/second
#define TICRATE  (35)


class EndGame : public VavoomError {
public:
  explicit EndGame (const char *txt) : VavoomError(txt) {}
};


void Host_Quit ();


#ifdef DEVELOPER
VCvarB developer("developer", true, "Developer (debug) mode?", 0/*CVAR_Archive*/);
#else
VCvarB developer("developer", false, "Developer (debug) mode?", 0/*CVAR_Archive*/);
#endif

#ifdef VAVOOM_K8_DEVELOPER
# define CVAR_K8_DEV_VALUE  true
#else
# define CVAR_K8_DEV_VALUE  false
#endif
VCvarB k8vavoom_developer_version("__k8vavoom_developer_version", CVAR_K8_DEV_VALUE, "Don't even think about this.", CVAR_Rom);


int host_frametics = 0; // used only in non-realtime mode
double host_frametime = 0;
double host_framefrac = 0; // unused frame time left from previous `SV_Ticker()` in realtime mode
double host_time = 0;
double realtime = 0;
double oldrealtime = 0;
int host_framecount = 0;

bool host_initialised = false;
bool host_request_exit = false;

extern VCvarB real_time;


#ifndef CLIENT
class VDedLog : public VLogListener {
public:
  virtual void Serialise (const char *Text, EName) override { printf("%s", Text); }
};
static VDedLog  DedLog;
#endif


#if defined(_WIN32) || defined(__SWITCH__)
VCvarB game_release_mode("_release_mode", true, "Affects some default settings.", CVAR_Rom);
#else
VCvarB game_release_mode("_release_mode", false, "Affects some default settings.", CVAR_Rom);
#endif

// for chex quest support
//VCvarI game_override_mode("_game_override", 0, "Override game type for DooM game.", CVAR_Rom);

static VCvarF host_framerate("dbg_framerate", "0", "Hard limit on frame time (in seconds); DEBUG CVAR, DON'T USE!", CVAR_PreInit);
//k8: this was `3`; why 3? looks like arbitrary number
VCvarI host_max_skip_frames("dbg_host_max_skip_frames", "12", "Process no more than this number of full frames if frame rate is too slow; DEBUG CVAR, DON'T USE!");
static VCvarB host_show_skip_limit("dbg_host_show_skip_limit", false, "Show skipframe limit hits? (DEBUG CVAR, DON'T USE!)", CVAR_PreInit);
static VCvarB host_show_skip_frames("dbg_host_show_skip_frames", false, "Show skipframe hits? (DEBUG CVAR, DON'T USE!)", CVAR_PreInit);

static double last_time;

static VCvarB randomclass("RandomClass", false, "Random player class?"); // checkparm of -randclass
VCvarB respawnparm("RespawnMonsters", false, "Respawn monsters?", CVAR_PreInit); // checkparm of -respawn
VCvarI fastparm("g_fast_monsters", "0", "Fast(1), slow(2), normal(0) monsters?", CVAR_PreInit); // checkparm of -fast

static VCvarI g_fastmon_override("g_fast_monsters_override", "0", "Fast(1), slow(2), normal(0) monsters?", CVAR_PreInit|CVAR_Archive);
static VCvarI g_respawn_override("g_monsters_respawn_override", "0", "Override monster respawn (time in seconds).", CVAR_PreInit|CVAR_Archive);
static VCvarI g_spawn_filter_override("g_skill_spawn_filter_override", "0", "Override spawn flags.", CVAR_PreInit);
static VCvarI g_spawn_limit_override("g_skill_respawn_limit_override", "0", "Override spawn limit.", CVAR_PreInit|CVAR_Archive);
static VCvarF g_ammo_factor_override("g_skill_ammo_factor_override", "0", "Override ammo multiplier.", CVAR_PreInit|CVAR_Archive);
static VCvarF g_damage_factor_override("g_skill_damage_factor_override", "0", "Override damage multiplier.", CVAR_PreInit|CVAR_Archive);
static VCvarF g_aggressiveness_override("g_skill_aggressiveness_override", "0", "Override monster aggresiveness.", CVAR_PreInit|CVAR_Archive);


static VCvarB show_time("dbg_show_times", false, "Show some debug times?", CVAR_PreInit);

static VCvarS configfile("configfile", "config.cfg", "Config file name.", CVAR_Archive|CVAR_PreInit);

static char CurrentLanguage[4];
static VCvarS Language("language", "en", "Game language.", CVAR_Archive|CVAR_PreInit);

static VCvarB cap_framerate("cl_cap_framerate", true, "Cap framerate for non-networking games?", CVAR_Archive);
static VCvarI cl_framerate("cl_framerate", "0", "Cap framerate for non-networking games?", CVAR_Archive);


//==========================================================================
//
//  Host_Init
//
//==========================================================================
void Host_Init () {
  (void)Sys_Time(); // this initializes timer

#ifdef CLIENT
  C_Init();
#else
  GLog.AddListener(&DedLog);
#endif

#if !defined(_WIN32)
  const char *HomeDir = getenv("HOME");
  if (HomeDir) {
    Sys_CreateDirectory(va("%s/.vavoom", HomeDir));
    OpenDebugFile(va("%s/.vavoom/debug.txt", HomeDir));
  } else {
    OpenDebugFile("basev/debug.txt");
  }
#else
  OpenDebugFile("basev/debug.txt");
#endif

  // seed the random-number generator
  RandomInit();

  // init subsystems
  VName::StaticInit();
  VObject::StaticInit();

  CVars_Init();
  VCommand::Init();

  GCon->Log(NAME_Init, "---------------------------------------------------------------");
  GCon->Logf(NAME_Init, "k8vavoom Game Engine, started by Janis Legzdinsh | %s", __DATE__);
  GCon->Log(NAME_Init, "also starring Francisco Ortega, and others (k8:drop me a note!)");
  GCon->Log(NAME_Init, "Ketmar Dark: improvements, bugfixes, new bugs, segfaults, etc.");
  GCon->Log(NAME_Init, "alot of invaluable help and testing (esp. x86_64): id0");
  GCon->Log(NAME_Init, "---------------------------------------------------------------");
  GCon->Log(NAME_Init, "REMEMBER: BY USING FOSS SOFTWARE, YOU ARE SUPPORTING COMMUNISM!");
  GCon->Log(NAME_Init, "---------------------------------------------------------------");

  if (GArgs.CheckParm("-vc-lax-override")) VMemberBase::optDeprecatedLaxOverride = true;
  if (GArgs.CheckParm("-vc-lax-states")) VMemberBase::optDeprecatedLaxStates = true;
  if (GArgs.CheckParm("-developer")) developer = true;

  if (GArgs.CheckParm("-vc-legacy-korax")) {
    VMemberBase::koraxCompatibility = true;
    VMemberBase::optDeprecatedLaxOverride = true;
  }
  if (GArgs.CheckParm("-vc-legacy-korax-no-warnings")) VMemberBase::koraxCompatibilityWarnings = false;

  if (GArgs.CheckParm("-gd-debug")) VObject::GCDebugMessagesAllowed = true;

  FL_ProcessPreInits();

  FL_Init();

  PR_Init();

  GLanguage.LoadStrings("en");
  strcpy(CurrentLanguage, "en");

  GSoundManager = new VSoundManager;
  GSoundManager->Init();
  R_InitData();
  R_InitTexture();

  GNet = VNetworkPublic::Create();
  GNet->Init();

  ReadLineSpecialInfos();

  //R_InitHiResTextures(false); // init multipatch textures, but no hires replacement

#ifdef SERVER
  SV_Init();
#endif

#ifdef CLIENT
  CL_Init();
#endif

  // "compile only"
  if (GArgs.CheckParm("-c") != 0) {
    exit(0);
  }

#ifdef CLIENT
  GInput = VInputPublic::Create();
  GInput->Init();
  GAudio = VAudioPublic::Create();
  GAudio->Init();
  SCR_Init();
  CT_Init();
  V_Init();

  R_Init();

  T_Init();

  MN_Init();
  AM_Init();
  SB_Init();
#endif

#ifdef SERVER
  R_InitSkyBoxes();
#endif

  VCommand::ProcessKeyConf();

  R_ParseEffectDefs();

  InitMapInfo();
  P_SetupMapinfoPlayerClasses();

  //GCon->Logf("WARP: n0=%d; n1=%d; cmd=<%s>", fsys_warp_n0, fsys_warp_n1, *fsys_warp_cmd);


#ifndef CLIENT
  bool wasWarp = false;

  if (GGameInfo->NetMode == NM_None) {
    //GCmdBuf << "MaxPlayers 4\n";
    VCommand::cliPreCmds += "MaxPlayers 4\n";
  }
#endif

  if (fsys_warp_n0 >= 0 && fsys_warp_n1 < 0) {
    VName maplump = P_TranslateMapEx(fsys_warp_n0);
    if (maplump != NAME_None) {
      GCon->Logf("WARP: %d translated to '%s'", fsys_warp_n0, *maplump);
      fsys_warp_n0 = -1;
      VStr mcmd = "map ";
      mcmd += *maplump;
      mcmd += "\n";
      //GCmdBuf.Insert(mcmd);
      VCommand::cliPreCmds += mcmd;
      fsys_warp_cmd = VStr();
      sv_skipOneTitlemap = true;
#ifndef CLIENT
      wasWarp = true;
#endif
    }
  }

  if (/*fsys_warp_n0 >= 0 &&*/ fsys_warp_cmd.length()) {
    //GCmdBuf.Insert(fsys_warp_cmd);
    VCommand::cliPreCmds += fsys_warp_cmd;
    if (!fsys_warp_cmd.endsWith("\n")) VCommand::cliPreCmds += '\n';
    sv_skipOneTitlemap = true;
#ifndef CLIENT
    wasWarp = true;
#endif
  }

  GCmdBuf.Exec();

  //FIXME
  R_InitHiResTextures(true); // init only hires replacements

#ifndef CLIENT
  if (GGameInfo->NetMode == NM_None && !wasWarp) {
    //GCmdBuf << "MaxPlayers 4\n";
    //GCmdBuf << "Map " << *P_TranslateMap(1) << "\n";
    VCommand::cliPreCmds += "Map \"";
    VCommand::cliPreCmds += VStr(*P_TranslateMap(1)).quote();
    VCommand::cliPreCmds += "\"\n";
    sv_skipOneTitlemap = true;
  }
#endif

  host_initialised = true;
  VCvar::HostInitComplete();
}


//==========================================================================
//
//  Host_GetConsoleCommands
//
//  Add them exactly as if they had been typed at the console
//
//==========================================================================
static void Host_GetConsoleCommands () {
  char *cmd;

#ifdef CLIENT
  if (GGameInfo->NetMode != NM_DedicatedServer) return;
#endif

  for (cmd = Sys_ConsoleInput(); cmd; cmd = Sys_ConsoleInput()) {
    GCmdBuf << cmd << "\n";
  }
}


//==========================================================================
//
//  Host_ResetSkipFrames
//
//  call this after saving/loading/map loading, so we won't
//  unnecessarily skip frames
//
//==========================================================================
void Host_ResetSkipFrames () {
  last_time = Sys_Time();
  host_frametics = 0;
  host_frametime = 0;
  host_framefrac = 0;
  //GCon->Logf("*** Host_ResetSkipFrames; ctime=%f", last_time);
}


//==========================================================================
//
//  FilterTime
//
//  Returns false if the time is too short to run a frame
//
//==========================================================================
static bool FilterTime () {
  double curr_time = Sys_Time();
  double time = curr_time-last_time;

  //GCon->Logf("*** FilterTime; lasttime=%f; ctime=%f; time=%f", last_time, curr_time, time);

  if (time < 0.004) return false;

  last_time = curr_time;
  //realtime += time;
  // now fix "real time"
  realtime = curr_time;

  const double timeDelta = realtime-oldrealtime;
  //const double timeDelta = curr_time-oldrealtime;

  if (real_time) {
    int cfr = cl_framerate;
    if (cfr < 1 || cfr > 200) cfr = 0;
    if (cfr && (GGameInfo->NetMode == NM_None || GGameInfo->NetMode == NM_Standalone)) {
      // capped fps
      if (timeDelta < 1.0/(double)cfr) return false; // framerate is too high
    } else if (!cap_framerate && (GGameInfo->NetMode == NM_None || GGameInfo->NetMode == NM_Standalone)) {
      // uncapped fps
      if (realtime <= oldrealtime) return false;
      if (timeDelta < max_fps_cap) return false;
    } else {
      // capped fps
      if (timeDelta < 1.0/90.0) return false; // framerate is too high
    }
  } else {
    if (timeDelta < 1.0/35.0) return false; // framerate is too high
  }

  host_frametime = realtime-oldrealtime+host_framefrac;
  host_framefrac = 0;

  int ticlimit = host_max_skip_frames;
  if (ticlimit < 3) ticlimit = 3; else if (ticlimit > 256) ticlimit = 256;

  if (host_framerate > 0) {
    host_frametime = host_framerate;
  } else {
    // don't allow really long or short frames
    double tld = (ticlimit+1)/(double)TICRATE;
    if (host_frametime > tld) {
      if (developer) GCon->Logf(NAME_Dev, "*** FRAME TOO LONG: %f (capped to %f)", host_frametime, tld);
      host_frametime = tld;
    }
    if (host_frametime < max_fps_cap) host_frametime = max_fps_cap; // just in case
  }

  int thistime;
  static int lasttime;

  thistime = (int)(realtime*TICRATE);
  host_frametics = thistime-lasttime;
  if (!real_time && host_frametics < 1) return false; // no tics to run
  if (host_frametics > ticlimit) {
         if (host_show_skip_limit) GCon->Logf(NAME_Warning, "want to skip %d tics, but only %d allowed", host_frametics, ticlimit);
    else if (developer) GCon->Logf(NAME_Dev, "want to skip %d tics, but only %d allowed", host_frametics, ticlimit);
    host_frametics = ticlimit; // don't run too slow
  }
  if (host_frametics > 1 && host_show_skip_frames) GCon->Logf(NAME_Warning, "processing %d ticframes", host_frametics);

  oldrealtime = realtime;
  lasttime = thistime;

  return true;
}


//==========================================================================
//
//  Host_UpdateLanguage
//
//==========================================================================
static void Host_UpdateLanguage () {
  if (!Language.IsModified()) return;

  VStr NewLang = VStr((const char*)Language).ToLower();
  if (NewLang.Length() != 2 && NewLang.Length() != 3) {
    GCon->Log("Language identifier must be 2 or 3 characters long");
    Language = CurrentLanguage;
    return;
  }

  if (Language == CurrentLanguage) return;

  GLanguage.LoadStrings(*NewLang);
  VStr::Cpy(CurrentLanguage, *NewLang);
}


//==========================================================================
//
//  Host_Frame
//
//  Runs all active servers
//
//==========================================================================
void Host_Frame () {
  static double time1 = 0;
  static double time2 = 0;
  static double time3 = 0;
  int pass1, pass2, pass3;

  try {
    // decide the simulation time
    if (!FilterTime()) {
      // don't run too fast, or packets will flood out
#ifndef CLIENT
      Sys_Yield();
#endif
      return;
    }

    Host_UpdateLanguage();

#ifdef CLIENT
    // get new key, mice and joystick events
    GInput->ProcessEvents();
#endif

    // check for commands typed to the host
    Host_GetConsoleCommands();

    // process console commands
    GCmdBuf.Exec();
    if (host_request_exit) Host_Quit();

    bool incFrame = false;

    GNet->Poll();

    // if we perfomed load/save, frame time will be near zero, so do nothing
    if (host_frametime >= max_fps_cap) {
      incFrame = true;

#ifdef CLIENT
      // make intentions
      CL_SendMove();
#endif

#ifdef SERVER
      if (GGameInfo->NetMode != NM_None && GGameInfo->NetMode != NM_Client) {
        // server operations
        ServerFrame(host_frametics);
      }
#endif

      host_time += host_frametime;

#ifdef CLIENT
      // fetch results from server
      CL_ReadFromServer();

      // update user interface
      GRoot->TickWidgets(host_frametime);
#endif
    }

    // collect all garbage
    VObject::CollectGarbage();

#ifdef CLIENT
    // update video
    if (show_time) time1 = Sys_Time();
    SCR_Update();
    if (show_time) time2 = Sys_Time();

    if (cls.signon) CL_DecayLights();

    // update audio
    GAudio->UpdateSounds();
#endif

    if (show_time) {
      pass1 = (int)((time1-time3)*1000);
      time3 = Sys_Time();
      pass2 = (int)((time2-time1)*1000);
      pass3 = (int)((time3-time2)*1000);
      GCon->Logf("%d tot | %d server | %d gfx | %d snd", pass1+pass2+pass3, pass1, pass2, pass3);
    }

    if (incFrame) {
      ++host_framecount;
    } else {
      if (developer) GCon->Log(NAME_Dev, "Frame delayed due to lengthy operation (this is perfectly ok)");
    }
  } catch (RecoverableError &e) {
    GCon->Logf("Host_Error: %s", e.message);

    // reset progs virtual machine
    PR_OnAbort();
    // make sure, that we use primary wad files
    W_CloseAuxiliary();

#ifdef CLIENT
    if (GGameInfo->NetMode == NM_DedicatedServer) {
      SV_ShutdownGame();
      Sys_Error("Host_Error: %s\n", e.message); // dedicated servers exit
    }

    SV_ShutdownGame();
    GClGame->eventOnHostError();
    C_StartFull();
#else
    SV_ShutdownGame();
    Sys_Error("Host_Error: %s\n", e.message); // dedicated servers exit
#endif
  } catch (EndGame &e) {
    GCon->Logf(NAME_Dev, "Host_EndGame: %s", e.message);

    // reset progs virtual machine
    PR_OnAbort();
    // make sure, that we use primary wad files
    W_CloseAuxiliary();

#ifdef CLIENT
    if (GGameInfo->NetMode == NM_DedicatedServer) {
      SV_ShutdownGame();
      Sys_Error("Host_EndGame: %s\n", e.message); // dedicated servers exit
    }

    SV_ShutdownGame();
    GClGame->eventOnHostEndGame();
#else
    SV_ShutdownGame();
    Sys_Error("Host_EndGame: %s\n", e.message); // dedicated servers exit
#endif
  }
}


//==========================================================================
//
//  Host_EndGame
//
//==========================================================================
void Host_EndGame (const char *message, ...) {
  va_list argptr;
  static char string[4096];

  va_start(argptr, message);
  vsnprintf(string, sizeof(string), message, argptr);
  va_end(argptr);

  throw EndGame(string);
}


//==========================================================================
//
//  Host_Error
//
//  This shuts down both the client and server
//
//==========================================================================
void Host_Error (const char *error, ...) {
  va_list argptr;
  static char string[4096];

  va_start(argptr, error);
  vsnprintf(string, sizeof(string), error, argptr);
  va_end(argptr);

  throw RecoverableError(string);
}


//==========================================================================
//
//  Version_f
//
//==========================================================================
COMMAND(Version) {
  GCon->Log("VAVOOM version " VERSION_TEXT ".");
  GCon->Log("Compiled " __DATE__ " " __TIME__ ".");
}


//==========================================================================
//
//  Host_GetConfigDir
//
//==========================================================================
VStr Host_GetConfigDir () {
  return FL_GetConfigDir();
}


#ifdef CLIENT
//==========================================================================
//
//  Host_SaveConfiguration
//
//  Saves game variables
//
//==========================================================================
void Host_SaveConfiguration () {
  if (!host_initialised) return;

  VStr cfgdir = Host_GetConfigDir();
  FL_CreatePath(*cfgdir);

  cfgdir = cfgdir+"/"+configfile;

  FILE *f = fopen(*cfgdir, "w");
  if (!f) {
    GCon->Logf("Host_SaveConfiguration: Failed to open config file \"%s\"", *cfgdir);
    return; // can't write the file, but don't fail
  }

  fprintf(f, "// Generated by Vavoom\n");
  fprintf(f, "//\n// Bindings\n//\n");
  GInput->WriteBindings(f);
  fprintf(f, "//\n// Aliases\n//\n");
  VCommand::WriteAlias(f);
  fprintf(f, "//\n// Variables\n//\n");
  VCvar::WriteVariablesToFile(f);

  fclose(f);
}

COMMAND(SaveConfig) {
  if (!host_initialised) return;
  VStr cfgdir = Host_GetConfigDir();
  cfgdir = cfgdir+"/"+configfile;
  GCon->Logf("writing config to '%s'...", *cfgdir);
  Host_SaveConfiguration();
}
#endif


//==========================================================================
//
//  Host_Quit
//
//==========================================================================
#ifndef _WIN32
# include <stdio.h>
# include <unistd.h>
#endif
void Host_Quit () {
  SV_ShutdownGame();
#ifdef CLIENT
  // save game configyration
  Host_SaveConfiguration();
#endif

  // get the lump with the end text
  // if option -noendtxt is set, don't print the text
  bool GotEndText = false;
  char EndText[80*25*2];
  if (GArgs.CheckParm("-endtxt")) {
    // find end text lump
    VStream *Strm = nullptr;
         if (W_CheckNumForName(NAME_endoom) >= 0) Strm = W_CreateLumpReaderName(NAME_endoom);
    else if (W_CheckNumForName(NAME_endtext) >= 0) Strm = W_CreateLumpReaderName(NAME_endtext);
    else if (W_CheckNumForName(NAME_endstrf) >= 0) Strm = W_CreateLumpReaderName(NAME_endstrf);
    // read it, if found
    if (Strm) {
      int Len = 80*25*2;
      if (Strm->TotalSize() < Len) {
        memset(EndText, 0, Len);
        Len = Strm->TotalSize();
      }
      Strm->Serialise(EndText, Len);
      delete Strm;
      GotEndText = true;
    }
#ifndef _WIN32
    if (!isatty(STDOUT_FILENO)) GotEndText = false;
#endif
  }

  Sys_Quit(GotEndText ? EndText : nullptr);
}


//==========================================================================
//
//  Quit
//
//==========================================================================
COMMAND(Quit) {
  host_request_exit = true;
}


//==========================================================================
//
//  Host_Shutdown
//
//  Return to default system state
//
//==========================================================================
void Host_Shutdown () {
  static bool shutting_down = false;

  if (shutting_down) {
    GCon->Log("Recursive shutdown");
    return;
  }
  shutting_down = true;

#define SAFE_SHUTDOWN(name, args) \
  try { /*GCon->Log("Doing "#name);*/ name args; } catch (...) { GCon->Log(#name" failed"); }

#ifdef CLIENT
  //k8:no need to do this:SAFE_SHUTDOWN(C_Shutdown, ()) // console
  SAFE_SHUTDOWN(CL_Shutdown, ())
#endif
#ifdef SERVER
  SAFE_SHUTDOWN(SV_Shutdown, ())
#endif
  if (GNet) {
    SAFE_SHUTDOWN(delete GNet,)
    GNet = nullptr;
  }
#ifdef CLIENT
  if (GInput) {
    SAFE_SHUTDOWN(delete GInput,)
    GInput = nullptr;
  }
  SAFE_SHUTDOWN(V_Shutdown, ()) // video
  if (GAudio) {
    SAFE_SHUTDOWN(delete GAudio,)
    GAudio = nullptr;
  }
  //k8:no need to do this:SAFE_SHUTDOWN(T_Shutdown, ()) // font system
#endif
  //k8:no need to do this:SAFE_SHUTDOWN(Sys_Shutdown, ()) // nothing at all

  if (GSoundManager) {
    SAFE_SHUTDOWN(delete GSoundManager,)
    GSoundManager = nullptr;
  }

  VMemberBase::DumpNameMaps();
  //k8:no need to do this:SAFE_SHUTDOWN(R_ShutdownTexture, ()) // texture manager
  //k8:no need to do this:SAFE_SHUTDOWN(R_ShutdownData, ()) // various game tables
  //k8:no need to do this:SAFE_SHUTDOWN(VCommand::Shutdown, ())
  //k8:no need to do this:SAFE_SHUTDOWN(VCvar::Shutdown, ())
  //k8:no need to do this:SAFE_SHUTDOWN(ShutdownMapInfo, ()) // mapinfo
  //k8:no need to do this:SAFE_SHUTDOWN(FL_Shutdown, ()) // filesystem
  //k8:no need to do this:SAFE_SHUTDOWN(W_Shutdown, ()) // wads
  //k8:no need to do this:SAFE_SHUTDOWN(GLanguage.FreeData, ())
  //k8:no need to do this:SAFE_SHUTDOWN(ShutdownDecorate, ())

  SAFE_SHUTDOWN(VObject::StaticExit, ())
  //k8:no need to do this:SAFE_SHUTDOWN(VName::StaticExit, ())
  //SAFE_SHUTDOWN(Z_Shutdown, ())
  //GCon->Log("VaVoom: shutdown complete");

#ifdef CLIENT
  C_Shutdown(); // save log
#endif
}
