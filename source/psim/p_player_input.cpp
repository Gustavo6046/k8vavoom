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
//**  Copyright (C) 2018-2021 Ketmar Dark
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
//**  KEY BUTTONS
//**
//**  Continuous button event tracking is complicated by the fact that two
//**  different input sources (say, mouse button 1 and the control key) can
//**  both press the same button, but the button should only be released
//**  when both of the pressing key have been released.
//**
//**  When a key event issues a button command (+forward, +attack, etc),
//**  it appends its key number as a parameter to the command so it can be
//**  matched up with the release.
//**
//**  state bit 0 is the current state of the key
//**  state bit 1 is edge triggered on the up to down transition
//**  state bit 2 is edge triggered on the down to up transition
//**
//**************************************************************************
#include "../gamedefs.h"
#ifdef CLIENT
# include "../client/cl_local.h"
#endif


extern VCvarB sv_ignore_nomlook;
extern VCvarB sv_ignore_nojump;
extern VCvarB sv_ignore_nocrouch;
extern VCvarB sv_ignore_nomlook;

extern VCvarB sv_disable_run;
extern VCvarB sv_disable_crouch;
extern VCvarB sv_disable_jump;
extern VCvarB sv_disable_mlook;


#define BUTTON(name) \
static TKButton   Key ## name; \
static TCmdKeyDown  name ## Down_f("+" #name, Key ## name); \
static TCmdKeyUp  name ## Up_f("-" #name, Key ## name);


#define ACS_BT_ATTACK     0x00000001
#define ACS_BT_USE        0x00000002
#define ACS_BT_JUMP       0x00000004
#define ACS_BT_CROUCH     0x00000008
#define ACS_BT_TURN180    0x00000010
#define ACS_BT_ALTATTACK  0x00000020
#define ACS_BT_RELOAD     0x00000040
#define ACS_BT_ZOOM       0x00000080
#define ACS_BT_SPEED      0x00000100
#define ACS_BT_STRAFE     0x00000200
#define ACS_BT_MOVERIGHT  0x00000400
#define ACS_BT_MOVELEFT   0x00000800
#define ACS_BT_BACK       0x00001000
#define ACS_BT_FORWARD    0x00002000
#define ACS_BT_RIGHT      0x00004000
#define ACS_BT_LEFT       0x00008000
#define ACS_BT_LOOKUP     0x00010000
#define ACS_BT_LOOKDOWN   0x00020000
#define ACS_BT_MOVEUP     0x00040000
#define ACS_BT_MOVEDOWN   0x00080000
#define ACS_BT_SHOWSCORES 0x00100000
#define ACS_BT_USER1      0x00200000
#define ACS_BT_USER2      0x00400000
#define ACS_BT_USER3      0x00800000
#define ACS_BT_USER4      0x01000000


class TKButton {
protected:
  enum {
    BitDown = 1u<<0,
    BitJustDown = 1u<<1,
    BitJustUp = 1u<<2,
  };

protected:
  TMapNC<vint32, bool> down; // key nums holding it down
  //int down[2]; // key nums holding it down
  // current state, and impulses
  //   bit 0: "currently pressed"
  //   bit 1: just pressed
  //   bit 2: just released
  unsigned state;
  // all known buttons
  TKButton *next;

  // fuck
  friend void UnpressAllButtons ();

public:
  TKButton ();

  void KeyDown (const char *c);
  void KeyUp (const char *c);
  float KeyState ();

  inline bool IsDown () const { return !!(state&BitDown); }
  inline bool IsJustDown () const { return !!(state&BitJustDown); }
  inline bool IsJustUp () const { return !!(state&BitJustUp); }

  inline void Unpress () { state = 0; down.reset(); }

  inline void ClearEdges () { state &= BitDown; }
  inline void SetDown (bool flag) { if (flag) state |= BitDown; else state &= ~BitDown; }
  inline void SetJustDown (bool flag) { if (flag) state |= BitJustDown; else state &= ~BitJustDown; }
  inline void SetJustUp (bool flag) { if (flag) state |= BitJustUp; else state &= ~BitJustUp; }
};


