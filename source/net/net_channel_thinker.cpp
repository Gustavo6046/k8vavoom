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
#include "gamedefs.h"
#include "network.h"
#include "net_message.h"

static VCvarI net_dbg_dump_thinker_channels("net_dbg_dump_thinker_channels", "0", "Dump thinker channels creation/closing (bit 0)?");
VCvarB net_dbg_dump_thinker_detach("net_dbg_dump_thinker_detach", false, "Dump thinker detaches?");


//==========================================================================
//
//  VThinkerChannel::VThinkerChannel
//
//==========================================================================
VThinkerChannel::VThinkerChannel (VNetConnection *AConnection, vint32 AIndex, vuint8 AOpenedLocally)
  : VChannel(AConnection, CHANNEL_Thinker, AIndex, AOpenedLocally)
  , Thinker(nullptr)
  , OldData(nullptr)
  , NewObj(false)
  , FieldCondValues(nullptr)
  , LastUpdateFrame(0)
{
  // it is ok to close it... or it isn't?
  //bAllowPrematureClose = true;
}


//==========================================================================
//
//  VThinkerChannel::~VThinkerChannel
//
//==========================================================================
VThinkerChannel::~VThinkerChannel () {
  if (Connection) {
    // mark channel as closing to prevent sending a message
    Closing = true;
    // if this is a client version of entity, destroy it
    RemoveThinkerFromGame();
  }
}


//==========================================================================
//
//  VThinkerChannel::GetName
//
//==========================================================================
VStr VThinkerChannel::GetName () const noexcept {
  if (Thinker) {
    return VStr(va("thchan #%d <%s:%u>", Index, Thinker->GetClass()->GetName(), Thinker->GetUniqueId()));
  } else {
    return VStr(va("thchan #%d <none>", Index));
  }
}


//==========================================================================
//
//  VThinkerChannel::IsQueueFull
//
//  limit thinkers by the number of outgoing packets instead
//
//==========================================================================
int VThinkerChannel::IsQueueFull () const noexcept {
  return
    OutListCount >= MAX_RELIABLE_BUFFER+8 ? -1 : // oversaturated
    OutListCount >= MAX_RELIABLE_BUFFER ? 1 : // full
    0; // ok
}


//==========================================================================
//
//  VThinkerChannel::RemoveThinkerFromGame
//
//==========================================================================
void VThinkerChannel::RemoveThinkerFromGame () {
  // if this is a client version of entity, destroy it
  if (!Thinker) return;
  bool doRemove = !OpenedLocally;
  if (doRemove) {
    // for clients: don't remove "authority" thinkers, they became autonomous
    if (Connection->Context->IsClient()) {
      if (Thinker->Role == ROLE_Authority) {
        doRemove = false;
        if (net_dbg_dump_thinker_detach) GCon->Logf(NAME_Debug, "VThinkerChannel::RemoveThinkerFromGame: skipping autonomous thinker '%s':%u", Thinker->GetClass()->GetName(), Thinker->GetUniqueId());
      }
    }
  }
  if (doRemove) {
    if (net_dbg_dump_thinker_channels.asInt()&1) GCon->Logf(NAME_Debug, "VThinkerChannel #%d: removing thinker '%s':%u from game...", Index, Thinker->GetClass()->GetName(), Thinker->GetUniqueId());
    // avoid loops
    VThinker *th = Thinker;
    Thinker = nullptr;
    th->DestroyThinker();
    //k8: oooh; hacks upon hacks, and more hacks... one of those methods can call `SetThinker(nullptr)` for us
    if (th && th->XLevel) {
      th->XLevel->RemoveThinker(th);
    }
    if (th) {
      th->ConditionalDestroy();
    }
    // restore it
    Thinker = th;
  }
  SetThinker(nullptr);
}


//==========================================================================
//
//  VThinkerChannel::SetClosing
//
//==========================================================================
void VThinkerChannel::SetClosing () {
  // we have to do this first!
  VChannel::SetClosing();
  RemoveThinkerFromGame();
}


