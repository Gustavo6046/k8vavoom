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
//**  VEntity collision, physics and related methods.
//**
//**************************************************************************
#include "../gamedefs.h"
#include "../server/sv_local.h"


// ////////////////////////////////////////////////////////////////////////// //
static VCvarB gm_smart_z("gm_smart_z", true, "Fix Z position for some things, so they won't fall thru ledge edges?", /*CVAR_Archive|*/CVAR_PreInit);
#ifdef CLIENT
VCvarB r_interpolate_thing_movement("r_interpolate_thing_movement", true, "Interpolate mobj movement?", CVAR_Archive);
VCvarB r_interpolate_thing_angles_models("r_interpolate_thing_angles_models", true, "Interpolate mobj rotation for 3D models?", CVAR_Archive);
VCvarB r_interpolate_thing_angles_sprites("r_interpolate_thing_angles_sprites", false, "Interpolate mobj rotation for sprites?", CVAR_Archive);
#endif


//==========================================================================
//
//  tmtSetupGap
//
//  the base floor / ceiling is from the subsector that contains the point
//  any contacted lines the step closer together will adjust them
//
//==========================================================================
static void tmtSetupGap (tmtrace_t *tmtrace, sector_t *sector, float Height, bool debugDump) {
  SV_FindGapFloorCeiling(sector, tmtrace->End, Height, tmtrace->EFloor, tmtrace->ECeiling, debugDump);
  tmtrace->FloorZ = tmtrace->EFloor.GetPointZClamped(tmtrace->End);
  tmtrace->DropOffZ = tmtrace->FloorZ;
  tmtrace->CeilingZ = tmtrace->ECeiling.GetPointZClamped(tmtrace->End);
}


//==========================================================================
//
//  VEntity::CopyTraceFloor
//
//==========================================================================
void VEntity::CopyTraceFloor (tmtrace_t *tr, bool setz) {
  EFloor = tr->EFloor;
  if (setz) FloorZ = tr->FloorZ;
}


//==========================================================================
//
//  VEntity::CopyTraceCeiling
//
//==========================================================================
void VEntity::CopyTraceCeiling (tmtrace_t *tr, bool setz) {
  ECeiling = tr->ECeiling;
  if (setz) CeilingZ = tr->CeilingZ;
}


//==========================================================================
//
//  VEntity::CopyRegFloor
//
//==========================================================================
void VEntity::CopyRegFloor (sec_region_t *r, bool setz) {
  EFloor = r->efloor;
  FloorZ = EFloor.GetPointZClamped(Origin);
}


//==========================================================================
//
//  VEntity::CopyRegCeiling
//
//==========================================================================
void VEntity::CopyRegCeiling (sec_region_t *r, bool setz) {
  ECeiling = r->eceiling;
  CeilingZ = ECeiling.GetPointZClamped(Origin);
}


// ////////////////////////////////////////////////////////////////////////// //
// searches though the surrounding mapblocks for monsters/players
// distance is in MAPBLOCKUNITS
class VRoughBlockSearchIterator : public VScriptIterator {
private:
  VEntity *Self;
  int Distance;
  VEntity *Ent;
  VEntity **EntPtr;

  int StartX;
  int StartY;
  int Count;
  int CurrentEdge;
  int BlockIndex;
  int FirstStop;
  int SecondStop;
  int ThirdStop;
  int FinalStop;

public:
  VRoughBlockSearchIterator (VEntity *, int, VEntity **);
  virtual bool GetNext () override;
};


//**************************************************************************
//
//  THING POSITION SETTING
//
//**************************************************************************

//=============================================================================
//
//  VEntity::Destroy
//
//=============================================================================
void VEntity::Destroy () {
  UnlinkFromWorld();
  if (XLevel) XLevel->DelSectorList();
  if (TID && Level) RemoveFromTIDList(); // remove from TID list
  Super::Destroy();
}


//=============================================================================
//
//  VEntity::CreateSecNodeList
//
//  phares 3/14/98
//
//  alters/creates the sector_list that shows what sectors the object
//  resides in
//
//=============================================================================
void VEntity::CreateSecNodeList () {
  msecnode_t *Node;

  // first, clear out the existing Thing fields. as each node is
  // added or verified as needed, Thing will be set properly.
  // when finished, delete all nodes where Thing is still nullptr.
  // these represent the sectors the Thing has vacated.
  Node = XLevel->SectorList;
  while (Node) {
    Node->Thing = nullptr;
    Node = Node->TNext;
  }

  // use RenderRadius here, so we can check sectors in renderer instead of going through all objects
  // no, we cannot do this, because touching sector list is used to move objects too
  // we need another list here, if we want to avoid loop in renderer
  //const float rad = max2(RenderRadius, Radius);
  const float rad = Radius;

  //++validcount; // used to make sure we only process a line once
  XLevel->IncrementValidCount();

  if (rad > 1.0f) {
    float tmbbox[4];
    tmbbox[BOX2D_TOP] = Origin.y+rad;
    tmbbox[BOX2D_BOTTOM] = Origin.y-rad;
    tmbbox[BOX2D_RIGHT] = Origin.x+rad;
    tmbbox[BOX2D_LEFT] = Origin.x-rad;

    const int xl = MapBlock(tmbbox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
    const int xh = MapBlock(tmbbox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
    const int yl = MapBlock(tmbbox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
    const int yh = MapBlock(tmbbox[BOX2D_TOP]-XLevel->BlockMapOrgY);

    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        line_t *ld;
        for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
          // locates all the sectors the object is in by looking at the lines that cross through it.
          // you have already decided that the object is allowed at this location, so don't
          // bother with checking impassable or blocking lines.
          if (tmbbox[BOX2D_RIGHT] <= ld->bbox2d[BOX2D_LEFT] ||
              tmbbox[BOX2D_LEFT] >= ld->bbox2d[BOX2D_RIGHT] ||
              tmbbox[BOX2D_TOP] <= ld->bbox2d[BOX2D_BOTTOM] ||
              tmbbox[BOX2D_BOTTOM] >= ld->bbox2d[BOX2D_TOP])
          {
            continue;
          }

          if (P_BoxOnLineSide(tmbbox, ld) != -1) continue;

          // this line crosses through the object

          // collect the sector(s) from the line and add to the SectorList you're examining.
          // if the Thing ends up being allowed to move to this position, then the sector_list will
          // be attached to the Thing's VEntity at TouchingSectorList.

          XLevel->SectorList = XLevel->AddSecnode(ld->frontsector, this, XLevel->SectorList);

          // don't assume all lines are 2-sided, since some Things like
          // MT_TFOG are allowed regardless of whether their radius
          // takes them beyond an impassable linedef.

          // killough 3/27/98, 4/4/98:
          // use sidedefs instead of 2s flag to determine two-sidedness

          if (ld->backsector && ld->backsector != ld->frontsector) {
            XLevel->SectorList = XLevel->AddSecnode(ld->backsector, this, XLevel->SectorList);
          }
        }
      }
    }
  }

  // add the sector of the (x,y) point to sector_list
  XLevel->SectorList = XLevel->AddSecnode(Sector, this, XLevel->SectorList);

  // now delete any nodes that won't be used
  // these are the ones where Thing is still nullptr
  Node = XLevel->SectorList;
  while (Node) {
    if (Node->Thing == nullptr) {
      if (Node == XLevel->SectorList) XLevel->SectorList = Node->TNext;
      Node = XLevel->DelSecnode(Node);
    } else {
      Node = Node->TNext;
    }
  }
}


//==========================================================================
//
//  VEntity::UnlinkFromWorld
//
//  unlinks a thing from block map and sectors. on each position change,
//  BLOCKMAP and other lookups maintaining lists ot things inside these
//  structures need to be updated.
//
//==========================================================================
void VEntity::UnlinkFromWorld () {
  //!MoveFlags &= ~MVF_JustMoved;

  if (!SubSector) return;

  if (!(EntityFlags&EF_NoSector)) {
    // invisible things don't need to be in sector list
    // unlink from subsector
    if (SNext) SNext->SPrev = SPrev;
    if (SPrev) SPrev->SNext = SNext; else Sector->ThingList = SNext;
    SNext = nullptr;
    SPrev = nullptr;

    // phares 3/14/98
    //
    // Save the sector list pointed to by TouchingSectorList. In
    // LinkToWorld, we'll keep any nodes that represent sectors the Thing
    // still touches. We'll add new ones then, and delete any nodes for
    // sectors the Thing has vacated. Then we'll put it back into
    // TouchingSectorList. It's done this way to avoid a lot of
    // deleting/creating for nodes, when most of the time you just get
    // back what you deleted anyway.
    //
    // If this Thing is being removed entirely, then the calling routine
    // will clear out the nodes in sector_list.
    //
    XLevel->DelSectorList();
    XLevel->SectorList = TouchingSectorList;
    TouchingSectorList = nullptr; // to be restored by LinkToWorld
  }

  if (BlockMapCell /*&& !(EntityFlags&EF_NoBlockmap)*/) {
    // unlink from block map
    if (BlockMapNext) BlockMapNext->BlockMapPrev = BlockMapPrev;
    if (BlockMapPrev) {
      BlockMapPrev->BlockMapNext = BlockMapNext;
    } else {
      // we are the first entity in blockmap cell
      BlockMapCell -= 1; // real cell number
      // do some sanity checks
      vassert(XLevel->BlockLinks[BlockMapCell] == this);
      // fix list head
      XLevel->BlockLinks[BlockMapCell] = BlockMapNext;
    }
    BlockMapCell = 0;
  }

  SubSector = nullptr;
  Sector = nullptr;
}


//==========================================================================
//
//  IsGoreBloodSpot
//
//==========================================================================
/*
static inline bool IsGoreBloodSpot (VClass *cls) {
  const char *name = cls->GetName();
  if (name[0] == 'K' &&
      name[1] == '8' &&
      name[2] == 'G' &&
      name[3] == 'o' &&
      name[4] == 'r' &&
      name[5] == 'e' &&
      name[6] == '_')
  {
    return
      VStr::startsWith(name, "K8Gore_BloodSpot") ||
      VStr::startsWith(name, "K8Gore_CeilBloodSpot") ||
      VStr::startsWith(name, "K8Gore_MinuscleBloodSpot") ||
      VStr::startsWith(name, "K8Gore_GrowingBloodPool");
  }
  return false;
}


static TArray<VEntity *> bspotList;
*/


