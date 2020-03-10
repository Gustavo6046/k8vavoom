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
//**
//**  Event handling
//**
//**  Events are asynchronous inputs generally generated by the game user.
//**  Events can be discarded if no responder claims them
//**
//**************************************************************************
#include "gamedefs.h"
#include "ui/ui.h"
#include "neoui/neoui.h"


static VCvarB allow_vanilla_cheats("allow_vanilla_cheats", true, "Allow vanilla keyboard cheat codes?", CVAR_Archive);

extern void UnpressAllButtons ();


// ////////////////////////////////////////////////////////////////////////// //
// input subsystem, handles all input events
class VInput : public VInputPublic {
public:
  VInput ();
  virtual ~VInput () override;

  // system device related functions
  virtual void Init () override;
  virtual void Shutdown () override;

  // input event handling
  virtual void ProcessEvents () override;
  virtual int ReadKey () override;

  // handling of key bindings
  virtual void ClearBindings () override;
  virtual void GetBindingKeys (VStr Binding, int &Key1, int &Key2, VStr modSection, int strifemode, int *isActive) override;
  virtual void GetDefaultModBindingKeys (VStr Binding, int &Key1, int &Key2, VStr modSection) override;
  virtual void GetBinding (int KeyNum, VStr &Down, VStr &Up) override;
  virtual void SetBinding (int KeyNum, VStr Down, VStr Up, VStr modSection, int strifemode, bool allowOverride=true) override;
  virtual void WriteBindings (VStream *st) override;
  virtual void AddActiveMod (VStr modSection) override;

  virtual int TranslateKey (int ch) override;

  virtual int KeyNumForName (VStr Name) override;
  virtual VStr KeyNameForNum (int KeyNr) override;

  virtual void RegrabMouse () override; // called by UI when mouse cursor is turned off

  virtual void SetClipboardText (VStr text) override;
  virtual bool HasClipboardText () override;
  virtual VStr GetClipboardText () override;

private:
  VInputDevice *Device;

  int strifeMode = -666;

  inline const bool IsStrife () {
    if (strifeMode == -666) strifeMode = (game_name.asStr().strEquCI("strife") ? 1 : 0);
    return strifeMode;
  }

  bool lastWasGameBinding = false;

public:
  struct Binding {
    VStr cmdDown;
    VStr cmdUp;
    // for mods
    VStr modName;
    int keyNum;
    bool defbind; // default mod binding?
    // note that mod binding may be empty, which means "user deleted it"

    Binding () : cmdDown(), cmdUp(), modName(), keyNum(0), defbind(false) {}
    inline bool IsEmpty () const { return (cmdDown.isEmpty() && cmdUp.isEmpty()); }
    inline void Clear () { cmdDown.clear(); cmdUp.clear(); }
  };

private:
  //typedef Binding BindingArray[256];

  Binding KeyBindingsAll[256];
  Binding KeyBindingsStrife[256];
  Binding KeyBindingsNonStrife[256];
  TArray<Binding> ModBindings;
  TArray<Binding> DefaultModBindings;
  // this is to map key to console command in input handler
  Binding ModKeyBindingsActive[256];
  TArray<VStr> ActiveModList;

  VStr getBinding (bool down, int idx);

  void sortModKeys ();
  // call after adding new active mod
  void rebuildModBindings ();

  bool hasModBinding (VStr Down, VStr modSection);
  bool isDefaultModBinding (VStr Down, VStr modSection);
  void clearDefaultModBindings (VStr Down, VStr modSection);

  static const char ShiftXForm[128];
};

VInputPublic *GInput;


// key shifting
const char VInput::ShiftXForm[128] = {
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', // shift-[
  '|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  '}', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};


TArray<VInputPublic::CheatCode> VInputPublic::kbcheats;
char VInputPublic::currkbcheat[VInputPublic::MAX_KBCHEAT_LENGTH+1];


extern "C" {
  static int cmpKeyBinding (const void *aa, const void *bb, void *) {
    if (aa == bb) return 0;
    const VInput::Binding *a = (const VInput::Binding *)aa;
    const VInput::Binding *b = (const VInput::Binding *)bb;
    int sdiff = a->modName.ICmp(b->modName);
    if (sdiff) return sdiff;
    // same mod, sort by key index
    return a->keyNum-b->keyNum;
  }
}


//==========================================================================
//
//  VInputPublic::Create
//
//==========================================================================
VInputPublic *VInputPublic::Create () {
  return new VInput();
}


//==========================================================================
//
//  VInputPublic::KBCheatClearAll
//
//==========================================================================
void VInputPublic::KBCheatClearAll () {
  kbcheats.clear();
}