class TCmdKeyDown : public VCommand {
public:
  TCmdKeyDown (const char *AName, TKButton &AKey) : VCommand(AName), Key(AKey) {}
  virtual void Run () override;

  TKButton &Key;
};


class TCmdKeyUp : public VCommand {
public:
  TCmdKeyUp (const char *AName, TKButton &AKey) : VCommand(AName), Key(AKey) {}
  virtual void Run () override;

  TKButton &Key;
};


enum {
  INPUT_OLDBUTTONS,
  INPUT_BUTTONS,
  INPUT_PITCH,
  INPUT_YAW,
  INPUT_ROLL,
  INPUT_FORWARDMOVE,
  INPUT_SIDEMOVE,
  INPUT_UPMOVE,
  MODINPUT_OLDBUTTONS,
  MODINPUT_BUTTONS,
  MODINPUT_PITCH,
  MODINPUT_YAW,
  MODINPUT_ROLL,
  MODINPUT_FORWARDMOVE,
  MODINPUT_SIDEMOVE,
  MODINPUT_UPMOVE
};


// mouse values are used once
static float mousex; // relative, scaled by sensitivity
static float mousey; // relative, scaled by sensitivity
// joystick values are repeated
static int joyxmove;
static int joyymove;

static int currImpulse;


static VCvarB always_run("always_run", false, "Always run?", CVAR_Archive);
static VCvarB artiskip("artiskip", true, "Should Shift+Enter skip an artifact?", CVAR_Archive); // whether shift-enter skips an artifact

static VCvarF cl_forwardspeed("cl_forwardspeed", "200", "Forward speed.", CVAR_Archive);
static VCvarF cl_backspeed("cl_backspeed", "200", "Backward speed.", CVAR_Archive);
static VCvarF cl_sidespeed("cl_sidespeed", "200", "Sidestepping speed.", CVAR_Archive);
static VCvarF cl_flyspeed("cl_flyspeed", "80", "Flying speed.", CVAR_Archive);

static VCvarF cl_movespeedkey("cl_movespeedkey", "2", "Running multiplier.", CVAR_Archive);

static VCvarF cl_yawspeed("cl_yawspeed", "140", "Yaw speed.", CVAR_Archive);
static VCvarF cl_pitchspeed("cl_pitchspeed", "150", "Pitch speed.", CVAR_Archive);
static VCvarF cl_pitchdriftspeed("cl_pitchdriftspeed", "270", "Pitch drifting speed.", CVAR_Archive);

static VCvarF cl_anglespeedkey("cl_anglespeedkey", "1.5", "Fast turning multiplier.", CVAR_Archive);

static VCvarB cl_deathroll_enabled("cl_deathroll_enabled", true, "Enable death roll?", CVAR_Archive);
static VCvarF cl_deathroll_amount("cl_deathroll_amount", "75", "Deathroll amount.", CVAR_Archive);
static VCvarF cl_deathroll_speed("cl_deathroll_speed", "80", "Deathroll speed.", CVAR_Archive);

VCvarF mouse_x_sensitivity("mouse_x_sensitivity", "5.5", "Horizontal mouse sensitivity.", CVAR_Archive);
VCvarF mouse_y_sensitivity("mouse_y_sensitivity", "5.5", "Vertical mouse sensitivity.", CVAR_Archive);
static VCvarB mouse_look("mouse_look", true, "Allow mouselook?", CVAR_Archive);
static VCvarB mouse_look_horisontal("mouse_look_horisontal", true, "Allow horisontal mouselook?", CVAR_Archive);
static VCvarB mouse_look_vertical("mouse_look_vertical", true, "Allow vertical mouselook?", CVAR_Archive);
static VCvarB invert_mouse("invert_mouse", false, "Invert mouse?", CVAR_Archive);
static VCvarB lookstrafe("lookstrafe", false, "Allow lookstrafe?", CVAR_Archive);
static VCvarB lookspring_mouse("lookspring_mouse", false, "Allow lookspring for mouselook key?", CVAR_Archive);
static VCvarB lookspring_keyboard("lookspring_keyboard", false, "Allow lookspring for keyboard view keys?", CVAR_Archive);