//==========================================================================
//
//  VThinkerChannel::SetThinker
//
//==========================================================================
void VThinkerChannel::SetThinker (VThinker *AThinker) {
  if (Thinker && !AThinker && net_dbg_dump_thinker_channels.asInt()&1) GCon->Logf(NAME_Debug, "VThinkerChannel #%d: clearing thinker '%s':%u", Index, Thinker->GetClass()->GetName(), Thinker->GetUniqueId());
  if (!Thinker && AThinker && net_dbg_dump_thinker_channels.asInt()&1) GCon->Logf(NAME_Debug, "VThinkerChannel #%d: setting thinker '%s':%u", Index, AThinker->GetClass()->GetName(), AThinker->GetUniqueId());
  if (Thinker && AThinker && net_dbg_dump_thinker_channels.asInt()&1) GCon->Logf(NAME_Debug, "VThinkerChannel #%d: replacing thinker '%s':%u with '%s':%u", Index, Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), AThinker->GetClass()->GetName(), AThinker->GetUniqueId());

  if (Thinker) {
    Connection->ThinkerChannels.Remove(Thinker);
    if (OldData) {
      for (VField *F = Thinker->GetClass()->NetFields; F; F = F->NextNetField) {
        VField::DestructField(OldData+F->Ofs, F->Type);
      }
      delete[] OldData;
      OldData = nullptr;
    }
    if (FieldCondValues) {
      delete[] FieldCondValues;
      FieldCondValues = nullptr;
    }
  }

  Thinker = AThinker;

  if (Thinker) {
    VClass *ThinkerClass = Thinker->GetClass();
    if (OpenedLocally) {
      VThinker *Def = (VThinker *)ThinkerClass->Defaults;
      OldData = new vuint8[ThinkerClass->ClassSize];
      memset(OldData, 0, ThinkerClass->ClassSize);
      for (VField *F = ThinkerClass->NetFields; F; F = F->NextNetField) {
        VField::CopyFieldValue((vuint8 *)Def+F->Ofs, OldData+F->Ofs, F->Type);
      }
      FieldCondValues = new vuint8[ThinkerClass->NumNetFields];
    }
    NewObj = true;
    Connection->ThinkerChannels.Set(Thinker, this);
  }
}


//==========================================================================
//
//  VThinkerChannel::EvalCondValues
//
//==========================================================================
void VThinkerChannel::EvalCondValues (VObject *Obj, VClass *Class, vuint8 *Values) {
  if (Class->GetSuperClass()) EvalCondValues(Obj, Class->GetSuperClass(), Values);
  for (int i = 0; i < Class->RepInfos.Num(); ++i) {
    bool Val = VObject::ExecuteFunctionNoArgs(Obj, Class->RepInfos[i].Cond, true).getBool(); // allow VMT lookups
    for (int j = 0; j < Class->RepInfos[i].RepFields.Num(); ++j) {
      if (Class->RepInfos[i].RepFields[j].Member->MemberType != MEMBER_Field) continue;
      Values[((VField *)Class->RepInfos[i].RepFields[j].Member)->NetIndex] = Val;
    }
  }
}