//==========================================================================
//
//  VEntity::LinkToWorld
//
//  Links a thing into both a block and a subsector based on it's x y.
//  Sets thing->subsector properly
//  pass -666 to force proper check (sorry for this hack)
//
//==========================================================================
void VEntity::LinkToWorld (int properFloorCheck) {
  if (SubSector) UnlinkFromWorld();

  // link into subsector
  subsector_t *ss = XLevel->PointInSubsector_Buggy(Origin);
  //vassert(ss); // meh, it will segfault on `nullptr` anyway
  SubSector = ss;
  Sector = ss->sector;

  if (properFloorCheck != -666) {
    if (!IsPlayer()) {
      if (properFloorCheck) {
        if (Radius < 4 || (EntityFlags&(EF_ColideWithWorld|EF_NoSector|EF_NoBlockmap|EF_Invisible|EF_Missile|EF_ActLikeBridge)) != EF_ColideWithWorld) {
          properFloorCheck = false;
        }
      }
    } else {
      properFloorCheck = true;
    }

    if (!gm_smart_z) properFloorCheck = false;
  }

  if (properFloorCheck) {
    //FIXME: this is copypasta from `CheckRelPos()`; factor it out
    tmtrace_t tmtrace;
    memset((void *)&tmtrace, 0, sizeof(tmtrace));
    subsector_t *newsubsec = ss;
    const TVec Pos = Origin;

    tmtrace.End = Pos;

    tmtrace.BBox[BOX2D_TOP] = Pos.y+Radius;
    tmtrace.BBox[BOX2D_BOTTOM] = Pos.y-Radius;
    tmtrace.BBox[BOX2D_RIGHT] = Pos.x+Radius;
    tmtrace.BBox[BOX2D_LEFT] = Pos.x-Radius;

    tmtSetupGap(&tmtrace, newsubsec->sector, Height, false);

    // check lines
    XLevel->IncrementValidCount();

    //tmtrace.FloorZ = tmtrace.DropOffZ;

    int xl = MapBlock(tmtrace.BBox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
    int xh = MapBlock(tmtrace.BBox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
    int yl = MapBlock(tmtrace.BBox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
    int yh = MapBlock(tmtrace.BBox[BOX2D_TOP]-XLevel->BlockMapOrgY);

    //float lastFZ, lastCZ;
    //sec_plane_t *lastFloor = nullptr;
    //sec_plane_t *lastCeiling = nullptr;

    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        line_t *ld;
        for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
          // we don't care about any blocking line info...
          (void)CheckRelLine(tmtrace, ld, true); // ...and we don't want to process any specials
        }
      }
    }

    CopyTraceFloor(&tmtrace);
    CopyTraceCeiling(&tmtrace);
  } else {
    // simplified check
    TSecPlaneRef floor, ceiling;
    SV_FindGapFloorCeiling(ss->sector, Origin, Height, EFloor, ECeiling);
    FloorZ = EFloor.GetPointZClamped(Origin);
    CeilingZ = ECeiling.GetPointZClamped(Origin);
  }

  // link into sector
  if (!(EntityFlags&EF_NoSector)) {
    // invisible things don't go into the sector links
    VEntity **Link = &Sector->ThingList;
    SPrev = nullptr;
    SNext = *Link;
    if (*Link) (*Link)->SPrev = this;
    *Link = this;

    // phares 3/16/98
    //
    // If sector_list isn't nullptr, it has a collection of sector
    // nodes that were just removed from this Thing.
    //
    // Collect the sectors the object will live in by looking at
    // the existing sector_list and adding new nodes and deleting
    // obsolete ones.
    //
    // When a node is deleted, its sector links (the links starting
    // at sector_t->touching_thinglist) are broken. When a node is
    // added, new sector links are created.
    CreateSecNodeList();
    TouchingSectorList = XLevel->SectorList; // attach to thing
    XLevel->SectorList = nullptr; // clear for next time
  } else {
    XLevel->DelSectorList();
  }

  // link into blockmap
  if (!(EntityFlags&EF_NoBlockmap)) {
    vassert(BlockMapCell == 0);
    // inert things don't need to be in blockmap
    int blockx = MapBlock(Origin.x-XLevel->BlockMapOrgX);
    int blocky = MapBlock(Origin.y-XLevel->BlockMapOrgY);

    if (blockx >= 0 && blockx < XLevel->BlockMapWidth &&
        blocky >= 0 && blocky < XLevel->BlockMapHeight)
    {
      BlockMapCell = ((unsigned)blocky*(unsigned)XLevel->BlockMapWidth+(unsigned)blockx);
      VEntity **link = &XLevel->BlockLinks[BlockMapCell];
      BlockMapPrev = nullptr;
      BlockMapNext = *link;
      if (*link) (*link)->BlockMapPrev = this;
      *link = this;
      BlockMapCell += 1;
    } else {
      // thing is off the map
      BlockMapNext = BlockMapPrev = nullptr;
    }

    // limit blood spots
    /*
    if (!(FlagsEx&EFEX_WasLinkedToWorld)) {
      FlagsEx |= EFEX_WasLinkedToWorld;
      if (BlockMapCell && IsGoreBloodSpot(GetClass())) {
        GCon->Logf(NAME_Debug, "counting blood spots... (%s)", GetClass()->GetName());
        int count = 0;
        for (VEntity *ee = XLevel->BlockLinks[BlockMapCell]; ee; ee = ee->BlockMapNext) {
          if (!ee->IsGoingToDie() && IsGoreBloodSpot(ee->GetClass())) ++count;
        }
        if (count >= 32) {
          // to much, reduce
          count = 26;
          bspotList.reset();
          for (VEntity *ee = XLevel->BlockLinks[BlockMapCell]; ee; ee = ee->BlockMapNext) {
            if (!ee->IsGoingToDie() && IsGoreBloodSpot(ee->GetClass())) {
              if (!count) bspotList.append(ee); else --count;
            }
          }
          if (bspotList.length()) {
            GCon->Logf(NAME_Debug, "removing %d blood spots", bspotList.length());
            for (int f = 0; f < bspotList.length(); ++f) bspotList[f]->DestroyThinker();
          }
        }
      } else {
        GCon->Logf(NAME_Debug, "linked `%s` first time", GetClass()->GetName());
      }
    }
    */
  }
}


//==========================================================================
//
//  VEntity::CheckWater
//
//  this sets `WaterLevel` and `WaterType`
//
//==========================================================================
void VEntity::CheckWater () {
  WaterLevel = 0;
  WaterType = 0;

  TVec point = Origin;
  point.z += 1.0f;
  const int contents = SV_PointContents(Sector, point);
  if (contents > 0) {
    WaterType = contents;
    WaterLevel = 1;
    point.z = Origin.z+Height*0.5f;
    if (SV_PointContents(Sector, point) > 0) {
      WaterLevel = 2;
      if (EntityFlags&EF_IsPlayer) {
        point = Player->ViewOrg;
        if (SV_PointContents(Sector, point) > 0) WaterLevel = 3;
      } else {
        point.z = Origin.z+Height*0.75f;
        if (SV_PointContents(Sector, point) > 0) WaterLevel = 3;
      }
    }
  }
  //return (WaterLevel > 1);
}


//**************************************************************************
//
//  CHECK ABSOLUTE POSITION
//
//**************************************************************************

//==========================================================================
//
//  VEntity::CheckPosition
//
//  This is purely informative, nothing is modified
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
//==========================================================================
bool VEntity::CheckPosition (TVec Pos) {
  //sec_region_t *gap;
  //sec_region_t *reg;
  tmtrace_t cptrace;
  bool good = true;

  memset((void *)&cptrace, 0, sizeof(cptrace));
  cptrace.Pos = Pos;

  cptrace.BBox[BOX2D_TOP] = Pos.y+Radius;
  cptrace.BBox[BOX2D_BOTTOM] = Pos.y-Radius;
  cptrace.BBox[BOX2D_RIGHT] = Pos.x+Radius;
  cptrace.BBox[BOX2D_LEFT] = Pos.x-Radius;

  subsector_t *newsubsec = XLevel->PointInSubsector_Buggy(Pos);

  // The base floor / ceiling is from the subsector that contains the point.
  // Any contacted lines the step closer together will adjust them.
  SV_FindGapFloorCeiling(newsubsec->sector, Pos, Height, cptrace.EFloor, cptrace.ECeiling);
  cptrace.DropOffZ = cptrace.FloorZ = cptrace.EFloor.GetPointZClamped(Pos);
  cptrace.CeilingZ = cptrace.ECeiling.GetPointZClamped(Pos);
  /*
  gap = SV_FindThingGap(newsubsec->sector, Pos, Height);
  reg = gap;
  while (reg->prev && (reg->efloor.splane->flags&SPF_NOBLOCKING) != 0) reg = reg->prev;
  cptrace.CopyRegFloor(reg, &Pos);
  cptrace.DropOffZ = cptrace.FloorZ;
  reg = gap;
  while (reg->next && (reg->eceiling.splane->flags&SPF_NOBLOCKING) != 0) reg = reg->next;
  cptrace.CopyRegCeiling(reg, &Pos);
  */

  //++validcount;
  XLevel->IncrementValidCount();

  if (EntityFlags&EF_ColideWithThings) {
    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    const int xl = MapBlock(cptrace.BBox[BOX2D_LEFT]-XLevel->BlockMapOrgX-MAXRADIUS);
    const int xh = MapBlock(cptrace.BBox[BOX2D_RIGHT]-XLevel->BlockMapOrgX+MAXRADIUS);
    const int yl = MapBlock(cptrace.BBox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY-MAXRADIUS);
    const int yh = MapBlock(cptrace.BBox[BOX2D_TOP]-XLevel->BlockMapOrgY+MAXRADIUS);

    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        for (VBlockThingsIterator It(XLevel, bx, by); It; ++It) {
          if (!CheckThing(cptrace, *It)) return false;
        }
      }
    }
  }

  if (EntityFlags&EF_ColideWithWorld) {
    // check lines
    const int xl = MapBlock(cptrace.BBox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
    const int xh = MapBlock(cptrace.BBox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
    const int yl = MapBlock(cptrace.BBox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
    const int yh = MapBlock(cptrace.BBox[BOX2D_TOP]-XLevel->BlockMapOrgY);

    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        line_t *ld;
        for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
          //good &= CheckLine(cptrace, ld);
          if (!CheckLine(cptrace, ld)) good = false; // no early exit!
        }
      }
    }

    if (!good) return false;
  }

  return true;
}


//==========================================================================
//
//  VEntity::CheckThing
//
//  returns `false` when blocked
//
//==========================================================================
bool VEntity::CheckThing (tmtrace_t &cptrace, VEntity *Other) {
  // don't clip against self
  if (Other == this) return true;
  //if (OwnerSUId && Other && Other->ServerUId == OwnerSUId) return true;

  // can't hit thing
  if (!(Other->EntityFlags&EF_ColideWithThings)) return true;
  if (!(Other->EntityFlags&EF_Solid)) return true;
  if (Other->EntityFlags&EF_Corpse) return true; //k8: skip corpses

  const float blockdist = Other->Radius+Radius;

  if (fabsf(Other->Origin.x-cptrace.Pos.x) >= blockdist ||
      fabsf(Other->Origin.y-cptrace.Pos.y) >= blockdist)
  {
    // didn't hit it
    return true;
  }

  if ((EntityFlags&(EF_PassMobj|EF_Missile)) || (Other->EntityFlags&EF_ActLikeBridge)) {
    // prevent some objects from overlapping
    if (EntityFlags&Other->EntityFlags&EF_DontOverlap) return false;
    // check if a mobj passed over/under another object
    if (cptrace.Pos.z >= Other->Origin.z+GetBlockingHeightFor(Other)) return true;
    if (cptrace.Pos.z+Height <= Other->Origin.z) return true; // under thing
  }

  return false;
}


//==========================================================================
//
//  VEntity::CheckLine
//
//  Adjusts cptrace.FloorZ and cptrace.CeilingZ as lines are contacted
//
//==========================================================================
bool VEntity::CheckLine (tmtrace_t &cptrace, line_t *ld) {
  if (cptrace.BBox[BOX2D_RIGHT] <= ld->bbox2d[BOX2D_LEFT] ||
      cptrace.BBox[BOX2D_LEFT] >= ld->bbox2d[BOX2D_RIGHT] ||
      cptrace.BBox[BOX2D_TOP] <= ld->bbox2d[BOX2D_BOTTOM] ||
      cptrace.BBox[BOX2D_BOTTOM] >= ld->bbox2d[BOX2D_TOP])
  {
    return true;
  }

  if (P_BoxOnLineSide(&cptrace.BBox[0], ld) != -1) return true;

  // a line has been hit
  if (!ld->backsector) return false; // one sided line

  if (!(ld->flags&ML_RAILING)) {
    if (ld->flags&ML_BLOCKEVERYTHING) return false; // explicitly blocking everything
    if ((EntityFlags&VEntity::EF_Missile) && (ld->flags&ML_BLOCKPROJECTILE)) return false; // blocks projectile
    if ((EntityFlags&VEntity::EF_CheckLineBlocking) && (ld->flags&ML_BLOCKING)) return false; // explicitly blocking everything
    if ((EntityFlags&VEntity::EF_CheckLineBlockMonsters) && (ld->flags&ML_BLOCKMONSTERS)) return false; // block monsters only
    if ((EntityFlags&VEntity::EF_IsPlayer) && (ld->flags&ML_BLOCKPLAYERS)) return false; // block players only
    if ((EntityFlags&VEntity::EF_Float) && (ld->flags&ML_BLOCK_FLOATERS)) return false; // block floaters only
  }

  // set openrange, opentop, openbottom
  TVec hit_point = cptrace.Pos-(DotProduct(cptrace.Pos, ld->normal)-ld->dist)*ld->normal;
  opening_t *open = SV_LineOpenings(ld, hit_point, SPF_NOBLOCKING, true); //!(EntityFlags&EF_Missile)); // missiles ignores 3dmidtex
  open = SV_FindOpening(open, cptrace.Pos.z, cptrace.Pos.z+Height);

  if (open) {
    // adjust floor / ceiling heights
    if (!(open->eceiling.splane->flags&SPF_NOBLOCKING) && open->top < cptrace.CeilingZ) {
      cptrace.CopyOpenCeiling(open);
    }

    if (!(open->efloor.splane->flags&SPF_NOBLOCKING) && open->bottom > cptrace.FloorZ) {
      cptrace.CopyOpenFloor(open);
    }

    if (open->lowfloor < cptrace.DropOffZ) cptrace.DropOffZ = open->lowfloor;

    if (ld->flags&ML_RAILING) cptrace.FloorZ += 32;
  } else {
    cptrace.CeilingZ = cptrace.FloorZ;
  }

  return true;
}


