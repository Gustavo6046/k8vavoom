uniform vec3 LightColor;
uniform float LightRadius;
uniform float LightMin;
//$include "common/texshade.inc" // in "texture_vars.fs"
#ifdef VV_SPOTLIGHT
$include "common/spotlight_vars.fs"
#endif

varying vec3 Normal;
varying vec3 VertToLight;
varying float Dist;
varying float VDist;

#ifdef VV_SHADOW_CHECK_TEXTURE
uniform sampler2D Texture;
$include "common/texture_vars.fs"
#endif

#ifdef VV_SHADOWMAPS
$include "shadowvol/smap_light_decl.fs"
#endif


void main () {
  if (VDist <= 0.0 || Dist <= 0.0) discard;

#ifdef VV_SHADOW_CHECK_TEXTURE
  vec4 TexColor = GetStdTexelSimpleShade(Texture, TextureCoordinate);
  //if (TexColor.a < ALPHA_MIN) discard; //FIXME
  if (TexColor.a < ALPHA_MASKED) discard; // only normal and masked walls should go thru this
#endif

  $include "shadowvol/common_light.fs"

#if 0
factored out
  float DistToLight = max(1.0, dot(VertToLight, VertToLight));
  if (DistToLight >= LightRadius*LightRadius) discard;

  vec3 normV2L = normalize(VertToLight);

#ifdef VV_SHADOWMAPS
  $include "shadowvol/smap_light_check.fs"
#endif

  DistToLight = sqrt(DistToLight);
  float attenuation = (LightRadius-DistToLight-LightMin)*(0.5+(0.5*dot(normV2L, Normal)));
#ifdef VV_SPOTLIGHT
  $include "common/spotlight_calc.fs"
#endif

  if (attenuation <= 0.0) discard;
  float finalA = min(attenuation/255.0, 1.0);

#ifdef VV_SHADOW_CHECK_TEXTURE
  float transp = clamp((TexColor.a-0.1)/0.9, 0.0, 1.0);
  finalA *= transp*transp*(3.0-(2.0*transp));
#endif

  //gl_FragColor = vec4(clamp(Normal, 0.0, 1.0), 0.1);
#ifdef VV_SHADOWMAPS
  //gl_FragColor = vec4(clamp(fromLightToFragment.xyz, 0.0, 1.0), 0.1);
  gl_FragColor = vec4(LightColor, finalA);
#else
  gl_FragColor = vec4(LightColor, finalA);
#endif

#endif
}
