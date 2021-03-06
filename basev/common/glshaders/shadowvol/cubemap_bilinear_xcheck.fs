  float ssd = SurfDist-dot(Normal, LightPos); // this is invariant
  float cctexX0 = floor(cubeTC.x*CubeSize);
  float cctexY0 = floor(cubeTC.y*CubeSize);
  float uc0 = 2.0*(cctexX0+0.5)/CubeSize-1.0;
  float vc0 = 2.0*(cctexY0+0.5)/CubeSize-1.0;

#define destvar    valat
#define tt_ltfdir  ltfdir
#define uc uc0
#define vc vc0
    $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
#undef destvar
#undef tt_ltfdir
#undef uc
#undef vc

  // this breaks shader requirements about texture sampling, but meh...
  #ifdef CUBE_FIX_EDGES
  if (ttexX+tshift2 > 1.0) {
    #ifdef CUBE_FIX_EDGES_BAD_GUESS
    valdiag = valhoriz = valat;
    #else
    valdiag = compareShadowTexelDistance(CUBE_FIX_EDGES_norm(ltf_diag), orgDist);
    valhoriz = compareShadowTexelDistance(CUBE_FIX_EDGES_norm(ltf_horiz), orgDist);
    #endif
    if (ttexY+tshift2 > 1.0) {
      #ifdef CUBE_FIX_EDGES_BAD_GUESS
      valvert = valat;
      #else
      valvert = compareShadowTexelDistance(CUBE_FIX_EDGES_norm(ltf_vert), orgDist);
      #endif
    } else {
      // overflow: only x
      float vc1 = 2.0*(cctexY0+1.5)/CubeSize-1.0;

      #define destvar    valvert
      #define tt_ltfdir  ltf_vert
      #define uc uc0
      #define vc vc1
          $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
      #undef destvar
      #undef tt_ltfdir
      #undef uc
      #undef vc
    }
  } else if (ttexY+tshift2 > 1.0) {
    // overflow: only y
    #ifdef CUBE_FIX_EDGES_BAD_GUESS
    valvert = valdiag = valat;
    #else
    valvert = compareShadowTexelDistance(CUBE_FIX_EDGES_norm(ltf_vert), orgDist);
    valdiag = compareShadowTexelDistance(CUBE_FIX_EDGES_norm(ltf_diag), orgDist);
    #endif

    float uc1 = 2.0*(cctexX0+1.5)/CubeSize-1.0;

    #define destvar    valhoriz
    #define tt_ltfdir  ltf_horiz
    #define uc uc1
    #define vc vc0
        $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
    #undef destvar
    #undef tt_ltfdir
    #undef uc
    #undef vc
  } else
  #endif
  {
    float uc1 = 2.0*(cctexX0+1.5)/CubeSize-1.0;
    float vc1 = 2.0*(cctexY0+1.5)/CubeSize-1.0;

#define destvar    valhoriz
#define tt_ltfdir  ltf_horiz
#define uc uc1
#define vc vc0
    $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
#undef destvar
#undef tt_ltfdir
#undef uc
#undef vc

#define destvar    valvert
#define tt_ltfdir  ltf_vert
#define uc uc0
#define vc vc1
    $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
#undef destvar
#undef tt_ltfdir
#undef uc
#undef vc

#define destvar    valdiag
#define tt_ltfdir  ltf_diag
#define uc uc1
#define vc vc1
    $include "shadowvol/cubemap_check_dispatch_inc_bipart.fs"
#undef destvar
#undef tt_ltfdir
#undef uc
#undef vc
  }