//**************************************************************************
//
//  MOVEMENT CLIPPING
//
//**************************************************************************

//==========================================================================
//
//  VEntity::CheckRelPosition
//
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//  VEntity *BlockingMobj = pointer to thing that blocked position (nullptr if not
//   blocked, or blocked by a line).
//
//==========================================================================
bool VEntity::CheckRelPosition (tmtrace_t &tmtrace, TVec Pos, bool noPickups, bool ignoreMonsters, bool ignorePlayers) {
  //if (IsPlayer()) GCon->Logf(NAME_Debug, "*** CheckRelPosition: pos=(%g,%g,%g)", Pos.x, Pos.y, Pos.z);

  tmtrace.End = Pos;

  tmtrace.BBox[BOX2D_TOP] = Pos.y+Radius;
  tmtrace.BBox[BOX2D_BOTTOM] = Pos.y-Radius;
  tmtrace.BBox[BOX2D_RIGHT] = Pos.x+Radius;
  tmtrace.BBox[BOX2D_LEFT] = Pos.x-Radius;

  subsector_t *newsubsec = XLevel->PointInSubsector_Buggy(Pos);
  tmtrace.CeilingLine = nullptr;

  tmtSetupGap(&tmtrace, newsubsec->sector, Height, false);

  XLevel->IncrementValidCount();
  tmtrace.SpecHit.reset(); // was `Clear()`

  tmtrace.BlockingMobj = nullptr;
  tmtrace.StepThing = nullptr;
  VEntity *thingblocker = nullptr;

  // check things first, possibly picking things up.
  // the bounding box is extended by MAXRADIUS
  // because mobj_ts are grouped into mapblocks
  // based on their origin point, and can overlap
  // into adjacent blocks by up to MAXRADIUS units.
  if (EntityFlags&EF_ColideWithThings) {
    const int xl = MapBlock(tmtrace.BBox[BOX2D_LEFT]-XLevel->BlockMapOrgX-MAXRADIUS);
    const int xh = MapBlock(tmtrace.BBox[BOX2D_RIGHT]-XLevel->BlockMapOrgX+MAXRADIUS);
    const int yl = MapBlock(tmtrace.BBox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY-MAXRADIUS);
    const int yh = MapBlock(tmtrace.BBox[BOX2D_TOP]-XLevel->BlockMapOrgY+MAXRADIUS);

    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        for (VBlockThingsIterator It(XLevel, bx, by); It; ++It) {
          if (ignoreMonsters || ignorePlayers) {
            VEntity *ent = *It;
            if (ignorePlayers && ent->IsPlayer()) continue;
            if (ignoreMonsters && (ent->IsMissile() || ent->IsMonster())) continue;
          }
          if (!CheckRelThing(tmtrace, *It, noPickups)) {
            // continue checking for other things in to see if we hit something
            if (!tmtrace.BlockingMobj || Level->GetNoPassOver()) {
              // slammed into something
              return false;
            } else if (!tmtrace.BlockingMobj->Player &&
                       !(EntityFlags&(VEntity::EF_Float|VEntity::EF_Missile)) &&
                       tmtrace.BlockingMobj->Origin.z+tmtrace.BlockingMobj->Height-tmtrace.End.z <= MaxStepHeight)
            {
              if (!thingblocker || tmtrace.BlockingMobj->Origin.z > thingblocker->Origin.z) thingblocker = tmtrace.BlockingMobj;
              tmtrace.BlockingMobj = nullptr;
            } else if (Player && tmtrace.End.z+Height-tmtrace.BlockingMobj->Origin.z <= MaxStepHeight) {
              if (thingblocker) {
                // something to step up on, set it as the blocker so that we don't step up
                return false;
              }
              // nothing is blocking, but this object potentially could
              // if there is something else to step on
              tmtrace.BlockingMobj = nullptr;
            } else {
              // blocking
              return false;
            }
          }
        }
      }
    }
  }

  // check lines
  XLevel->IncrementValidCount();

  float thingdropoffz = tmtrace.FloorZ;
  tmtrace.FloorZ = tmtrace.DropOffZ;
  tmtrace.BlockingMobj = nullptr;

  if (EntityFlags&EF_ColideWithWorld) {
    const int xl = MapBlock(tmtrace.BBox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
    const int xh = MapBlock(tmtrace.BBox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
    const int yl = MapBlock(tmtrace.BBox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
    const int yh = MapBlock(tmtrace.BBox[BOX2D_TOP]-XLevel->BlockMapOrgY);
    bool good = true;

    line_t *fuckhit = nullptr;
    float lastFrac = 1e7f;
    for (int bx = xl; bx <= xh; ++bx) {
      for (int by = yl; by <= yh; ++by) {
        line_t *ld;
        for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
          //good &= CheckRelLine(tmtrace, ld);
          // if we don't want pickups, don't activate specials
          if (!CheckRelLine(tmtrace, ld, noPickups)) {
            good = false;
            // find the fractional intercept point along the trace line
            const float den = DotProduct(ld->normal, tmtrace.End-Pos);
            if (den == 0) {
              fuckhit = ld;
              lastFrac = 0;
            } else {
              const float num = ld->dist-DotProduct(Pos, ld->normal);
              const float frac = num/den;
              if (fabsf(frac) < lastFrac) {
                fuckhit = ld;
                lastFrac = fabsf(frac);
              }
            }
          }
        }
      }
    }

    if (!good) {
      if (fuckhit) {
        if (!tmtrace.BlockingLine) tmtrace.BlockingLine = fuckhit;
        if (!tmtrace.AnyBlockingLine) tmtrace.AnyBlockingLine = fuckhit;
      }
      return false;
    }

    if (tmtrace.CeilingZ-tmtrace.FloorZ < Height) {
      if (fuckhit) {
        if (!tmtrace.CeilingLine && !tmtrace.FloorLine && !tmtrace.BlockingLine) {
          // this can happen when you're crouching, for example
          // `fuckhit` is not set in that case too
          GCon->Logf(NAME_Warning, "CheckRelPosition for `%s` is height-blocked, but no block line determined!", GetClass()->GetName());
          tmtrace.BlockingLine = fuckhit;
        }
        if (!tmtrace.AnyBlockingLine) tmtrace.AnyBlockingLine = fuckhit;
      }
      return false;
    }
  }

  if (tmtrace.StepThing) tmtrace.DropOffZ = thingdropoffz;

  tmtrace.BlockingMobj = thingblocker;
  if (tmtrace.BlockingMobj) return false;

  return true;
}


//==========================================================================
//
//  VEntity::CheckRelThing
//
//  returns `false` when blocked
//
//==========================================================================
bool VEntity::CheckRelThing (tmtrace_t &tmtrace, VEntity *Other, bool noPickups) {
  // don't clip against self
  if (Other == this) return true;
  //if (OwnerSUId && Other && Other->ServerUId == OwnerSUId) return true;

  // can't hit thing
  if (!(Other->EntityFlags&EF_ColideWithThings)) return true;

  const float blockdist = Other->Radius+Radius;

  if (fabsf(Other->Origin.x-tmtrace.End.x) >= blockdist ||
      fabsf(Other->Origin.y-tmtrace.End.y) >= blockdist)
  {
    // didn't hit it
    return true;
  }

  // can't walk on corpses (but can touch them)
  const bool isOtherCorpse = !!(Other->EntityFlags&EF_Corpse);

  if (!isOtherCorpse) {
    //tmtrace.BlockingMobj = Other;

    // check bridges
    if (/*!(Level->LevelInfoFlags2&VLevelInfo::LIF2_CompatNoPassOver) && !compat_nopassover*/!Level->GetNoPassOver() &&
        !(EntityFlags&(EF_Float|EF_Missile|EF_NoGravity)) &&
        (Other->EntityFlags&(EF_Solid|EF_ActLikeBridge)) == (EF_Solid|EF_ActLikeBridge))
    {
      // allow actors to walk on other actors as well as floors
      if (fabsf(Other->Origin.x-tmtrace.End.x) < Other->Radius ||
          fabsf(Other->Origin.y-tmtrace.End.y) < Other->Radius)
      {
        const float ohgt = GetBlockingHeightFor(Other);
        if (Other->Origin.z+ohgt >= tmtrace.FloorZ &&
            Other->Origin.z+ohgt <= tmtrace.End.z+MaxStepHeight)
        {
          //tmtrace.BlockingMobj = Other;
          tmtrace.StepThing = Other;
          tmtrace.FloorZ = Other->Origin.z+ohgt;
        }
      }
    }
  }

  //if (!(tmtrace.Thing->EntityFlags & VEntity::EF_NoPassMobj) || Actor(Other).bSpecial)
  if ((((EntityFlags&EF_PassMobj) || (Other->EntityFlags&EF_ActLikeBridge)) &&
       /*!(Level->LevelInfoFlags2&VLevelInfo::LIF2_CompatNoPassOver) && !compat_nopassover*/!Level->GetNoPassOver()) ||
      (EntityFlags&EF_Missile))
  {
    // prevent some objects from overlapping
    if (!isOtherCorpse && ((EntityFlags&Other->EntityFlags)&EF_DontOverlap)) {
      tmtrace.BlockingMobj = Other;
      return false;
    }
    // check if a mobj passed over/under another object
    if (tmtrace.End.z >= Other->Origin.z+GetBlockingHeightFor(Other)) return true; // overhead
    if (tmtrace.End.z+Height <= Other->Origin.z) return true;  // underneath
  }

  //FIXME: call VC to determine blocking
  if (noPickups) {
    if (isOtherCorpse) return true; // k8: corpses won't block
    if (Other->EntityFlags&EF_Solid) {
      tmtrace.BlockingMobj = Other;
      return false;
    }
    // non-solids won't block
    return true;
  }

  //if (EntityFlags&EF_IsPlayer) GCon->Logf("RelThing: self=`%s`; other=`%s`; ocp=%d", *GetClass()->GetFullName(), *Other->GetClass()->GetFullName(), (int)isOtherCorpse);

  if (!eventTouch(Other)) {
    // just in case
    tmtrace.BlockingMobj = Other;
    return false;
  }

  return true;
}


//==========================================================================
//
//  VEntity::CheckRelLine
//
//  Adjusts tmtrace.FloorZ and tmtrace.CeilingZ as lines are contacted
//
//  returns `false` if blocked
//
//==========================================================================
bool VEntity::CheckRelLine (tmtrace_t &tmtrace, line_t *ld, bool skipSpecials) {
  if (GGameInfo->NetMode == NM_Client) skipSpecials = true;

  //if (IsPlayer()) GCon->Logf(NAME_Debug, "  trying line: %d", (int)(ptrdiff_t)(ld-&XLevel->Lines[0]));
  // check line bounding box for early out
  if (tmtrace.BBox[BOX2D_RIGHT] <= ld->bbox2d[BOX2D_LEFT] ||
      tmtrace.BBox[BOX2D_LEFT] >= ld->bbox2d[BOX2D_RIGHT] ||
      tmtrace.BBox[BOX2D_TOP] <= ld->bbox2d[BOX2D_BOTTOM] ||
      tmtrace.BBox[BOX2D_BOTTOM] >= ld->bbox2d[BOX2D_TOP])
  {
    return true;
  }

  if (P_BoxOnLineSide(&tmtrace.BBox[0], ld) != -1) return true;

  // a line has been hit

  // The moving thing's destination position will cross the given line.
  // If this should not be allowed, return false.
  // If the line is special, keep track of it to process later if the move is proven ok.
  // NOTE: specials are NOT sorted by order, so two special lines that are only 8 pixels apart
  //       could be crossed in either order.

  // k8: this code is fuckin' mess. why some lines are processed immediately, and
  //     other lines are pushed to be processed later? what the fuck is going on here?!
  //     it seems that the original intent was to immediately process blocking lines,
  //     but push non-blocking lines. wtf?!

  if (!ld->backsector) {
    // one sided line
    if (!skipSpecials) BlockedByLine(ld);
    // mark the line as blocking line
    tmtrace.BlockingLine = ld;
    return false;
  }

  if (!(ld->flags&ML_RAILING)) {
    if (ld->flags&ML_BLOCKEVERYTHING) {
      // explicitly blocking everything
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }

    if ((EntityFlags&VEntity::EF_Missile) && (ld->flags&ML_BLOCKPROJECTILE)) {
      // blocks projectile
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }

    if ((EntityFlags&VEntity::EF_CheckLineBlocking) && (ld->flags&ML_BLOCKING)) {
      // explicitly blocking everything
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }

    if ((EntityFlags&VEntity::EF_CheckLineBlockMonsters) && (ld->flags&ML_BLOCKMONSTERS)) {
      // block monsters only
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }

    if ((EntityFlags&VEntity::EF_IsPlayer) && (ld->flags&ML_BLOCKPLAYERS)) {
      // block players only
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }

    if ((EntityFlags&VEntity::EF_Float) && (ld->flags&ML_BLOCK_FLOATERS)) {
      // block floaters only
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    }
  }

  // set openrange, opentop, openbottom
  const float hgt = (Height > 0 ? Height : 0.0f);
  TVec hit_point = tmtrace.End-(DotProduct(tmtrace.End, ld->normal)-ld->dist)*ld->normal;
  opening_t *open = SV_LineOpenings(ld, hit_point, SPF_NOBLOCKING, true); //!(EntityFlags&EF_Missile)); // missiles ignores 3dmidtex

  /*
  if (IsPlayer()) {
    GCon->Logf(NAME_Debug, "  checking line: %d; sz=%g; ez=%g; hgt=%g", (int)(ptrdiff_t)(ld-&XLevel->Lines[0]), tmtrace.End.z, tmtrace.End.z+hgt, hgt);
    for (opening_t *op = open; op; op = op->next) {
      GCon->Logf(NAME_Debug, "   %p: bot=%g; top=%g; range=%g; lowfloor=%g; highceil=%g", op, op->bottom, op->top, op->range, op->lowfloor, op->highceiling);
    }
  }
  */

  open = SV_FindRelOpening(open, tmtrace.End.z, tmtrace.End.z+hgt);
  //if (IsPlayer()) GCon->Logf(NAME_Debug, "  open=%p", open);

  if (open) {
    // adjust floor / ceiling heights
    if (!(open->eceiling.splane->flags&SPF_NOBLOCKING) && open->top < tmtrace.CeilingZ) {
      if (!skipSpecials || open->top+hgt >= Origin.z+hgt) {
        tmtrace.CopyOpenCeiling(open);
        tmtrace.CeilingLine = ld;
      }
    }

    if (!(open->efloor.splane->flags&SPF_NOBLOCKING) && open->bottom > tmtrace.FloorZ) {
      if (!skipSpecials || open->bottom <= Origin.z) {
        tmtrace.CopyOpenFloor(open);
        tmtrace.FloorLine = ld;
      }
    }

    if (open->lowfloor < tmtrace.DropOffZ) tmtrace.DropOffZ = open->lowfloor;

    if (ld->flags&ML_RAILING) tmtrace.FloorZ += 32;
  } else {
    tmtrace.CeilingZ = tmtrace.FloorZ;
    //k8: oops; it seems that we have to return `false` here
    //    but only if this is not a special line, otherwise monsters cannot open doors
    if (!ld->special) {
      //GCon->Logf(NAME_Debug, "BLK: %s (hgt=%g); line=%d", GetClass()->GetName(), hgt, (int)(ptrdiff_t)(ld-&XLevel->Lines[0]));
      if (!skipSpecials) BlockedByLine(ld);
      tmtrace.BlockingLine = ld;
      return false;
    } else {
      // this is special line, don't block movement (but remember this line as blocking!)
      tmtrace.BlockingLine = ld;
    }
  }

  // if contacted a special line, add it to the list
  if (!skipSpecials && ld->special) tmtrace.SpecHit.Append(ld);

  return true;
}


//==========================================================================
//
//  VEntity::BlockedByLine
//
//==========================================================================
void VEntity::BlockedByLine (line_t *ld) {
  if (EntityFlags&EF_Blasted) eventBlastedHitLine();
  if (ld->special) eventCheckForPushSpecial(ld, 0);
}


//==========================================================================
//
//  VEntity::GetDrawOrigin
//
//==========================================================================
TVec VEntity::GetDrawOrigin () {
  TVec sprorigin = Origin+GetDrawDelta();
  sprorigin.z -= FloorClip;
  // do floating bob here, so the dlight will not move
#ifdef CLIENT
  // perform bobbing
  if (FlagsEx&EFEX_FloatBob) {
    //float FloatBobPhase; // in seconds; <0 means "choose at random"; should be converted to ticks; amp is [0..63]
    //float FloatBobStrength;
    if (FloatBobPhase < 0) FloatBobPhase = Random()*256.0f/35.0f; // just in case
    const float amp = FloatBobStrength*8.0f;
    const float phase = fmodf(FloatBobPhase*35.0, 64.0f);
    const float angle = phase*360.0f/64.0f;
    #if 0
    float zofs = msin(angle)*amp;
    if (Sector) {
           if (Origin.z+zofs < FloorZ && Origin.z >= FloorZ) zofs = Origin.z-FloorZ;
      else if (Origin.z+zofs+Height > CeilingZ && Origin.z+Height <= CeilingZ) zofs = CeilingZ-Height-Origin.z;
    }
    res.z += zofs;
    #else
    sprorigin.z += msin(angle)*amp;
    #endif
  }
#endif
  return sprorigin;
}


//==========================================================================
//
//  VEntity::GetDrawDelta
//
//==========================================================================
TVec VEntity::GetDrawDelta () {
#ifdef CLIENT
  // movement interpolation
  if (r_interpolate_thing_movement && (MoveFlags&MVF_JustMoved)) {
    const float ctt = XLevel->Time-LastMoveTime;
    //GCon->Logf(NAME_Debug, "%s:%u: ctt=%g; delta=(%g,%g,%g)", GetClass()->GetName(), GetUniqueId(), ctt, (Origin-LastMoveOrigin).x, (Origin-LastMoveOrigin).y, (Origin-LastMoveOrigin).z);
    if (ctt >= 0.0f && ctt < LastMoveDuration && LastMoveDuration > 0.0f) {
      TVec delta = Origin-LastMoveOrigin;
      if (!delta.isZero()) {
        delta *= ctt/LastMoveDuration;
        //GCon->Logf(NAME_Debug, "%s:%u:   ...realdelta=(%g,%g,%g)", GetClass()->GetName(), GetUniqueId(), delta.x, delta.y, delta.z);
        return (LastMoveOrigin+delta)-Origin;
      } else {
        // reset if angles are equal
        if (LastMoveAngles.yaw == Angles.yaw) MoveFlags &= ~VEntity::MVF_JustMoved;
      }
    } else {
      // unconditional reset
      MoveFlags &= ~VEntity::MVF_JustMoved;
    }
  }
#endif
  return TVec(0.0f, 0.0f, 0.0f);
}


//==========================================================================
//
//  VEntity::GetInterpolatedDrawAngles
//
//==========================================================================
TAVec VEntity::GetInterpolatedDrawAngles () {
#ifdef CLIENT
  // movement interpolation
  if (MoveFlags&MVF_JustMoved) {
    const float ctt = XLevel->Time-LastMoveTime;
    if (ctt >= 0.0f && ctt < LastMoveDuration && LastMoveDuration > 0.0f) {
      float deltaYaw = AngleDiff(AngleMod(LastMoveAngles.yaw), AngleMod(Angles.yaw));
      if (deltaYaw) {
        deltaYaw *= ctt/LastMoveDuration;
        return TAVec(Angles.pitch, AngleMod(LastMoveAngles.yaw+deltaYaw), Angles.roll);
      } else {
        // reset if origins are equal
        if ((Origin-LastMoveOrigin).isZero()) MoveFlags &= ~VEntity::MVF_JustMoved;
      }
    } else {
      // unconditional reset
      MoveFlags &= ~VEntity::MVF_JustMoved;
    }
  }
#endif
  return Angles;
}


//==========================================================================
//
//  VEntity::TryMove
//
//  attempt to move to a new position, crossing special lines.
//  returns `false` if move is blocked.
//
//==========================================================================
bool VEntity::TryMove (tmtrace_t &tmtrace, TVec newPos, bool AllowDropOff, bool checkOnly, bool noPickups) {
  bool check;
  TVec oldorg(0, 0, 0);
  line_t *ld;
  sector_t *OldSec = Sector;

  if (IsGoingToDie() || !Sector) return false; // just in case, dead object is immovable

  const bool isClient = (GGameInfo->NetMode == NM_Client);

  bool skipEffects = (checkOnly || noPickups);

  check = CheckRelPosition(tmtrace, newPos, skipEffects);
  tmtrace.TraceFlags &= ~tmtrace_t::TF_FloatOk;
  //if (IsPlayer()) GCon->Logf(NAME_Debug, "trying to move from (%g,%g,%g) to (%g,%g,%g); check=%d", Origin.x, Origin.y, Origin.z, newPos.x, newPos.y, newPos.z, (int)check);

  if (isClient) skipEffects = true;

  if (!check) {
    // cannot fit into destination point
    VEntity *O = tmtrace.BlockingMobj;
    //GCon->Logf("HIT! %s", O->GetClass()->GetName());
    if (!O) {
      // can't step up or doesn't fit
      PushLine(tmtrace, skipEffects);
      return false;
    }

    const float ohgt = GetBlockingHeightFor(O);
    if (!(EntityFlags&EF_IsPlayer) ||
        (O->EntityFlags&EF_IsPlayer) ||
        O->Origin.z+ohgt-Origin.z > MaxStepHeight ||
        O->CeilingZ-(O->Origin.z+ohgt) < Height ||
        tmtrace.CeilingZ-(O->Origin.z+ohgt) < Height)
    {
      // can't step up or doesn't fit
      PushLine(tmtrace, skipEffects);
      return false;
    }

    if (!(EntityFlags&EF_PassMobj) || Level->GetNoPassOver()) {
      // can't go over
      return false;
    }
  }

  if (EntityFlags&EF_ColideWithWorld) {
    if (tmtrace.CeilingZ-tmtrace.FloorZ < Height) {
      // doesn't fit
      PushLine(tmtrace, skipEffects);
      //printf("*** WORLD(0)!\n");
      return false;
    }

    tmtrace.TraceFlags |= tmtrace_t::TF_FloatOk;

    if (tmtrace.CeilingZ-Origin.z < Height && !(EntityFlags&EF_Fly) && !(EntityFlags&EF_IgnoreCeilingStep)) {
      // mobj must lower itself to fit
      PushLine(tmtrace, skipEffects);
      //printf("*** WORLD(1)!\n");
      return false;
    }

    if (EntityFlags&EF_Fly) {
      // when flying, slide up or down blocking lines until the actor is not blocked
      if (Origin.z+Height > tmtrace.CeilingZ) {
        // if sliding down, make sure we don't have another object below
        if ((!tmtrace.BlockingMobj || !tmtrace.BlockingMobj->CheckOnmobj() ||
            (tmtrace.BlockingMobj->CheckOnmobj() &&
             tmtrace.BlockingMobj->CheckOnmobj() != this)) &&
            (!CheckOnmobj() || (CheckOnmobj() &&
             CheckOnmobj() != tmtrace.BlockingMobj)))
        {
          if (!checkOnly && !isClient) Velocity.z = -8.0f*35.0f;
        }
        PushLine(tmtrace, skipEffects);
        return false;
      } else if (Origin.z < tmtrace.FloorZ && tmtrace.FloorZ-tmtrace.DropOffZ > MaxStepHeight) {
        // check to make sure there's nothing in the way for the step up
        if ((!tmtrace.BlockingMobj || !tmtrace.BlockingMobj->CheckOnmobj() ||
            (tmtrace.BlockingMobj->CheckOnmobj() &&
             tmtrace.BlockingMobj->CheckOnmobj() != this)) &&
            (!CheckOnmobj() || (CheckOnmobj() &&
             CheckOnmobj() != tmtrace.BlockingMobj)))
        {
          if (!checkOnly && !isClient) Velocity.z = 8.0f*35.0f;
        }
        PushLine(tmtrace, skipEffects);
        return false;
      }
    }

    if (!(EntityFlags&EF_IgnoreFloorStep)) {
      if (tmtrace.FloorZ-Origin.z > MaxStepHeight) {
        // too big a step up
        if ((EntityFlags&EF_CanJump) && Health > 0) {
          // check to make sure there's nothing in the way for the step up
          if (!Velocity.z || tmtrace.FloorZ-Origin.z > 48.0f ||
              (tmtrace.BlockingMobj && tmtrace.BlockingMobj->CheckOnmobj()) ||
              TestMobjZ(TVec(newPos.x, newPos.y, tmtrace.FloorZ)))
          {
            PushLine(tmtrace, skipEffects);
            //printf("*** WORLD(2)!\n");
            return false;
          }
        } else {
          PushLine(tmtrace, skipEffects);
          //printf("*** WORLD(3)!\n");
          return false;
        }
      }

      if ((EntityFlags&EF_Missile) && !(EntityFlags&EF_StepMissile) && tmtrace.FloorZ > Origin.z) {
        PushLine(tmtrace, skipEffects);
        //printf("*** WORLD(4)!\n");
        return false;
      }

      if (Origin.z < tmtrace.FloorZ) {
        if (EntityFlags&EF_StepMissile) {
          Origin.z = tmtrace.FloorZ;
          // if moving down, cancel vertical component of velocity
          if (Velocity.z < 0) Velocity.z = 0.0f;
        }
        // check to make sure there's nothing in the way for the step up
        if (TestMobjZ(TVec(newPos.x, newPos.y, tmtrace.FloorZ))) {
          PushLine(tmtrace, skipEffects);
          //printf("*** WORLD(5)!\n");
          return false;
        }
      }
    }

    // killough 3/15/98: Allow certain objects to drop off
    if ((!AllowDropOff && !(EntityFlags&EF_DropOff) &&
        !(EntityFlags&EF_Float) && !(EntityFlags&EF_Missile)) ||
        (EntityFlags&EF_NoDropOff))
    {
      if (!(EntityFlags&EF_AvoidingDropoff)) {
        float floorz = tmtrace.FloorZ;
        // [RH] If the thing is standing on something, use its current z as the floorz.
        // This is so that it does not walk off of things onto a drop off.
        if (EntityFlags&EF_OnMobj) floorz = max2(Origin.z, tmtrace.FloorZ);

        if ((floorz-tmtrace.DropOffZ > MaxDropoffHeight) && !(EntityFlags&EF_Blasted)) {
          // Can't move over a dropoff unless it's been blasted
          //printf("*** WORLD(6)!\n");
          return false;
        }
      } else {
        // special logic to move a monster off a dropoff
        // this intentionally does not check for standing on things
        if (FloorZ-tmtrace.FloorZ > MaxDropoffHeight || DropOffZ-tmtrace.DropOffZ > MaxDropoffHeight) {
          //printf("*** WORLD(7)!\n");
          return false;
        }
      }
    }

    if ((EntityFlags&EF_CantLeaveFloorpic) &&
        (tmtrace.EFloor.splane->pic != EFloor.splane->pic || tmtrace.FloorZ != Origin.z))
    {
      // must stay within a sector of a certain floor type
      //printf("*** WORLD(8)!\n");
      return false;
    }
  }

  bool OldAboveFakeFloor = false;
  bool OldAboveFakeCeiling = false;
  if (Sector->heightsec) {
    float EyeZ = (Player ? Player->ViewOrg.z : Origin.z+Height*0.5f);
    OldAboveFakeFloor = (EyeZ > Sector->heightsec->floor.GetPointZClamped(Origin));
    OldAboveFakeCeiling = (EyeZ > Sector->heightsec->ceiling.GetPointZClamped(Origin));
  }

  if (checkOnly) return true;

  // the move is ok, so link the thing into its new position
  UnlinkFromWorld();

  oldorg = Origin;
  Origin = newPos;

  LinkToWorld();

  FloorZ = tmtrace.FloorZ;
  CeilingZ = tmtrace.CeilingZ;
  DropOffZ = tmtrace.DropOffZ;
  CopyTraceFloor(&tmtrace, false);
  CopyTraceCeiling(&tmtrace, false);

  if (EntityFlags&EF_FloorClip) {
    eventHandleFloorclip();
  } else {
    FloorClip = 0.0f;
  }

  if (!noPickups && !isClient) {
    // if any special lines were hit, do the effect
    if (EntityFlags&EF_ColideWithWorld) {
      while (tmtrace.SpecHit.Num() > 0) {
        int side;
        int oldside;

        // see if the line was crossed
        ld = tmtrace.SpecHit[tmtrace.SpecHit.Num()-1];
        tmtrace.SpecHit.SetNum(tmtrace.SpecHit.Num()-1, false);
        side = ld->PointOnSide(Origin);
        oldside = ld->PointOnSide(oldorg);
        if (side != oldside) {
          if (ld->special) eventCrossSpecialLine(ld, oldside);
        }
      }
    }

    // do additional check here to avoid calling progs
    if ((OldSec->heightsec && Sector->heightsec && Sector->ActionList) ||
        (OldSec != Sector && (OldSec->ActionList || Sector->ActionList)))
    {
      eventCheckForSectorActions(OldSec, OldAboveFakeFloor, OldAboveFakeCeiling);
    }
  }

  return true;
}


//==========================================================================
//
//  VEntity::PushLine
//
//==========================================================================
void VEntity::PushLine (const tmtrace_t &tmtrace, bool checkOnly) {
  if (checkOnly || GGameInfo->NetMode == NM_Client) return;
  if (EntityFlags&EF_ColideWithWorld) {
    if (EntityFlags&EF_Blasted) eventBlastedHitLine();
    int NumSpecHitTemp = tmtrace.SpecHit.Num();
    while (NumSpecHitTemp > 0) {
      --NumSpecHitTemp;
      // see if the line was crossed
      line_t *ld = tmtrace.SpecHit[NumSpecHitTemp];
      int side = ld->PointOnSide(Origin);
      eventCheckForPushSpecial(ld, side);
    }
  }
}


//**************************************************************************
//
//  SLIDE MOVE
//
//  Allows the player to slide along any angled walls.
//
//**************************************************************************

//==========================================================================
//
//  VEntity::ClipVelocity
//
//  Slide off of the impacting object
//
//==========================================================================
TVec VEntity::ClipVelocity (const TVec &in, const TVec &normal, float overbounce) {
  return in-normal*(DotProduct(in, normal)*overbounce);
}


//==========================================================================
//
//  VEntity::SlidePathTraverse
//
//==========================================================================
void VEntity::SlidePathTraverse (float &BestSlideFrac, line_t *&BestSlideLine, float x, float y, float StepVelScale) {
  TVec SlideOrg(x, y, Origin.z);
  TVec SlideDir = Velocity*StepVelScale;
  intercept_t *in;
  for (VPathTraverse It(this, &in, x, y, x+SlideDir.x, y+SlideDir.y, PT_ADDLINES); It.GetNext(); ) {
    if (!(in->Flags&intercept_t::IF_IsALine)) Host_Error("PTR_SlideTraverse: not a line?");

    line_t *li = in->line;

    bool IsBlocked = false;
    if (!(li->flags&ML_TWOSIDED) || !li->backsector) {
      if (li->PointOnSide(Origin)) continue; // don't hit the back side
      IsBlocked = true;
    } else if (li->flags&(ML_BLOCKING|ML_BLOCKEVERYTHING)) {
      IsBlocked = true;
    } else if ((EntityFlags&EF_IsPlayer) && (li->flags&ML_BLOCKPLAYERS)) {
      IsBlocked = true;
    } else if ((EntityFlags&EF_CheckLineBlockMonsters) && (li->flags&ML_BLOCKMONSTERS)) {
      IsBlocked = true;
    }

    if (!IsBlocked) {
      // set openrange, opentop, openbottom
      TVec hit_point = SlideOrg+in->frac*SlideDir;
      opening_t *open = SV_LineOpenings(li, hit_point, SPF_NOBLOCKING, true); //!(EntityFlags&EF_Missile)); // missiles ignores 3dmidtex
      open = SV_FindOpening(open, Origin.z, Origin.z+Height);

      if (open && open->range >= Height && // fits
          open->top-Origin.z >= Height && // mobj is not too high
          open->bottom-Origin.z <= MaxStepHeight) // not too big a step up
      {
        // this line doesn't block movement
        if (Origin.z < open->bottom) {
          // check to make sure there's nothing in the way for the step up
          TVec CheckOrg = Origin;
          CheckOrg.z = open->bottom;
          if (!TestMobjZ(CheckOrg)) continue;
        } else {
          continue;
        }
      }
    }

    // the line blocks movement, see if it is closer than best so far
    if (in->frac < BestSlideFrac) {
      BestSlideFrac = in->frac;
      BestSlideLine = li;
    }

    break;  // stop
  }
}


//==========================================================================
//
//  VEntity::SlideMove
//
//  The momx / momy move is bad, so try to slide along a wall.
//  Find the first line hit, move flush to it, and slide along it.
//  This is a kludgy mess.
//
//  `noPickups` means "don't activate specials" too.
//
//  k8: TODO: switch to beveled BSP!
//
//==========================================================================
void VEntity::SlideMove (float StepVelScale, bool noPickups) {
  float leadx;
  float leady;
  float trailx;
  float traily;
  float newx;
  float newy;
  int hitcount;
  tmtrace_t tmtrace;
  memset((void *)&tmtrace, 0, sizeof(tmtrace)); // valgrind: AnyBlockingLine

  hitcount = 0;

  float XMove = Velocity.x*StepVelScale;
  float YMove = Velocity.y*StepVelScale;
  if (XMove == 0.0f && YMove == 0.0f) return; // just in case

  do {
    if (++hitcount == 3) {
      // don't loop forever
      if (!TryMove(tmtrace, TVec(Origin.x, Origin.y+YMove, Origin.z), true, noPickups)) {
        TryMove(tmtrace, TVec(Origin.x+XMove, Origin.y, Origin.z), true, noPickups);
      }
      return;
    }

    // trace along the three leading corners
    if (XMove > 0.0f) {
      leadx = Origin.x+Radius;
      trailx = Origin.x-Radius;
    } else {
      leadx = Origin.x-Radius;
      trailx = Origin.x+Radius;
    }

    if (Velocity.y > 0.0f) {
      leady = Origin.y+Radius;
      traily = Origin.y-Radius;
    } else {
      leady = Origin.y-Radius;
      traily = Origin.y+Radius;
    }

    float BestSlideFrac = 1.00001f;
    line_t *BestSlideLine = nullptr;

    SlidePathTraverse(BestSlideFrac, BestSlideLine, leadx, leady, StepVelScale);
    SlidePathTraverse(BestSlideFrac, BestSlideLine, trailx, leady, StepVelScale);
    SlidePathTraverse(BestSlideFrac, BestSlideLine, leadx, traily, StepVelScale);

    // move up to the wall
    if (BestSlideFrac == 1.00001f) {
      // the move must have hit the middle, so stairstep
      if (!TryMove(tmtrace, TVec(Origin.x, Origin.y+YMove, Origin.z), true)) {
        TryMove(tmtrace, TVec(Origin.x+XMove, Origin.y, Origin.z), true);
      }
      return;
    }

    // fudge a bit to make sure it doesn't hit
    BestSlideFrac -= 0.03125f;
    if (BestSlideFrac > 0.0f) {
      newx = XMove*BestSlideFrac;
      newy = YMove*BestSlideFrac;

      if (!TryMove(tmtrace, TVec(Origin.x+newx, Origin.y+newy, Origin.z), true)) {
        if (!TryMove(tmtrace, TVec(Origin.x, Origin.y+YMove, Origin.z), true)) {
          TryMove(tmtrace, TVec(Origin.x+XMove, Origin.y, Origin.z), true);
        }
        return;
      }
    }

    // now continue along the wall
    // first calculate remainder
    BestSlideFrac = 1.0f-(BestSlideFrac+0.03125f);

    if (BestSlideFrac > 1.0f) BestSlideFrac = 1.0f;
    if (BestSlideFrac <= 0.0f) return;

    // clip the moves
    // k8: don't multiply z, 'cause it makes jumping against a wall unpredictably hard
    Velocity.x *= BestSlideFrac;
    Velocity.y *= BestSlideFrac;
    Velocity = ClipVelocity(Velocity, BestSlideLine->normal, 1.0f);
    //Velocity = ClipVelocity(Velocity*BestSlideFrac, BestSlideLine->normal, 1.0f);
    XMove = Velocity.x*StepVelScale;
    YMove = Velocity.y*StepVelScale;
  } while (!TryMove(tmtrace, TVec(Origin.x+XMove, Origin.y+YMove, Origin.z), true));
}


//==========================================================================
//
//  VEntity::SlideMove
//
//  this is used to move chase camera
//
//==========================================================================
TVec VEntity::SlideMoveCamera (TVec org, TVec end, float radius) {
  TVec velo = end-org;
  if (!velo.isValid() || velo.isZero()) return org;

  if (radius < 2) {
    // just trace
    linetrace_t ltr;
    if (XLevel->TraceLine(ltr, org, end, 0/*SPF_NOBLOCKSIGHT*/)) return end; // no hit
    // hit something, move slightly forward
    TVec mdelta = ltr.LineEnd-org;
    //const float wantdist = velo.length();
    const float movedist = mdelta.length();
    if (movedist > 2.0f) {
      //GCon->Logf("*** hit! (%g,%g,%g)", ltr.HitPlaneNormal.x, ltr.HitPlaneNormal.y, ltr.HitPlaneNormal.z);
      if (ltr.HitPlaneNormal.z) {
        // floor
        //GCon->Logf("floor hit! (%g,%g,%g)", ltr.HitPlaneNormal.x, ltr.HitPlaneNormal.y, ltr.HitPlaneNormal.z);
        ltr.LineEnd += ltr.HitPlaneNormal*2;
      } else {
        ltr.LineEnd -= mdelta.normalised()*2;
      }
    }
    return ltr.LineEnd;
  }

  // split move in multiple steps if moving too fast
  const float xmove = velo.x;
  const float ymove = velo.y;

  int Steps = 1;
  float XStep = fabsf(xmove);
  float YStep = fabsf(ymove);
  float MaxStep = radius-1.0f;

  if (XStep > MaxStep || YStep > MaxStep) {
    if (XStep > YStep) {
      Steps = int(XStep/MaxStep)+1;
    } else {
      Steps = int(YStep/MaxStep)+1;
    }
  }

  const float StepXMove = xmove/float(Steps);
  const float StepYMove = ymove/float(Steps);
  const float StepZMove = velo.z/float(Steps);

  //GCon->Logf("*** *** Steps=%d; move=(%g,%g,%g); onestep=(%g,%g,%g)", Steps, xmove, ymove, velo.z, StepXMove, StepYMove, StepZMove);
  tmtrace_t tmtrace;
  for (int step = 0; step < Steps; ++step) {
    float ptryx = org.x+StepXMove;
    float ptryy = org.y+StepYMove;
    float ptryz = org.z+StepZMove;

    TVec newPos = TVec(ptryx, ptryy, ptryz);
    bool check = CheckRelPosition(tmtrace, newPos, true);
    if (check) {
      org = newPos;
      continue;
    }

    // blocked move; trace back until we got a good position
    // this sux, but we'll do it only once per frame, so...
    float len = (newPos-org).length();
    TVec dir = (newPos-org).normalised(); // to newpos
    //GCon->Logf("*** len=%g; dir=(%g,%g,%g)", len, dir.x, dir.y, dir.z);
    float curr = 1.0f;
    while (curr <= len) {
      if (!CheckRelPosition(tmtrace, org+dir*curr, true)) {
        curr -= 1.0f;
        break;
      }
      curr += 1.0f;
    }
    if (curr > len) curr = len;
    //GCon->Logf("   final len=%g", curr);
    org += dir*curr;
    break;
  }

  // clamp to floor/ceiling
  CheckRelPosition(tmtrace, org, true);
  if (org.z < tmtrace.FloorZ+radius) org.z = tmtrace.FloorZ+radius;
  if (org.z > tmtrace.CeilingZ-radius) org.z = tmtrace.CeilingZ-radius;
  return org;
}


//**************************************************************************
//
//  BOUNCING
//
//  Bounce missile against walls
//
//**************************************************************************

//============================================================================
//
//  VEntity::BounceWall
//
//============================================================================
void VEntity::BounceWall (float overbounce, float bouncefactor) {
  TVec SlideOrg;

  if (Velocity.x > 0.0f) SlideOrg.x = Origin.x+Radius; else SlideOrg.x = Origin.x-Radius;
  if (Velocity.y > 0.0f) SlideOrg.y = Origin.y+Radius; else SlideOrg.y = Origin.y-Radius;
  SlideOrg.z = Origin.z;
  TVec SlideDir = Velocity*host_frametime;
  line_t *BestSlideLine = nullptr;
  intercept_t *in;

  for (VPathTraverse It(this, &in, SlideOrg.x, SlideOrg.y, SlideOrg.x+SlideDir.x, SlideOrg.y+SlideDir.y, PT_ADDLINES); It.GetNext(); ) {
    if (!(in->Flags&intercept_t::IF_IsALine)) Host_Error("PTR_BounceTraverse: not a line?");
    line_t *li = in->line;
    TVec hit_point = SlideOrg+in->frac*SlideDir;

    if (li->flags&ML_TWOSIDED) {
      // set openrange, opentop, openbottom
      opening_t *open = SV_LineOpenings(li, hit_point, SPF_NOBLOCKING, true); //!(EntityFlags&EF_Missile)); // missiles ignores 3dmidtex
      open = SV_FindOpening(open, Origin.z, Origin.z+Height);

      if (open && open->range >= Height && // fits
          Origin.z+Height <= open->top &&
          Origin.z >= open->bottom) // mobj is not too high
      {
        continue; // this line doesn't block movement
      }
    } else {
      if (li->PointOnSide(Origin)) continue; // don't hit the back side
    }

    BestSlideLine = li;
    break; // don't bother going farther
  }

  if (BestSlideLine) {
    TAVec delta_ang;
    TAVec lineang;
    TVec delta(0, 0, 0);

    // convert BesSlideLine normal to an angle
    VectorAngles(BestSlideLine->normal, lineang);
    if (BestSlideLine->PointOnSide(Origin) == 1) lineang.yaw += 180.0f;

    // convert the line angle back to a vector, so that
    // we can use it to calculate the delta against
    // the Velocity vector
    AngleVector(lineang, delta);
    delta = (delta*2.0f)-Velocity;

    // finally get the delta angle to use
    VectorAngles(delta, delta_ang);

    Velocity.x = (Velocity.x*bouncefactor)*cos(delta_ang.yaw);
    Velocity.y = (Velocity.y*bouncefactor)*sin(delta_ang.yaw);
    Velocity = ClipVelocity(Velocity, BestSlideLine->normal, overbounce);
  }
}



//**************************************************************************
//
//  TEST ON MOBJ
//
//**************************************************************************

//=============================================================================
//
//  TestMobjZ
//
//  Checks if the new Z position is legal
//  returns blocking thing
//
//=============================================================================
VEntity *VEntity::TestMobjZ (const TVec &TryOrg) {
  // can't hit things, or not solid?
  if ((EntityFlags&(EF_ColideWithThings|EF_Solid)) != (EF_ColideWithThings|EF_Solid)) return nullptr;

  // the bounding box is extended by MAXRADIUS because mobj_ts are grouped
  // into mapblocks based on their origin point, and can overlap into adjacent
  // blocks by up to MAXRADIUS units
  const int xl = MapBlock(TryOrg.x-Radius-XLevel->BlockMapOrgX-MAXRADIUS);
  const int xh = MapBlock(TryOrg.x+Radius-XLevel->BlockMapOrgX+MAXRADIUS);
  const int yl = MapBlock(TryOrg.y-Radius-XLevel->BlockMapOrgY-MAXRADIUS);
  const int yh = MapBlock(TryOrg.y+Radius-XLevel->BlockMapOrgY+MAXRADIUS);

  // xl->xh, yl->yh determine the mapblock set to search
  for (int bx = xl; bx <= xh; ++bx) {
    for (int by = yl; by <= yh; ++by) {
      for (VBlockThingsIterator Other(XLevel, bx, by); Other; ++Other) {
        if (*Other == this) continue; // don't clip against self
        //if (OwnerSUId && Other->ServerUId == OwnerSUId) continue;
        //k8: can't hit corpse
        if ((Other->EntityFlags&(EF_ColideWithThings|EF_Solid|EF_Corpse)) != (EF_ColideWithThings|EF_Solid)) continue; // can't hit things, or not solid
        const float ohgt = GetBlockingHeightFor(*Other);
        if (TryOrg.z > Other->Origin.z+ohgt) continue; // over thing
        if (TryOrg.z+Height < Other->Origin.z) continue; // under thing
        const float blockdist = Other->Radius+Radius;
        if (fabsf(Other->Origin.x-TryOrg.x) >= blockdist ||
            fabsf(Other->Origin.y-TryOrg.y) >= blockdist)
        {
          // didn't hit thing
          continue;
        }
        return *Other;
      }
    }
  }

  return nullptr;
}


//=============================================================================
//
//  VEntity::FakeZMovement
//
//  Fake the zmovement so that we can check if a move is legal
//
//=============================================================================
TVec VEntity::FakeZMovement () {
  TVec Ret = TVec(0, 0, 0);
  eventCalcFakeZMovement(Ret, host_frametime);
  // clip movement
  if (Ret.z <= FloorZ) Ret.z = FloorZ; // hit the floor
  if (Ret.z+Height > CeilingZ) Ret.z = CeilingZ-Height; // hit the ceiling
  return Ret;
}


//=============================================================================
//
//  VEntity::CheckOnmobj
//
//  Checks if an object is above another object
//
//=============================================================================
VEntity *VEntity::CheckOnmobj () {
  // can't hit things, or not solid? (check it here to save one VM invocation)
  if ((EntityFlags&(EF_ColideWithThings|EF_Solid)) != (EF_ColideWithThings|EF_Solid)) return nullptr;
  return TestMobjZ(FakeZMovement());
}


//==========================================================================
//
//  VEntity::CheckSides
//
//  This routine checks for Lost Souls trying to be spawned    // phares
//  across 1-sided lines, impassible lines, or "monsters can't //   |
//  cross" lines. Draw an imaginary line between the PE        //   V
//  and the new Lost Soul spawn spot. If that line crosses
//  a 'blocking' line, then disallow the spawn. Only search
//  lines in the blocks of the blockmap where the bounding box
//  of the trajectory line resides. Then check bounding box
//  of the trajectory vs. the bounding box of each blocking
//  line to see if the trajectory and the blocking line cross.
//  Then check the PE and LS to see if they're on different
//  sides of the blocking line. If so, return true, otherwise
//  false.
//
//==========================================================================
bool VEntity::CheckSides (TVec lsPos) {
  // here is the bounding box of the trajectory
  float tmbbox[4];
  tmbbox[BOX2D_LEFT] = min2(Origin.x, lsPos.x);
  tmbbox[BOX2D_RIGHT] = max2(Origin.x, lsPos.x);
  tmbbox[BOX2D_TOP] = max2(Origin.y, lsPos.y);
  tmbbox[BOX2D_BOTTOM] = min2(Origin.y, lsPos.y);

  // determine which blocks to look in for blocking lines
  const int xl = MapBlock(tmbbox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
  const int xh = MapBlock(tmbbox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
  const int yl = MapBlock(tmbbox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
  const int yh = MapBlock(tmbbox[BOX2D_TOP]-XLevel->BlockMapOrgY);

  //k8: is this right?
  int projblk = (EntityFlags&VEntity::EF_Missile ? ML_BLOCKPROJECTILE : 0);

  // xl->xh, yl->yh determine the mapblock set to search
  //++validcount; // prevents checking same line twice
  XLevel->IncrementValidCount();
  for (int bx = xl; bx <= xh; ++bx) {
    for (int by = yl; by <= yh; ++by) {
      line_t *ld;
      for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
        // Checks to see if a PE->LS trajectory line crosses a blocking
        // line. Returns false if it does.
        //
        // tmbbox holds the bounding box of the trajectory. If that box
        // does not touch the bounding box of the line in question,
        // then the trajectory is not blocked. If the PE is on one side
        // of the line and the LS is on the other side, then the
        // trajectory is blocked.
        //
        // Currently this assumes an infinite line, which is not quite
        // correct. A more correct solution would be to check for an
        // intersection of the trajectory and the line, but that takes
        // longer and probably really isn't worth the effort.

        if (ld->flags&(ML_BLOCKING|ML_BLOCKMONSTERS|ML_BLOCKEVERYTHING|projblk)) {
          if (tmbbox[BOX2D_LEFT] <= ld->bbox2d[BOX2D_RIGHT] &&
              tmbbox[BOX2D_RIGHT] >= ld->bbox2d[BOX2D_LEFT] &&
              tmbbox[BOX2D_TOP] >= ld->bbox2d[BOX2D_BOTTOM] &&
              tmbbox[BOX2D_BOTTOM] <= ld->bbox2d[BOX2D_TOP])
          {
            if (ld->PointOnSide(Origin) != ld->PointOnSide(lsPos)) return true; // line blocks trajectory
          }
        }

        // line doesn't block trajectory
      }
    }
  }

  return false;
}


//==========================================================================
//
//  VEntity::FixMapthingPos
//
//  if the thing is exactly on a line, move it into the sector
//  slightly in order to resolve clipping issues in the renderer
//
//  code adapted from GZDoom
//
//==========================================================================
bool VEntity::FixMapthingPos () {
  sector_t *sec = XLevel->PointInSubsector_Buggy(Origin)->sector; // here buggy original should be used!

  bool res = false;

  // here is the bounding box of the trajectory
  float tmbbox[4];
  tmbbox[BOX2D_TOP] = Origin.y+Radius;
  tmbbox[BOX2D_BOTTOM] = Origin.y-Radius;
  tmbbox[BOX2D_RIGHT] = Origin.x+Radius;
  tmbbox[BOX2D_LEFT] = Origin.x-Radius;

  // determine which blocks to look in for blocking lines
  // determine which blocks to look in for blocking lines
  const int xl = MapBlock(tmbbox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
  const int xh = MapBlock(tmbbox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
  const int yl = MapBlock(tmbbox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
  const int yh = MapBlock(tmbbox[BOX2D_TOP]-XLevel->BlockMapOrgY);

  // xl->xh, yl->yh determine the mapblock set to search
  //++validcount; // prevents checking same line twice
  XLevel->IncrementValidCount();
  for (int bx = xl; bx <= xh; ++bx) {
    for (int by = yl; by <= yh; ++by) {
      line_t *ld;
      for (VBlockLinesIterator It(XLevel, bx, by, &ld); It.GetNext(); ) {
        if (ld->frontsector == ld->backsector) continue; // skip two-sided lines inside a single sector

        // skip two-sided lines without any height difference on either side
        if (ld->frontsector && ld->backsector) {
          if (ld->frontsector->floor.minz == ld->backsector->floor.minz &&
              ld->frontsector->floor.maxz == ld->backsector->floor.maxz &&
              ld->frontsector->ceiling.minz == ld->backsector->ceiling.minz &&
              ld->frontsector->ceiling.maxz == ld->backsector->ceiling.maxz)
          {
            continue;
          }
        }

        // check line bounding box for early out
        if (tmbbox[BOX2D_RIGHT] <= ld->bbox2d[BOX2D_LEFT] ||
            tmbbox[BOX2D_LEFT] >= ld->bbox2d[BOX2D_RIGHT] ||
            tmbbox[BOX2D_TOP] <= ld->bbox2d[BOX2D_BOTTOM] ||
            tmbbox[BOX2D_BOTTOM] >= ld->bbox2d[BOX2D_TOP])
        {
          continue;
        }

        // get the exact distance to the line
        float linelen = ld->dir.length();
        if (linelen < 0.0001f) continue; // just in case

        //divline_t dll, dlv;
        //P_MakeDivline(ld, &dll);
        float dll_x = ld->v1->x;
        float dll_y = ld->v1->y;
        float dll_dx = ld->dir.x;
        float dll_dy = ld->dir.y;

        float dlv_x = Origin.x;
        float dlv_y = Origin.y;
        float dlv_dx = dll_dy/linelen;
        float dlv_dy = -dll_dx/linelen;

        //double distance = fabs(P_InterceptVector(&dlv, &dll));
        float distance = 0;
        {
          const double v1x = dll_x;
          const double v1y = dll_y;
          const double v1dx = dll_dx;
          const double v1dy = dll_dy;
          const double v2x = dlv_x;
          const double v2y = dlv_y;
          const double v2dx = dlv_dx;
          const double v2dy = dlv_dy;

          const double den = v1dy*v2dx - v1dx*v2dy;

          if (den == 0) {
            // parallel
            distance = 0;
          } else {
            const double num = (v1x-v2x)*v1dy+(v2y-v1y)*v1dx;
            distance = num/den;
          }
        }

        if (distance < Radius) {
          /*
          float angle = matan(ld->dir.y, ld->dir.x);
          angle += (ld->backsector && ld->backsector == sec ? 90 : -90);
          // get the distance we have to move the object away from the wall
          distance = Radius-distance;
          UnlinkFromWorld();
          Origin += AngleVectorYaw(angle)*distance;
          LinkToWorld();
          */

          //k8: we already have a normal to a wall, let's use it instead
          TVec movedir = ld->normal;
          if (ld->backsector && ld->backsector == sec) movedir = -movedir;
          // get the distance we have to move the object away from the wall
          distance = Radius-distance;
          UnlinkFromWorld();
          Origin += movedir*distance;
          LinkToWorld();

          res = true;
        }
      }
    }
  }

  return res;
}


//=============================================================================
//
//  CheckDropOff
//
//  killough 11/98:
//
//  Monsters try to move away from tall dropoffs.
//
//  In Doom, they were never allowed to hang over dropoffs, and would remain
//  stuck if involuntarily forced over one. This logic, combined with P_TryMove,
//  allows monsters to free themselves without making them tend to hang over
//  dropoffs.
//
//=============================================================================
void VEntity::CheckDropOff (float &DeltaX, float &DeltaY, float baseSpeed) {
  float t_bbox[4];

  // try to move away from a dropoff
  DeltaX = DeltaY = 0;

  t_bbox[BOX2D_TOP] = Origin.y+Radius;
  t_bbox[BOX2D_BOTTOM] = Origin.y-Radius;
  t_bbox[BOX2D_RIGHT] = Origin.x+Radius;
  t_bbox[BOX2D_LEFT] = Origin.x-Radius;

  const int xl = MapBlock(t_bbox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
  const int xh = MapBlock(t_bbox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
  const int yl = MapBlock(t_bbox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
  const int yh = MapBlock(t_bbox[BOX2D_TOP]-XLevel->BlockMapOrgY);

  // check lines
  //++validcount;
  XLevel->IncrementValidCount();
  for (int bx = xl; bx <= xh; ++bx) {
    for (int by = yl; by <= yh; ++by) {
      line_t *line;
      for (VBlockLinesIterator It(XLevel, bx, by, &line); It.GetNext(); ) {
        if (!line->backsector) continue; // ignore one-sided linedefs
        // linedef must be contacted
        if (t_bbox[BOX2D_RIGHT] > line->bbox2d[BOX2D_LEFT] &&
            t_bbox[BOX2D_LEFT] < line->bbox2d[BOX2D_RIGHT] &&
            t_bbox[BOX2D_TOP] > line->bbox2d[BOX2D_BOTTOM] &&
            t_bbox[BOX2D_BOTTOM] < line->bbox2d[BOX2D_TOP] &&
            P_BoxOnLineSide(t_bbox, line) == -1)
        {
          // new logic for 3D Floors
          /*
          sec_region_t *FrontReg = SV_FindThingGap(line->frontsector, Origin, Height);
          sec_region_t *BackReg = SV_FindThingGap(line->backsector, Origin, Height);
          float front = FrontReg->efloor.GetPointZClamped(Origin);
          float back = BackReg->efloor.GetPointZClamped(Origin);
          */
          TSecPlaneRef ffloor, fceiling;
          TSecPlaneRef bfloor, bceiling;
          SV_FindGapFloorCeiling(line->frontsector, Origin, Height, ffloor, fceiling);
          SV_FindGapFloorCeiling(line->backsector, Origin, Height, bfloor, bceiling);
          const float front = ffloor.GetPointZClamped(Origin);
          const float back = bfloor.GetPointZClamped(Origin);

          // the monster must contact one of the two floors, and the other must be a tall dropoff
          TVec Dir;
          if (back == Origin.z && front < Origin.z-MaxDropoffHeight) {
            // front side dropoff
            Dir = line->normal;
          } else if (front == Origin.z && back < Origin.z-MaxDropoffHeight) {
            // back side dropoff
            Dir = -line->normal;
          } else {
            continue;
          }
          // move away from dropoff at a standard speed
          // multiple contacted linedefs are cumulative (e.g. hanging over corner)
          DeltaX += Dir.x*baseSpeed;
          DeltaY += Dir.y*baseSpeed;
        }
      }
    }
  }
}


//=============================================================================
//
//  FindDropOffLines
//
//  find dropoff lines (the same as `CheckDropOff()` is using)
//
//=============================================================================
int VEntity::FindDropOffLine (TArray<VDropOffLineInfo> *list, TVec pos) {
  int res = 0;
  float t_bbox[4];

  t_bbox[BOX2D_TOP] = Origin.y+Radius;
  t_bbox[BOX2D_BOTTOM] = Origin.y-Radius;
  t_bbox[BOX2D_RIGHT] = Origin.x+Radius;
  t_bbox[BOX2D_LEFT] = Origin.x-Radius;

  const int xl = MapBlock(t_bbox[BOX2D_LEFT]-XLevel->BlockMapOrgX);
  const int xh = MapBlock(t_bbox[BOX2D_RIGHT]-XLevel->BlockMapOrgX);
  const int yl = MapBlock(t_bbox[BOX2D_BOTTOM]-XLevel->BlockMapOrgY);
  const int yh = MapBlock(t_bbox[BOX2D_TOP]-XLevel->BlockMapOrgY);

  // check lines
  //++validcount;
  XLevel->IncrementValidCount();
  for (int bx = xl; bx <= xh; ++bx) {
    for (int by = yl; by <= yh; ++by) {
      line_t *line;
      for (VBlockLinesIterator It(XLevel, bx, by, &line); It.GetNext(); ) {
        if (!line->backsector) continue; // ignore one-sided linedefs
        // linedef must be contacted
        if (t_bbox[BOX2D_RIGHT] > line->bbox2d[BOX2D_LEFT] &&
            t_bbox[BOX2D_LEFT] < line->bbox2d[BOX2D_RIGHT] &&
            t_bbox[BOX2D_TOP] > line->bbox2d[BOX2D_BOTTOM] &&
            t_bbox[BOX2D_BOTTOM] < line->bbox2d[BOX2D_TOP] &&
            P_BoxOnLineSide(t_bbox, line) == -1)
        {
          // new logic for 3D Floors
          /*
          sec_region_t *FrontReg = SV_FindThingGap(line->frontsector, Origin, Height);
          sec_region_t *BackReg = SV_FindThingGap(line->backsector, Origin, Height);
          float front = FrontReg->efloor.GetPointZClamped(Origin);
          float back = BackReg->efloor.GetPointZClamped(Origin);
          */
          TSecPlaneRef ffloor, fceiling;
          TSecPlaneRef bfloor, bceiling;
          SV_FindGapFloorCeiling(line->frontsector, Origin, Height, ffloor, fceiling);
          SV_FindGapFloorCeiling(line->backsector, Origin, Height, bfloor, bceiling);
          const float front = ffloor.GetPointZClamped(Origin);
          const float back = bfloor.GetPointZClamped(Origin);

          // the monster must contact one of the two floors, and the other must be a tall dropoff
          int side;
          if (back == Origin.z && front < Origin.z-MaxDropoffHeight) {
            // front side dropoff
            side = 0;
          } else if (front == Origin.z && back < Origin.z-MaxDropoffHeight) {
            // back side dropoff
            side = 1;
          } else {
            continue;
          }

          ++res;
          if (list) {
            VDropOffLineInfo *la = &list->alloc();
            la->line = line;
            la->side = side;
          }
        }
      }
    }
  }

  return res;
}


//=============================================================================
//
//  VEntity::UpdateVelocity
//
//  called from entity `Physics()`
//
//=============================================================================
void VEntity::UpdateVelocity (float DeltaTime) {
  if (!Sector) return; // just in case

  /*
  if (Origin.z <= FloorZ && !Velocity && !bCountKill && !bIsPlayer) {
    // no gravity for non-moving things on ground to prevent static objects from sliding on slopes
    return;
  }
  */

  // don't add gravity if standing on slope with normal.z > 0.7 (aprox 45 degrees)
  if (!(EntityFlags&EF_NoGravity) && (Origin.z > FloorZ || EFloor.GetNormalZ() <= 0.7f)) {
    if (WaterLevel < 2) {
      Velocity.z -= Gravity*Level->Gravity*Sector->Gravity*DeltaTime;
    } else if (!IsPlayer() || Health <= 0) {
      // water gravity
      Velocity.z -= Gravity*Level->Gravity*Sector->Gravity/10.0f*DeltaTime;
      float startvelz = Velocity.z;
      float sinkspeed = -WaterSinkSpeed/(IsRealCorpse() ? 3.0f : 1.0f);
      if (Velocity.z < sinkspeed) {
        Velocity.z = (startvelz < sinkspeed ? startvelz : sinkspeed);
      } else {
        Velocity.z = startvelz+(Velocity.z-startvelz)*WaterSinkFactor;
      }
    }
  }
}


//==========================================================================
//
//  VRoughBlockSearchIterator
//
//==========================================================================
VRoughBlockSearchIterator::VRoughBlockSearchIterator (VEntity *ASelf, int ADistance, VEntity **AEntPtr)
  : Self(ASelf)
  , Distance(ADistance)
  , Ent(nullptr)
  , EntPtr(AEntPtr)
  , Count(1)
  , CurrentEdge(-1)
{
  StartX = MapBlock(Self->Origin.x-Self->XLevel->BlockMapOrgX);
  StartY = MapBlock(Self->Origin.y-Self->XLevel->BlockMapOrgY);

  // start with current block
  if (StartX >= 0 && StartX < Self->XLevel->BlockMapWidth &&
      StartY >= 0 && StartY < Self->XLevel->BlockMapHeight)
  {
    Ent = Self->XLevel->BlockLinks[StartY*Self->XLevel->BlockMapWidth+StartX];
  }
}


//==========================================================================
//
//  VRoughBlockSearchIterator::GetNext
//
//==========================================================================
bool VRoughBlockSearchIterator::GetNext () {
  int BlockX;
  int BlockY;

  for (;;) {
    if (Ent) {
      *EntPtr = Ent;
      Ent = Ent->BlockMapNext;
      return true;
    }

    switch (CurrentEdge) {
      case 0:
        // trace the first block section (along the top)
        if (BlockIndex <= FirstStop) {
          Ent = Self->XLevel->BlockLinks[BlockIndex];
          ++BlockIndex;
        } else {
          CurrentEdge = 1;
          --BlockIndex;
        }
        break;
      case 1:
        // trace the second block section (right edge)
        if (BlockIndex <= SecondStop) {
          Ent = Self->XLevel->BlockLinks[BlockIndex];
          BlockIndex += Self->XLevel->BlockMapWidth;
        } else {
          CurrentEdge = 2;
          BlockIndex -= Self->XLevel->BlockMapWidth;
        }
        break;
      case 2:
        // trace the third block section (bottom edge)
        if (BlockIndex >= ThirdStop) {
          Ent = Self->XLevel->BlockLinks[BlockIndex];
          --BlockIndex;
        } else {
          CurrentEdge = 3;
          ++BlockIndex;
        }
        break;
      case 3:
        // trace the final block section (left edge)
        if (BlockIndex > FinalStop) {
          Ent = Self->XLevel->BlockLinks[BlockIndex];
          BlockIndex -= Self->XLevel->BlockMapWidth;
        } else {
          CurrentEdge = -1;
        }
        break;
      default:
        if (Count > Distance) return false; // we are done
        BlockX = StartX-Count;
        BlockY = StartY-Count;

        if (BlockY < 0) {
          BlockY = 0;
        } else if (BlockY >= Self->XLevel->BlockMapHeight) {
          BlockY = Self->XLevel->BlockMapHeight-1;
        }
        if (BlockX < 0) {
          BlockX = 0;
        } else if (BlockX >= Self->XLevel->BlockMapWidth) {
          BlockX = Self->XLevel->BlockMapWidth-1;
        }
        BlockIndex = BlockY*Self->XLevel->BlockMapWidth+BlockX;
        FirstStop = StartX+Count;
        if (FirstStop < 0) { ++Count; break; }
        if (FirstStop >= Self->XLevel->BlockMapWidth) FirstStop = Self->XLevel->BlockMapWidth-1;
        SecondStop = StartY+Count;
        if (SecondStop < 0) { ++Count; break; }
        if (SecondStop >= Self->XLevel->BlockMapHeight) SecondStop = Self->XLevel->BlockMapHeight-1;
        ThirdStop = SecondStop*Self->XLevel->BlockMapWidth+BlockX;
        SecondStop = SecondStop*Self->XLevel->BlockMapWidth+FirstStop;
        FirstStop += BlockY*Self->XLevel->BlockMapWidth;
        FinalStop = BlockIndex;
        ++Count;
        CurrentEdge = 0;
        break;
    }
  }
  return false;
}


//==========================================================================
//
//  Script natives
//
//==========================================================================
IMPLEMENT_FUNCTION(VEntity, CheckWater) {
  vobjGetParamSelf();
  Self->CheckWater();
}

IMPLEMENT_FUNCTION(VEntity, CheckDropOff) {
  float *DeltaX;
  float *DeltaY;
  VOptParamFloat baseSpeed(32.0f);
  vobjGetParamSelf(DeltaX, DeltaY, baseSpeed);
  Self->CheckDropOff(*DeltaX, *DeltaY, baseSpeed);
}

// native final int FindDropOffLine (ref array!VDropOffLineInfo list, TVec pos);
IMPLEMENT_FUNCTION(VEntity, FindDropOffLine) {
  TArray<VDropOffLineInfo> *list;
  TVec pos;
  vobjGetParamSelf(list, pos);
  RET_INT(Self->FindDropOffLine(list, pos));
}

IMPLEMENT_FUNCTION(VEntity, CheckPosition) {
  TVec pos;
  vobjGetParamSelf(pos);
  RET_BOOL(Self->CheckPosition(pos));
}

// native final bool CheckRelPosition (out tmtrace_t tmtrace, TVec Pos, optional bool noPickups/*=false*/, optional bool ignoreMonsters, optional bool ignorePlayers);
IMPLEMENT_FUNCTION(VEntity, CheckRelPosition) {
  tmtrace_t tmp;
  tmtrace_t *tmtrace = nullptr;
  TVec Pos;
  VOptParamBool noPickups(false);
  VOptParamBool ignoreMonsters(false);
  VOptParamBool ignorePlayers(false);
  vobjGetParamSelf(tmtrace, Pos, noPickups, ignoreMonsters, ignorePlayers);
  if (!tmtrace) tmtrace = &tmp;
  RET_BOOL(Self->CheckRelPosition(*tmtrace, Pos, noPickups, ignoreMonsters, ignorePlayers));
}

IMPLEMENT_FUNCTION(VEntity, CheckSides) {
  TVec lsPos;
  vobjGetParamSelf(lsPos);
  RET_BOOL(Self->CheckSides(lsPos));
}

IMPLEMENT_FUNCTION(VEntity, FixMapthingPos) {
  vobjGetParamSelf();
  RET_BOOL(Self->FixMapthingPos());
}

IMPLEMENT_FUNCTION(VEntity, TryMove) {
  tmtrace_t tmtrace;
  TVec Pos;
  bool AllowDropOff;
  VOptParamBool checkOnly(false);
  vobjGetParamSelf(Pos, AllowDropOff, checkOnly);
  RET_BOOL(Self->TryMove(tmtrace, Pos, AllowDropOff, checkOnly));
}

IMPLEMENT_FUNCTION(VEntity, TryMoveEx) {
  tmtrace_t tmp;
  tmtrace_t *tmtrace = nullptr;
  TVec Pos;
  bool AllowDropOff;
  VOptParamBool checkOnly(false);
  vobjGetParamSelf(tmtrace, Pos, AllowDropOff, checkOnly);
  if (!tmtrace) tmtrace = &tmp;
  RET_BOOL(Self->TryMove(*tmtrace, Pos, AllowDropOff, checkOnly));
}

IMPLEMENT_FUNCTION(VEntity, TestMobjZ) {
  vobjGetParamSelf();
  RET_BOOL(!Self->TestMobjZ(Self->Origin));
}

IMPLEMENT_FUNCTION(VEntity, SlideMove) {
  float StepVelScale;
  VOptParamBool noPickups(false);
  vobjGetParamSelf(StepVelScale, noPickups);
  Self->SlideMove(StepVelScale, noPickups);
}

IMPLEMENT_FUNCTION(VEntity, BounceWall) {
  float overbounce;
  float bouncefactor;
  vobjGetParamSelf(overbounce, bouncefactor);
  Self->BounceWall(overbounce, bouncefactor);
}

IMPLEMENT_FUNCTION(VEntity, CheckOnmobj) {
  vobjGetParamSelf();
  RET_REF(Self->CheckOnmobj());
}

IMPLEMENT_FUNCTION(VEntity, LinkToWorld) {
  VOptParamInt properFloorCheck(0);
  vobjGetParamSelf(properFloorCheck);
  Self->LinkToWorld(properFloorCheck);
}

IMPLEMENT_FUNCTION(VEntity, UnlinkFromWorld) {
  vobjGetParamSelf();
  Self->UnlinkFromWorld();
}

IMPLEMENT_FUNCTION(VEntity, CanSee) {
  VEntity *Other;
  VOptParamBool disableBetterSight(false);
  vobjGetParamSelf(Other, disableBetterSight);
  if (!Self) { VObject::VMDumpCallStack(); Sys_Error("empty `self`!"); }
  RET_BOOL(Self->CanSee(Other, disableBetterSight));
}

IMPLEMENT_FUNCTION(VEntity, CanSeeAdv) {
  VEntity *Other;
  vobjGetParamSelf(Other);
  if (!Self) { VObject::VMDumpCallStack(); Sys_Error("empty `self`!"); }
  RET_BOOL(Self->CanSee(Other, false, true));
}

IMPLEMENT_FUNCTION(VEntity, CanShoot) {
  VEntity *Other;
  vobjGetParamSelf(Other);
  if (!Self) { VObject::VMDumpCallStack(); Sys_Error("empty `self`!"); }
  RET_BOOL(Self->CanShoot(Other));
}

IMPLEMENT_FUNCTION(VEntity, RoughBlockSearch) {
  VEntity **EntPtr;
  int Distance;
  vobjGetParamSelf(EntPtr, Distance);
  RET_PTR(new VRoughBlockSearchIterator(Self, Distance, EntPtr));
}


// native void UpdateVelocity (float DeltaTime);
IMPLEMENT_FUNCTION(VEntity, UpdateVelocity) {
  float DeltaTime;
  vobjGetParamSelf(DeltaTime);
  Self->UpdateVelocity(DeltaTime);
}


// native final float GetBlockingHeightFor (Entity other);
IMPLEMENT_FUNCTION(VEntity, GetBlockingHeightFor) {
  VEntity *other;
  vobjGetParamSelf(other);
  RET_FLOAT(other ? Self->GetBlockingHeightFor(other) : 0.0f);
}