//==========================================================================
//
//  VInputPublic::KBCheatAppend
//
//==========================================================================
void VInputPublic::KBCheatAppend (VStr keys, VStr concmd) {
  if (keys.length() == 0) return;
  for (int f = 0; f < kbcheats.length(); ++f) {
    if (kbcheats[f].keys.strEquCI(keys)) {
      if (concmd.length() == 0) {
        kbcheats.removeAt(f);
      } else {
        kbcheats[f].concmd = concmd;
      }
      return;
    }
  }
  // reset cheat
  currkbcheat[0] = 0;
  CheatCode &cc = kbcheats.alloc();
  cc.keys = keys;
  cc.concmd = concmd;
  //GCon->Logf("added cheat code '%s' (command: '%s')", *keys, *concmd);
}


//==========================================================================
//
//  VInputPublic::KBCheatProcessor
//
//==========================================================================
bool VInputPublic::KBCheatProcessor (event_t *ev) {
  if (!allow_vanilla_cheats) { currkbcheat[0] = 0; return false; }
  if (ev->type != ev_keydown) return false;
  if (ev->data1 < ' ' || ev->data1 >= 127) { currkbcheat[0] = 0; return false; }
  int slen = (int)strlen(currkbcheat);
  if (slen >= MAX_KBCHEAT_LENGTH) { currkbcheat[0] = 0; return false; }
  currkbcheat[slen] = (char)ev->data1;
  currkbcheat[slen+1] = 0;
  ++slen;
  int clen = kbcheats.length();
  //GCon->Logf("C:<%s> (clen=%d)", currkbcheat, clen);
  char digits[MAX_KBCHEAT_LENGTH+1];
  for (int f = 0; f <= MAX_KBCHEAT_LENGTH; ++f) digits[f] = '0';
  int digcount = 0;
  bool wasCheat = false;
  for (int cnum = 0; cnum < clen; ++cnum) {
    CheatCode &cc = kbcheats[cnum];
    //GCon->Logf("  check00:<%s> (with <%s>)", *cc.keys, currkbcheat);
    if (cc.keys.length() < slen) continue; // too short
    bool ok = true;
    for (int f = 0; f < slen; ++f) {
      char c1 = currkbcheat[f];
      if (c1 >= 'A' && c1 <= 'Z') c1 = c1-'A'+'a';
      char c2 = cc.keys[f];
      if (c2 >= 'A' && c2 <= 'Z') c2 = c2-'A'+'a';
      if (c1 == c2) continue;
      if (c2 == '#' && (c1 >= '0' && c1 <= '9')) {
        digits[digcount++] = c1;
        continue;
      }
      ok = false;
      break;
    }
    //GCon->Logf("  check01:<%s> (with <%s>): ok=%d", *cc.keys, currkbcheat, (ok ? 1 : 0));
    if (!ok) continue;
    if (cc.keys.length() == slen) {
      // cheat complete
      VStr concmd;
      if (digcount > 0) {
        // preprocess console command
        int dignum = 0;
        for (int f = 0; f < cc.concmd.length(); ++f) {
          if (cc.concmd[f] == '#') {
            if (dignum < digcount) {
              concmd += digits[dignum++];
            } else {
              concmd += '0';
            }
          } else {
            concmd += cc.concmd[f];
          }
        }
      } else {
        concmd = cc.concmd;
      }
      if (concmd.length()) {
        if (concmd[concmd.length()-1] != '\n') concmd += "\n";
        GCmdBuf << concmd;
      }
      // reset cheat
      currkbcheat[0] = 0;
      return true;
    }
    wasCheat = true;
  }
  // nothing was found, reset
  if (!wasCheat) currkbcheat[0] = 0;
  return false;
}


//==========================================================================
//
//  VInputPublic::UnpressAll
//
//==========================================================================
void VInputPublic::UnpressAll () {
  UnpressAllButtons();
}


//==========================================================================
//
//  VInput::VInput
//
//==========================================================================
VInput::VInput () : Device(0) {
  memset((void *)&KeyBindingsAll[0], 0, sizeof(KeyBindingsAll[0]));
  memset((void *)&KeyBindingsStrife[0], 0, sizeof(KeyBindingsStrife[0]));
  memset((void *)&KeyBindingsNonStrife[0], 0, sizeof(KeyBindingsNonStrife[0]));
  memset((void *)&ModKeyBindingsActive[0], 0, sizeof(ModKeyBindingsActive[0]));
}


//==========================================================================
//
//  VInput::~VInput
//
//==========================================================================
VInput::~VInput () {
  Shutdown();
}


//==========================================================================
//
//  VInput::ClearBindings
//
//==========================================================================
void VInput::ClearBindings () {
  for (int f = 0; f < 256; ++f) {
    ModKeyBindingsActive[f].Clear();
    KeyBindingsAll[f].Clear();
    KeyBindingsStrife[f].Clear();
    KeyBindingsNonStrife[f].Clear();
  }
  // restore default mod bindings
  ModBindings.clear();
  for (auto &&bind : DefaultModBindings) ModBindings.append(bind);
}


//==========================================================================
//
//  VInput::sortModKeys
//
//==========================================================================
void VInput::sortModKeys () {
  if (ModBindings.length() < 2) return;
  timsort_r(ModBindings.ptr(), ModBindings.length(), sizeof(Binding), &cmpKeyBinding, nullptr);
}


