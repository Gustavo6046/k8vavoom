// if a is 0, there is no glow
uniform vec4 GlowColorFloor;
uniform vec4 GlowColorCeiling;

uniform float FloorGlowHeight;
uniform float CeilingGlowHeight;

varying float floorHeight;
varying float ceilingHeight;


vec4 calcGlow (vec4 light) {
  float fh = ((FloorGlowHeight-clamp(abs(floorHeight), 0.0, FloorGlowHeight))*GlowColorFloor.a)/FloorGlowHeight;
  float ch = ((CeilingGlowHeight-clamp(abs(ceilingHeight), 0.0, CeilingGlowHeight))*GlowColorCeiling.a)/CeilingGlowHeight;
  vec4 lt;
  lt.r = clamp(light.r+GlowColorFloor.r*fh+GlowColorCeiling.r*ch, 0.0, 1.0);
  lt.g = clamp(light.g+GlowColorFloor.g*fh+GlowColorCeiling.g*ch, 0.0, 1.0);
  lt.b = clamp(light.b+GlowColorFloor.b*fh+GlowColorCeiling.b*ch, 0.0, 1.0);
  lt.a = light.a;
  return lt;
}