VCvarF m_yaw("m_yaw", "0.022", "Mouse yaw speed.", CVAR_Archive);
VCvarF m_pitch("m_pitch", "0.022", "Mouse pitch speed.", CVAR_Archive);
static VCvarF m_forward("m_forward", "1", "Mouse forward speed.", CVAR_Archive);
static VCvarF m_side("m_side", "0.8", "Mouse sidestepping speed.", CVAR_Archive);

static VCvarF joy_yaw("joy_yaw", "140", "Joystick yaw speed.", CVAR_Archive);


static TKButton *knownButtons = nullptr;

BUTTON(Forward)
BUTTON(Backward)
BUTTON(Left)
BUTTON(Right)
BUTTON(LookUp)
BUTTON(LookDown)
BUTTON(LookCenter)
BUTTON(MoveLeft)
BUTTON(MoveRight)
BUTTON(FlyUp)
BUTTON(FlyDown)
BUTTON(FlyCenter)
BUTTON(Attack)
BUTTON(Use)
BUTTON(Jump)
BUTTON(Crouch)
BUTTON(AltAttack)
BUTTON(Button5)
BUTTON(Button6)
BUTTON(Button7)
BUTTON(Button8)
BUTTON(Speed)
BUTTON(Strafe)
BUTTON(MouseLook)
BUTTON(Reload)
BUTTON(Flashlight)
BUTTON(SuperBullet)
BUTTON(Zoom)


//==========================================================================
//
//  UnpressAllButtons
//
//  this is called on window activation/deactivation
//
//==========================================================================
void UnpressAllButtons () {
  for (TKButton *bt = knownButtons; bt; bt = bt->next) bt->Unpress();
}


//==========================================================================
//
//  TKButton::TKButton
//
//==========================================================================
TKButton::TKButton () {
  state = 0;
  // register in list
  next = knownButtons;
  knownButtons = this;
}


//==========================================================================
//
//  TKButton::KeyDown
//
//==========================================================================
void TKButton::KeyDown (const char *c) {
  vint32 k = -1;

  if (c && c[0]) k = VStr::atoi(c); // otherwise, typed manually at the console for continuous down

  /*
  if (k == down[0] || k == down[1]) return; // repeating key

       if (!down[0]) down[0] = k;
  else if (!down[1]) down[1] = k;
  else { GCon->Log(NAME_Dev, "Three keys down for a button!"); return; }
  */
  if (k > 0) down.put(k, true);

  if (IsDown()) return; // still down

  SetDown(true);
  SetJustDown(true);
}


//==========================================================================
//
//  TKButton::KeyUp
//
//==========================================================================
void TKButton::KeyUp (const char *c) {
  if (!c || !c[0]) {
    // typed manually at the console, assume for unsticking, so clear all
    //down[0] = down[1] = 0;
    Unpress();
    SetJustUp(true); // impulse up
    return;
  }

  int k = VStr::atoi(c);

  /*
       if (down[0] == k) down[0] = 0;
  else if (down[1] == k) down[1] = 0;
  else return; // key up without coresponding down (menu pass through)

  if (down[0] || down[1]) return; // some other key is still holding it down
  */

  down.del(k);
  if (down.length() != 0) return; // some other key is still holding it down

  if (!IsDown()) return; // still up (this should not happen)

  SetDown(false);
  SetJustUp(true);
}