//==========================================================================
//
//  VInput::rebuildModBindings
//
//  call after adding new active mod
//
//==========================================================================
void VInput::rebuildModBindings () {
  for (auto &&bd : ModKeyBindingsActive) {
    bd.Clear();
    bd.modName.clear();
  }

  sortModKeys();
  for (int midx = ActiveModList.length()-1; midx >= 0; --midx) {
    VStr modname = ActiveModList[midx];
    int stidx;
    for (stidx = 0; stidx < ModBindings.length(); ++stidx) {
      const Binding &bind = ModBindings[stidx];
      if (bind.modName.strEquCI(modname)) break;
    }
    if (stidx >= ModBindings.length()) continue;
    for (; stidx < ModBindings.length(); ++stidx) {
      const Binding &bind = ModBindings[stidx];
      if (!bind.modName.strEquCI(modname)) break;
      if (bind.IsEmpty()) continue;
      vassert(bind.keyNum > 0 && bind.keyNum < 256);
      // default bindings cannot override game bindings
      if (bind.defbind) {
        if (!getBinding(true, bind.keyNum).isEmpty() || !getBinding(false, bind.keyNum).isEmpty()) continue;
      }
      ModKeyBindingsActive[bind.keyNum].cmdDown = bind.cmdDown;
      ModKeyBindingsActive[bind.keyNum].cmdUp = bind.cmdUp;
      ModKeyBindingsActive[bind.keyNum].modName = bind.modName;
    }
  }
}


//==========================================================================
//
//  VInput::hasModBinding
//
//==========================================================================
bool VInput::hasModBinding (VStr Down, VStr modSection) {
  if (Down.isEmpty() || modSection.isEmpty()) return false;
  for (auto &&bind : ModBindings) {
    if (!bind.modName.strEquCI(modSection)) continue;
    if (bind.cmdDown == Down) return true;
  }
  return false;
}


//==========================================================================
//
//  VInput::isDefaultModBinding
//
//==========================================================================
bool VInput::isDefaultModBinding (VStr Down, VStr modSection) {
  if (Down.isEmpty() || modSection.isEmpty()) return false;
  bool found = false;
  for (auto &&bind : ModBindings) {
    if (!bind.modName.strEquCI(modSection)) continue;
    if (bind.cmdDown != Down) continue;
    if (!bind.defbind) return false;
    found = true;
  }
  return found;
}


//==========================================================================
//
//  VInput::clearDefaultModBindings
//
//==========================================================================
void VInput::clearDefaultModBindings (VStr Down, VStr modSection) {
  if (Down.isEmpty() || modSection.isEmpty()) return;
  for (int stidx = 0; stidx < ModBindings.length(); ++stidx) {
    Binding &bind = ModBindings[stidx];
    if (!bind.defbind) continue;
    if (!bind.modName.strEquCI(modSection)) continue;
    if (bind.cmdDown != Down) continue;
    ModBindings.removeAt(stidx);
    --stidx;
  }
}


//==========================================================================
//
//  VInput::AddActiveMod
//
//==========================================================================
void VInput::AddActiveMod (VStr modSection) {
  //modSection = modSection.xstrip();
  if (modSection.IsEmpty()) return;
  for (int f = 0; f < ActiveModList.length(); ++f) {
    if (ActiveModList[f].strEquCI(modSection)) {
      ActiveModList.removeAt(f);
      break;
    }
  }
  ActiveModList.append(modSection);
  rebuildModBindings();
}


//==========================================================================
//
//  VInput::getBinding
//
//==========================================================================
VStr VInput::getBinding (bool down, int idx) {
  if (idx < 1 || idx > 255) return VStr::EmptyString;
  if (!ModKeyBindingsActive[idx].IsEmpty()) {
    return (down ? ModKeyBindingsActive[idx].cmdDown : ModKeyBindingsActive[idx].cmdUp);
  }
  // for all games
  if (IsStrife()) {
    if (!KeyBindingsStrife[idx].IsEmpty()) {
      return (down ? KeyBindingsStrife[idx].cmdDown : KeyBindingsStrife[idx].cmdUp);
    }
  } else {
    if (!KeyBindingsNonStrife[idx].IsEmpty()) {
      return (down ? KeyBindingsNonStrife[idx].cmdDown : KeyBindingsNonStrife[idx].cmdUp);
    }
  }
  return (down ? KeyBindingsAll[idx].cmdDown : KeyBindingsAll[idx].cmdUp);
}


//==========================================================================
//
//  VInput::Init
//
//==========================================================================
void VInput::Init () {
  Device = VInputDevice::CreateDevice();
}


//==========================================================================
//
//  VInput::Shutdown
//
//==========================================================================
void VInput::Shutdown () {
  if (Device) {
    delete Device;
    Device = nullptr;
  }
}


