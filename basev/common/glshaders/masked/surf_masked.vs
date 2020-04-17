#version 120
$include "common/common.inc"

#ifdef GL4ES_HACKS
attribute vec3 Position;
#endif
attribute vec2 TexCoord;

varying vec2 TextureCoordinate;

#ifdef VV_MASKED_GLOW
$include "common/glow_vars.vs"
#endif


void main () {
  // transforming the vertex
#ifdef GL4ES_HACKS
  gl_Position = gl_ModelViewProjectionMatrix*vec4(Position, 1.0);
#else
  gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
#endif
  // pass texture coordinates
  TextureCoordinate = TexCoord;
#ifdef VV_MASKED_GLOW
  $include "common/glow_calc.vs"
#endif
}