//==========================================================================
//
//  TKButton::KeyState
//
//  Returns 0.25 if a key was pressed and released during the frame,
//  0.5 if it was pressed and held
//  0 if held then released, and
//  1.0 if held for the entire time
//
//==========================================================================
float TKButton::KeyState () {
  /*static*/ const float newVal[8] = {
    0.0f, // up the entire frame
    1.0f, // held the entire frame
    0.0f, // Sys_Error();
    0.5f, // pressed and held this frame
    0.0f, // released this frame
    0.0f, // Sys_Error();
    0.25f,// pressed and released this frame
    0.75f // released and re-pressed this frame
  };

  const float val = newVal[state&7];
  ClearEdges();
  return val;
}


//==========================================================================
//
//  TCmdKeyDown::Run
//
//==========================================================================
void TCmdKeyDown::Run () {
  Key.KeyDown(Args.Num() > 1 ? *Args[1] : "");
}


//==========================================================================
//
//  TCmdKeyUp::Run
//
//==========================================================================
void TCmdKeyUp::Run () {
  Key.KeyUp(Args.Num() > 1 ? *Args[1] : "");
}


//==========================================================================
//
//  COMMAND Impulse
//
//==========================================================================
COMMAND(Impulse) {
  if (Args.Num() < 2) return;
  currImpulse = VStr::atoi(*Args[1]);
  //GCon->Logf(NAME_Debug, "IMPULSE COMMAND: %d (%s)", currImpulse, *Args[1]);
}


//==========================================================================
//
//  COMMAND ToggleAlwaysRun
//
//==========================================================================
COMMAND(ToggleAlwaysRun) {
#ifdef CLIENT
  if (!cl || !GClGame || !GGameInfo || GClGame->InIntermission() || GGameInfo->NetMode <= NM_TitleMap) {
    GCon->Log(NAME_Warning, "Cannot toggle autorun while not in game!");
    return;
  }
  if (GGameInfo->IsPaused()) {
    if (cl) cl->Printf("Cannot toggle autorun while the game is paused!"); else GCon->Log(NAME_Warning, "Cannot toggle autorun while the game is paused!");
    return;
  }
#endif
  always_run = !always_run;
#ifdef CLIENT
  if (cl) {
    cl->Printf(always_run ? "Always run on" : "Always run off");
  } else
#endif
  {
    GCon->Log(always_run ? "Always run on" : "Always run off");
  }
}


//==========================================================================
//
//  COMMAND Use
//
//==========================================================================
COMMAND(Use) {
  if (Args.Num() < 1) return;
#ifdef CLIENT
  if (!cl || !GClGame || !GGameInfo || GClGame->InIntermission() || GGameInfo->NetMode <= NM_TitleMap) {
    if (cl) cl->Printf("Cannot use artifact while not in game!"); else GCon->Log(NAME_Warning, "Cannot use artifact while not in game!");
    return;
  }
  //if (GGameInfo->IsPaused()) return;
  //GCon->Logf(NAME_Debug, "USE: <%s>", *Args[1]);
  cl->eventUseInventory(*Args[1]);
#endif
}


//==========================================================================
//
//  VBasePlayer::StartPitchDrift
//
//==========================================================================
void VBasePlayer::StartPitchDrift () {
  PlayerFlags |= PF_Centering;
}


//==========================================================================
//
//  VBasePlayer::StopPitchDrift
//
//==========================================================================
void VBasePlayer::StopPitchDrift () {
  PlayerFlags &= ~PF_Centering;
}


//==========================================================================
//
//  VBasePlayer::IsRunEnabled
//
//==========================================================================
bool VBasePlayer::IsRunEnabled () const noexcept {
  if (GGameInfo->NetMode == NM_Client) return !(GGameInfo->clientFlags&VGameInfo::CLF_RUN_DISABLED);
  return !sv_disable_run.asBool();
}