//==========================================================================
//
//  VInputPublic::PostKeyEvent
//
//  Called by the I/O functions when a key or button is pressed or released
//
//==========================================================================
bool VInputPublic::PostKeyEvent (int key, int press, vuint32 modflags) {
  if (!key) return true; // always succeed
  event_t ev;
  memset((void *)&ev, 0, sizeof(event_t));
  ev.type = (press ? ev_keydown : ev_keyup);
  ev.data1 = key;
  ev.modflags = modflags;
  return VObject::PostEvent(ev);
}


//==========================================================================
//
//  VInput::ProcessEvents
//
//  Send all the Events of the given timestamp down the responder chain
//
//==========================================================================
void VInput::ProcessEvents () {
  bool reachedBinding = false;
  bool wasEvent = false;
  Device->ReadInput();
  for (int count = VObject::CountQueuedEvents(); count > 0; --count) {
    event_t ev;
    if (!VObject::GetEvent(&ev)) break;
    wasEvent = true;

    // shift key state
    if (ev.data1 == K_RSHIFT) { if (ev.type == ev_keydown) ShiftDown |= 1; else ShiftDown &= ~1; }
    if (ev.data1 == K_LSHIFT) { if (ev.type == ev_keydown) ShiftDown |= 2; else ShiftDown &= ~2; }
    // ctrl key state
    if (ev.data1 == K_RCTRL) { if (ev.type == ev_keydown) CtrlDown |= 1; else CtrlDown &= ~1; }
    if (ev.data1 == K_LCTRL) { if (ev.type == ev_keydown) CtrlDown |= 2; else CtrlDown &= ~2; }
    // alt key state
    if (ev.data1 == K_RALT) { if (ev.type == ev_keydown) AltDown |= 1; else AltDown &= ~1; }
    if (ev.data1 == K_LALT) { if (ev.type == ev_keydown) AltDown |= 2; else AltDown &= ~2; }

    // initial network client data transmit?
    bool initNetClient = (CL_GetNetState() == CLState_Init);

    if (!initNetClient) {
      if (C_Responder(&ev)) continue; // console
      if (NUI_Responder(&ev)) continue; // new UI
      if (CT_Responder(&ev)) continue; // chat
      if (MN_Responder(&ev)) continue; // menu
      if (GRoot->Responder(&ev)) continue; // root widget
    } else {
      // check for user abort
      if (ev.type == ev_keydown && ev.keycode == K_ESCAPE) {
        GCmdBuf << "Disconnect\n";
      }
    }

    //k8: this hack prevents "keyup" to be propagated when console is active
    //    this should be in console responder, but...
    //if (C_Active() && (ev.type == ev_keydown || ev.type == ev_keyup)) continue;
    // actually, when console is active, it eats everything
    if (initNetClient || C_Active()) continue;

    reachedBinding = true;
    if (!lastWasGameBinding) {
      //GCon->Log("unpressing(0)...");
      lastWasGameBinding = true;
      UnpressAll();
    }

    if (CL_IsInGame()) {
      if (KBCheatProcessor(&ev)) continue; // cheatcode typed
      if (SB_Responder(&ev)) continue; // status window ate it
      if (AM_Responder(&ev)) continue; // automap ate it
    }

    if (F_Responder(&ev)) continue; // finale

    // key bindings
    if ((ev.type == ev_keydown || ev.type == ev_keyup) && (ev.data1 > 0 && ev.data1 < 256)) {
      //VStr kb;
      //if (isAllowed(ev.data1&0xff)) kb = (ev.type == ev_keydown ? KeyBindingsDown[ev.data1&0xff] : KeyBindingsUp[ev.data1&0xff]);
      VStr kb = getBinding((ev.type == ev_keydown), ev.data1&0xff);
      //GCon->Logf("KEY %s is %s; action is '%s'", *GInput->KeyNameForNum(ev.data1&0xff), (ev.type == ev_keydown ? "down" : "up"), *kb);
      if (kb.IsNotEmpty()) {
        if (kb[0] == '+' || kb[0] == '-') {
          // button commands add keynum as a parm
          if (kb.length() > 1) GCmdBuf << kb << " " << VStr(ev.data1) << "\n";
        } else {
          GCmdBuf << kb << "\n";
        }
        continue;
      }
    }

    if (CL_Responder(&ev)) continue;
  }

  if (wasEvent && !reachedBinding) {
    // something ate all the keys, so unpress buttons
    if (lastWasGameBinding) {
      //GCon->Log("unpressing(1)...");
      lastWasGameBinding = false;
      UnpressAll();
    }
  }
}


//==========================================================================
//
//  VInput::ReadKey
//
//==========================================================================
int VInput::ReadKey () {
  int ret = 0;
  do {
    Device->ReadInput();
    event_t ev;
    while (!ret && VObject::GetEvent(&ev)) {
      if (ev.type == ev_keydown) ret = ev.data1;
    }
  } while (!ret);
  return ret;
}


//==========================================================================
//
//  VInput::GetBinding
//
//==========================================================================
void VInput::GetBinding (int KeyNum, VStr &Down, VStr &Up) {
  //Down = KeyBindingsDown[KeyNum];
  //Up = KeyBindingsUp[KeyNum];
  Down = getBinding(true, KeyNum);
  Up = getBinding(false, KeyNum);
}


