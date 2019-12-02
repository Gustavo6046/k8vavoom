#version 120
$include "common/common.inc"
// color scaling shader
// because glColor can't go outside the range 0..1

uniform vec3 Scale;
uniform sampler2D TextureSource;

varying vec2 TextureCoordinate;


void main () {
  //gl_FragColor = texture2D(TextureSource, gl_TexCoord[0].st)*vec4(Scale, 1.0);
  gl_FragColor = texture2D(TextureSource, TextureCoordinate.st)*vec4(Scale, 1.0);
}