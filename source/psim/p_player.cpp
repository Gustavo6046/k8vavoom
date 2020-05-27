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
//**  Copyright (C) 2018-2020 Ketmar Dark
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
#include "../gamedefs.h"
#include "../net/network.h" /* for demos and RPC */
#include "../server/sv_local.h"
#include "../client/cl_local.h"


IMPLEMENT_CLASS(V, BasePlayer)

bool VBasePlayer::isCheckpointSpawn = false;

static VCvarF hud_notify_time("hud_notify_time", "3", "Notification timeout, in seconds.", CVAR_Archive);
static VCvarF center_msg_time("hud_center_message_time", "3", "Centered message timeout.", CVAR_Archive);
static VCvarB hud_msg_echo("hud_msg_echo", true, "Echo messages?", CVAR_Archive);
static VCvarI hud_font_color("hud_font_color", "11", "Font color.", CVAR_Archive);
static VCvarI hud_font_color_centered("hud_font_color_centered", "11", "Secondary font color.", CVAR_Archive);

static VCvarF hud_chat_time("hud_chat_time", "8", "Chat messages timeout, in seconds.", CVAR_Archive);
static VCvarI hud_chat_nick_color("hud_chat_nick_color", "8", "Chat nick color.", CVAR_Archive);
static VCvarI hud_chat_text_color("hud_chat_text_color", "13", "Chat font color.", CVAR_Archive);

#ifdef CLIENT
extern VCvarF cl_fov;
#endif

//VField *VBasePlayer::fldPendingWeapon = nullptr;
//VField *VBasePlayer::fldReadyWeapon = nullptr;

#define WEAPONBOTTOM  (128.0f)
#define WEAPONTOP     (32.0f)


struct SavedVObjectPtr {
  VObject **ptr;
  VObject *saved;
  SavedVObjectPtr (VObject **aptr) : ptr(aptr), saved(*aptr) {}
  ~SavedVObjectPtr() { *ptr = saved; }
};


//==========================================================================
//
//  VBasePlayer::PostCtor
//
//==========================================================================
void VBasePlayer::PostCtor () {
  Super::PostCtor();
  /*
  GCon->Logf("********** BASEPLAYER POSTCTOR (%s) **********", *GetClass()->GetFullName());
  VField *field = GetClass()->FindField("k8ElvenGiftMessageTime");
  if (field) GCon->Logf("  k8ElvenGiftMessageTime=%g", field->GetFloat(this));
  abort();
  */
}