//==========================================================================
//
//  VInput::GetDefaultModBindingKeys
//
//==========================================================================
void VInput::GetDefaultModBindingKeys (VStr bindStr, int &Key1, int &Key2, VStr modSection) {
  Key1 = -1;
  Key2 = -1;
  if (bindStr.isEmpty() || modSection.isEmpty()) return;
  for (auto &&bind : DefaultModBindings) {
    if (bind.keyNum < 1 || bind.keyNum > 255) continue; // just in case
    if (!bind.modName.strEquCI(modSection)) continue;
    int kf = -1;
    if (!bind.IsEmpty() && bind.cmdDown.strEquCI(bindStr)) kf = bind.keyNum;
    if (kf > 0) {
      if (Key1 != -1) { Key2 = kf; return; }
      Key1 = kf;
    }
  }
}


//==========================================================================
//
//  VInput::GetBindingKeys
//
//==========================================================================
void VInput::GetBindingKeys (VStr bindStr, int &Key1, int &Key2, VStr modSection, int strifemode, int *isActive) {
  Key1 = -1;
  Key2 = -1;
  if (isActive) *isActive = 3;
  if (bindStr.isEmpty()) return;
  // mod?
  if (!modSection.isEmpty()) {
    //GCon->Logf(NAME_Debug, "*** bstr=\"%s\"; mod=\"%s\" (%d)", *bindStr.quote(), *modSection.quote(), ModBindings.length());
    if (isActive) *isActive = 0;
    for (auto &&bind : ModBindings) {
      //GCon->Logf(NAME_Debug, "module %s: key=%d (%s : %s)", *bind.modName, bind.keyNum, *bind.cmdDown, *bind.cmdUp);
      if (bind.keyNum < 1 || bind.keyNum > 255) continue; // just in case
      if (!bind.modName.strEquCI(modSection)) continue;
      //GCon->Logf(NAME_Debug, "module %s: key=%d (%s : %s)", *bind.modName, bind.keyNum, *bind.cmdDown, *bind.cmdUp);
      int kf = -1;
      if (!bind.IsEmpty() && bind.cmdDown.strEquCI(bindStr)) kf = bind.keyNum;
      if (kf > 0) {
        // check if it is active
        if (isActive) {
          if (!ModKeyBindingsActive[kf].IsEmpty() && ModKeyBindingsActive[kf].modName.strEquCI(modSection)) {
            *isActive |= (Key1 != -1 ? 0x02 : 0x01);
          }
        }
        if (Key1 != -1) { Key2 = kf; return; }
        Key1 = kf;
      }
    }
  } else {
    // normal
    for (int i = 1; i < 256; ++i) {
      int kf = -1;
      if (strifemode < 0) {
        if (!KeyBindingsNonStrife[i].IsEmpty() && KeyBindingsNonStrife[i].cmdDown.strEquCI(bindStr)) kf = i;
      } else if (strifemode > 0) {
        if (!KeyBindingsStrife[i].IsEmpty() && KeyBindingsStrife[i].cmdDown.strEquCI(bindStr)) kf = i;
      } else {
        if (!KeyBindingsAll[i].IsEmpty() && KeyBindingsAll[i].cmdDown.strEquCI(bindStr)) kf = i;
      }
      if (kf > 0) {
        if (Key1 != -1) { Key2 = kf; return; }
        Key1 = kf;
      }
    }
  }
}