//==========================================================================
//
//  VBasePlayer::IsMLookEnabled
//
//==========================================================================
bool VBasePlayer::IsMLookEnabled () const noexcept {
  if (GGameInfo->NetMode == NM_Client) return !(GGameInfo->clientFlags&VGameInfo::CLF_MLOOK_DISABLED);
  if (sv_disable_mlook.asBool()) return false;
  if (!Level) return true; // no map
  if (sv_ignore_nomlook.asBool()) return true; // ignore mapinfo option
  // dedicated server ignores map flags in DM mode
  if (GGameInfo->NetMode == NM_DedicatedServer && svs.deathmatch) return true;
  // don't forget to send this flags to client!
  return !(Level->LevelInfoFlags&VLevelInfo::LIF_NoFreelook);
}


//==========================================================================
//
//  VBasePlayer::IsCrouchEnabled
//
//==========================================================================
bool VBasePlayer::IsCrouchEnabled () const noexcept {
  if (GGameInfo->NetMode == NM_Client) return !(GGameInfo->clientFlags&VGameInfo::CLF_CROUCH_DISABLED);
  if (sv_disable_crouch.asBool()) return false;
  if (!Level) return true; // no map
  if (sv_ignore_nocrouch.asBool()) return true; // ignore mapinfo option
  // dedicated server ignores map flags in DM mode
  if (GGameInfo->NetMode == NM_DedicatedServer && svs.deathmatch) return true;
  // don't forget to send this flags to client!
  return !(Level->LevelInfoFlags2&VLevelInfo::LIF2_NoCrouch);
}


//==========================================================================
//
//  VBasePlayer::IsJumpEnabled
//
//==========================================================================
bool VBasePlayer::IsJumpEnabled () const noexcept {
  if (GGameInfo->NetMode == NM_Client) return !(GGameInfo->clientFlags&VGameInfo::CLF_JUMP_DISABLED);
  if (sv_disable_jump.asBool()) return false;
  if (!Level) return true; // no map
  if (sv_ignore_nojump.asBool()) return true; // ignore mapinfo option
  // dedicated server ignores map flags in DM mode
  if (GGameInfo->NetMode == NM_DedicatedServer && svs.deathmatch) return true;
  // don't forget to send this flags to client!
  return !(Level->LevelInfoFlags&VLevelInfo::LIF_NoJump);
}


