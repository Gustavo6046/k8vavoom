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
#include "sv_local.h"
#include "cl_local.h"


IMPLEMENT_CLASS(V, BasePlayer)

static VCvarF hud_notify_time("hud_notify_time", "5", "Notification timeout, in seconds.", CVAR_Archive);
static VCvarF centre_msg_time("hud_centre_message_time", "7", "Centered message timeout.", CVAR_Archive);
static VCvarB hud_msg_echo("hud_msg_echo", true, "Echo messages?", CVAR_Archive);
static VCvarI hud_font_color("hud_font_color", "11", "Font color.", CVAR_Archive);
static VCvarI hud_font_color_centered("hud_font_color_centered", "11", "Secondary font color.", CVAR_Archive);

VField *VBasePlayer::fldPendingWeapon = nullptr;


struct SavedVObjectPtr {
  VObject **ptr;
  VObject *saved;
  SavedVObjectPtr (VObject **aptr) : ptr(aptr), saved(*aptr) {}
  ~SavedVObjectPtr() { *ptr = saved; }
};


//==========================================================================
//
//  VBasePlayer::ExecuteNetMethod
//
//==========================================================================
bool VBasePlayer::ExecuteNetMethod (VMethod *Func) {
  guard(VBasePlayer::ExecuteNetMethod);
  if (GDemoRecordingContext) {
    // find initial version of the method
    VMethod *Base = Func;
    while (Base->SuperMethod) Base = Base->SuperMethod;
    // execute it's replication condition method
    check(Base->ReplCond);
    P_PASS_REF(this);
    vuint32 SavedFlags = PlayerFlags;
    PlayerFlags &= ~VBasePlayer::PF_IsClient;
    bool ShouldSend = false;
    if (VObject::ExecuteFunctionNoArgs(Base->ReplCond).getBool()) ShouldSend = true;
    PlayerFlags = SavedFlags;

    if (ShouldSend) {
      // replication condition is true, the method must be replicated
      GDemoRecordingContext->ClientConnections[0]->Channels[CHANIDX_Player]->SendRpc(Func, this);
    }
  }

#ifdef CLIENT
  if (GGameInfo->NetMode == NM_TitleMap ||
      GGameInfo->NetMode == NM_Standalone ||
      (GGameInfo->NetMode == NM_ListenServer && this == cl))
  {
    return false;
  }
#endif

  // find initial version of the method
  VMethod *Base = Func;
  while (Base->SuperMethod) Base = Base->SuperMethod;
  // execute it's replication condition method
  check(Base->ReplCond);
  P_PASS_REF(this);
  if (!VObject::ExecuteFunctionNoArgs(Base->ReplCond).getBool()) return false;

  if (Net) {
    // replication condition is true, the method must be replicated
    Net->Channels[CHANIDX_Player]->SendRpc(Func, this);
  }

  // clean up parameters
  guard(VBasePlayer::ExecuteNetMethod::CleanUp);
  Func->CleanupParams();
  unguard;

  // it's been handled here
  return true;
  unguard;
}


//==========================================================================
//
//  VBasePlayer::SpawnClient
//
//==========================================================================
void VBasePlayer::SpawnClient () {
  guard(VBasePlayer::SpawnClient);
  if (!sv_loading) {
    if (PlayerFlags & PF_Spawned) GCon->Log(NAME_Dev, "Already spawned");
    if (MO) GCon->Log(NAME_Dev, "Mobj already spawned");
    eventSpawnClient();
    for (int i = 0; i < Level->XLevel->ActiveSequences.Num(); ++i) {
      eventClientStartSequence(
        Level->XLevel->ActiveSequences[i].Origin,
        Level->XLevel->ActiveSequences[i].OriginId,
        Level->XLevel->ActiveSequences[i].Name,
        Level->XLevel->ActiveSequences[i].ModeNum);
      for (int j = 0; j < Level->XLevel->ActiveSequences[i].Choices.Num(); ++j) {
        eventClientAddSequenceChoice(
          Level->XLevel->ActiveSequences[i].OriginId,
          Level->XLevel->ActiveSequences[i].Choices[j]);
      }
    }
  } else {
    if (!MO) Host_Error("Player without Mobj\n");
  }

  ViewAngles.roll = 0;
  eventClientSetAngles(ViewAngles);
  PlayerFlags &= ~PF_FixAngle;

  PlayerFlags |= PF_Spawned;

  if ((GGameInfo->NetMode == NM_TitleMap || GGameInfo->NetMode == NM_Standalone) && run_open_scripts) {
    // start open scripts
    Level->XLevel->Acs->StartTypedACScripts(SCRIPT_Open, 0, 0, 0, nullptr, false, false);
  }

  if (!sv_loading) {
    Level->XLevel->Acs->StartTypedACScripts(SCRIPT_Enter, 0, 0, 0, MO, true, false);
  } else if (sv_map_travel) {
    Level->XLevel->Acs->StartTypedACScripts(SCRIPT_Return, 0, 0, 0, MO, true, false);
  }

  if (GGameInfo->NetMode < NM_DedicatedServer || svs.num_connected == sv_load_num_players) {
    sv_loading = false;
    sv_map_travel = false;
  }

  // for single play, save immediately into the reborn slot
  //!if (GGameInfo->NetMode < NM_DedicatedServer) SV_SaveGameToReborn();
  unguard;
}