//==========================================================================
//
//  VInput::SetBinding
//
//==========================================================================
void VInput::SetBinding (int KeyNum, VStr Down, VStr Up, VStr modSection, int strifemode, bool allowOverride) {
  if (KeyNum < 1 || KeyNum > 255) return;

  // totally remove mod binding?
  if (Down.strEquCI("<modclear>")) {
    for (int stpos = 0; stpos < ModBindings.length(); ++stpos) {
      Binding &bind = ModBindings[stpos];
      if (bind.keyNum != KeyNum) continue;
      if (!bind.modName.strEquCI(modSection)) continue;
      ModBindings.removeAt(stpos);
      --stpos;
    }
    rebuildModBindings();
    return;
  }

  // restore default mod binding?
  if (Down.strEquCI("<default>")) {
    if (modSection.isEmpty()) return;
    Binding *defbp = nullptr;
    for (auto &&bind : DefaultModBindings) {
      if (bind.keyNum != KeyNum) continue;
      if (!bind.modName.strEquCI(modSection)) continue;
      defbp = &bind;
      break;
    }
    // remove?
    if (!defbp) {
      for (int stpos = 0; stpos < ModBindings.length(); ++stpos) {
        Binding &bind = ModBindings[stpos];
        if (bind.keyNum != KeyNum) continue;
        if (!bind.modName.strEquCI(modSection)) continue;
        ModBindings.removeAt(stpos);
        --stpos;
      }
    } else {
      // set from default
      bool foundIt = false;
      for (int stpos = 0; stpos < ModBindings.length(); ++stpos) {
        Binding &bind = ModBindings[stpos];
        if (bind.keyNum != KeyNum) continue;
        if (!bind.modName.strEquCI(modSection)) continue;
        bind.defbind = true;
        bind.cmdDown = defbp->cmdDown;
        bind.cmdUp = defbp->cmdUp;
        foundIt = true;
      }
      // if not found, add default binding
      if (!foundIt) {
        Binding &bind = ModBindings.alloc();
        bind.keyNum = KeyNum;
        bind.modName = defbp->modName;
        bind.defbind = true;
        bind.cmdDown = defbp->cmdDown;
        bind.cmdUp = defbp->cmdUp;
      }
    }
    rebuildModBindings();
    return;
  }

  // mod?
  if (modSection.length()) {
    // find default binding
    Binding *defbp = nullptr;
    for (auto &&bind : DefaultModBindings) {
      if (bind.keyNum != KeyNum) continue;
      if (!bind.modName.strEquCI(modSection)) continue;
      defbp = &bind;
      break;
    }
    bool newdefbind = false;
    // append default binding (if it is a default one)
    if (!allowOverride) {
      if (!defbp) {
        newdefbind = true;
        defbp = &DefaultModBindings.alloc();
        vassert(defbp);
        defbp->modName = modSection;
        defbp->keyNum = KeyNum;
      }
      vassert(defbp);
      vassert(defbp->keyNum == KeyNum);
      vassert(defbp->modName.strEquCI(modSection));
      defbp->defbind = true;
      defbp->cmdDown = Down;
      defbp->cmdUp = Up;
      // if we already have binding for this mod action, do not modify anything (this is keyconf parser)
      //GCon->Logf(NAME_Debug, "DEFAULT MOD(%s): down=%s", *modSection, *Down);
      //if (hasModBinding(Down, modSection)) return;
    } else {
      //GCon->Logf(NAME_Debug, "MOD(%s): down=%s", *modSection, *Down);
      // clear default bindings, if this is not a default one
      if (isDefaultModBinding(Down, modSection)) clearDefaultModBindings(Down, modSection);
    }
    // find existing one
    Binding *bp = nullptr;
    for (auto &&bind : ModBindings) {
      if (bind.keyNum != KeyNum) continue;
      if (!bind.modName.strEquCI(modSection)) continue;
      bp = &bind;
      break;
    }
    // not found?
    if (!bp) {
      //GCon->Logf(NAME_Debug, "NEW MOD: \"%s\"", *modSection.quote());
      // add new mod section
      bp = &ModBindings.alloc();
      vassert(bp);
      //allowOverride = true;
      bp->modName = modSection;
      bp->keyNum = KeyNum;
      bp->defbind = !allowOverride;
      if (Down.isEmpty() && Up.isEmpty()) {
        rebuildModBindings();
        return;
      }
    }
    vassert(bp);
    vassert(bp->keyNum == KeyNum);
    vassert(bp->modName.strEquCI(modSection));
    if (allowOverride || bp->IsEmpty()) {
      //GCon->Logf(NAME_Debug, "SETTING KEY (mod=\"%s\"): key=%d; down=\"%s\"; up=\"%s\"", *modSection.quote(), KeyNum, *Down.quote(), *Up.quote());
      bp->cmdDown = Down;
      bp->cmdUp = Up;
      bp->defbind = !allowOverride;
    }
    if (newdefbind && bp->cmdDown == Down && bp->cmdUp == Up) {
      bp->defbind = true;
    }
  } else {
    // normal; ignores "allow override"
    if (strifemode == 0) {
      KeyBindingsAll[KeyNum].cmdDown = Down;
      KeyBindingsAll[KeyNum].cmdUp = Up;
    } else if (strifemode < 0) {
      KeyBindingsNonStrife[KeyNum].cmdDown = Down;
      KeyBindingsNonStrife[KeyNum].cmdUp = Up;
    } else {
      KeyBindingsStrife[KeyNum].cmdDown = Down;
      KeyBindingsStrife[KeyNum].cmdUp = Up;
    }
  }
  rebuildModBindings();
  /*
  for (int f = 1; f < 256; ++f) {
    if (ModKeyBindingsActive[f].IsEmpty()) continue;
    GCon->Logf(NAME_Debug, "  key #%2d: name=\"%s\"; mod=\"%s\"; down=\"%s\"; up=\"%s\"", f, *KeyNameForNum(f).quote(), *ModKeyBindingsActive[f].modName.quote(), *ModKeyBindingsActive[f].cmdDown.quote(), *ModKeyBindingsActive[f].cmdUp.quote());
  }
  */
}