//==========================================================================
//
//  VBasePlayer::ExecuteNetMethod
//
//==========================================================================
bool VBasePlayer::ExecuteNetMethod (VMethod *Func) {
  //if (onExecuteNetMethodCB) return onExecuteNetMethodCB(this, func);

  if (GDemoRecordingContext) {
    // find initial version of the method
    VMethod *Base = Func;
    while (Base->SuperMethod) Base = Base->SuperMethod;
    // execute it's replication condition method
    vassert(Base->ReplCond);
    vuint32 SavedFlags = PlayerFlags;
    PlayerFlags &= ~VBasePlayer::PF_IsClient;
    bool ShouldSend = false;
    if (VObject::ExecuteFunctionNoArgs(this, Base->ReplCond, true).getBool()) ShouldSend = true; // no VMT lookups
    PlayerFlags = SavedFlags;

    if (ShouldSend) {
      // replication condition is true, the method must be replicated
      GDemoRecordingContext->ClientConnections[0]->GetPlayerChannel()->SendRpc(Func, this);
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
  vassert(Base->ReplCond);
  if (!VObject::ExecuteFunctionNoArgs(this, Base->ReplCond, true).getBool()) return false; // no VMT lookups

  if (Net) {
    // replication condition is true, the method must be replicated
    Net->GetPlayerChannel()->SendRpc(Func, this);
  }

  // clean up parameters
  Func->CleanupParams();

  // it's been handled here
  return true;
}


//==========================================================================
//
//  VBasePlayer::SpawnClient
//
//==========================================================================
void VBasePlayer::SpawnClient () {
  if (!sv_loading) {
    if (PlayerFlags&PF_Spawned) GCon->Log(NAME_Dev, "Already spawned");
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

  //GCon->Logf(NAME_Debug, "MO Origin: (%g,%g,%g)", MO->Origin.x, MO->Origin.y, MO->Origin.z);

  // for single play, save immediately into the reborn slot
  //!if (GGameInfo->NetMode < NM_DedicatedServer) SV_SaveGameToReborn();
}


//==========================================================================
//
//  VBasePlayer::Printf
//
//==========================================================================
__attribute__((format(printf,2,3))) void VBasePlayer::Printf (const char *s, ...) {
  va_list v;
  static char buf[4096];

  va_start(v, s);
  vsnprintf(buf, sizeof(buf), s, v);
  va_end(v);

  eventClientPrint(buf);
}


//==========================================================================
//
//  VBasePlayer::CenterPrintf
//
//==========================================================================
__attribute__((format(printf,2,3))) void VBasePlayer::CenterPrintf (const char *s, ...) {
  va_list v;
  static char buf[4096];

  va_start(v, s);
  vsnprintf(buf, sizeof(buf), s, v);
  va_end(v);

  eventClientCenterPrint(buf);
}


//===========================================================================
//
//  VBasePlayer::SetViewState
//
//===========================================================================
void VBasePlayer::SetViewState (int position, VState *stnum) {
  if (position < 0 || position >= NUMPSPRITES) return; // sanity check
  if (position == PS_WEAPON && !stnum) ViewStates[PS_WEAPON_OVL].State = nullptr;
  //if (position == PS_WEAPON_OVL /*&& stnum*/) GCon->Logf("ticking OVERLAY (%s)", (stnum ? *stnum->Loc.toStringNoCol() : "ona"));
  //if (position == PS_WEAPON /*&& stnum*/) GCon->Logf("*** START ticking WEAPON (%s)", (stnum ? *stnum->Loc.toStringNoCol() : "ona"));
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
#if 0
  if (position == PS_WEAPON && stnum) {
    static VClass *WeaponClass = nullptr;
    if (!WeaponClass) WeaponClass = VClass::FindClass("Weapon");
    if (!fldReadyWeapon) {
      fldReadyWeapon = GetClass()->FindFieldChecked("ReadyWeapon");
      if (fldReadyWeapon->Type.Type != TYPE_Reference) Sys_Error("'ReadyWeapon' in playerpawn should be a reference");
    }
    if (WeaponClass) {
      VObject *wobj = fldReadyWeapon->GetObjectValue(this);
      if (wobj != lastReadyWeapon) {
        lastReadyWeapon = wobj;
        if (wobj && wobj->IsA(WeaponClass)) {
          VEntity *wpn = (VEntity *)wobj;
          lastReadyWeaponReadyState = wpn->FindState("Ready");
          //GCon->Logf("WEAPON CHANGED TO '%s'", *wpn->GetClass()->Name);
          SetViewState(PS_WEAPON_OVL, wpn->FindState("Display"));
          /*
          ViewStates[PS_WEAPON_OVL].State = wpn->FindState("Display");
          if (ViewStates[PS_WEAPON_OVL].State) {
            GCon->Logf("WEAPON CHANGED TO '%s', 'Display' substate found", *wpn->GetClass()->Name);
          }
          */
        } else {
          lastReadyWeaponReadyState = nullptr;
          ViewStates[PS_WEAPON_OVL].State = nullptr;
        }
      }
      /*
      if (lastReadyWeaponReadyState == stnum) {
        GCon->Logf("SetViewState: %s", *stnum->Name);
      }
      */
    }
  }
#else
  if (_stateRouteSelf != LastViewObject[position]) {
    LastViewObject[position] = _stateRouteSelf;
    ViewStates[position].SX = ViewStates[position].OfsY = ViewStates[position].BobOfsX = ViewStates[position].BobOfsY = 0;
    // "display" state
    if (position == PS_WEAPON) {
      LastViewObject[PS_WEAPON_OVL] = _stateRouteSelf;
      static VClass *WeaponClass = nullptr;
      if (!WeaponClass) WeaponClass = VClass::FindClass("Weapon");
      if (_stateRouteSelf && _stateRouteSelf->IsA(WeaponClass)) {
        //GCon->Logf(NAME_Warning, "*** NEW DISPLAY STATE: %s", *_stateRouteSelf->GetClass()->GetFullName());
        VEntity *wpn = (VEntity *)_stateRouteSelf;
        SetViewState(PS_WEAPON_OVL, wpn->FindState("Display"));
      } else {
        //GCon->Logf(NAME_Warning, "*** NEW DISPLAY STATE: EMPTY (%s)", (_stateRouteSelf ? *_stateRouteSelf->GetClass()->GetFullName() : "<none>"));
        ViewStates[PS_WEAPON_OVL].State = nullptr;
        LastViewObject[PS_WEAPON_OVL] = nullptr;
      }
    }
  }
#endif

  VViewState &VSt = ViewStates[position];
  VState *state = stnum;
  int watchcatCount = 1024;
  do {
    if (!state) {
      if (position == PS_WEAPON && developer) GCon->Logf(NAME_Dev, "*** WEAPON removed itself!");
      // object removed itself
      //_stateRouteSelf = nullptr; // why not?
      DispSpriteFrame[position] = 0;
      DispSpriteName[position] = NAME_None;
      VSt.State = nullptr;
      VSt.StateTime = -1;
      break;
    }

    if (--watchcatCount <= 0) {
      //k8: FIXME!
      GCon->Logf(NAME_Error, "WatchCat interrupted `VBasePlayer::SetViewState()` in '%s'!", *state->Loc.toStringNoCol());
      VSt.StateTime = -1;
      break;
    }

    //if (position == PS_WEAPON) GCon->Logf("*** ... ticking WEAPON (%s)", *state->Loc.toStringNoCol());

    // remember current sprite and frame
    UpdateDispFrameFrom(position, state);

    VSt.State = state;
    VSt.StateTime = state->Time; // could be 0
    if (state->Misc1) VSt.SX = state->Misc1;
    if (state->Misc2) VSt.OfsY = state->Misc2-32;

    //GCon->Logf("sprite #%d: '%s' %c (ofs: %g, %g)", position, *DispSpriteName[position], ('A'+(DispSpriteFrame[position]&0xff)), VSt.SX, VSt.OfsY);

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
        if (!MO->_stateRouteSelf /*&& position == 0*/) {
          GCon->Logf(NAME_Warning, "Player: viewobject is not set! state is %s", *state->Loc.toStringNoCol());
        }
        /*
        if (position == PS_WEAPON) {
          VObject::VMDumpCallStack();
          //GCon->Logf("player(%s), MO(%s)-viewobject: `%s`, state `%s` (at %s)", *GetClass()->GetFullName(), *MO->GetClass()->GetFullName(), MO->_stateRouteSelf->GetClass()->GetName(), *state->GetFullName(), *state->Loc.toStringNoCol());
        }
        */
        ExecuteFunctionNoArgs(MO, state->Function); // allow VMT lookups
      }
      if (!VSt.State) {
        //GCon->Logf("Player: viewobject DEAD(0)! state is %s", *state->Loc.toStringNoCol());
        DispSpriteFrame[position] = 0;
        DispSpriteName[position] = NAME_None;
        break;
      }
    }
    state = VSt.State->NextState;
  } while (!VSt.StateTime); // an initial state of 0 could cycle through
  //fprintf(stderr, "  VBasePlayer::SetViewState: DONE: position=%d; stnum=%s\n", position, (stnum ? *stnum->GetFullName() : "<none>"));
  if (!VSt.State) {
    //GCon->Logf("Player: viewobject DEAD(1)!");
    LastViewObject[position] = nullptr;
    if (position == PS_WEAPON) LastViewObject[PS_WEAPON_OVL] = nullptr;
  }
}


//==========================================================================
//
//  VBasePlayer::AdvanceViewStates
//
//==========================================================================
void VBasePlayer::AdvanceViewStates (float deltaTime) {
  if (deltaTime <= 0.0f) return;
  int dfchecked = -1;
  for (unsigned i = 0; i < NUMPSPRITES; ++i) {
    VViewState &St = ViewStates[i];
    // a null state means not active
    // a -1 tic count never changes
    if (!St.State) continue;
    if (St.StateTime < 0.0f) { St.StateTime = -1.0f; continue; } // force `-1` here just in case
    if (dfchecked < 0) dfchecked = (eventCheckDoubleFiringSpeed() ? 1 : 0); // call VM only once
    const float dtime = deltaTime*(dfchecked ? 2.0f : 1.0f); // [BC] Apply double firing speed
    // drop tic count and possibly change state
    //GCon->Logf(NAME_Debug, "*** %u:%s:%s: i=%d: StateTime=%g (%g) (nst=%g); delta=%g (%g)", GetUniqueId(), GetClass()->GetName(), *St.State->Loc.toStringShort(), i, St.StateTime, St.StateTime*35.0f, St.StateTime-dtime, dtime, dtime*35.0f);
    if (St.StateTime > 0.0f) St.StateTime -= dtime;
    while (St.StateTime <= 0.0f) {
      // this somewhat compensates freestep instability
      const float tleft = St.StateTime; // "overjump time"
      St.StateTime = 0.0f;
      SetViewState(i, St.State->NextState);
      //if (i == PS_WEAPON) GCon->Logf("AdvanceViewStates: after weapon=%s; route=%s", (LastViewObject[i] ? *LastViewObject[i]->GetClass()->GetFullName() : "<none>"), (_stateRouteSelf ? *_stateRouteSelf->GetClass()->GetFullName() : "<none>"));
      if (!St.State) break;
      vassert(St.StateTime != 0.0f);
      if (St.StateTime < 0.0f) { St.StateTime = -1.0f; break; } // force `-1` here just in case
      //GCon->Logf(NAME_Debug, "*** %u:%s:%s: i=%d:     tleft=%g; StateTime=%g (%g); rest=%g", GetUniqueId(), GetClass()->GetName(), *St.State->Loc.toStringShort(), i, tleft, St.StateTime, St.StateTime*35.0f, St.StateTime+tleft);
      St.StateTime += tleft; // freestep compensation
    }
    //if (St.State) GCon->Logf(NAME_Debug, "    %u:%s:%s: i=%d:     END; StateTime=%g (%g); delta=%g (%g)", GetUniqueId(), GetClass()->GetName(), *St.State->Loc.toStringShort(), i, St.StateTime, St.StateTime*35.0f, deltaTime, deltaTime*35.0f);
  }
}


//==========================================================================
//
//  VBasePlayer::SetUserInfo
//
//==========================================================================
void VBasePlayer::SetUserInfo (VStr info) {
  if (!sv_loading) {
    UserInfo = info.cloneUnique();
    ReadFromUserInfo();
  }
}


//==========================================================================
//
//  VBasePlayer::ReadFromUserInfo
//
//==========================================================================
void VBasePlayer::ReadFromUserInfo () {
  if (!sv_loading) BaseClass = VStr::atoi(*Info_ValueForKey(UserInfo, "class"));
  PlayerName = Info_ValueForKey(UserInfo, "name");
  VStr val = Info_ValueForKey(UserInfo, "color");
  Color = M_ParseColor(*val);
  eventUserinfoChanged();
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
  GAudio->PlaySound(SoundId, Org, TVec(0, 0, 0), OriginId, Channel, Volume, Attenuation, Loop);
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStopSound
//
//==========================================================================
void VBasePlayer::DoClientStopSound (int OriginId, int Channel) {
#ifdef CLIENT
  GAudio->StopSound(OriginId, Channel);
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStartSequence
//
//==========================================================================
void VBasePlayer::DoClientStartSequence (TVec Origin, int OriginId, VName Name, int ModeNum) {
#ifdef CLIENT
  GAudio->StartSequence(OriginId, Origin, Name, ModeNum);
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientAddSequenceChoice
//
//==========================================================================
void VBasePlayer::DoClientAddSequenceChoice (int OriginId, VName Choice) {
#ifdef CLIENT
  GAudio->AddSeqChoice(OriginId, Choice);
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientStopSequence
//
//==========================================================================
void VBasePlayer::DoClientStopSequence (int OriginId) {
#ifdef CLIENT
  GAudio->StopSequence(OriginId);
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientPrint
//
//==========================================================================
void VBasePlayer::DoClientPrint (VStr AStr) {
  VStr Str(AStr);

  if (Str.IsEmpty()) return;
  if (Str[0] == '$') Str = GLanguage[*VStr(Str.ToLower(), 1, Str.Length()-1)];
  if (hud_msg_echo) GCon->Logf("\034S%s", *Str);

  ClGame->eventAddNotifyMessage(Str);
}


//==========================================================================
//
//  VBasePlayer::DoClientChatPrint
//
//==========================================================================
void VBasePlayer::DoClientChatPrint (VStr nick, VStr text) {
  if (text.IsEmpty()) return;
  GCon->Logf(NAME_Chat, "[%s]: %s", *nick.RemoveColors().xstrip(), *text.RemoveColors().xstrip());
  ClGame->eventAddChatMessage(nick, text);
}


//==========================================================================
//
//  VBasePlayer::DoClientCenterPrint
//
//==========================================================================
void VBasePlayer::DoClientCenterPrint (VStr Str) {
  VStr Msg(Str);

  if (Msg.IsEmpty()) return;
  if (Msg[0] == '$') Msg = GLanguage[*VStr(Msg.ToLower(), 1, Msg.Length()-1)];
  if (hud_msg_echo) {
    //GCon->Log("<-------------------------------->");
    GCon->Logf("\034X%s", *Msg);
    //GCon->Log("<-------------------------------->");
  }

  ClGame->eventAddCenterMessage(Msg);
}


//==========================================================================
//
//  VBasePlayer::DoClientSetAngles
//
//  called via RPC
//
//==========================================================================
void VBasePlayer::DoClientSetAngles (TAVec Angles) {
  ViewAngles = Angles;
}


enum { ClusterText_Exit, ClusterText_Enter };


//==========================================================================
//
//  SetupClusterText
//
//==========================================================================
static void SetupClusterText (int cttype, IntermissionText &im, const VClusterDef *CDef) {
  im.clear();
  if (!CDef) return;

  VStr text = (cttype == ClusterText_Exit ? CDef->ExitText : CDef->EnterText);
  if (text.isEmpty()) return;

  const bool dotrans = !!(CDef->Flags&(cttype == ClusterText_Exit ? CLUSTERF_LookupExitText : CLUSTERF_LookupEnterText));
  const bool islump = !!(CDef->Flags&(cttype == ClusterText_Exit ? CLUSTERF_ExitTextIsLump : CLUSTERF_EnterTextIsLump));

  if (dotrans) text = GLanguage[*text];
  im.Text = text;
  //GCon->Logf(NAME_Debug, "cttype=%d; text=\"%s\"", cttype, *text.quote());
  if (islump) im.Flags |= IntermissionText::IMF_TextIsLump;

  if (CDef->Flags&CLUSTERF_FinalePic) {
    im.TextFlat = NAME_None;
    im.TextPic = CDef->Flat;
  } else {
    im.TextFlat = CDef->Flat;
    im.TextPic = NAME_None;
  }

  im.TextMusic = CDef->Music;
}


//==========================================================================
//
//  VBasePlayer::DoClientIntermission
//
//==========================================================================
void VBasePlayer::DoClientIntermission (VName NextMap) {
  IntermissionInfo &im = ClGame->im;

  im.LeaveText.clear();
  im.EnterText.clear();

  const VMapInfo &linfo = P_GetMapInfo(Level->XLevel->MapName);
  im.LeaveMap = Level->XLevel->MapName;
  im.LeaveCluster = linfo.Cluster;
  im.LeaveName = linfo.GetName();
  im.LeaveTitlePatch = linfo.TitlePatch;
  im.ExitPic = linfo.ExitPic;
  im.InterMusic = linfo.InterMusic;

  const VMapInfo &einfo = P_GetMapInfo(NextMap);
  im.EnterMap = NextMap;
  im.EnterCluster = einfo.Cluster;
  im.EnterName = einfo.GetName();
  im.EnterTitlePatch = einfo.TitlePatch;
  im.EnterPic = einfo.EnterPic;

  /*
  GCon->Logf("******************** DoClientIntermission ********************");
  linfo.dump("leaving");
  einfo.dump("entering");
  */

#ifdef CLIENT
  AM_Stop();
  GAudio->StopAllSequences();
#endif

  if (linfo.Cluster != einfo.Cluster || G_CheckFinale()) {
    // cluster leaving text
    if (linfo.Cluster) SetupClusterText(ClusterText_Exit, im.LeaveText, P_GetClusterDef(linfo.Cluster));
    // cluster entering text
    if (einfo.Cluster) SetupClusterText(ClusterText_Enter, im.EnterText, P_GetClusterDef(einfo.Cluster));
  }

       if (im.LeaveText.isActive()) ClGame->StartIntermissionLeave();
  else if (im.EnterText.isActive()) ClGame->StartIntermissionEnter();
  else ClGame->StartIntermissionLeave(); // anyway
}


//==========================================================================
//
//  VBasePlayer::DoClientPause
//
//==========================================================================
void VBasePlayer::DoClientPause (bool Paused) {
#ifdef CLIENT
  if (Paused) {
    GGameInfo->Flags |= VGameInfo::GIF_Paused;
    GAudio->PauseSound();
  } else {
    GGameInfo->Flags &= ~VGameInfo::GIF_Paused;
    GAudio->ResumeSound();
  }
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientFOV
//
//==========================================================================
void VBasePlayer::DoClientFOV (float fov) {
  #ifdef CLIENT
  cl_fov = fov;
  #endif
}


//==========================================================================
//
//  VBasePlayer::DoClientSkipIntermission
//
//==========================================================================
void VBasePlayer::DoClientSkipIntermission () {
  ClGame->ClientFlags |= VClientGameBase::CF_SkipIntermission;
}


//==========================================================================
//
//  VBasePlayer::DoClientFinale
//
//==========================================================================
void VBasePlayer::DoClientFinale (VStr Type) {
#ifdef CLIENT
  AM_Stop();
#endif
  ClGame->StartFinale(*Type);
}


//==========================================================================
//
//  VBasePlayer::DoClientChangeMusic
//
//==========================================================================
void VBasePlayer::DoClientChangeMusic (VName Song) {
  Level->SongLump = Song;
#ifdef CLIENT
  GAudio->MusicChanged();
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientSetServerInfo
//
//==========================================================================
void VBasePlayer::DoClientSetServerInfo (VStr Key, VStr Value) {
  Info_SetValueForKey(ClGame->serverinfo, Key, Value);
#ifdef CLIENT
  CL_ReadFromServerInfo();
#endif
}


//==========================================================================
//
//  VBasePlayer::DoClientHudMessage
//
//==========================================================================
void VBasePlayer::DoClientHudMessage (VStr Message, VName Font, int Type,
  int Id, int Color, VStr ColorName, float x, float y,
  int HudWidth, int HudHeight, float HoldTime, float Time1, float Time2)
{
  ClGame->eventAddHudMessage(Message, Font, Type, Id, Color, ColorName,
    x, y, HudWidth, HudHeight, HoldTime, Time1, Time2);
}


//==========================================================================
//
//  VBasePlayer::WriteViewData
//
//  this is called from `SV_SendClientMessages()`
//
//==========================================================================
void VBasePlayer::WriteViewData () {
  // update bam_angles (after teleportation)
  if (PlayerFlags&PF_FixAngle) {
    PlayerFlags &= ~PF_FixAngle;
    //k8: this should enforce view angles on client (it is done via RPC)
    //GCon->Logf(NAME_Debug, "FIXANGLES va=(%g,%g,%g)", ViewAngles.pitch, ViewAngles.yaw, ViewAngles.roll);
    eventClientSetAngles(ViewAngles);
  }
}


//==========================================================================
//
//  VBasePlayer::CallDumpInventory
//
//==========================================================================
void VBasePlayer::CallDumpInventory () {
  VStr name = "DumpInventory";
  VMethod *mt = GetClass()->FindConCommandMethod(name);
  if (!mt) return;
  // i found her!
  if ((mt->Flags&FUNC_Static) == 0) P_PASS_SELF;
  (void)ExecuteFunction(mt);
}


//==========================================================================
//
//  VBasePlayer::ListConCommands
//
//  append player commands with the given prefix
//
//==========================================================================
void VBasePlayer::ListConCommands (TArray<VStr> &list, VStr pfx) {
  for (auto it = GetClass()->ConCmdListMts.first(); it; ++it) {
    const char *mtname = *it.getValue()->Name;
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
bool VBasePlayer::IsConCommand (VStr name) {
  return !!GetClass()->FindConCommandMethod(name);
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
  VMethod *mt = GetClass()->FindConCommandMethod(name);
  if (!mt) return false;
  // i found her!
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
  VMethod *mt = GetClass()->FindConCommandMethodExact(name+"_AC");
  if (mt) {
    // i found her!
    // build command line
    //args.removeAt(0); // remove command name
    if ((mt->Flags&FUNC_Static) == 0) P_PASS_SELF;
    P_PASS_PTR((void *)&args);
    P_PASS_INT(newArg ? 1 : 0);
    P_PASS_PTR((void *)&aclist);
    (void)ExecuteFunction(mt);
    return true;
  }
  return !!GetClass()->FindConCommandMethod(name);  // has such cheat?
}


//==========================================================================
//
//  VBasePlayer::ResetButtons
//
//==========================================================================
void VBasePlayer::ResetButtons () {
  ForwardMove = 0;
  SideMove = 0;
  FlyMove = 0;
  Buttons = 0;
  Impulse = 0;
  AcsCurrButtonsPressed = 0;
  AcsCurrButtons = 0;
  AcsButtons = 0;
  OldButtons = 0;
  AcsNextButtonUpdate = 0;
  AcsPrevMouseX = 0;
  AcsPrevMouseY = 0;
  AcsMouseX = 0;
  AcsMouseY = 0;
  // reset "down" flags
  PlayerFlags &= ~VBasePlayer::PF_AttackDown;
  PlayerFlags &= ~VBasePlayer::PF_UseDown;
}


//==========================================================================
//
//  COMMAND SetInfo
//
//==========================================================================
COMMAND(SetInfo) {
  if (Source != SRC_Client) {
    GCon->Log("SetInfo is not valid from console");
    return;
  }

  if (Args.Num() != 3) return;

  Info_SetValueForKey(Player->UserInfo, *Args[1], *Args[2]);
  Player->ReadFromUserInfo();
}


//==========================================================================
//
//  Natives
//
//==========================================================================
IMPLEMENT_FUNCTION(VBasePlayer, get_IsCheckpointSpawn) {
  RET_BOOL(isCheckpointSpawn);
}

IMPLEMENT_FUNCTION(VBasePlayer, IsRunEnabled) {
  vobjGetParamSelf();
  RET_BOOL(Self->IsRunEnabled());
}

IMPLEMENT_FUNCTION(VBasePlayer, IsMLookEnabled) {
  vobjGetParamSelf();
  RET_BOOL(Self->IsMLookEnabled());
}

IMPLEMENT_FUNCTION(VBasePlayer, IsCrouchEnabled) {
  vobjGetParamSelf();
  RET_BOOL(Self->IsCrouchEnabled());
}

IMPLEMENT_FUNCTION(VBasePlayer, IsJumpEnabled) {
  vobjGetParamSelf();
  RET_BOOL(Self->IsJumpEnabled());
}

IMPLEMENT_FUNCTION(VBasePlayer, cprint) {
  VStr msg = PF_FormatString();
  vobjGetParamSelf();
  Self->eventClientPrint(*msg);
}

IMPLEMENT_FUNCTION(VBasePlayer, centerprint) {
  VStr msg = PF_FormatString();
  vobjGetParamSelf();
  Self->eventClientCenterPrint(*msg);
}

IMPLEMENT_FUNCTION(VBasePlayer, GetPlayerNum) {
  vobjGetParamSelf();
  RET_INT(SV_GetPlayerNum(Self));
}

IMPLEMENT_FUNCTION(VBasePlayer, ClearPlayer) {
  vobjGetParamSelf();

  Self->PClass = 0;
  Self->ForwardMove = 0;
  Self->SideMove = 0;
  Self->FlyMove = 0;
  Self->Buttons = 0;
  Self->Impulse = 0;
  Self->AcsCurrButtonsPressed = 0;
  Self->AcsCurrButtons = 0;
  Self->AcsButtons = 0;
  Self->OldButtons = 0;
  Self->AcsNextButtonUpdate = 0;
  Self->AcsPrevMouseX = 0;
  Self->AcsPrevMouseY = 0;
  Self->AcsMouseX = 0;
  Self->AcsMouseY = 0;
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
  Self->FixedColormap = 0;
  Self->CShift = 0;
  Self->PSpriteSY = 0;
  Self->PSpriteWeaponLowerPrev = 0;
  Self->PSpriteWeaponLoweringStartTime = 0;
  Self->PSpriteWeaponLoweringDuration = 0;
  memset((void *)Self->LastViewObject, 0, sizeof(Self->LastViewObject));

  vuint8 *Def = Self->GetClass()->Defaults;
  for (VField *F = Self->GetClass()->Fields; F; F = F->Next) {
    VField::CopyFieldValue(Def+F->Ofs, (vuint8 *)Self+F->Ofs, F->Type);
  }
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewObject) {
  VObject *vobj;
  vobjGetParamSelf(vobj);
  //if (!vobj) GCon->Logf("RESET VIEW OBJECT; WTF?!");
  //GCon->Logf(NAME_Warning, "*** SetViewObject: %s (old: %s)", (vobj ? *vobj->GetClass()->GetFullName() : "<none>"), (Self->_stateRouteSelf ? *Self->_stateRouteSelf->GetClass()->GetFullName() : "<none>"));
  if (Self) Self->_stateRouteSelf = vobj;
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewObjectIfNone) {
  VObject *vobj;
  vobjGetParamSelf(vobj);
  //GCon->Logf(NAME_Warning, "*** SetViewObjectIfNone: %s (old: %s)", (vobj ? *vobj->GetClass()->GetFullName() : "<none>"), (Self->_stateRouteSelf ? *Self->_stateRouteSelf->GetClass()->GetFullName() : "<none>"));
  if (Self && !Self->_stateRouteSelf) Self->_stateRouteSelf = vobj;
}

IMPLEMENT_FUNCTION(VBasePlayer, SetViewState) {
  //fprintf(stderr, "*** SVS ***\n");
  VState *stnum;
  int position;
  vobjGetParamSelf(position, stnum);
  Self->SetViewState(position, stnum);
}

IMPLEMENT_FUNCTION(VBasePlayer, AdvanceViewStates) {
  float deltaTime;
  vobjGetParamSelf(deltaTime);
  Self->AdvanceViewStates(deltaTime);
}

IMPLEMENT_FUNCTION(VBasePlayer, DisconnectBot) {
  vobjGetParamSelf();
  vassert(Self->PlayerFlags&PF_IsBot);
  SV_DropClient(Self, false);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStartSound) {
  int SoundId;
  TVec Org;
  int OriginId, Channel;
  float Volume, Attenuation;
  bool Loop;
  vobjGetParamSelf(SoundId, Org, OriginId, Channel, Volume, Attenuation, Loop);
  Self->DoClientStartSound(SoundId, Org, OriginId, Channel, Volume, Attenuation, Loop);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStopSound) {
  int OriginId, Channel;
  vobjGetParamSelf(OriginId, Channel);
  Self->DoClientStopSound(OriginId, Channel);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStartSequence) {
  TVec Origin;
  int OriginId;
  VName Name;
  int ModeNum;
  vobjGetParamSelf(Origin, OriginId, Name, ModeNum);
  Self->DoClientStartSequence(Origin, OriginId, Name, ModeNum);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientAddSequenceChoice) {
  int OriginId;
  VName Choice;
  vobjGetParamSelf(OriginId, Choice);
  Self->DoClientAddSequenceChoice(OriginId, Choice);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientStopSequence) {
  int OriginId;
  vobjGetParamSelf(OriginId);
  Self->DoClientStopSequence(OriginId);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientPrint) {
  VStr Str;
  vobjGetParamSelf(Str);
  Self->DoClientPrint(Str);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientChatPrint) {
  VStr nick, str;
  vobjGetParamSelf(nick, str);
  Self->DoClientChatPrint(nick, str);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientCenterPrint) {
  VStr Str;
  vobjGetParamSelf(Str);
  Self->DoClientCenterPrint(Str);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSetAngles) {
  TAVec Angles;
  vobjGetParamSelf(Angles);
  //GCon->Logf(NAME_Debug, "!!! va=(%g,%g,%g); aa=(%g,%g,%g)", Self->ViewAngles.pitch, Self->ViewAngles.yaw, Self->ViewAngles.roll, Angles.pitch, Angles.yaw, Angles.roll);
  Self->DoClientSetAngles(Angles);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientIntermission) {
  VName NextMap;
  vobjGetParamSelf(NextMap);
  Self->DoClientIntermission(NextMap);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientPause) {
  bool Paused;
  vobjGetParamSelf(Paused);
  Self->DoClientPause(Paused);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSkipIntermission) {
  vobjGetParamSelf();
  Self->DoClientSkipIntermission();
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientFinale) {
  VStr Type;
  vobjGetParamSelf(Type);
  Self->DoClientFinale(Type);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientChangeMusic) {
  VName Song;
  vobjGetParamSelf(Song);
  Self->DoClientChangeMusic(Song);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientSetServerInfo) {
  VStr Key, Value;
  vobjGetParamSelf(Key, Value);
  Self->DoClientSetServerInfo(Key, Value);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientHudMessage) {
  VStr Message;
  VName Font;
  int Type, Id, Color;
  VStr ColorName;
  float x, y;
  int HudWidth, HudHeight;
  float HoldTime, Time1, Time2;
  vobjGetParamSelf(Message, Font, Type, Id, Color, ColorName, x, y, HudWidth, HudHeight, HoldTime, Time1, Time2);
  Self->DoClientHudMessage(Message, Font, Type, Id, Color, ColorName, x, y, HudWidth, HudHeight, HoldTime, Time1, Time2);
}

IMPLEMENT_FUNCTION(VBasePlayer, ClientFOV) {
  float fov;
  vobjGetParamSelf(fov);
  Self->DoClientFOV(fov);
}

IMPLEMENT_FUNCTION(VBasePlayer, ServerSetUserInfo) {
  VStr Info;
  vobjGetParamSelf(Info);
  Self->SetUserInfo(Info);
}

// native final void QS_PutInt (string fieldname, int value);
IMPLEMENT_FUNCTION(VBasePlayer, QS_PutInt) {
  VStr name;
  int value;
  vobjGetParamSelf(name, value);
  (void)Self;
  QS_PutValue(QSValue::CreateInt(nullptr, name, value));
}

// native final void QS_PutName (string fieldname, name value);
IMPLEMENT_FUNCTION(VBasePlayer, QS_PutName) {
  VStr name;
  VName value;
  vobjGetParamSelf(name, value);
  (void)Self;
  QS_PutValue(QSValue::CreateName(nullptr, name, value));
}

// native final void QS_PutStr (string fieldname, string value);
IMPLEMENT_FUNCTION(VBasePlayer, QS_PutStr) {
  VStr name;
  VStr value;
  vobjGetParamSelf(name, value);
  (void)Self;
  QS_PutValue(QSValue::CreateStr(nullptr, name, value));
}

// native final void QS_PutFloat (string fieldname, float value);
IMPLEMENT_FUNCTION(VBasePlayer, QS_PutFloat) {
  VStr name;
  float value;
  vobjGetParamSelf(name, value);
  (void)Self;
  QS_PutValue(QSValue::CreateFloat(nullptr, name, value));
}

// native final int QS_GetInt (string fieldname, optional int defvalue);
IMPLEMENT_FUNCTION(VBasePlayer, QS_GetInt) {
  VStr name;
  VOptParamInt value(0);
  vobjGetParamSelf(name, value);
  (void)Self;
  QSValue ret = QS_GetValue(nullptr, name);
  if (ret.type != QSType::QST_Int) {
    if (!value.specified) Host_Error("value '%s' not found for player", *name);
    ret.ival = value;
  }
  RET_INT(ret.ival);
}

// native final name QS_GetName (string fieldname, optional name defvalue);
IMPLEMENT_FUNCTION(VBasePlayer, QS_GetName) {
  VStr name;
  VOptParamName value(NAME_None);
  vobjGetParamSelf(name, value);
  (void)Self;
  QSValue ret = QS_GetValue(nullptr, name);
  if (ret.type != QSType::QST_Name) {
    if (!value.specified) Host_Error("value '%s' not found for player", *name);
    ret.nval = value;
  }
  RET_NAME(ret.nval);
}

// native final string QS_GetStr (string fieldname, optional string defvalue);
IMPLEMENT_FUNCTION(VBasePlayer, QS_GetStr) {
  VStr name;
  VOptParamStr value(VStr::EmptyString);
  vobjGetParamSelf(name, value);
  (void)Self;
  QSValue ret = QS_GetValue(nullptr, name);
  if (ret.type != QSType::QST_Str) {
    if (!value.specified) Host_Error("value '%s' not found for player", *name);
    ret.sval = value;
  }
  RET_STR(ret.sval);
}

// native final float QS_GetFloat (name fieldname, optional float defvalue);
IMPLEMENT_FUNCTION(VBasePlayer, QS_GetFloat) {
  VStr name;
  VOptParamFloat value(0.0f);
  vobjGetParamSelf(name, value);
  (void)Self;
  QSValue ret = QS_GetValue(nullptr, name);
  if (ret.type != QSType::QST_Float) {
    if (!value.specified) Host_Error("value '%s' not found for player", *name);
    ret.fval = value;
  }
  RET_FLOAT(ret.fval);
}


//==========================================================================
//
//  ChangeWeapon
//
//==========================================================================
COMMAND(ChangeWeapon) {
  CMD_FORWARD_TO_SERVER();

  if (!Player) return;
  if (Args.Num() < 2) { GCon->Logf(NAME_Warning, "ChangeWeapon expects weapon list"); return; }

  VEntity *curwpn = Player->eventGetReadyWeapon();

  int nextIndex = 1;
  if (curwpn) {
    //GCon->Logf("*** CW: active weapon %s", curwpn->GetClass()->GetName());
    for (int widx = 1; widx < Args.length(); ++widx) {
      //if (Args[widx].strEquCI(*curwpn->GetClass()->Name)) { nextIndex = widx+1; break; }
      if (Player->eventIsReadyWeaponByName(Args[widx], true)) { // allow replacements
        //GCon->Logf("*** CW: active weapon at index=%d; str=%s; wpn=%s", widx, *Args[widx], curwpn->GetClass()->GetName());
        nextIndex = widx+1;
        break;
      }
    }
  }

  for (int widx = 1; widx < Args.length(); ++widx, ++nextIndex) {
    if (nextIndex >= Args.length()) nextIndex = 1;
    VEntity *newwpn = Player->eventFindInventoryWeapon(Args[nextIndex], true); // allow replacements
    /*
    if (newwpn) {
      GCon->Logf("*** CW: index=%d; str=%s; wpn=%s", nextIndex, *Args[nextIndex], newwpn->GetClass()->GetName());
    } else {
      GCon->Logf(NAME_Warning, "*** CW: index=%d; str=%s; SKIP", nextIndex, *Args[nextIndex]);
    }
    */
    if (newwpn && Player->eventSetPendingWeapon(newwpn)) return;
  }
}