//==========================================================================
//
//  VBasePlayer::Printf
//
//==========================================================================
__attribute__((format(printf,2,3))) void VBasePlayer::Printf (const char *s, ...) {
  guard(VBasePlayer::Printf);
  va_list v;
  static char buf[4096];

  va_start(v, s);
  vsnprintf(buf, sizeof(buf), s, v);
  va_end(v);

  eventClientPrint(buf);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::CentrePrintf
//
//==========================================================================
__attribute__((format(printf,2,3))) void VBasePlayer::CentrePrintf (const char *s, ...) {
  guard(VBasePlayer::CentrePrintf);
  va_list v;
  static char buf[4096];

  va_start(v, s);
  vsnprintf(buf, sizeof(buf), s, v);
  va_end(v);

  eventClientCentrePrint(buf);
  unguard;
}


//===========================================================================
//
//  VBasePlayer::SetViewState
//
//===========================================================================
void VBasePlayer::SetViewState (int position, VState *stnum) {
  guard(VBasePlayer::SetViewState);
  /*
  if (!fldPendingWeapon) {
    fldPendingWeapon = GetClass()->FindFieldChecked("PendingWeapon");
    if (fldPendingWeapon->Type.Type != TYPE_Reference) Sys_Error("'PendingWeapon' in playerpawn should be a reference");
    //fprintf(stderr, "*** TP=<%s>\n", *fldPendingWeapon->Type.GetName());
  }
  SavedVObjectPtr svp(&_stateRouteSelf);
  _stateRouteSelf = fldPendingWeapon->GetObjectValue(this);
  */
  //fprintf(stderr, "VBasePlayer::SetViewState(%s): route=<%s>\n", GetClass()->GetName(), (_stateRouteSelf ? _stateRouteSelf->GetClass()->GetName() : "<fuck>"));
  //VField *VBasePlayer::fldPendingWeapon = nullptr;
  //fprintf(stderr, "  VBasePlayer::SetViewState: position=%d; stnum=%s\n", position, (stnum ? *stnum->GetFullName() : "<none>"));
  VViewState &VSt = ViewStates[position];
  VState *state = stnum;
  int watchcatCount = 1024;
  do {
    if (--watchcatCount <= 0) {
      //k8: FIXME!
      GCon->Logf("ERROR: WatchCat interrupted `VBasePlayer::SetViewState`!");
      break;
    }
    if (!state) {
      // object removed itself
      //_stateRouteSelf = nullptr; // why not?
      DispSpriteFrame[position] = 0;
      DispSpriteName[position] = NAME_None;
      VSt.State = nullptr;
      VSt.StateTime = -1;
      break;
    }

    // remember current sprite and frame
    UpdateDispFrameFrom(position, state);

    VSt.State = state;
    VSt.StateTime = state->Time; // could be 0
    if (state->Misc1) VSt.SX = state->Misc1;
    if (state->Misc2) VSt.SY = state->Misc2;
    // call action routine
    if (state->Function) {
      //fprintf(stderr, "    VBasePlayer::SetViewState: CALLING '%s'(%s): position=%d; stnum=%s\n", *state->Function->GetFullName(), *state->Function->Loc.toStringNoCol(), position, (stnum ? *stnum->GetFullName() : "<none>"));
      Level->XLevel->CallingState = state;
      if (!MO) Sys_Error("PlayerPawn is dead (wtf?!)");
      {
        SavedVObjectPtr svp(&MO->_stateRouteSelf);
        MO->_stateRouteSelf = _stateRouteSelf; // always, 'cause player is The Authority
        /*
        if (!MO->_stateRouteSelf) {
          MO->_stateRouteSelf = _stateRouteSelf;
        } else {
          //GCon->Logf("player(%s), MO(%s)-viewobject: `%s`, state `%s` (at %s)", *GetClass()->GetFullName(), *MO->GetClass()->GetFullName(), MO->_stateRouteSelf->GetClass()->GetName(), *state->GetFullName(), *state->Loc.toStringNoCol());
        }
        */
        if (!MO->_stateRouteSelf /*&& position == 0*/) GCon->Logf("Player: viewobject is not set!");
        /*
        VObject::VMDumpCallStack();
        GCon->Logf("player(%s), MO(%s)-viewobject: `%s`, state `%s` (at %s)", *GetClass()->GetFullName(), *MO->GetClass()->GetFullName(), MO->_stateRouteSelf->GetClass()->GetName(), *state->GetFullName(), *state->Loc.toStringNoCol());
        */
        P_PASS_REF(MO);
        ExecuteFunctionNoArgs(state->Function);
      }
      if (!VSt.State) {
        DispSpriteFrame[position] = 0;
        DispSpriteName[position] = NAME_None;
        break;
      }
    }
    state = VSt.State->NextState;
  } while (!VSt.StateTime); // an initial state of 0 could cycle through
  //fprintf(stderr, "  VBasePlayer::SetViewState: DONE: position=%d; stnum=%s\n", position, (stnum ? *stnum->GetFullName() : "<none>"));
  unguard;
}


//==========================================================================
//
//  VBasePlayer::AdvanceViewStates
//
//==========================================================================
void VBasePlayer::AdvanceViewStates (float deltaTime) {
  for (int i = 0; i < NUMPSPRITES; ++i) {
    VViewState &St = ViewStates[i];
    // a null state means not active
    if (St.State) {
      // drop tic count and possibly change state
      // a -1 tic count never changes
      if (St.StateTime != -1.0) {
        St.StateTime -= deltaTime;
        if (eventCheckDoubleFiringSpeed()) {
          // [BC] Apply double firing speed
          St.StateTime -= deltaTime;
        }
        if (St.StateTime <= 0.0) {
          St.StateTime = 0.0;
          SetViewState(i, St.State->NextState);
        }
      }
    }
  }
}


//==========================================================================
//
//  VBasePlayer::SetUserInfo
//
//==========================================================================
void VBasePlayer::SetUserInfo (const VStr &info) {
  guard(VBasePlayer::SetUserInfo);
  if (!sv_loading) {
    UserInfo = info;
    ReadFromUserInfo();
  }
  unguard;
}


//==========================================================================
//
//  VBasePlayer::ReadFromUserInfo
//
//==========================================================================
void VBasePlayer::ReadFromUserInfo () {
  guard(VBasePlayer::ReadFromUserInfo);
  if (!sv_loading) BaseClass = atoi(*Info_ValueForKey(UserInfo, "class"));
  PlayerName = Info_ValueForKey(UserInfo, "name");
  Colour = M_ParseColour(Info_ValueForKey(UserInfo, "colour"));
  eventUserinfoChanged();
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientStartSound
//
//==========================================================================
void VBasePlayer::DoClientStartSound (int SoundId, TVec Org, int OriginId,
  int Channel, float Volume, float Attenuation, bool Loop)
{
#ifdef CLIENT
  guard(VBasePlayer::DoClientStartSound);
  GAudio->PlaySound(SoundId, Org, TVec(0, 0, 0), OriginId, Channel, Volume, Attenuation, Loop);
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStopSound
//
//==========================================================================
void VBasePlayer::DoClientStopSound (int OriginId, int Channel) {
#ifdef CLIENT
  guard(VBasePlayer::DoClientStopSound);
  GAudio->StopSound(OriginId, Channel);
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStartSequence
//
//==========================================================================
void VBasePlayer::DoClientStartSequence (TVec Origin, int OriginId, VName Name, int ModeNum) {
#ifdef CLIENT
  guard(VBasePlayer::DoClientStartSequence);
  GAudio->StartSequence(OriginId, Origin, Name, ModeNum);
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientAddSequenceChoice
//
//==========================================================================
void VBasePlayer::DoClientAddSequenceChoice (int OriginId, VName Choice) {
#ifdef CLIENT
  guard(VBasePlayer::DoClientAddSequenceChoice);
  GAudio->AddSeqChoice(OriginId, Choice);
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStopSequence
//
//==========================================================================
void VBasePlayer::DoClientStopSequence (int OriginId) {
#ifdef CLIENT
  guard(VBasePlayer::DoClientStopSequence);
  GAudio->StopSequence(OriginId);
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientPrint
//
//==========================================================================
void VBasePlayer::DoClientPrint (VStr AStr) {
  guard(VBasePlayer::DoClientPrint);
  VStr Str(AStr);

  if (Str.IsEmpty()) return;
  if (Str[0] == '$') Str = GLanguage[*VStr(Str.ToLower(), 1, Str.Length()-1)];
  if (hud_msg_echo) GCon->Logf("\034S%s", *Str);

  ClGame->eventAddNotifyMessage(Str);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientCentrePrint
//
//==========================================================================
void VBasePlayer::DoClientCentrePrint (VStr Str) {
  guard(VBasePlayer::DoClientCentrePrint);
  VStr Msg(Str);

  if (Msg.IsEmpty()) return;
  if (Msg[0] == '$') Msg = GLanguage[*VStr(Msg.ToLower(), 1, Msg.Length()-1)];
  if (hud_msg_echo) {
    //GCon->Log("<-------------------------------->");
    GCon->Logf("\034X%s", *Msg);
    //GCon->Log("<-------------------------------->");
  }

  ClGame->eventAddCentreMessage(Msg);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientSetAngles
//
//==========================================================================
void VBasePlayer::DoClientSetAngles (TAVec Angles) {
  guard(VBasePlayer::DoClientSetAngles);
  ViewAngles = Angles;
  ViewAngles.pitch = AngleMod180(ViewAngles.pitch);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientIntermission
//
//==========================================================================
void VBasePlayer::DoClientIntermission (VName NextMap) {
  guard(VBasePlayer::DoClientIntermission);
  im_t &im = ClGame->im;

  im.Text.Clean();
  im.IMFlags = 0;

  const mapInfo_t &linfo = P_GetMapInfo(Level->XLevel->MapName);
  im.LeaveMap = Level->XLevel->MapName;
  im.LeaveCluster = linfo.Cluster;
  im.LeaveName = linfo.GetName();
  im.LeaveTitlePatch = linfo.TitlePatch;
  im.ExitPic = linfo.ExitPic;
  im.InterMusic = linfo.InterMusic;

  const mapInfo_t &einfo = P_GetMapInfo(NextMap);
  im.EnterMap = NextMap;
  im.EnterCluster = einfo.Cluster;
  im.EnterName = einfo.GetName();
  im.EnterTitlePatch = einfo.TitlePatch;
  im.EnterPic = einfo.EnterPic;

  if (linfo.Cluster != einfo.Cluster) {
    if (einfo.Cluster) {
      const VClusterDef *CDef = P_GetClusterDef(einfo.Cluster);
      if (CDef->EnterText.Length()) {
        if (CDef->Flags & CLUSTERF_LookupEnterText) {
          im.Text = GLanguage[*CDef->EnterText];
        } else {
          im.Text = CDef->EnterText;
        }
        if (CDef->Flags & CLUSTERF_EnterTextIsLump) im.IMFlags |= im_t::IMF_TextIsLump;
        if (CDef->Flags & CLUSTERF_FinalePic) {
          im.TextFlat = NAME_None;
          im.TextPic = CDef->Flat;
        } else {
          im.TextFlat = CDef->Flat;
          im.TextPic = NAME_None;
        }
        im.TextMusic = CDef->Music;
      }
    }
    if (im.Text.Length() == 0 && linfo.Cluster) {
      const VClusterDef *CDef = P_GetClusterDef(linfo.Cluster);
      if (CDef->ExitText.Length()) {
        if (CDef->Flags & CLUSTERF_LookupExitText) {
          im.Text = GLanguage[*CDef->ExitText];
        } else {
          im.Text = CDef->ExitText;
        }
        if (CDef->Flags & CLUSTERF_ExitTextIsLump) im.IMFlags |= im_t::IMF_TextIsLump;
        if (CDef->Flags & CLUSTERF_FinalePic) {
          im.TextFlat = NAME_None;
          im.TextPic = CDef->Flat;
        } else {
          im.TextFlat = CDef->Flat;
          im.TextPic = NAME_None;
        }
        im.TextMusic = CDef->Music;
      }
    }
  }

  ClGame->intermission = 1;
#ifdef CLIENT
  AM_Stop();
  GAudio->StopAllSequences();
#endif

  ClGame->eventIintermissionStart();
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientPause
//
//==========================================================================
void VBasePlayer::DoClientPause (bool Paused) {
#ifdef CLIENT
  guard(VBasePlayer::DoClientPause);
  if (Paused) {
    GGameInfo->Flags |= VGameInfo::GIF_Paused;
    GAudio->PauseSound();
  } else {
    GGameInfo->Flags &= ~VGameInfo::GIF_Paused;
    GAudio->ResumeSound();
  }
  unguard;
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientSkipIntermission
//
//==========================================================================
void VBasePlayer::DoClientSkipIntermission () {
  guard(VBasePlayer::DoClientSkipIntermission);
  ClGame->ClientFlags |= VClientGameBase::CF_SkipIntermission;
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientFinale
//
//==========================================================================
void VBasePlayer::DoClientFinale (VStr Type) {
  guard(VBasePlayer::DoClientFinale);
  ClGame->intermission = 2;
#ifdef CLIENT
  AM_Stop();
#endif
  ClGame->eventStartFinale(*Type);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientChangeMusic
//
//==========================================================================
void VBasePlayer::DoClientChangeMusic (VName Song) {
  guard(VBasePlayer::DoClientChangeMusic);
  Level->SongLump = Song;
#ifdef CLIENT
  GAudio->MusicChanged();
#endif
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientSetServerInfo
//
//==========================================================================
void VBasePlayer::DoClientSetServerInfo (VStr Key, VStr Value) {
  guard(VBasePlayer::DoClientSetServerInfo);
  Info_SetValueForKey(ClGame->serverinfo, Key, Value);
#ifdef CLIENT
  CL_ReadFromServerInfo();
#endif
  unguard;
}


//==========================================================================
//
//  VBasePlayer::DoClientHudMessage
//
//==========================================================================
void VBasePlayer::DoClientHudMessage (const VStr &Message, VName Font, int Type,
  int Id, int Colour, const VStr &ColourName, float x, float y,
  int HudWidth, int HudHeight, float HoldTime, float Time1, float Time2)
{
  guard(VBasePlayer::DoClientHudMessage);
  ClGame->eventAddHudMessage(Message, Font, Type, Id, Colour, ColourName,
    x, y, HudWidth, HudHeight, HoldTime, Time1, Time2);
  unguard;
}


//==========================================================================
//
//  VBasePlayer::WriteViewData
//
//==========================================================================
void VBasePlayer::WriteViewData () {
  guard(VBasePlayer::WriteViewData);
  // update bam_angles (after teleportation)
  if (PlayerFlags&PF_FixAngle) {
    PlayerFlags &= ~PF_FixAngle;
    eventClientSetAngles(ViewAngles);
  }
  unguard;
}


//==========================================================================
//
//  IsGoodAC
//
//==========================================================================
static bool IsGoodAC (VMethod *mt) {
  if (!mt) return false;
  if (mt->NumParams != 3) return false;
  if (mt->ReturnType.Type != TYPE_Void) return false;
  // first arg should be `const ref array!string`
  if (mt->ParamFlags[0] != (FPARM_Const|FPARM_Ref)) return false;
  VFieldType tp = mt->ParamTypes[0];
  if (tp.Type != TYPE_DynamicArray) return false;
  tp = tp.GetArrayInnerType();
  if (tp.Type != TYPE_String) return false;
  // second arg should be int (actually, bool, but it is converted to int)
  if (mt->ParamFlags[1]&~FPARM_Const) return false;
  tp = mt->ParamTypes[1];
  if (tp.Type != TYPE_Int && tp.Type != TYPE_Bool) return false;
  // third arg should be `out array!string`
  if (mt->ParamFlags[2] != FPARM_Out) return false;
  tp = mt->ParamTypes[2];
  if (tp.Type != TYPE_DynamicArray) return false;
  tp = tp.GetArrayInnerType();
  if (tp.Type != TYPE_String) return false;
  return true;
}


//==========================================================================
//
//  VBasePlayer::FindConCommandMethodIdx
//
//==========================================================================
int VBasePlayer::FindConCommandMethodIdx (const VStr &name, bool exact) {
  if (name.isEmpty()) return -1;
  if (!ConCmdListIdx.length()) BuildConCmdCache();
  const int len = ConCmdList.length();
  for (int f = 0; f < len; ++f) {
    const char *mtname = *ConCmdList[f];
    mtname += 6;
    if (!exact && VStr::endsWithNoCase(mtname, "_AC")) continue;
    if (name.ICmp(mtname) == 0) return f;
  }
  return -1;
}


//==========================================================================
//
//  VBasePlayer::BuildConCmdCache
//
//==========================================================================
void VBasePlayer::BuildConCmdCache () {
  if (ConCmdListIdx.length()) return;
  VClass *cls = GetClass();
  while (cls) {
    for (int f = 0; f < cls->Methods.length(); ++f) {
      VMethod *mt = cls->Methods[f];
      if (!mt || mt->Name == NAME_None) continue;
      if (mt->ReturnType.Type != TYPE_Void) continue;
      const char *mtname = *mt->Name;
      if (!VStr::startsWith(mtname, "Cheat_")) continue;
      if (!mtname[6] || mtname[6] == '_') continue;
      // should not be final, etc.
      if (mt->Flags&(/*FUNC_Static|*/FUNC_VarArgs/*|FUNC_NonVirtual*/|FUNC_Spawner|FUNC_Net|FUNC_NetReliable|FUNC_Iterator/*|FUNC_Private*/)) continue;
      if (VStr::endsWithNoCase(mtname, "_AC")) {
        if (!IsGoodAC(mt)) continue;
      } else {
        if (mt->NumParams != 0) continue;
      }
      bool found = false;
      for (int cc = 0; cc < ConCmdList.length(); ++cc) {
        if (VStr::ICmp(*ConCmdList[cc], *mt->Name) == 0) { found = true; break; }
      }
      if (!found) {
        int idx = GetClass()->GetMethodIndex(mt->Name);
        ConCmdList.append(mt->Name);
        ConCmdListIdx.append(idx);
        ConCmdListMts.append(idx >= 0 ? nullptr : mt);
      }
    }
    cls = cls->GetSuperClass();
  }
  if (ConCmdListIdx.length() == 0) ConCmdListIdx.append(-1);
}


//==========================================================================
//
//  VBasePlayer::ListConCommands
//
//  append player commands with the given prefix
//
//==========================================================================
void VBasePlayer::ListConCommands (TArray<VStr> &list, const VStr &pfx) {
  if (!ConCmdListIdx.length()) BuildConCmdCache();
  const int len = ConCmdList.length();
  for (int f = 0; f < len; ++f) {
    const char *mtname = *ConCmdList[f];
    mtname += 6;
    if (VStr::endsWithNoCase(mtname, "_AC")) continue;
    if (!pfx.isEmpty()) {
      if (!VStr::startsWithNoCase(mtname, *pfx)) continue;
    }
    list.append(VStr(mtname));
  }
}


//==========================================================================
//
//  VBasePlayer::IsConCommand
//
//==========================================================================
bool VBasePlayer::IsConCommand (const VStr &name) {
  return (FindConCommandMethodIdx(name) >= 0);
}


//==========================================================================
//
//  VBasePlayer::ExecConCommand
//
//  returns `true` if command was found and executed
//  uses VCommand command line
//
//==========================================================================
bool VBasePlayer::ExecConCommand () {
  if (VCommand::GetArgC() < 1) return false;
  VStr name = VCommand::GetArgV(0);
  int listidx = FindConCommandMethodIdx(name);
  if (listidx < 0) return false;
  // i found her!
  VMethod *mt;
  if (ConCmdListIdx[listidx] >= 0) {
    mt = GetVFunctionIdx(ConCmdListIdx[listidx]);
  } else {
    mt = ConCmdListMts[listidx];
    check(mt);
  }
  if ((mt->Flags&FUNC_Static) == 0) P_PASS_SELF;
  (void)ExecuteFunction(mt);
  return true;
}


//==========================================================================
//
//  VBasePlayer::ExecConCommandAC
//
//  returns `true` if command was found (autocompleter may be still missing)
//  autocompleter should filter list
//
//==========================================================================
bool VBasePlayer::ExecConCommandAC (TArray<VStr> &args, bool newArg, TArray<VStr> &aclist) {
  if (args.length() < 1) return false;
  VStr name = args[0];
  if (name.isEmpty()) return false;
  int listidx = FindConCommandMethodIdxExact(name+"_AC");
  if (listidx >= 0) {
    // i found her!
    // build command line
    //args.removeAt(0); // remove command name
    VMethod *mt;
    if (ConCmdListIdx[listidx] >= 0) {
      mt = GetVFunctionIdx(ConCmdListIdx[listidx]);
    } else {
      mt = ConCmdListMts[listidx];
      check(mt);
    }
    if ((mt->Flags&FUNC_Static) == 0) P_PASS_SELF;
    P_PASS_PTR((void *)&args);
    P_PASS_INT(newArg ? 1 : 0);
    P_PASS_PTR((void *)&aclist);
    (void)ExecuteFunction(mt);
    return true;
  }
  return (FindConCommandMethodIdx(name) >= 0);  // has such cheat?
}


//==========================================================================
//
//  COMMAND SetInfo
//
//==========================================================================
COMMAND(SetInfo) {
  guard(COMMAND SetInfo);
  if (Source != SRC_Client) {
    GCon->Log("SetInfo is not valid from console");
    return;
  }

  if (Args.Num() != 3) return;

  Info_SetValueForKey(Player->UserInfo, *Args[1], *Args[2]);
  Player->ReadFromUserInfo();
  unguard;
}


//==========================================================================
//
//  Natives
//
//==========================================================================
IMPLEMENT_FUNCTION(VBasePlayer, cprint) {
  VStr msg = PF_FormatString();
  P_GET_SELF;
  Self->eventClientPrint(*msg);
}

IMPLEMENT_FUNCTION(VBasePlayer, centreprint) {
  VStr msg = PF_FormatString();
  P_GET_SELF;
  Self->eventClientCentrePrint(*msg);
}

IMPLEMENT_FUNCTION(VBasePlayer, GetPlayerNum) {
  P_GET_SELF;
  RET_INT(SV_GetPlayerNum(Self));
}

IMPLEMENT_FUNCTION(VBasePlayer, ClearPlayer) {
  P_GET_SELF;

  Self->PClass = 0;
  Self->ForwardMove = 0;
  Self->SideMove = 0;
  Self->FlyMove = 0;
  Self->Buttons = 0;
  Self->Impulse = 0;
  Self->AcsButtons = 0;
  Self->OldButtons = 0;
  Self->MO = nullptr;
  Self->PlayerState = 0;
  Self->ViewOrg = TVec(0, 0, 0);
  Self->PlayerFlags &= ~VBasePlayer::PF_FixAngle;
  Self->Health = 0;
  Self->PlayerFlags &= ~VBasePlayer::PF_AttackDown;
  Self->PlayerFlags &= ~VBasePlayer::PF_UseDown;
  Self->PlayerFlags &= ~VBasePlayer::PF_AutomapRevealed;
  Self->PlayerFlags &= ~VBasePlayer::PF_AutomapShowThings;
  Self->ExtraLight = 0;
  Self->FixedColourmap = 0;
  Self->CShift = 0;
  Self->PSpriteSY = 0;

  vuint8 *Def = Self->GetClass()->Defaults;
  for (VField *F = Self->GetClass()->Fields; F; F = F->Next)
  {
    VField::CopyFieldValue(Def + F->Ofs, (vuint8*)Self + F->Ofs, F->Type);
  }
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewObject) {
  P_GET_PTR(VObject, vobj);
  P_GET_SELF;
  //if (!vobj) GCon->Logf("RESET VIEW OBJECT; WTF?!");
  if (Self) Self->_stateRouteSelf = vobj;
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewObjectIfNone) {
  P_GET_PTR(VObject, vobj);
  P_GET_SELF;
  if (Self && !Self->_stateRouteSelf) Self->_stateRouteSelf = vobj;
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewState) {
  //fprintf(stderr, "*** SVS ***\n");
  P_GET_PTR(VState, stnum);
  P_GET_INT(position);
  P_GET_SELF;
  Self->SetViewState(position, stnum);
}

IMPLEMENT_FUNCTION(VBasePlayer, AdvanceViewStates) {
  P_GET_FLOAT(deltaTime);
  P_GET_SELF;
  Self->AdvanceViewStates(deltaTime);
}

IMPLEMENT_FUNCTION(VBasePlayer, DisconnectBot) {
  P_GET_SELF;
  check(Self->PlayerFlags & PF_IsBot);
  SV_DropClient(Self, false);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStartSound) {
  P_GET_BOOL(Loop);
  P_GET_FLOAT(Attenuation);
  P_GET_FLOAT(Volume);
  P_GET_INT(Channel);
  P_GET_INT(OriginId);
  P_GET_VEC(Org);
  P_GET_INT(SoundId);
  P_GET_SELF;
  Self->DoClientStartSound(SoundId, Org, OriginId, Channel, Volume,
    Attenuation, Loop);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStopSound) {
  P_GET_INT(Channel);
  P_GET_INT(OriginId);
  P_GET_SELF;
  Self->DoClientStopSound(OriginId, Channel);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStartSequence) {
  P_GET_INT(ModeNum);
  P_GET_NAME(Name);
  P_GET_INT(OriginId);
  P_GET_VEC(Origin);
  P_GET_SELF;
  Self->DoClientStartSequence(Origin, OriginId, Name, ModeNum);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientAddSequenceChoice) {
  P_GET_NAME(Choice);
  P_GET_INT(OriginId);
  P_GET_SELF;
  Self->DoClientAddSequenceChoice(OriginId, Choice);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStopSequence) {
  P_GET_INT(OriginId);
  P_GET_SELF;
  Self->DoClientStopSequence(OriginId);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientPrint) {
  P_GET_STR(Str);
  P_GET_SELF;
  Self->DoClientPrint(Str);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientCentrePrint) {
  P_GET_STR(Str);
  P_GET_SELF;
  Self->DoClientCentrePrint(Str);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSetAngles) {
  P_GET_AVEC(Angles);
  P_GET_SELF;
  Self->DoClientSetAngles(Angles);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientIntermission) {
  P_GET_NAME(NextMap);
  P_GET_SELF;
  Self->DoClientIntermission(NextMap);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientPause) {
  P_GET_BOOL(Paused);
  P_GET_SELF;
  Self->DoClientPause(Paused);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSkipIntermission) {
  P_GET_SELF;
  Self->DoClientSkipIntermission();
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientFinale) {
  P_GET_STR(Type);
  P_GET_SELF;
  Self->DoClientFinale(Type);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientChangeMusic) {
  P_GET_NAME(Song);
  P_GET_SELF;
  Self->DoClientChangeMusic(Song);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSetServerInfo) {
  P_GET_STR(Value);
  P_GET_STR(Key);
  P_GET_SELF;
  Self->DoClientSetServerInfo(Key, Value);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientHudMessage) {
  P_GET_FLOAT(Time2);
  P_GET_FLOAT(Time1);
  P_GET_FLOAT(HoldTime);
  P_GET_INT(HudHeight);
  P_GET_INT(HudWidth);
  P_GET_FLOAT(y);
  P_GET_FLOAT(x);
  P_GET_STR(ColourName);
  P_GET_INT(Colour);
  P_GET_INT(Id);
  P_GET_INT(Type);
  P_GET_NAME(Font);
  P_GET_STR(Message);
  P_GET_SELF;
  Self->DoClientHudMessage(Message, Font, Type, Id, Colour, ColourName,
    x, y, HudWidth, HudHeight, HoldTime, Time1, Time2);
}

IMPLEMENT_FUNCTION(VBasePlayer, ServerSetUserInfo) {
  P_GET_STR(Info);
  P_GET_SELF;
  Self->SetUserInfo(Info);
}