//==========================================================================
//
//  VBasePlayer::AdjustAngles
//
//==========================================================================
void VBasePlayer::AdjustAngles () {
  const bool isRunning = (IsRunEnabled() ? KeySpeed.IsDown() : false);
  float speed = host_frametime*(isRunning ? cl_anglespeedkey : 1.0f);

  bool mlookJustUp = KeyMouseLook.IsJustUp();
  bool mlookIsDown = KeyMouseLook.IsDown();
  bool klookAnyDown = KeyLookUp.IsDown() || KeyLookDown.IsDown();
  bool klookJustUp = !klookAnyDown && (KeyLookUp.IsJustUp() || KeyLookDown.IsJustUp());

  if (lookspring_mouse) {
    if (!lookspring_keyboard) klookAnyDown = false;
    if (mlookJustUp && !klookAnyDown) StartPitchDrift();
  }
  if (lookspring_keyboard) {
    if (!lookspring_mouse) mlookIsDown = false;
    if (klookJustUp && !mlookIsDown) StartPitchDrift();
  }
  KeyMouseLook.ClearEdges();

  // yaw
  if (!KeyStrafe.IsDown()) {
    ViewAngles.yaw -= KeyRight.KeyState()*cl_yawspeed*speed;
    ViewAngles.yaw += KeyLeft.KeyState()*cl_yawspeed*speed;
    if (joyxmove > 0) ViewAngles.yaw -= joy_yaw*speed;
    if (joyxmove < 0) ViewAngles.yaw += joy_yaw*speed;
  }
  if (mouse_look_horisontal && !KeyStrafe.IsDown() && (!lookstrafe || (!mouse_look && !KeyMouseLook.IsDown()))) ViewAngles.yaw -= mousex*m_yaw;
  ViewAngles.yaw = AngleMod(ViewAngles.yaw);

  // pitch
  float up = KeyLookUp.KeyState();
  float down = KeyLookDown.KeyState();
  ViewAngles.pitch -= cl_pitchspeed*up*speed;
  ViewAngles.pitch += cl_pitchspeed*down*speed;
  if (up || down || KeyMouseLook.IsDown()) StopPitchDrift();
  if (mouse_look_vertical && (mouse_look || KeyMouseLook.IsDown()) && !KeyStrafe.IsDown()) ViewAngles.pitch -= mousey*m_pitch;

  // reset pitch if mouse look is disabled
  if (!IsMLookEnabled()) ViewAngles.pitch = 0;

  // center look
  if (KeyLookCenter.IsDown() || KeyFlyCenter.IsDown()) StartPitchDrift();
  if (PlayerFlags&PF_Centering) {
    float adelta = cl_pitchdriftspeed*host_frametime;
    if (fabsf(ViewAngles.pitch) < adelta) {
      ViewAngles.pitch = 0;
      PlayerFlags &= ~PF_Centering;
    } else {
           if (ViewAngles.pitch > 0.0f) ViewAngles.pitch -= adelta;
      else if (ViewAngles.pitch < 0.0f) ViewAngles.pitch += adelta;
    }
  }

  // roll
  if (Health <= 0) {
    if (cl_deathroll_enabled && cl_deathroll_amount.asFloat() > 0) {
      const float drspeed = cl_deathroll_speed.asFloat();
      const float dramount = min2(180.0f, cl_deathroll_amount.asFloat());
      // use random roll direction
      const float dir =
        (ViewAngles.roll == 0 ? (RandomFull() < 0.5f ? -1.0f : 1.0f) :
         ViewAngles.roll < 0 ? -1.0f : 1.0f);
      if (drspeed <= 0) {
        ViewAngles.roll = dramount*dir;
      } else {
        ViewAngles.roll += drspeed*dir*host_frametime;
      }
      ViewAngles.roll = clampval(ViewAngles.roll, -dramount, dramount);
    } else {
      ViewAngles.roll = 0.0f;
    }
  } else {
    ViewAngles.roll = 0.0f;
  }

  // check angles
  if (ViewAngles.pitch > 80.0f) ViewAngles.pitch = 80.0f;
  if (ViewAngles.pitch < -80.0f) ViewAngles.pitch = -80.0f;

  if (ViewAngles.roll > 80.0f) ViewAngles.roll = 80.0f;
  if (ViewAngles.roll < -80.0f) ViewAngles.roll = -80.0f;
}