//==========================================================================
//
//  VInput::WriteBindings
//
//  Writes lines containing "bind key value"
//
//==========================================================================
void VInput::WriteBindings (VStream *st) {
  st->writef("UnbindAll\n");
  for (int i = 1; i < 256; ++i) {
    if (!KeyBindingsAll[i].IsEmpty()) st->writef("bind \"%s\" \"%s\" \"%s\"\n", *KeyNameForNum(i).quote(), *KeyBindingsAll[i].cmdDown.quote(), *KeyBindingsAll[i].cmdUp.quote());
    if (!KeyBindingsStrife[i].IsEmpty()) st->writef("bind strife \"%s\" \"%s\" \"%s\"\n", *KeyNameForNum(i).quote(), *KeyBindingsStrife[i].cmdDown.quote(), *KeyBindingsStrife[i].cmdUp.quote());
    if (!KeyBindingsNonStrife[i].IsEmpty()) st->writef("bind notstrife \"%s\" \"%s\" \"%s\"\n", *KeyNameForNum(i).quote(), *KeyBindingsNonStrife[i].cmdDown.quote(), *KeyBindingsNonStrife[i].cmdUp.quote());
  }
  // write mod bindings
  sortModKeys();
  VStr lastHeader;
  for (auto &&bind : ModBindings) {
    if (bind.defbind) continue;
    if (!bind.modName.strEquCI(lastHeader)) {
      lastHeader = bind.modName;
      st->writef("// module '%s'\n", *lastHeader);
    }
    st->writef("bind module \"%s\"  \"%s\" \"%s\" \"%s\"\n", *bind.modName.quote(), *KeyNameForNum(bind.keyNum).quote(), *bind.cmdDown.quote(), *bind.cmdUp.quote());
  }
  st->writef("// bindings complete\n\n");
}


//==========================================================================
//
//  VInput::TranslateKey
//
//==========================================================================
int VInput::TranslateKey (int ch) {
  switch (ch) {
    case K_PADDIVIDE: return '/';
    case K_PADMULTIPLE: return '*';
    case K_PADMINUS: return '-';
    case K_PADPLUS: return '+';
    case K_PADDOT: return '.';
  }
  if (ch <= 0 || ch > 127) return ch;
  return (ShiftDown ? ShiftXForm[ch] : ch);
}


//==========================================================================
//
//  VInput::KeyNumForName
//
//  Searches in key names for given name
//  return key code
//
//==========================================================================
int VInput::KeyNumForName (VStr Name) {
  if (Name.IsEmpty()) return -1;
  int res = VObject::VKeyFromName(Name);
  if (res <= 0) res = -1;
  return res;
}


//==========================================================================
//
//  VInput::KeyNameForNum
//
//  Writes into given string key name
//
//==========================================================================
VStr VInput::KeyNameForNum (int KeyNr) {
       if (KeyNr == ' ') return "SPACE";
  else if (KeyNr == K_ESCAPE) return VStr("ESCAPE");
  else if (KeyNr == K_ENTER) return VStr("ENTER");
  else if (KeyNr == K_TAB) return VStr("TAB");
  else if (KeyNr == K_BACKSPACE) return VStr("BACKSPACE");
  else return VObject::NameFromVKey(KeyNr);
}


//==========================================================================
//
//  VInput::RegrabMouse
//
//  Called by UI when mouse cursor is turned off
//
//==========================================================================
void VInput::RegrabMouse () {
  if (Device) Device->RegrabMouse();
}


//==========================================================================
//
//  VInput::SetClipboardText
//
//==========================================================================
void VInput::SetClipboardText (VStr text) {
  if (Device) Device->SetClipboardText(text);
}


//==========================================================================
//
//  VInput::HasClipboardText
//
//==========================================================================
bool VInput::HasClipboardText () {
  return (Device ? Device->HasClipboardText() : false);
}


//==========================================================================
//
//  VInput::GetClipboardText
//
//==========================================================================
VStr VInput::GetClipboardText () {
  return (Device ? Device->GetClipboardText() : VStr::EmptyString);
}


//==========================================================================
//
//  COMMAND Unbind
//
//==========================================================================
COMMAND(Unbind) {
  if (ParsingKeyConf) return; // in keyconf
  int stidx = 1;
  int strifeFlag = 0;
  VStr modSection;

  if (Args.length() > 1) {
         if (Args[1].strEquCI("strife")) { strifeFlag = 1; ++stidx; }
    else if (Args[1].strEquCI("notstrife")) { strifeFlag = -1; ++stidx; }
    else if (Args[1].strEquCI("all")) { strifeFlag = 0; ++stidx; }
    else if (Args[stidx].strEquCI("module")) {
      ++stidx;
      if (stidx >= Args.length()) return; //FIXME: show error
      modSection = Args[stidx++];
    }
  }

  if (Args.length() != stidx+1) {
    GCon->Log("unbind <key> : remove commands from a key");
    return;
  }

  int b = GInput->KeyNumForName(Args[stidx]);
  if (b == -1) {
    GCon->Logf("\"%s\" isn't a valid key", *Args[stidx].quote());
    return;
  }

  GInput->SetBinding(b, VStr(), VStr(), modSection, strifeFlag);
}