//==========================================================================
//
//  VThinkerChannel::Update
//
//==========================================================================
void VThinkerChannel::Update () {
  if (Closing || !Thinker) return;

  // saturation check already done
  // also, if this is client, and the thinker is authority, it is detached, don't send anything
  if (Connection->IsClient() && Thinker->Role == ROLE_Authority) return;

  //GCon->Logf(NAME_DevNet, "%s:%u: updating", Thinker->GetClass()->GetName(), Thinker->GetUniqueId());

  VEntity *Ent = Cast<VEntity>(Thinker);

  // set up thinker flags that can be used by field condition
  if (NewObj) Thinker->ThinkerFlags |= VThinker::TF_NetInitial;
  if (Ent != nullptr && Ent->GetTopOwner() == Connection->Owner->MO) Thinker->ThinkerFlags |= VThinker::TF_NetOwner;

  // we need to set `EFEX_NetDetach` flag for replication condition checking if we're going to detach it
  const bool isServer = Connection->Context->IsServer();
  const bool detachEntity = (isServer && !Connection->AutoAck && Ent && (Ent->FlagsEx&VEntity::EFEX_NoTickGrav));

  // temporarily set `bNetDetach`
  if (detachEntity) Ent->FlagsEx |= VEntity::EFEX_NetDetach;

  // it is important to call this *BEFORE* changing roles!
  EvalCondValues(Thinker, Thinker->GetClass(), FieldCondValues);

  // switch roles if we're going to detach this entity
  vuint8 oldRole = 0, oldRemoteRole = 0;
  if (detachEntity) {
    // this is Role on the client
    oldRole = Thinker->Role;
    oldRemoteRole = Thinker->RemoteRole;
    // set role on the client (completely detached)
    Thinker->RemoteRole = ROLE_Authority;
    // set role on the server
    Thinker->Role = ROLE_DumbProxy;
  }

  vuint8 *Data = (vuint8 *)Thinker;
  VObject *NullObj = nullptr;

  //FIXME: use bitstream and split it to the messages here
  VMessageOut Msg(this);
  Msg.bOpen = NewObj;

  if (NewObj) {
    VClass *TmpClass = Thinker->GetClass();
    Connection->ObjMap->SerialiseClass(Msg, TmpClass);
    NewObj = false;
  }

  TAVec SavedAngles;
  if (Ent) {
    SavedAngles = Ent->Angles;
    if (Ent->EntityFlags&VEntity::EF_IsPlayer) {
      // clear look angles, because they must not affect model orientation
      Ent->Angles.pitch = 0;
      Ent->Angles.roll = 0;
    }
  } else {
    // shut up compiler warnings
    SavedAngles.yaw = 0;
    SavedAngles.pitch = 0;
    SavedAngles.roll = 0;
  }

  for (VField *F = Thinker->GetClass()->NetFields; F; F = F->NextNetField) {
    if (!FieldCondValues[F->NetIndex]) continue;

    // set up pointer to the value and do swapping for the role fields
    bool forceSend = false;
    vuint8 *FieldValue = Data+F->Ofs;
         if (F == Connection->Context->RoleField) { forceSend = detachEntity; FieldValue = Data+Connection->Context->RemoteRoleField->Ofs; }
    else if (F == Connection->Context->RemoteRoleField) { forceSend = detachEntity; FieldValue = Data+Connection->Context->RoleField->Ofs; }

    if (!forceSend && VField::IdenticalValue(FieldValue, OldData+F->Ofs, F->Type)) {
      //GCon->Logf(NAME_DevNet, "%s:%u: skipped field #%d (%s : %s)", Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), F->NetIndex, F->GetName(), *F->Type.GetName());
      continue;
    }

    if (F->Type.Type == TYPE_Array) {
      VFieldType IntType = F->Type;
      IntType.Type = F->Type.ArrayInnerType;
      int InnerSize = IntType.GetSize();
      for (int i = 0; i < F->Type.GetArrayDim(); ++i) {
        vuint8 *Val = FieldValue+i*InnerSize;
        vuint8 *OldVal = OldData+F->Ofs+i*InnerSize;
        if (VField::IdenticalValue(Val, OldVal, IntType)) {
          //GCon->Logf(NAME_DevNet, "%s:%u: skipped array field #%d [%d] (%s : %s)", Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), F->NetIndex, i, F->GetName(), *IntType.GetName());
          continue;
        }

        // if it's an object reference that cannot be serialised, send it as nullptr reference
        if (IntType.Type == TYPE_Reference && !Connection->ObjMap->CanSerialiseObject(*(VObject **)Val)) {
          if (!*(VObject **)OldVal) continue; // already sent as nullptr
          Val = (vuint8 *)&NullObj;
        }

        Msg.WriteInt(F->NetIndex/*, Thinker->GetClass()->NumNetFields*/);
        Msg.WriteInt(i/*, F->Type.GetArrayDim()*/);
        if (VField::NetSerialiseValue(Msg, Connection->ObjMap, Val, IntType)) {
          //GCon->Logf(NAME_DevNet, "%s:%u: sent array field #%d [%d] (%s : %s)", Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), F->NetIndex, i, F->GetName(), *IntType.GetName());
          VField::CopyFieldValue(Val, OldVal, IntType);
        }
      }
    } else {
      // if it's an object reference that cannot be serialised, send it as nullptr reference
      if (F->Type.Type == TYPE_Reference && !Connection->ObjMap->CanSerialiseObject(*(VObject **)FieldValue)) {
        if (!*(VObject **)(OldData+F->Ofs)) continue; // already sent as nullptr
        FieldValue = (vuint8 *)&NullObj;
      }

      Msg.WriteInt(F->NetIndex/*, Thinker->GetClass()->NumNetFields*/);
      if (VField::NetSerialiseValue(Msg, Connection->ObjMap, FieldValue, F->Type)) {
        //GCon->Logf(NAME_DevNet, "%s:%u: sent field #%d (%s : %s)", Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), F->NetIndex, F->GetName(), *F->Type.GetName());
        VField::CopyFieldValue(FieldValue, OldData+F->Ofs, F->Type);
      }
    }
  }

  if (Ent && (Ent->EntityFlags&VEntity::EF_IsPlayer)) Ent->Angles = SavedAngles;
  LastUpdateFrame = Connection->UpdateFrameCounter;

  // clear temporary networking flags
  Thinker->ThinkerFlags &= ~(VThinker::TF_NetInitial|VThinker::TF_NetOwner);

  // if this is initial send, we have to flush the message, even if it is empty
  if (Msg.bOpen || Msg.GetNumBits() ||
      (Thinker->ThinkerFlags&VThinker::TF_AlwaysRelevant) ||
      Thinker == Connection->Owner->MO ||
      (Ent && Ent->IsPlayer()) ||
      (Ent && Ent->GetTopOwner() == Connection->Owner->MO))
  {
    SendMessage(&Msg);
  }

  // if this object becomes "dumb proxy", mark it as detached, and close the channel
  if (detachEntity) {
    if (net_dbg_dump_thinker_detach) GCon->Logf(NAME_DevNet, "%s:%u: became notick, closing channel%s", Thinker->GetClass()->GetName(), Thinker->GetUniqueId(), (OpenedLocally ? " (opened locally)" : ""));
    // remember that we already did this thinker
    Connection->DetachedThinkers.put(Thinker, true);
    // reset `bNetDetach`
    if (Ent) Ent->FlagsEx &= ~VEntity::EFEX_NetDetach;
    // restore roles
    Thinker->Role = oldRole;
    Thinker->RemoteRole = oldRemoteRole;
    Close();
  }
}