//==========================================================================
//
//  VBasePlayer::HandleInput
//
//  Creates movement commands from all of the available inputs.
//
//==========================================================================
void VBasePlayer::HandleInput () {
  const bool runEnabled = IsRunEnabled();

  float forward = 0;
  float side = 0;
  float flyheight = 0;

  AdjustAngles();

  // let movement keys cancel each other out
  if (KeyStrafe.IsDown()) {
    side += KeyRight.KeyState()*cl_sidespeed;
    side -= KeyLeft.KeyState()*cl_sidespeed;
    if (joyxmove > 0) side += cl_sidespeed;
    if (joyxmove < 0) side -= cl_sidespeed;
  }

  forward += KeyForward.KeyState()*cl_forwardspeed;
  forward -= KeyBackward.KeyState()*cl_backspeed;

  side += KeyMoveRight.KeyState()*cl_sidespeed;
  side -= KeyMoveLeft.KeyState()*cl_sidespeed;

  if (joyymove < 0) forward += cl_forwardspeed;
  if (joyymove > 0) forward -= cl_backspeed;

  // fly up/down/drop keys
  flyheight += KeyFlyUp.KeyState()*cl_flyspeed; // note that the actual flyheight will be twice this
  flyheight -= KeyFlyDown.KeyState()*cl_flyspeed;

  if ((!mouse_look && !KeyMouseLook.IsDown()) || KeyStrafe.IsDown()) {
    forward += m_forward*mousey;
  }

  if (KeyStrafe.IsDown() || (lookstrafe && (mouse_look || KeyMouseLook.IsDown()))) {
    side += m_side*mousex;
  }

  forward = midval(forward, -cl_backspeed, cl_forwardspeed.asFloat());
  side = midval(side, -cl_sidespeed, cl_sidespeed.asFloat());

  if (runEnabled && (always_run || KeySpeed.IsDown())) {
    forward *= cl_movespeedkey;
    side *= cl_movespeedkey;
    flyheight *= cl_movespeedkey;
  }

  flyheight = midval(flyheight, -127.0f, 127.0f);
  if (KeyFlyCenter.KeyState()) flyheight = TOCENTRE;

  // buttons
  Buttons = 0;

  if (KeyAttack.KeyState()) Buttons |= BT_ATTACK;
  if (KeyUse.KeyState()) Buttons |= BT_USE;
  if (KeyJump.KeyState()) Buttons |= BT_JUMP;
  if (KeyCrouch.KeyState()) Buttons |= BT_CROUCH;
  if (KeyAltAttack.KeyState()) Buttons |= BT_ALT_ATTACK;
  if (KeyButton5.KeyState()) Buttons |= BT_BUTTON_5;
  if (KeyButton6.KeyState()) Buttons |= BT_BUTTON_6;
  if (KeyButton7.KeyState()) Buttons |= BT_BUTTON_7;
  if (KeyButton8.KeyState()) Buttons |= BT_BUTTON_8;

  if (KeyForward.KeyState()) Buttons |= BT_FORWARD;
  if (KeyBackward.KeyState()) Buttons |= BT_BACKWARD;
  if (KeyLeft.KeyState()) Buttons |= BT_LEFT;
  if (KeyRight.KeyState()) Buttons |= BT_RIGHT;
  if (KeyMoveLeft.KeyState()) Buttons |= BT_MOVELEFT;
  if (KeyMoveRight.KeyState()) Buttons |= BT_MOVERIGHT;
  if (KeyStrafe.KeyState()) Buttons |= BT_STRAFE;
  if (KeySpeed.KeyState()) Buttons |= BT_SPEED;
  if (KeyReload.KeyState()) Buttons |= BT_RELOAD;
  if (KeyFlashlight.KeyState()) Buttons |= BT_FLASHLIGHT;

  if (KeySuperBullet.KeyState()) Buttons |= BT_SUPERBULLET;
  if (KeyZoom.KeyState()) Buttons |= BT_ZOOM;
  //GCon->Logf("VBasePlayer::HandleInput(%p): Buttons=0x%08x", this, Buttons);

  AcsCurrButtonsPressed |= Buttons;
  AcsCurrButtons = Buttons; // scripts can change `Buttons`, but not this
  //AcsButtons = Buttons; // this logic is handled by `SV_RunClients()`
  //GCon->Logf("VBasePlayer::HandleInput(%p): %d; Buttons=0x%08x; OldButtons=0x%08x", this, (KeyJump.KeyState() ? 1 : 0), Buttons, OldButtons);

  // impulse
  if (currImpulse) {
    eventServerImpulse(currImpulse);
    currImpulse = 0;
  }

  ClientForwardMove = forward;
  ClientSideMove = side;
  FlyMove = flyheight;

  AcsPrevMouseX += mousex*m_yaw;
  AcsPrevMouseY += mousey*m_pitch;

  mousex = 0;
  mousey = 0;
}