//==========================================================================
//
//  COMMAND UnbindAll
//
//==========================================================================
COMMAND(UnbindAll) {
  if (ParsingKeyConf) return; // in keyconf
  GInput->ClearBindings();
}


//==========================================================================
//
//  COMMAND Bind
//
//==========================================================================
static void bindCommon (const TArray<VStr> &Args, bool allowOverride=true) {
  const int argc = Args.length();

  /*
  if (!allowOverride) {
    VStr cmds;
    for (auto &&s : Args) cmds += va("\"%s\" ", *s.quote());
    GCon->Logf(NAME_Debug, "bindCommon: %s", *cmds);
  }
  */

  if (argc < 2 || argc > 6) {
    GCon->Logf("%s [strife|nostrife|all|module <name>] <key> [down_command] [up_command]: attach a command to a key", *Args[0]);
    return;
  }

  int strifeFlag = 0;
  int stidx = 1;
  VStr modSection;

       if (Args[stidx].strEquCI("strife")) { strifeFlag = 1; ++stidx; }
  else if (Args[stidx].strEquCI("notstrife")) { strifeFlag = -1; ++stidx; }
  else if (Args[stidx].strEquCI("all")) { strifeFlag = 0; ++stidx; }
  else if (Args[stidx].strEquCI("module")) {
    ++stidx;
    if (stidx >= Args.length()) return; //FIXME: show error
    modSection = Args[stidx++];
  }

  int alen = argc-stidx;

  if (alen < 1) {
    GCon->Logf("%s [strife|nostrife|all|module <name>] <key> [down_command] [up_command]: attach a command to a key", *Args[0]);
    return;
  }

  VStr kname = Args[stidx];

  if (kname.length() == 0) {
    GCon->Logf("%s: key name?", *Args[0]);
    return;
  }

  int b = GInput->KeyNumForName(kname);
  if (b == -1) {
    GCon->Logf(NAME_Error, "\"%s\" isn't a valid key", *kname.quote());
    return;
  }

  if (alen == 1) {
    VStr Down, Up;
    GInput->GetBinding(b, Down, Up);
    if (Down.IsNotEmpty() || Up.IsNotEmpty()) {
      if (Up.IsNotEmpty()) {
        GCon->Logf("%s \"%s\" \"%s\" \"%s\"", *Args[0], *kname.quote(), *Down.quote(), *Up.quote());
      } else {
        GCon->Logf("%s \"%s\" \"%s\" \"\"", *Args[0], *kname.quote(), *Down.quote());
      }
    } else {
      GCon->Logf("\"%s\" is not bound", *kname.quote());
    }
  } else {
    //GCon->Logf("key \"%s\" (%d); down=\"%s\"; up=\"%s\", mod=\"%s\", strifeFlag=%d; allowOverride=%d", *kname.quote(), b, *Args[stidx+1].quote(), (alen > 2 ? *Args[stidx+2].quote() : ""), *modSection.quote(), strifeFlag, (int)allowOverride);
    GInput->SetBinding(b, Args[stidx+1], (alen > 2 ? Args[stidx+2] : VStr()), modSection, strifeFlag, allowOverride);
  }
}


//==========================================================================
//
//  COMMAND Bind
//
//==========================================================================
COMMAND(Bind) {
  if (ParsingKeyConf) {
    // in keyconf
    if (CurrKeyConfKeySection.isEmpty()) return; // alas
    Args.insert(1, "module");
    Args.insert(2, CurrKeyConfKeySection);
    bindCommon(Args, false);
  } else {
    bindCommon(Args);
  }
}


//==========================================================================
//
//  COMMAND DefaultBind
//
//==========================================================================
COMMAND(DefaultBind) {
  if (!ParsingKeyConf) return; // not in keyconf
  if (CurrKeyConfKeySection.isEmpty()) return; // alas
  Args.insert(1, "module");
  Args.insert(2, CurrKeyConfKeySection);
  bindCommon(Args, false);
}


//==========================================================================
//
//  COMMAND AddKeySection
//
//==========================================================================
COMMAND(AddKeySection) {
  // new keybinding section
  if (!ParsingKeyConf) return; // not in keyconf
  CurrKeyConfKeySection.clear();
  //if (Args.length() == 1) { CurrKeyConfKeySection = "(unknown)"; return; }
  if (Args.length() > 2) CurrKeyConfKeySection = Args[2];
  if (CurrKeyConfKeySection.isEmpty() && Args.length() > 1) CurrKeyConfKeySection = Args[1];
  if (CurrKeyConfKeySection.isEmpty()) CurrKeyConfKeySection = "(unknown)";
#ifdef CLIENT
  if (GInput) {
    GInput->AddActiveMod(CurrKeyConfKeySection);
    GCon->Logf(NAME_Debug, "mod keyconf key section '%s'", *CurrKeyConfKeySection);
  } else {
    GCon->Logf(NAME_Error, "'AddKeySection' without initialised input system");
  }
#endif
}


//==========================================================================
//
//  COMMAND AddMenuKey
//
//==========================================================================
COMMAND(AddMenuKey) {
}