//==========================================================================
//
//  VThinkerChannel::ParseMessage
//
//==========================================================================
void VThinkerChannel::ParseMessage (VMessageIn &Msg) {
  if (Closing) return;

  if (Connection->IsServer()) {
    // if this thinker is detached, don't process any messages
    if (Thinker && Connection->DetachedThinkers.has(Thinker)) return;
  }

  if (Msg.bOpen) {
    VClass *C = nullptr;
    Connection->ObjMap->SerialiseClass(Msg, C);

    if (!C) {
      //Sys_Error("%s: cannot spawn `none` thinker", *GetDebugName());
      GCon->Logf(NAME_Debug, "%s: tried to spawn thinker without name (or with invalid name)", *GetDebugName());
      Connection->State = NETCON_Closed;
      return;
    }

    // in no case client can spawn anything on the server
    if (Connection->IsServer()) {
      GCon->Logf(NAME_Debug, "%s: client tried to spawn thinker `%s` on the server, dropping client", *GetDebugName(), C->GetName());
      Connection->State = NETCON_Closed;
      return;
    }

    VThinker *Th = Connection->Context->GetLevel()->SpawnThinker(C, TVec(0, 0, 0), TAVec(0, 0, 0), nullptr, false); // no replacements
    #ifdef CLIENT
    //GCon->Logf(NAME_DevNet, "%s spawned thinker with class `%s`(%u)", *GetDebugName(), Th->GetClass()->GetName(), Th->GetUniqueId());
    if (Th->IsA(VLevelInfo::StaticClass())) {
      //GCon->Logf(NAME_DevNet, "*** %s: got LevelInfo, sending 'client_spawn' command", *GetDebugName());
      VLevelInfo *LInfo = (VLevelInfo *)Th;
      LInfo->Level = LInfo;
      GClLevel->LevelInfo = LInfo;
      cl->Level = LInfo;
      cl->Net->SendCommand("Client_Spawn\n");
      cls.signon = 1;
    }
    #endif
    SetThinker(Th);
  }

  if (!Thinker) {
    if (Connection->IsServer()) {
      GCon->Logf(NAME_Error, "SERVER: %s: for some reason it doesn't have thinker!", *GetDebugName());
      //Close();
      return;
    } else {
      GCon->Logf(NAME_Error, "CLIENT: %s: for some reason it doesn't have thinker!", *GetDebugName());
      return;
    }
  }

  vassert(Thinker);
  VClass *ThinkerClass = Thinker->GetClass();

  VEntity *Ent = Cast<VEntity>(Thinker);
  TVec oldOrg(0.0f, 0.0f, 0.0f);
  TAVec oldAngles(0.0f, 0.0f, 0.0f);
  if (Ent) {
    Ent->UnlinkFromWorld();
    //TODO: use this to interpolate movements
    //      actually, we need to quantize movements by frame tics (1/35), and
    //      setup interpolation variables
    oldOrg = Ent->Origin;
    oldAngles = Ent->Angles;
  }

  while (!Msg.AtEnd()) {
    int FldIdx = Msg.ReadInt();

    // find field
    VField *F = nullptr;
    for (VField *CF = ThinkerClass->NetFields; CF; CF = CF->NextNetField) {
      if (CF->NetIndex == FldIdx) {
        F = CF;
        break;
      }
    }

    // got field?
    if (F) {
      if (F->Type.Type == TYPE_Array) {
        int Idx = Msg.ReadInt(/*F->Type.GetArrayDim()*/);
        VFieldType IntType = F->Type;
        IntType.Type = F->Type.ArrayInnerType;
        VField::NetSerialiseValue(Msg, Connection->ObjMap, (vuint8 *)Thinker+F->Ofs+Idx*IntType.GetSize(), IntType);
      } else {
        VField::NetSerialiseValue(Msg, Connection->ObjMap, (vuint8 *)Thinker+F->Ofs, F->Type);
      }
      continue;
    }

    // not a field: this must be RPC
    if (ReadRpc(Msg, FldIdx, Thinker)) continue;

    Sys_Error("Bad net field %d", FldIdx);
  }

  if (Ent) {
    Ent->LinkToWorld(true);
    //TODO: do not interpolate players?
    TVec newOrg = Ent->Origin;
    if (newOrg != oldOrg) {
      //GCon->Logf(NAME_Debug, "ENTITY '%s':%u moved! statetime=%g; state=%s", Ent->GetClass()->GetName(), Ent->GetUniqueId(), Ent->StateTime, (Ent->State ? *Ent->State->Loc.toStringNoCol() : "<none>"));
      if (Ent->StateTime < 0) {
        Ent->MoveFlags &= ~VEntity::MVF_JustMoved;
      } else if (Ent->StateTime > 0 && (newOrg-oldOrg).length2DSquared() < 64*64) {
        Ent->LastMoveOrigin = oldOrg;
        //Ent->LastMoveAngles = oldAngles;
        Ent->LastMoveAngles = Ent->Angles;
        Ent->LastMoveTime = Ent->XLevel->Time;
        Ent->LastMoveDuration = Ent->StateTime;
        Ent->MoveFlags |= VEntity::MVF_JustMoved;
      } else {
        Ent->MoveFlags &= ~VEntity::MVF_JustMoved;
      }
    }
  }
}