//==========================================================================
//
//  VBasePlayer::Responder
//
//  Get info needed to make movement commands for the players.
//
//==========================================================================
bool VBasePlayer::Responder (event_t *ev) {
  switch (ev->type) {
    case ev_mouse:
      mousex = ev->dx*mouse_x_sensitivity;
      mousey = ev->dy*mouse_y_sensitivity;
      if (invert_mouse) mousey = -mousey;
      return true; // eat events
    case ev_joystick:
      joyxmove = ev->dx;
      joyymove = ev->dy;
      return true; // eat events
    default:
      break;
  }
  return false;
}


//==========================================================================
//
//  VBasePlayer::ClearInput
//
//==========================================================================
void VBasePlayer::ClearInput () {
  // clear cmd building stuff
  joyxmove = joyymove = 0;
  mousex = mousey = 0;
  currImpulse = 0;
}


//==========================================================================
//
//  VBasePlayer::AcsGetInput
//
//==========================================================================
int VBasePlayer::AcsGetInput (int InputType) {
  int Btn;
  int Ret = 0;
  //float angle0 = 0, angle1 = 0;
  //static int n = 0;
  switch (InputType) {
    case INPUT_OLDBUTTONS: case MODINPUT_OLDBUTTONS:
    case INPUT_BUTTONS: case MODINPUT_BUTTONS:
      if (InputType == INPUT_OLDBUTTONS || InputType == MODINPUT_OLDBUTTONS) {
        Btn = OldButtons;
        //k8: hack for DooM:ONE
        //Btn &= ~BT_USE;
      } else {
        Btn = AcsButtons;
      }
      // convert buttons to what ACS expects
      // /*if (Btn)*/ GCon->Logf("VBasePlayer::AcsGetInput(%p): Buttons: %08x; curr=%08x; old=%08x; Buttons=%08x; OldButtons=%08x", this, (unsigned)Btn, AcsButtons, OldButtons, Buttons, OldButtons);
      if (Btn&BT_ATTACK) Ret |= ACS_BT_ATTACK;
      if (Btn&BT_USE) Ret |= ACS_BT_USE;
      if (Btn&BT_JUMP) Ret |= ACS_BT_JUMP;
      if (Btn&BT_CROUCH) Ret |= ACS_BT_CROUCH;
      if (Btn&BT_ALT_ATTACK) Ret |= ACS_BT_ALTATTACK;
      if (Btn&BT_FORWARD) Ret |= ACS_BT_FORWARD;
      if (Btn&BT_BACKWARD) Ret |= ACS_BT_BACK;
      if (Btn&BT_LEFT) Ret |= ACS_BT_LEFT;
      if (Btn&BT_RIGHT) Ret |= ACS_BT_RIGHT;
      if (Btn&BT_MOVELEFT) Ret |= ACS_BT_MOVELEFT;
      if (Btn&BT_MOVERIGHT) Ret |= ACS_BT_MOVERIGHT;
      if (Btn&BT_STRAFE) Ret |= ACS_BT_STRAFE;
      if (Btn&BT_SPEED) Ret |= ACS_BT_SPEED;
      if (Btn&BT_RELOAD) Ret |= ACS_BT_RELOAD;
      if (Btn&BT_ZOOM) Ret |= ACS_BT_ZOOM;
      return Ret;

    case INPUT_YAW: case MODINPUT_YAW:
      return (int)(-AcsMouseX*65536.0f/360.0f);

    case INPUT_PITCH: case MODINPUT_PITCH:
      return (int)(AcsMouseY*65536.0f/360.0f);

    case INPUT_ROLL: case MODINPUT_ROLL:
      // player cannot do it yet
      return 0;

    case INPUT_FORWARDMOVE: return (int)(ClientForwardMove*0x32/400);
    case MODINPUT_FORWARDMOVE: return (int)(ForwardMove*0x32/400);

    case INPUT_SIDEMOVE: return (int)(ClientSideMove*0x32/400);
    case MODINPUT_SIDEMOVE: return (int)(SideMove*0x32/400);

    case INPUT_UPMOVE:
    case MODINPUT_UPMOVE:
      return (int)(FlyMove*3*256/80);
  }
  return 0;
}
