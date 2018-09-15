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
#include "glvisint.h"
#include "../../libs/core/core.h"

namespace VavoomUtils {

#include "fmapdefs.h"

#ifdef WIN32
# define StrCmpCI  _stricmp
#else
# define StrCmpCI  stricmp
#endif


//==========================================================================
//
//  GLVisMalloc
//
//==========================================================================
/*
static void *GLVisMalloc(size_t size)
{
  if (!size)
  {
    size = 4;
  }
  size = (size + 3) & ~3;
  void *ptr = Z_Malloc(size);
  if (!ptr)
  {
    throw GLVisError("Couldn't alloc %d bytes", size);
  }
  memset(ptr, 0, size);
  return ptr;
}
*/


//==========================================================================
//
//  GLVisFree
//
//==========================================================================
/*
static void GLVisFree(void *ptr)
{
  Z_Free(ptr);
  ptr = NULL;
}
*/


//==========================================================================
//
//  TVisBuilder::TVisBuilder
//
//==========================================================================
TVisBuilder::TVisBuilder(TGLVis &AOwner)
  : Owner(AOwner)
  , numvertexes(0)
  , vertexes(nullptr)
  , gl_vertexes(nullptr)
  , numsegs(0)
  , segs(nullptr)
  , numsubsectors(0)
  , subsectors(nullptr)
  , numportals(0)
  , portals(nullptr)
  , vissize(0)
  , vis(nullptr)
  , bitbytes(0)
  , bitlongs(0)
  , portalsee(0)
  , c_leafsee(0)
  , c_portalsee(0)
  , c_chains(0)
  , c_portalskip(0)
  , c_leafskip(0)
  , c_vistest(0)
  , c_mighttest(0)
  , c_portaltest(0)
  , c_portalpass(0)
  , c_portalcheck(0)
  , totalvis(0)
  , rowbytes(0)
{
}


//==========================================================================
//
//  TVisBuilder::LoadVertexes
//
//==========================================================================
void TVisBuilder::LoadVertexes (int lump, int gl_lump) {
  int i;
  void *data;
  void *gldata;
  mapvertex_t *ml;
  vertex_t *li;
  int base_verts;
  int gl_verts;

  base_verts = mainwad->LumpSize(lump)/sizeof(mapvertex_t);

  gldata = glwad->GetLump(gl_lump);
  if (!strncmp((char *)gldata, GL_V5_MAGIC, 4)) GLNodesV5 = true;
  if (GLNodesV5 || !strncmp((char *)gldata, GL_V2_MAGIC, 4)) {
    gl_verts = (glwad->LumpSize(gl_lump)-4) / sizeof(gl_mapvertex_t);
  } else {
    gl_verts = glwad->LumpSize(gl_lump)/sizeof(mapvertex_t);
  }
  numvertexes = base_verts+gl_verts;

  // allocate zone memory for buffer
  li = vertexes = new vertex_t[numvertexes];

  // load data into cache
  data = mainwad->GetLump(lump);
  ml = (mapvertex_t *)data;

  // copy and convert vertex, internal representation as vector
  for (i = 0; i < base_verts; i++, li++, ml++) {
    *li = TVec2D(LittleShort(ml->x), LittleShort(ml->y));
  }

  // free buffer memory
  Z_Free(data);

  // save pointer to GL vertexes for seg loading
  gl_vertexes = li;

  if (GLNodesV5 || !strncmp((char *)gldata, GL_V2_MAGIC, 4)) {
    gl_mapvertex_t *glml;

    glml = (gl_mapvertex_t *)((vuint8 *)gldata+4);

    // copy and convert vertex, internal representation as vector
    for (i = 0; i < gl_verts; ++i, ++li, ++glml) {
      *li = TVec2D((double)LittleLong(glml->x)/(double)0x10000,
                   (double)LittleLong(glml->y)/(double)0x10000);
    }
  } else {
    ml = (mapvertex_t *)gldata;
    // copy and convert vertex, internal representation as vector
    for (i = 0; i < gl_verts; ++i, ++li, ++ml) {
      *li = TVec2D(LittleShort(ml->x), LittleShort(ml->y));
    }
  }

  // free buffer memory
  Z_Free(gldata);
}


//==========================================================================
//
//  TVisBuilder::LoadSectors
//
//==========================================================================
void TVisBuilder::LoadSectors (int lump) {
  numsectors = mainwad->LumpSize(lump)/sizeof(mapsector_t);
}


//==========================================================================
//
//  TVisBuilder::LoadSideDefs
//
//==========================================================================
void TVisBuilder::LoadSideDefs (int lump) {
  numsides = mainwad->LumpSize(lump)/sizeof(mapsidedef_t);
  sidesecs = new vint32[numsides];
  mapsidedef_t *data = (mapsidedef_t *)mainwad->GetLump(lump);
  for (int i = 0; i < numsides; ++i) sidesecs[i] = LittleShort(data[i].sector);
  Z_Free(data);
}


//==========================================================================
//
//  TVisBuilder::LoadLineDefs1
//
//  For Doom and Heretic
//
//==========================================================================
void TVisBuilder::LoadLineDefs1 (int lump) {
  numlines = mainwad->LumpSize(lump)/sizeof(maplinedef1_t);
  lines = new line_t[numlines];
  void *data = mainwad->GetLump(lump);

  maplinedef1_t *mld = (maplinedef1_t *)data;
  line_t *ld = lines;
  for (int i = 0; i < numlines; ++i, ++mld, ++ld) {
    int side0 = LittleShort(mld->sidenum[0]);
    int side1 = LittleShort(mld->sidenum[1]);

    if (side0 != -1) {
      ld->secnum[0] = sidesecs[side0];
    } else {
      ld->secnum[0] = -1;
    }

    if (side1 != -1) {
      ld->secnum[1] = sidesecs[side1];
    } else {
      ld->secnum[1] = -1;
    }
  }

  Z_Free(data);
}


//==========================================================================
//
//  TVisBuilder::LoadLineDefs2
//
//  For Hexen
//
//==========================================================================
void TVisBuilder::LoadLineDefs2 (int lump) {
  numlines = mainwad->LumpSize(lump) / sizeof(maplinedef2_t);
  lines = new line_t[numlines];
  void *data = mainwad->GetLump(lump);

  maplinedef2_t *mld = (maplinedef2_t *)data;
  line_t *ld = lines;
  for (int i = 0; i < numlines; ++i, ++mld, ++ld) {
    int side0 = LittleShort(mld->sidenum[0]);
    int side1 = LittleShort(mld->sidenum[1]);

    if (side0 != -1) {
      ld->secnum[0] = sidesecs[side0];
    } else {
      ld->secnum[0] = -1;
    }

    if (side1 != -1) {
      ld->secnum[1] = sidesecs[side1];
    } else {
      ld->secnum[1] = -1;
    }
  }

  Z_Free(data);
}


//==========================================================================
//
//  TVisBuilder::LoadSegs
//
//==========================================================================
void TVisBuilder::LoadSegs (int lump) {
  void *data = glwad->GetLump(lump);
  if (GLNodesV5 || !strncmp((char*)data, GL_V3_MAGIC, 4)) {
    int HdrSize = (GLNodesV5 ? 0 : 4);
    vuint32 GLVertFlag = (GLNodesV5 ? GL_VERTEX_V5 : GL_VERTEX_V3);

    numsegs = (glwad->LumpSize(lump)-HdrSize)/sizeof(mapglseg_v3_t);
    segs = new seg_t[numsegs];
    memset((void *)segs, 0, sizeof(seg_t)*numsegs);

    mapglseg_v3_t *ml = (mapglseg_v3_t *)((vuint8 *)data+HdrSize);
    seg_t *li = segs;
    numportals = 0;

    for (int i = 0; i < numsegs; ++i, ++li, ++ml) {
      vuint32 v1num = LittleLong(ml->v1);
      vuint32 v2num = LittleLong(ml->v2);

      if (v1num&GLVertFlag) {
        v1num ^= GLVertFlag;
        li->v1 = &gl_vertexes[v1num];
      } else {
        li->v1 = &vertexes[v1num];
      }
      if (v2num&GLVertFlag) {
        v2num ^= GLVertFlag;
        li->v2 = &gl_vertexes[v2num];
      } else {
        li->v2 = &vertexes[v2num];
      }

      int partner = LittleLong(ml->partner);
      if (partner >= 0) {
        li->partner = &segs[partner];
        ++numportals;
      }

      int linenum = LittleShort(ml->linedef);
      if (linenum >= 0) {
        li->secnum = lines[linenum].secnum[LittleShort(ml->flags)&GL_SEG_FLAG_SIDE];
      } else {
        li->secnum = -1;
      }

      // calc seg's plane params
      li->Set2Points(*li->v1, *li->v2);
    }
  } else {
    numsegs = glwad->LumpSize(lump)/sizeof(mapglseg_t);
    segs = new seg_t[numsegs];
    memset((void *)segs, 0, sizeof(seg_t)*numsegs);

    mapglseg_t *ml = (mapglseg_t *)data;
    seg_t *li = segs;
    numportals = 0;

    for (int i = 0; i < numsegs; ++i, ++li, ++ml) {
      vuint16 v1num = LittleShort(ml->v1);
      vuint16 v2num = LittleShort(ml->v2);

      if (v1num&GL_VERTEX) {
        v1num ^= GL_VERTEX;
        li->v1 = &gl_vertexes[v1num];
      } else {
        li->v1 = &vertexes[v1num];
      }
      if (v2num&GL_VERTEX) {
        v2num ^= GL_VERTEX;
        li->v2 = &gl_vertexes[v2num];
      } else {
        li->v2 = &vertexes[v2num];
      }

      int partner = LittleShort(ml->partner);
      if (partner >= 0) {
        li->partner = &segs[partner];
        ++numportals;
      }

      int linenum = LittleShort(ml->linedef);
      if (linenum >= 0) {
        li->secnum = lines[linenum].secnum[LittleShort(ml->side)];
      } else {
        li->secnum = -1;
      }

      // calc seg's plane params
      li->Set2Points(*li->v1, *li->v2);
    }
  }

  Z_Free(data);

  portals = new portal_t[numportals];
  memset((void *)portals, 0, sizeof(portal_t)*numportals);
}


//==========================================================================
//
//  TVisBuilder::LoadSubsectors
//
//==========================================================================
void TVisBuilder::LoadSubsectors (int lump) {
  void *data = glwad->GetLump(lump);
  if (GLNodesV5 || !strncmp((char *)data, GL_V3_MAGIC, 4)) {
    int HdrSize = (GLNodesV5 ? 0 : 4);

    numsubsectors = (glwad->LumpSize(lump)-HdrSize)/sizeof(mapglsubsector_v3_t);
    subsectors = new subsector_t[numsubsectors];
    memset((void *)subsectors, 0, sizeof(subsector_t)*numsubsectors);

    mapglsubsector_v3_t *ms = (mapglsubsector_v3_t *)((vuint8 *)data+HdrSize);
    subsector_t *ss = subsectors;

    for (int i = 0; i < numsubsectors; ++i, ++ss, ++ms) {
      // set seg subsector links
      int count = LittleLong(ms->numsegs);
      seg_t *line = &segs[LittleLong(ms->firstseg)];
      ss->secnum = -1;
      while (count--) {
        line->leaf = i;
        if (ss->secnum == -1 && line->secnum >= 0) {
          ss->secnum = line->secnum;
        } else if (ss->secnum != -1 && line->secnum >= 0 && ss->secnum != line->secnum) {
          Owner.DisplayMessage("Segs from different sectors\n");
        }
        ++line;
      }
      if (ss->secnum == -1) throw GLVisError("Subsector without sector");
    }
  } else {
    numsubsectors = glwad->LumpSize(lump)/sizeof(mapsubsector_t);
    subsectors = new subsector_t[numsubsectors];
    memset((void *)subsectors, 0, sizeof(subsector_t)*numsubsectors);

    mapsubsector_t *ms = (mapsubsector_t *)data;
    subsector_t *ss = subsectors;

    for (int i = 0; i < numsubsectors; ++i, ++ss, ++ms) {
      // set seg subsector links
      int count = (vuint16)LittleShort(ms->numsegs);
      seg_t *line = &segs[(vuint16)LittleShort(ms->firstseg)];
      ss->secnum = -1;
      while (count--) {
        line->leaf = i;
        if (ss->secnum == -1 && line->secnum >= 0) {
          ss->secnum = line->secnum;
        } else if (ss->secnum != -1 && line->secnum >= 0 && ss->secnum != line->secnum) {
          Owner.DisplayMessage("Segs from different sectors\n");
        }
        ++line;
      }
      if (ss->secnum == -1) throw GLVisError("Subsector without sector");
    }
  }

  Z_Free(data);
}


//==========================================================================
//
//  TVisBuilder::CreatePortals
//
//==========================================================================
void TVisBuilder::CreatePortals () {
  portal_t *p = portals;
  for (int i = 0; i < numsegs; ++i) {
    seg_t *line = &segs[i];
    subsector_t *sub = &subsectors[line->leaf];
    if (line->partner) {
      // skip self-referencing subsector segs
      if (line->leaf == line->partner->leaf) {
        Owner.DisplayMessage("Self-referencing subsector detected\n");
        --numportals;
        continue;
      }

      // create portal
      if (sub->numportals == MAX_PORTALS_ON_LEAF) throw GLVisError("Leaf with too many portals");
      sub->portals[sub->numportals] = p;
      ++sub->numportals;

      p->winding.original = true;
      p->winding.points[0] = *line->v1;
      p->winding.points[1] = *line->v2;
      p->normal = line->partner->normal;
      p->dist = line->partner->dist;
      p->leaf = line->partner->leaf;
      ++p;
    }
  }
  if (p-portals != numportals) throw GLVisError("Portals miscounted");
}


//==========================================================================
//
//  TVisBuilder::LoadLevel
//
//==========================================================================
void TVisBuilder::LoadLevel (int lumpnum, int gl_lumpnum) {
  const char *levelname = mainwad->LumpName(lumpnum);
  bool extended = !StrCmpCI(mainwad->LumpName(lumpnum + ML_BEHAVIOR), "BEHAVIOR");

  GLNodesV5 = false;
  LoadVertexes(lumpnum+ML_VERTEXES, gl_lumpnum+ML_GL_VERT);
  LoadSectors(lumpnum+ML_SECTORS);
  LoadSideDefs(lumpnum+ML_SIDEDEFS);
  if (extended) {
    LoadLineDefs2(lumpnum+ML_LINEDEFS);
  } else {
    LoadLineDefs1(lumpnum+ML_LINEDEFS);
  }
  LoadSegs(gl_lumpnum+ML_GL_SEGS);
  LoadSubsectors(gl_lumpnum+ML_GL_SSECT);
  reject = nullptr;

  CreatePortals();

  Owner.DisplayMessage(
    "\nLoaded %s(%c), %d vertexes, %d segs, %d subsectors, %d portals\n",
    levelname, (IsInverseMode(levelname) ? 'I' : 'n'), numvertexes, numsegs, numsubsectors, numportals);
  Owner.DisplayStartMap(levelname);
}


//==========================================================================
//
//  TVisBuilder::FreeLevel
//
//==========================================================================
void TVisBuilder::FreeLevel () {
  delete[] vertexes;
  delete[] sidesecs;
  delete[] lines;
  delete[] segs;
  delete[] subsectors;
  delete[] portals;
  delete[] vis;
  vertexes = nullptr;
  sidesecs = nullptr;
  lines = nullptr;
  segs = nullptr;
  subsectors = nullptr;
  portals = nullptr;
  vis = nullptr;
  if (reject) {
    delete[] reject;
    reject = nullptr;
  }
}


//==========================================================================
//
//  TVisBuilder::IsInverseMode
//
//==========================================================================
bool TVisBuilder::IsInverseMode (const char *levname) {
  if (Owner.num_inv_mode_maps < 1 || !levname || !levname[0]) return false;
  for (int i = 0; i < Owner.num_inv_mode_maps; ++i) {
    if (!StrCmpCI(Owner.inv_mode_maps[i], levname)) return true;
  }
  return false;
}


//==========================================================================
//
//  TVisBuilder::IsLevelName
//
//==========================================================================
bool TVisBuilder::IsLevelName (int lump) {
  if (lump+10 >= glwad->numlumps) return false; //k8: 10?!

  const char *name = glwad->LumpName(lump);

  // first, process rejections
  if (Owner.num_skip_maps > 0) {
    for (int i = 0; i < Owner.num_skip_maps; ++i) {
      if (!StrCmpCI(Owner.skip_maps[i], name)) return false;
    }
  }

  if (Owner.num_specified_maps > 0) {
    for (int i = 0; i < Owner.num_specified_maps; ++i) {
      if (!StrCmpCI(Owner.specified_maps[i], name)) return true;
    }
  } else {
    if (!strcmp(glwad->LumpName(lump+1), "THINGS") &&
        !strcmp(glwad->LumpName(lump+2), "LINEDEFS") &&
        !strcmp(glwad->LumpName(lump+3), "SIDEDEFS") &&
        !strcmp(glwad->LumpName(lump+4), "VERTEXES"))
    {
      return true;
    }
  }

  return false;
}


//==========================================================================
//
//  TVisBuilder::IsGLLevelName
//
//==========================================================================
bool TVisBuilder::IsGLLevelName (int lump) {
  if (lump+4 >= glwad->numlumps) return false;

  const char* name = glwad->LumpName(lump);

  if (name[0] != 'G' || name[1] != 'L' || name[2] != '_') return false;

  // first, process rejections
  if (Owner.num_skip_maps > 0) {
    for (int i = 0; i < Owner.num_skip_maps; ++i) {
      if (!StrCmpCI(Owner.skip_maps[i], name+3)) return false;
    }
  }

  if (Owner.num_specified_maps) {
    for (int i = 0; i < Owner.num_specified_maps; ++i) {
      if (!StrCmpCI(Owner.specified_maps[i], name+3)) return true;
    }
  } else {
    if (!strcmp(glwad->LumpName(lump+1), "GL_VERT") &&
        !strcmp(glwad->LumpName(lump+2), "GL_SEGS") &&
        !strcmp(glwad->LumpName(lump+3), "GL_SSECT") &&
        !strcmp(glwad->LumpName(lump+4), "GL_NODES"))
    {
      return true;
    }
  }

  return false;
}


//==========================================================================
//
//  TVisBuilder::CopyLump
//
//==========================================================================
void TVisBuilder::CopyLump (int i) {
  void *ptr = glwad->GetLump(i);
  outwad.AddLump(glwad->LumpName(i), ptr, glwad->LumpSize(i));
  Z_Free(ptr);
}


//==========================================================================
//
//  TVisBuilder::BuildGWA
//
//==========================================================================
void TVisBuilder::BuildGWA () {
  int i = 0;
  while (i < glwad->numlumps) {
    if (IsGLLevelName(i)) {
      const char *name = glwad->LumpName(i);
      char LevName[12];
      if (!StrCmpCI(name, "GL_LEVEL")) {
        char *Ptr = (char *)glwad->GetLump(i);
        int Len = glwad->LumpSize(i);
        if (Len < 12 || memcmp(Ptr, "LEVEL=", 6)) {
          Z_Free(Ptr);
          throw GLVisError("Bad GL_LEVEL format");
        }
        int EndCh = 12;
        while (EndCh < Len && EndCh < 14 && Ptr[EndCh] != '\n' && Ptr[EndCh] != '\r') ++EndCh;
        memcpy(LevName, Ptr+6, EndCh-6);
        LevName[EndCh-6] = 0;
        Z_Free(Ptr);
      } else {
        strcpy(LevName, name+3);
      }

      LoadLevel(mainwad->LumpNumForName(LevName), i);
      bool oldfast = Owner.fastvis;
      if (IsInverseMode(LevName)) Owner.fastvis = !Owner.fastvis;
      BuildPVS();
      Owner.fastvis = oldfast;

      // write lumps
      for (int ii = 0; ii < 5; ++ii) CopyLump(i+ii);
      i += 5;
      if (!strcmp("GL_PVS", glwad->LumpName(i))) ++i;
      outwad.AddLump("GL_PVS", vis, vissize);

      FreeLevel();
    } else {
      CopyLump(i);
      ++i;
    }
  }
}


//==========================================================================
//
//  TVisBuilder::BuildWAD
//
//==========================================================================
void TVisBuilder::BuildWAD () {
  int i = 0;
  while (i < glwad->numlumps) {
    if (IsLevelName(i)) {
      const char *name = glwad->LumpName(i);
      int gl_lump;
      if (strlen(name) < 6) {
        gl_lump = mainwad->LumpNumForName(va("GL_%s", name));
      } else {
        gl_lump = -1;
        for (int gli = 0; gli < mainwad->numlumps; ++gli) {
          if (!StrCmpCI(mainwad->LumpName(gli), "GL_LEVEL")) {
            char *Ptr = (char*)mainwad->GetLump(gli);
            int Len = mainwad->LumpSize(gli);
            if (Len < 12 || memcmp(Ptr, "LEVEL=", 6)) {
              Z_Free(Ptr);
              throw GLVisError("Bad GL_LEVEL format");
            }
            int EndCh = 12;
            while (EndCh < Len && EndCh < 14 && Ptr[EndCh] != '\n' && Ptr[EndCh] != '\r') ++EndCh;
            char LevName[12];
            memcpy(LevName, Ptr+6, EndCh-6);
            LevName[EndCh-6] = 0;
            if (!StrCmpCI(LevName, name)) {
              gl_lump = gli;
              break;
            }
            Z_Free(Ptr);
          }
        }
        if (gl_lump == -1) {
          gl_lump = mainwad->LumpNumForName(va("GL_%s", name));
        }
      }

      LoadLevel(i, gl_lump);
      bool oldfast = Owner.fastvis;
      if (IsInverseMode(glwad->LumpName(i))) Owner.fastvis = !Owner.fastvis;
      BuildPVS();
      Owner.fastvis = oldfast;
      if (!Owner.no_reject) BuildReject();

      //  Write lumps. Assume GL-nodes immediately follows map data
      for (int mi = i; mi < gl_lump+5; ++mi) {
        if (mi == i+ML_REJECT && !Owner.no_reject) {
          outwad.AddLump("REJECT", reject, rejectsize);
        } else {
          CopyLump(mi);
        }
      }
      i = gl_lump+5;
      if (!strcmp("GL_PVS", glwad->LumpName(i))) ++i;
      outwad.AddLump("GL_PVS", vis, vissize);

      FreeLevel();
    } else {
      CopyLump(i);
      ++i;
    }
  }
}


//==========================================================================
//
//  TVisBuilder::Run
//
//==========================================================================
void TVisBuilder::Run (const char *srcfile, const char *gwafile) {
  char filename[1024];
  char destfile[1024];
  char tempfile[1024];
  char bakext[8];

  strcpy(filename, srcfile);
  DefaultExtension(filename, ".wad");
  strcpy(destfile, filename);
  inwad.Open(filename);
  mainwad = &inwad;

  if (gwafile) {
    strcpy(filename, gwafile);
  } else {
    StripExtension(filename);
    strcat(filename, ".gwa");
  }

  FILE *ff = fopen(filename, "rb");
  if (ff) {
    fclose(ff);
    gwa.Open(filename);
    glwad = &gwa;
    strcpy(destfile, filename);
    strcpy(bakext, ".~gw");
  } else {
    glwad = &inwad;
    strcpy(bakext, ".~wa");
  }

  strcpy(tempfile, destfile);
  StripFilename(tempfile);
  if (tempfile[0]) strcat(tempfile, "/");
  strcat(tempfile, "$glvis$$.$$$");
  outwad.Open(tempfile, glwad->wadid);

  // process lumps
  if (mainwad == glwad) {
    BuildWAD();
  } else {
    BuildGWA();
  }

  inwad.Close();
  if (gwa.handle) gwa.Close();
  outwad.Close();

  strcpy(filename, destfile);
  StripExtension(filename);
  strcat(filename, bakext);
  remove(filename);
  rename(destfile, filename);
  rename(tempfile, destfile);
}

} // namespace VavoomUtils


using namespace VavoomUtils;

//==========================================================================
//
//  GLVisError::GLVisError
//
//==========================================================================
GLVisError::GLVisError (const char *error, ...) {
  va_list argptr;
  va_start(argptr, error);
  vsprintf(message, error, argptr);
  va_end(argptr);
}


//==========================================================================
//
//  TGLVis::Build
//
//==========================================================================
void TGLVis::Build (const char *srcfile, const char *gwafile) {
  try {
    TVisBuilder VisBuilder(*this);
    VisBuilder.Run(srcfile, gwafile);
  } catch (WadLibError &e) {
    throw GLVisError("%s", e.message);
  }
}
