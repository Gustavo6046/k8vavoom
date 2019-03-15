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
//**  Copyright (C) 2018-2019 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, either version 3 of the License, or
//**  (at your option) any later version.
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
#ifndef VAVOOM_GL_LOCAL_HEADER
#define VAVOOM_GL_LOCAL_HEADER

#ifdef _WIN32
# include <windows.h>
#endif
#ifdef USE_GLAD
# include "glad.h"
#else
# include <GL/gl.h>
#endif
#ifdef _WIN32
# include <GL/glext.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#include "gamedefs.h"
#include "cl_local.h"
#include "render/r_shared.h"


// extensions

// ARB_multitexture
#ifndef GL_ARB_multitexture
#define GL_TEXTURE0_ARB           0x84C0
#define GL_TEXTURE1_ARB           0x84C1
#define GL_TEXTURE2_ARB           0x84C2
#define GL_TEXTURE3_ARB           0x84C3
#define GL_TEXTURE4_ARB           0x84C4
#define GL_TEXTURE5_ARB           0x84C5
#define GL_TEXTURE6_ARB           0x84C6
#define GL_TEXTURE7_ARB           0x84C7
#define GL_TEXTURE8_ARB           0x84C8
#define GL_TEXTURE9_ARB           0x84C9
#define GL_TEXTURE10_ARB          0x84CA
#define GL_TEXTURE11_ARB          0x84CB
#define GL_TEXTURE12_ARB          0x84CC
#define GL_TEXTURE13_ARB          0x84CD
#define GL_TEXTURE14_ARB          0x84CE
#define GL_TEXTURE15_ARB          0x84CF
#define GL_TEXTURE16_ARB          0x84D0
#define GL_TEXTURE17_ARB          0x84D1
#define GL_TEXTURE18_ARB          0x84D2
#define GL_TEXTURE19_ARB          0x84D3
#define GL_TEXTURE20_ARB          0x84D4
#define GL_TEXTURE21_ARB          0x84D5
#define GL_TEXTURE22_ARB          0x84D6
#define GL_TEXTURE23_ARB          0x84D7
#define GL_TEXTURE24_ARB          0x84D8
#define GL_TEXTURE25_ARB          0x84D9
#define GL_TEXTURE26_ARB          0x84DA
#define GL_TEXTURE27_ARB          0x84DB
#define GL_TEXTURE28_ARB          0x84DC
#define GL_TEXTURE29_ARB          0x84DD
#define GL_TEXTURE30_ARB          0x84DE
#define GL_TEXTURE31_ARB          0x84DF
#define GL_ACTIVE_TEXTURE_ARB       0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB    0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB      0x84E2
#endif

//typedef void (APIENTRY*glMultiTexCoord2fARB_t)(GLenum, GLfloat, GLfloat);
typedef void (APIENTRY*glActiveTextureARB_t)(GLenum);

// EXT_point_parameters
#ifndef GL_EXT_point_parameters
#define GL_POINT_SIZE_MIN_EXT       0x8126
#define GL_POINT_SIZE_MAX_EXT       0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_DISTANCE_ATTENUATION_EXT     0x8129
#endif

typedef void (APIENTRY*glPointParameterfEXT_t)(GLenum, GLfloat);
typedef void (APIENTRY*glPointParameterfvEXT_t)(GLenum, const GLfloat *);

// EXT_texture_filter_anisotropic
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT   0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

// SGIS_texture_edge_clamp
#ifndef GL_SGIS_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_SGIS       0x812F
#endif

typedef void (APIENTRY*glStencilFuncSeparate_t)(GLenum, GLenum, GLint, GLuint);
typedef void (APIENTRY*glStencilOpSeparate_t)(GLenum, GLenum, GLenum, GLenum);

#ifndef GL_EXT_stencil_wrap
#define GL_INCR_WRAP_EXT          0x8507
#define GL_DECR_WRAP_EXT          0x8508
#endif

#ifndef GL_ARB_shader_objects
#define GL_PROGRAM_OBJECT_ARB       0x8B40
#define GL_SHADER_OBJECT_ARB        0x8B48
#define GL_OBJECT_TYPE_ARB          0x8B4E
#define GL_OBJECT_SUBTYPE_ARB       0x8B4F
#define GL_FLOAT_VEC2_ARB         0x8B50
#define GL_FLOAT_VEC3_ARB         0x8B51
#define GL_FLOAT_VEC4_ARB         0x8B52
#define GL_INT_VEC2_ARB           0x8B53
#define GL_INT_VEC3_ARB           0x8B54
#define GL_INT_VEC4_ARB           0x8B55
#define GL_BOOL_ARB             0x8B56
#define GL_BOOL_VEC2_ARB          0x8B57
#define GL_BOOL_VEC3_ARB          0x8B58
#define GL_BOOL_VEC4_ARB          0x8B59
#define GL_FLOAT_MAT2_ARB         0x8B5A
#define GL_FLOAT_MAT3_ARB         0x8B5B
#define GL_FLOAT_MAT4_ARB         0x8B5C
#define GL_SAMPLER_1D_ARB         0x8B5D
#define GL_SAMPLER_2D_ARB         0x8B5E
#define GL_SAMPLER_3D_ARB         0x8B5F
#define GL_SAMPLER_CUBE_ARB         0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB      0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB      0x8B62
#define GL_SAMPLER_2D_RECT_ARB        0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB   0x8B64
#define GL_OBJECT_DELETE_STATUS_ARB     0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB    0x8B81
#define GL_OBJECT_LINK_STATUS_ARB     0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB   0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB   0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB    0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB   0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB  0x8B88

typedef char GLcharARB;
typedef unsigned int GLhandleARB;
#endif

typedef void (APIENTRY*glDeleteObjectARB_t)(GLhandleARB);
typedef GLhandleARB (APIENTRY*glGetHandleARB_t)(GLenum);
typedef void (APIENTRY*glDetachObjectARB_t)(GLhandleARB, GLhandleARB);
typedef GLhandleARB (APIENTRY*glCreateShaderObjectARB_t)(GLenum);
typedef void (APIENTRY*glShaderSourceARB_t)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *);
typedef void (APIENTRY*glCompileShaderARB_t)(GLhandleARB);
typedef GLhandleARB (APIENTRY*glCreateProgramObjectARB_t)(void);
typedef void (APIENTRY*glAttachObjectARB_t)(GLhandleARB, GLhandleARB);
typedef void (APIENTRY*glLinkProgramARB_t)(GLhandleARB);
typedef void (APIENTRY*glUseProgramObjectARB_t)(GLhandleARB);
typedef void (APIENTRY*glValidateProgramARB_t)(GLhandleARB);
typedef void (APIENTRY*glUniform1fARB_t)(GLint, GLfloat);
typedef void (APIENTRY*glUniform2fARB_t)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY*glUniform3fARB_t)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY*glUniform4fARB_t)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY*glUniform1iARB_t)(GLint, GLint);
typedef void (APIENTRY*glUniform2iARB_t)(GLint, GLint, GLint);
typedef void (APIENTRY*glUniform3iARB_t)(GLint, GLint, GLint, GLint);
typedef void (APIENTRY*glUniform4iARB_t)(GLint, GLint, GLint, GLint, GLint);
typedef void (APIENTRY*glUniform1fvARB_t)(GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY*glUniform2fvARB_t)(GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY*glUniform3fvARB_t)(GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY*glUniform4fvARB_t)(GLint, GLsizei, const GLfloat *);
typedef void (APIENTRY*glUniform1ivARB_t)(GLint, GLsizei, const GLint *);
typedef void (APIENTRY*glUniform2ivARB_t)(GLint, GLsizei, const GLint *);
typedef void (APIENTRY*glUniform3ivARB_t)(GLint, GLsizei, const GLint *);
typedef void (APIENTRY*glUniform4ivARB_t)(GLint, GLsizei, const GLint *);
typedef void (APIENTRY*glUniformMatrix2fvARB_t)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY*glUniformMatrix3fvARB_t)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY*glUniformMatrix4fvARB_t)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void (APIENTRY*glGetObjectParameterfvARB_t)(GLhandleARB, GLenum, GLfloat *);
typedef void (APIENTRY*glGetObjectParameterivARB_t)(GLhandleARB, GLenum, GLint *);
typedef void (APIENTRY*glGetInfoLogARB_t)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
typedef void (APIENTRY*glGetAttachedObjectsARB_t)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
typedef GLint (APIENTRY*glGetUniformLocationARB_t)(GLhandleARB, const GLcharARB *);
typedef void (APIENTRY*glGetActiveUniformARB_t)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
typedef void (APIENTRY*glGetUniformfvARB_t)(GLhandleARB, GLint, GLfloat *);
typedef void (APIENTRY*glGetUniformivARB_t)(GLhandleARB, GLint, GLint *);
typedef void (APIENTRY*glGetShaderSourceARB_t)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
typedef void (APIENTRY*glDepthBoundsEXT_t)(GLclampd zmin, GLclampd zmax);

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB        0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB  0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB     0x8B4B
#define GL_MAX_VERTEX_ATTRIBS_ARB     0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB     0x8871
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB   0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB  0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB    0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB  0x8645
#endif

typedef void (APIENTRY*glVertexAttrib1dARB_t)(GLuint, GLdouble);
typedef void (APIENTRY*glVertexAttrib1dvARB_t)(GLuint, const GLdouble *);
typedef void (APIENTRY*glVertexAttrib1fARB_t)(GLuint, GLfloat);
typedef void (APIENTRY*glVertexAttrib1fvARB_t)(GLuint, const GLfloat *);
typedef void (APIENTRY*glVertexAttrib1sARB_t)(GLuint, GLshort);
typedef void (APIENTRY*glVertexAttrib1svARB_t)(GLuint, const GLshort *);
typedef void (APIENTRY*glVertexAttrib2dARB_t)(GLuint, GLdouble, GLdouble);
typedef void (APIENTRY*glVertexAttrib2dvARB_t)(GLuint, const GLdouble *);
typedef void (APIENTRY*glVertexAttrib2fARB_t)(GLuint, GLfloat, GLfloat);
typedef void (APIENTRY*glVertexAttrib2fvARB_t)(GLuint, const GLfloat *);
typedef void (APIENTRY*glVertexAttrib2sARB_t)(GLuint, GLshort, GLshort);
typedef void (APIENTRY*glVertexAttrib2svARB_t)(GLuint, const GLshort *);
typedef void (APIENTRY*glVertexAttrib3dARB_t)(GLuint, GLdouble, GLdouble, GLdouble);
typedef void (APIENTRY*glVertexAttrib3dvARB_t)(GLuint, const GLdouble *);
typedef void (APIENTRY*glVertexAttrib3fARB_t)(GLuint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY*glVertexAttrib3fvARB_t)(GLuint, const GLfloat *);
typedef void (APIENTRY*glVertexAttrib3sARB_t)(GLuint, GLshort, GLshort, GLshort);
typedef void (APIENTRY*glVertexAttrib3svARB_t)(GLuint, const GLshort *);
typedef void (APIENTRY*glVertexAttrib4NbvARB_t)(GLuint, const GLbyte *);
typedef void (APIENTRY*glVertexAttrib4NivARB_t)(GLuint, const GLint *);
typedef void (APIENTRY*glVertexAttrib4NsvARB_t)(GLuint, const GLshort *);
typedef void (APIENTRY*glVertexAttrib4NubARB_t)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
typedef void (APIENTRY*glVertexAttrib4NubvARB_t)(GLuint, const GLubyte *);
typedef void (APIENTRY*glVertexAttrib4NuivARB_t)(GLuint, const GLuint *);
typedef void (APIENTRY*glVertexAttrib4NusvARB_t)(GLuint, const GLushort *);
typedef void (APIENTRY*glVertexAttrib4bvARB_t)(GLuint, const GLbyte *);
typedef void (APIENTRY*glVertexAttrib4dARB_t)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (APIENTRY*glVertexAttrib4dvARB_t)(GLuint, const GLdouble *);
typedef void (APIENTRY*glVertexAttrib4fARB_t)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY*glVertexAttrib4fvARB_t)(GLuint, const GLfloat *);
typedef void (APIENTRY*glVertexAttrib4ivARB_t)(GLuint, const GLint *);
typedef void (APIENTRY*glVertexAttrib4sARB_t)(GLuint, GLshort, GLshort, GLshort, GLshort);
typedef void (APIENTRY*glVertexAttrib4svARB_t)(GLuint, const GLshort *);
typedef void (APIENTRY*glVertexAttrib4ubvARB_t)(GLuint, const GLubyte *);
typedef void (APIENTRY*glVertexAttrib4uivARB_t)(GLuint, const GLuint *);
typedef void (APIENTRY*glVertexAttrib4usvARB_t)(GLuint, const GLushort *);
typedef void (APIENTRY*glVertexAttribPointerARB_t)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void (APIENTRY*glEnableVertexAttribArrayARB_t)(GLuint);
typedef void (APIENTRY*glDisableVertexAttribArrayARB_t)(GLuint);
typedef void (APIENTRY*glBindAttribLocationARB_t)(GLhandleARB, GLuint, const GLcharARB *);
typedef void (APIENTRY*glGetActiveAttribARB_t)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
typedef GLint (APIENTRY*glGetAttribLocationARB_t)(GLhandleARB, const GLcharARB *);
typedef void (APIENTRY*glGetVertexAttribdvARB_t)(GLuint, GLenum, GLdouble *);
typedef void (APIENTRY*glGetVertexAttribfvARB_t)(GLuint, GLenum, GLfloat *);
typedef void (APIENTRY*glGetVertexAttribivARB_t)(GLuint, GLenum, GLint *);
typedef void (APIENTRY*glGetVertexAttribPointervARB_t)(GLuint, GLenum, GLvoid **);

#ifndef GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB        0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB  0x8B49
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB  0x8B8B
#endif

#ifndef GL_ARB_shading_language_100
#define GL_SHADING_LANGUAGE_VERSION_ARB   0x8B8C
#endif

#ifndef GL_ARB_depth_clamp
#define GL_DEPTH_CLAMP            0x864F
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB          0x8764
#define GL_BUFFER_USAGE_ARB         0x8765
#define GL_ARRAY_BUFFER_ARB         0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB     0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB     0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB  0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB  0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB  0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB  0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB          0x88B8
#define GL_WRITE_ONLY_ARB         0x88B9
#define GL_READ_WRITE_ARB         0x88BA
#define GL_BUFFER_ACCESS_ARB        0x88BB
#define GL_BUFFER_MAPPED_ARB        0x88BC
#define GL_BUFFER_MAP_POINTER_ARB     0x88BD
#define GL_STREAM_DRAW_ARB          0x88E0
#define GL_STREAM_READ_ARB          0x88E1
#define GL_STREAM_COPY_ARB          0x88E2
#define GL_STATIC_DRAW_ARB          0x88E4
#define GL_STATIC_READ_ARB          0x88E5
#define GL_STATIC_COPY_ARB          0x88E6
#define GL_DYNAMIC_DRAW_ARB         0x88E8
#define GL_DYNAMIC_READ_ARB         0x88E9
#define GL_DYNAMIC_COPY_ARB         0x88EA

/* GL types for handling large vertex buffer objects */
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

typedef void (APIENTRY*glBindBufferARB_t)(GLenum, GLuint);
typedef void (APIENTRY*glDeleteBuffersARB_t)(GLsizei, const GLuint *);
typedef void (APIENTRY*glGenBuffersARB_t)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY*glIsBufferARB_t)(GLuint);
typedef void (APIENTRY*glBufferDataARB_t)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
typedef void (APIENTRY*glBufferSubDataARB_t)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
typedef void (APIENTRY*glGetBufferSubDataARB_t)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
typedef GLvoid *(APIENTRY*glMapBufferARB_t)(GLenum, GLenum);
typedef GLboolean (APIENTRY*glUnmapBufferARB_t)(GLenum);
typedef void (APIENTRY*glGetBufferParameterivARB_t)(GLenum, GLenum, GLint *);
typedef void (APIENTRY*glGetBufferPointervARB_t)(GLenum, GLenum, GLvoid **);

#ifndef GL_EXT_draw_range_elements
#define GL_MAX_ELEMENTS_VERTICES_EXT    0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT     0x80E9
#endif

#ifndef GL_DEPTH_BOUNDS_TEST_EXT
# define GL_DEPTH_BOUNDS_TEST_EXT  0x8890
#endif

#ifndef GL_DEPTH_BOUNDS_EXT
# define GL_DEPTH_BOUNDS_EXT  0x8891
#endif

typedef void (APIENTRY*glDrawRangeElementsEXT_t)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);

#ifndef GL_FRAMEBUFFER
# define GL_FRAMEBUFFER  0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
# define GL_COLOR_ATTACHMENT0  0x8CE0
#endif
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
# define GL_DEPTH_STENCIL_ATTACHMENT  0x821A
#endif

#ifndef GL_FRAMEBUFFER_COMPLETE
# define GL_FRAMEBUFFER_COMPLETE  0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT  0x8CD6
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT  0x8CD7
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
# define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS  0x8CD9
#endif
#ifndef GL_FRAMEBUFFER_UNSUPPORTED
# define GL_FRAMEBUFFER_UNSUPPORTED  0x8CDD
#endif

#ifndef GL_COLOR_LOGIC_OP
# define GL_COLOR_LOGIC_OP  0x0BF2
#endif
#ifndef GL_CLEAR
# define GL_CLEAR  0x1500
#endif
#ifndef GL_COPY
# define GL_COPY  0x1503
#endif
#ifndef GL_XOR
# define GL_XOR  0x1506
#endif

#ifndef GL_FRAMEBUFFER_BINDING
# define GL_FRAMEBUFFER_BINDING  0x8CA6
#endif

#ifndef GL_DEPTH_STENCIL
# define GL_DEPTH_STENCIL  0x84F9
#endif

#ifndef GL_UNSIGNED_INT_24_8
# define GL_UNSIGNED_INT_24_8  0x84FA
#endif


typedef void (APIENTRY *glFramebufferTexture2DFn) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *glDeleteFramebuffersFn) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *glGenFramebuffersFn) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY *glCheckFramebufferStatusFn) (GLenum target);
typedef void (APIENTRY *glBindFramebufferFn) (GLenum target, GLuint framebuffer);

typedef void (APIENTRY *glClipControl_t) (GLenum origin, GLenum depth);


// ////////////////////////////////////////////////////////////////////////// //
extern VCvarF gl_alpha_threshold;
extern VCvarB gl_sort_textures;
extern VCvarI r_ambient;
extern VCvarB r_allow_ambient;
extern VCvarB r_decals_enabled;
extern VCvarB r_decals_wall_masked;
extern VCvarB r_decals_wall_alpha;
extern VCvarB r_adv_masked_wall_vertex_light;
extern VCvarB r_adv_masked_wall_vertex_light;
extern VCvarB r_adv_limit_extrude;
extern VCvarB gl_decal_debug_nostencil;
extern VCvarB gl_decal_debug_noalpha;
extern VCvarB gl_decal_dump_max;
extern VCvarB gl_decal_reset_max;
extern VCvarB gl_sort_textures;
extern VCvarB gl_dbg_adv_render_textures_surface;
extern VCvarB gl_dbg_adv_render_offset_shadow_volume;
extern VCvarB gl_dbg_adv_render_never_offset_shadow_volume;
extern VCvarB gl_dbg_render_stack_portal_bounds;
extern VCvarB gl_use_stencil_quad_clear;
extern VCvarB gl_dbg_use_zpass;


// ////////////////////////////////////////////////////////////////////////// //
class VOpenGLDrawer : public VDrawer {
public:
  // many shaders require common variables
  // instead of doing copypasta, we'll use the following classes
  class VGLShaderCommonLocs {
  public:
    VOpenGLDrawer *owner;
    // compiled vertex program
    GLhandleARB prog;
    // texture variable locations
    GLint locSAxis;
    GLint locTAxis;
    GLint locSOffs;
    GLint locTOffs;
    GLint locTexIW;
    GLint locTexIH;
    GLint locTexture;
    // lightmap variable locations
    GLint locTexMinS;
    GLint locTexMinT;
    GLint locCacheS;
    GLint locCacheT;
    GLint locLightMap;
    // fog variable locations
    GLint locFogEnabled;
    GLint locFogType;
    GLint locFogColour;
    GLint locFogDensity;
    GLint locFogStart;
    GLint locFogEnd;

  public:
    VGLShaderCommonLocs () : owner(nullptr), prog(0) {}

    // call this first!
    void setupProg (VOpenGLDrawer *aowner, GLhandleARB aprog);

    void setupTexture (); // setup texture variables
    void setupLMap (); // setup lightmap variables
    void setupLMapOnly (); // setup lightmap variables for shader without texture params (decals)
    void setupFog (); // setup fog variables

    void storeTexture (GLint tid);

    // `SetTexture()` must be called! it sets `tex_iw` and `tex_ih`
    void storeTextureParams (const texinfo_t *textr);

    // `SetTexture()` must be called! it sets `tex_iw` and `tex_ih`
    void storeTextureLMapParams (const texinfo_t *textr, const surface_t *surf, const surfcache_t *cache);

    void storeLMap (GLint tid);
    void storeLMapParams (const surface_t *surf, const surfcache_t *cache); // call `storeTextureParams()` first!
    void storeLMapOnlyParams (const texinfo_t *textr, const surface_t *surf, const surfcache_t *cache);

    void storeFogType ();
    void storeFogFade (vuint32 Fade, float Alpha);
  };

private:
  bool usingZPass; // if we are rendering shadow volumes, should we do "z-pass"?

public:
  // VDrawer interface
  VOpenGLDrawer ();
  virtual ~VOpenGLDrawer () override;
  virtual void InitResolution () override;
  virtual void StartUpdate (bool allowClear=true) override;
  virtual void Setup2D () override;
  //virtual void BeginDirectUpdate () override;
  //virtual void EndDirectUpdate () override;
  virtual void *ReadScreen (int *, bool *) override;
  virtual void ReadBackScreen (int, int, rgba_t *) override;

  void FinishUpdate ();

  // rendering stuff
  virtual void SetupView (VRenderLevelDrawer *, const refdef_t *) override;
  virtual void SetupViewOrg () override;
  virtual void EndView () override;

  // texture stuff
  virtual void PrecacheTexture (VTexture *) override;

  // polygon drawing
  virtual void WorldDrawing () override;
  virtual void DrawWorldAmbientPass () override;

  virtual void BeginShadowVolumesPass () override;
  virtual void BeginLightShadowVolumes (bool useZPass, bool hasScissor, const int scoords[4]) override;
  virtual void EndLightShadowVolumes () override;
  virtual void RenderSurfaceShadowVolume (const surface_t *surf, const TVec &LightPos, float Radius) override;

  virtual void BeginLightPass (const TVec &LightPos, float Radius, vuint32 Colour, bool doShadow) override;
  virtual void DrawSurfaceLight (surface_t *surf) override;

  virtual void DrawWorldTexturesPass () override;
  virtual void DrawWorldFogPass () override;
  virtual void EndFogPass () override;

  virtual void DrawSkyPolygon (surface_t *, bool, VTexture *, float, VTexture *, float, int) override;
  virtual void DrawMaskedPolygon (surface_t *, float, bool) override;
  virtual void DrawSpritePolygon (const TVec *cv, VTexture *Tex,
                                  float Alpha, bool Additive,
                                  VTextureTranslation *Translation, int CMap,
                                  vuint32 light, vuint32 Fade,
                                  const TVec &sprnormal, float sprpdist,
                                  const TVec &saxis, const TVec &taxis, const TVec &texorg,
                                  int hangup) override;
  virtual void DrawAliasModel(const TVec&, const TAVec&, const TVec&, const TVec&,
    VMeshModel*, int, int, VTexture*, VTextureTranslation*, int, vuint32,
    vuint32, float, bool, bool, float, bool, bool, bool, bool) override;
  virtual void DrawAliasModelAmbient(const TVec&, const TAVec&, const TVec&,
    const TVec&, VMeshModel*, int, int, VTexture*, vuint32, float, float, bool,
    bool, bool) override;
  virtual void DrawAliasModelTextures(const TVec&, const TAVec&, const TVec&, const TVec&,
    VMeshModel*, int, int, VTexture*, VTextureTranslation*, int, float, float, bool,
    bool, bool) override;
  virtual void BeginModelsLightPass(TVec&, float, vuint32) override;
  virtual void DrawAliasModelLight(const TVec&, const TAVec&, const TVec&,
    const TVec&, VMeshModel*, int, int, VTexture*, float, float, bool, bool) override;
  virtual void BeginModelsShadowsPass(TVec&, float) override;
  virtual void DrawAliasModelShadow(const TVec&, const TAVec&, const TVec&,
    const TVec&, VMeshModel*, int, int, float, bool, const TVec&, float) override;
  virtual void DrawAliasModelFog(const TVec&, const TAVec&, const TVec&,
    const TVec&, VMeshModel*, int, int, VTexture*, vuint32, float, float, bool, bool) override;
  virtual bool StartPortal(VPortal*, bool) override;
  virtual void EndPortal(VPortal*, bool) override;

  // particles
  virtual void StartParticles () override;
  virtual void DrawParticle (particle_t *) override;
  virtual void EndParticles () override;

  // drawing
  virtual void DrawPic(float, float, float, float, float, float, float, float,
    VTexture*, VTextureTranslation*, float) override;
  virtual void DrawPicShadow(float, float, float, float, float, float, float,
    float, VTexture*, float) override;
  virtual void FillRectWithFlat(float, float, float, float, float, float, float,
    float, VTexture*) override;
  virtual void FillRectWithFlatRepeat(float, float, float, float, float, float, float,
    float, VTexture*) override;
  virtual void FillRect(float, float, float, float, vuint32) override;
  virtual void ShadeRect(int, int, int, int, float) override;
  virtual void DrawConsoleBackground(int) override;
  virtual void DrawSpriteLump(float, float, float, float, VTexture*,
    VTextureTranslation*, bool) override;

  // automap
  virtual void StartAutomap () override;
  virtual void DrawLine (float x1, float y1, vuint32 c1, float x2, float y2, vuint32 c2) override;
  virtual void EndAutomap () override;

  // advanced drawing.
  virtual bool SupportsAdvancedRendering () override;

  virtual void CopyToSecondaryFBO () override;

  virtual void GetProjectionMatrix (VMatrix4 &mat) override;
  virtual void GetModelMatrix (VMatrix4 &mat) override;

  virtual int SetupLightScissor (const TVec &org, float radius, int scoord[4]) override;
  virtual void ResetScissor () override;

  static inline float getAlphaThreshold () { float res = gl_alpha_threshold; if (res < 0) res = 0; else if (res > 1) res = 1; return res; }

  //virtual void GetRealWindowSize (int *rw, int *rh) override;

private:
  vuint8 decalStcVal;
  bool decalUsedStencil;
  bool stencilBufferDirty;

  enum DecalType { DT_SIMPLE, DT_LIGHTMAP, DT_ADVANCED };

  // this is required for decals
  inline void NoteStencilBufferDirty () { stencilBufferDirty = true; }
  inline bool IsStencilBufferDirty () const { return stencilBufferDirty; }
  inline void ClearStencilBuffer () { if (stencilBufferDirty) glClear(GL_STENCIL_BUFFER_BIT); stencilBufferDirty = false; decalUsedStencil = false; }

  void RenderPrepareShaderDecals (surface_t *surf);
  bool RenderFinishShaderDecals (DecalType dtype, surface_t *surf, surfcache_t *cache, int cmap);

  void UpdateAndUploadSurfaceTexture (surface_t *surf);

  // regular renderer building parts
  // returns `true` if we need to re-setup texture
  bool RenderSimpleSurface (bool textureChanged, surface_t *surf);
  bool RenderLMapSurface (bool textureChanged, surface_t *surf, surfcache_t *cache);

  void RestoreDepthFunc ();
  inline bool CanUseRevZ () const { return useReverseZ; }

private:
  vuint8 *readBackTempBuf;
  int readBackTempBufSize;

public:
  struct SurfListItem {
    surface_t *surf;
    surfcache_t *cache;
  };

private:
  SurfListItem *surfList;
  vuint32 surfListUsed;
  vuint32 surfListSize;

  inline void surfListClear () {
    surfListUsed = 0;
  }

  inline void surfListAppend (surface_t *surf, surfcache_t *cache=nullptr) {
    UpdateAndUploadSurfaceTexture(surf);
    if (surfListUsed == surfListSize) {
      surfListSize += 65536;
      surfList = (SurfListItem *)Z_Realloc(surfList, surfListSize*sizeof(surfList[0]));
    }
    SurfListItem *si = &surfList[surfListUsed++];
    si->surf = surf;
    si->cache = cache;
  }

private:
  static inline float getSurfLightLevel (const surface_t *surf) {
    if (!surf || !r_allow_ambient) return 0;
    int slins = (surf->Light>>24)&0xff;
    if (slins < r_ambient) slins = clampToByte(r_ambient);
    //slins = MAX(slins, r_ambient);
    //if (slins > 255) slins = 255;
    return float(slins)/255.0f;
  }

  static inline void glVertex (const TVec &v) {
    glVertex3f(v.x, v.y, v.z);
  }

protected:
  //enum { M_INFINITY = 8000 };
  //enum { M_INFINITY = 36000 };
  enum { M_INFINITY = 64000 };

  vuint8 *tmpImgBuf0;
  vuint8 *tmpImgBuf1;
  int tmpImgBufSize;

  bool useReverseZ;
  bool hasNPOT;
  bool hasBoundsTest; // GL_EXT_depth_bounds_test

  GLuint mainFBO;
  GLuint mainFBOColorTid;
  GLuint mainFBODepthStencilTid;

  GLuint secondFBO; // for transition effects
  GLuint secondFBOColorTid;

  // we'll copy ambient light texture here, so we can use it in decal renderer to light decals
  GLuint ambLightFBO;
  GLuint ambLightFBOColorTid;

  GLint maxTexSize;
  bool texturesGenerated;

  GLuint lmap_id[NUM_BLOCK_SURFS];
  GLuint addmap_id[NUM_BLOCK_SURFS];

  float tex_iw, tex_ih;
  int tex_w, tex_h;

  //GLenum maxfilter;
  //GLenum minfilter;
  //GLenum mipfilter;
  GLenum ClampToEdge;
  GLfloat max_anisotropy; // 1.0: off
  bool anisotropyExists;

  bool usingFPZBuffer;

  //GLenum spr_maxfilter;
  //GLenum spr_mipfilter;

  int lastgamma;
  int CurrentFade;

  bool HaveDepthClamp;
  bool HaveStencilWrap;
  bool HaveDrawRangeElements;

  int MaxTextureUnits;

  TArray<GLhandleARB> CreatedShaderObjects;
  TArray<VMeshModel *> UploadedModels;

  GLhandleARB DrawSimple_Program;
  GLint DrawSimple_TextureLoc;
  GLint DrawSimple_AlphaLoc;

  GLhandleARB DrawShadow_Program;
  GLint DrawShadow_TextureLoc;
  GLint DrawShadow_AlphaLoc;

  GLhandleARB DrawFixedCol_Program;
  GLint DrawFixedCol_ColourLoc;

  GLhandleARB DrawAutomap_Program;

  GLhandleARB SurfZBuf_Program;

  GLhandleARB SurfAdvDecal_Program;
  GLint SurfAdvDecal_TextureLoc;
  GLint SurfAdvDecal_AmbLightTextureLoc;
  GLint SurfAdvDecal_SplatAlphaLoc;
  GLint SurfAdvDecal_FullBright;
  GLint SurfAdvDecal_ScreenSize;

  GLhandleARB SurfDecalNoLMap_Program;
  GLint SurfDecalNoLMap_TextureLoc;
  GLint SurfDecalNoLMap_SplatAlphaLoc;
  GLint SurfDecalNoLMap_LightLoc;
  VGLShaderCommonLocs SurfDecalNoLMap_Locs; // only fog

  GLhandleARB SurfDecalLMap_Program;
  GLint SurfDecalLMap_TextureLoc;
  GLint SurfDecalLMap_SplatAlphaLoc;
  GLint SurfDecalLMap_LightLoc;
  VGLShaderCommonLocs SurfDecalLMap_Locs; // fog, texture and lightmap
  GLint SurfDecalLMap_SpecularMapLoc;

  GLhandleARB SurfSimple_Program;
  VGLShaderCommonLocs SurfSimple_Locs; // fog, texture
  GLint SurfSimple_LightLoc;

  GLhandleARB SurfLightmap_Program;
  VGLShaderCommonLocs SurfLightmap_Locs; // fog, texture, and lightmap
  GLint SurfLightmap_SpecularMapLoc;

  GLhandleARB SurfSky_Program;
  GLint SurfSky_TextureLoc;
  GLint SurfSky_BrightnessLoc;
  GLint SurfSky_TexCoordLoc;

  GLhandleARB SurfDSky_Program;
  GLint SurfDSky_TextureLoc;
  GLint SurfDSky_Texture2Loc;
  GLint SurfDSky_BrightnessLoc;
  GLint SurfDSky_TexCoordLoc;
  GLint SurfDSky_TexCoord2Loc;

  GLhandleARB SurfMasked_Program;
  VGLShaderCommonLocs SurfMasked_Locs; // fog
  GLint SurfMasked_TextureLoc;
  GLint SurfMasked_LightLoc;
  GLint SurfMasked_AlphaRefLoc;
  GLint SurfMasked_TexCoordLoc;

  GLhandleARB SurfModel_Program;
  GLint SurfModel_InterLoc;
  GLint SurfModel_TextureLoc;
  VGLShaderCommonLocs SurfModel_Locs; // fog
  GLint SurfModel_Vert2Loc;
  GLint SurfModel_TexCoordLoc;
  GLint SurfModel_AlphaLoc;
  GLint SurfModel_LightValLoc;
  GLint SurfModel_ViewOriginLoc;
  GLint SurfModel_AllowTransparencyLoc;

  GLhandleARB SurfPartSq_Program;
  GLint SurfPartSq_TexCoordLoc;
  GLint SurfPartSq_LightValLoc;

  GLhandleARB SurfPartSm_Program;
  GLint SurfPartSm_TexCoordLoc;
  GLint SurfPartSm_LightValLoc;

  GLhandleARB ShadowsAmbient_Program;
  VGLShaderCommonLocs ShadowsAmbient_Locs; // texture
  GLint ShadowsAmbient_LightLoc;

  GLhandleARB ShadowsLight_Program;
  GLint ShadowsLight_LightPosLoc;
  GLint ShadowsLight_LightRadiusLoc;
  GLint ShadowsLight_LightColourLoc;
  GLint ShadowsLight_SurfNormalLoc;
  GLint ShadowsLight_SurfDistLoc;
  VGLShaderCommonLocs ShadowsLight_Locs; // texture
  GLint ShadowsLight_AlphaLoc;
  GLint ShadowsLight_ViewOriginLoc;

  GLhandleARB ShadowsTexture_Program;
  VGLShaderCommonLocs ShadowsTexture_Locs; // texture

  GLhandleARB ShadowsModelAmbient_Program;
  GLint ShadowsModelAmbient_InterLoc;
  GLint ShadowsModelAmbient_TextureLoc;
  GLint ShadowsModelAmbient_LightLoc;
  GLint ShadowsModelAmbient_ModelToWorldMatLoc;
  GLint ShadowsModelAmbient_NormalToWorldMatLoc;
  GLint ShadowsModelAmbient_Vert2Loc;
  GLint ShadowsModelAmbient_VertNormalLoc;
  GLint ShadowsModelAmbient_Vert2NormalLoc;
  GLint ShadowsModelAmbient_TexCoordLoc;
  GLint ShadowsModelAmbient_AlphaLoc;
  GLint ShadowsModelAmbient_ViewOriginLoc;
  GLint ShadowsModelAmbient_AllowTransparencyLoc;

  GLhandleARB ShadowsModelTextures_Program;
  GLint ShadowsModelTextures_InterLoc;
  GLint ShadowsModelTextures_TextureLoc;
  GLint ShadowsModelTextures_AlphaLoc;
  GLint ShadowsModelTextures_ModelToWorldMatLoc;
  GLint ShadowsModelTextures_NormalToWorldMatLoc;
  GLint ShadowsModelTextures_Vert2Loc;
  GLint ShadowsModelTextures_VertNormalLoc;
  GLint ShadowsModelTextures_Vert2NormalLoc;
  GLint ShadowsModelTextures_TexCoordLoc;
  GLint ShadowsModelTextures_ViewOriginLoc;
  GLint ShadowsModelTextures_AllowTransparencyLoc;
  GLint ShadowsModelTextures_AmbLightTextureLoc;
  GLint ShadowsModelTextures_ScreenSizeLoc;

  GLhandleARB ShadowsModelLight_Program;
  GLint ShadowsModelLight_InterLoc;
  GLint ShadowsModelLight_TextureLoc;
  GLint ShadowsModelLight_LightPosLoc;
  GLint ShadowsModelLight_LightRadiusLoc;
  GLint ShadowsModelLight_LightColourLoc;
  GLint ShadowsModelLight_ModelToWorldMatLoc;
  GLint ShadowsModelLight_NormalToWorldMatLoc;
  GLint ShadowsModelLight_Vert2Loc;
  GLint ShadowsModelLight_VertNormalLoc;
  GLint ShadowsModelLight_Vert2NormalLoc;
  GLint ShadowsModelLight_TexCoordLoc;
  GLint ShadowsModelLight_ViewOriginLoc;
  GLint ShadowsModelLight_AllowTransparencyLoc;
  GLint ShadowsModelLight_AlphaLoc;

  GLhandleARB ShadowsModelShadow_Program;
  GLint ShadowsModelShadow_InterLoc;
  GLint ShadowsModelShadow_LightPosLoc;
  GLint ShadowsModelShadow_ModelToWorldMatLoc;
  GLint ShadowsModelShadow_Vert2Loc;
  GLint ShadowsModelShadow_OffsetLoc;
  GLint ShadowsModelShadow_ViewOriginLoc;

  GLhandleARB ShadowsFog_Program;
  VGLShaderCommonLocs ShadowsFog_Locs; // only fog

  GLhandleARB ShadowsModelFog_Program;
  GLint ShadowsModelFog_InterLoc;
  GLint ShadowsModelFog_ModelToWorldMatLoc;
  GLint ShadowsModelFog_TextureLoc;
  GLint ShadowsModelFog_FogTypeLoc;
  GLint ShadowsModelFog_FogColourLoc;
  GLint ShadowsModelFog_FogDensityLoc;
  GLint ShadowsModelFog_FogStartLoc;
  GLint ShadowsModelFog_FogEndLoc;
  GLint ShadowsModelFog_Vert2Loc;
  GLint ShadowsModelFog_TexCoordLoc;
  GLint ShadowsModelFog_AlphaLoc;
  GLint ShadowsModelFog_ViewOriginLoc;
  GLint ShadowsModelFog_AllowTransparencyLoc;

  // console variables
  static VCvarI texture_filter;
  static VCvarI sprite_filter;
  static VCvarI model_filter;
  static VCvarI gl_texture_filter_anisotropic;
  static VCvarB clear;
  static VCvarB blend_sprites;
  static VCvarB ext_anisotropy;
  static VCvarF maxdist;
  //static VCvarB model_lighting;
  static VCvarB specular_highlights;
  static VCvarI multisampling_sample;
  static VCvarB gl_smooth_particles;
  static VCvarB gl_dump_vendor;
  static VCvarB gl_dump_extensions;

  //  extensions
  bool CheckExtension(const char*);
  virtual void *GetExtFuncPtr(const char*) = 0;

  void SetFade(vuint32 NewFade);

  void GenerateTextures();
  virtual void FlushOneTexture (VTexture *tex) override; // unload one texture
  virtual void FlushTextures () override; // unload all textures
  void DeleteTextures();
  void FlushTexture(VTexture*);
  void DeleteTexture(VTexture*);
  void SetTexture(VTexture*, int);
  void SetSpriteLump(VTexture*, VTextureTranslation*, int, bool asPicture);
  void SetPic(VTexture*, VTextureTranslation*, int);
  void SetPicModel(VTexture*, VTextureTranslation*, int);
  void GenerateTexture(VTexture*, GLuint*, VTextureTranslation*, int, bool asPicture);
  void UploadTexture8(int, int, const vuint8*, const rgba_t*);
  void UploadTexture8A(int, int, const pala_t*, const rgba_t*);
  void UploadTexture(int, int, const rgba_t*);

  void DoHorizonPolygon(surface_t*);
  void DrawPortalArea(VPortal*);

  GLhandleARB LoadShader(GLenum Type, const VStr &FileName);
  GLhandleARB CreateProgram(GLhandleARB VertexShader, GLhandleARB FragmentShader);

  void UploadModel(VMeshModel *Mdl);
  void UnloadModels();

  void SetupTextureFiltering (int level); // level is taken from the appropriate cvar

#define _(x)  x##_t p_##x
  //_(glMultiTexCoord2fARB);
  _(glActiveTextureARB);

  _(glPointParameterfEXT);
  _(glPointParameterfvEXT);

  _(glStencilFuncSeparate);
  _(glStencilOpSeparate);

  _(glDeleteObjectARB);
  _(glGetHandleARB);
  _(glDetachObjectARB);
  _(glCreateShaderObjectARB);
  _(glShaderSourceARB);
  _(glCompileShaderARB);
  _(glCreateProgramObjectARB);
  _(glAttachObjectARB);
  _(glLinkProgramARB);
  _(glUseProgramObjectARB);
  _(glValidateProgramARB);
  _(glUniform1fARB);
  _(glUniform2fARB);
  _(glUniform3fARB);
  _(glUniform4fARB);
  _(glUniform1iARB);
  _(glUniform2iARB);
  _(glUniform3iARB);
  _(glUniform4iARB);
  _(glUniform1fvARB);
  _(glUniform2fvARB);
  _(glUniform3fvARB);
  _(glUniform4fvARB);
  _(glUniform1ivARB);
  _(glUniform2ivARB);
  _(glUniform3ivARB);
  _(glUniform4ivARB);
  _(glUniformMatrix2fvARB);
  _(glUniformMatrix3fvARB);
  _(glUniformMatrix4fvARB);
  _(glGetObjectParameterfvARB);
  _(glGetObjectParameterivARB);
  _(glGetInfoLogARB);
  _(glGetAttachedObjectsARB);
  _(glGetUniformLocationARB);
  _(glGetActiveUniformARB);
  _(glGetUniformfvARB);
  _(glGetUniformivARB);
  _(glGetShaderSourceARB);

  _(glVertexAttrib1dARB);
  _(glVertexAttrib1dvARB);
  _(glVertexAttrib1fARB);
  _(glVertexAttrib1fvARB);
  _(glVertexAttrib1sARB);
  _(glVertexAttrib1svARB);
  _(glVertexAttrib2dARB);
  _(glVertexAttrib2dvARB);
  _(glVertexAttrib2fARB);
  _(glVertexAttrib2fvARB);
  _(glVertexAttrib2sARB);
  _(glVertexAttrib2svARB);
  _(glVertexAttrib3dARB);
  _(glVertexAttrib3dvARB);
  _(glVertexAttrib3fARB);
  _(glVertexAttrib3fvARB);
  _(glVertexAttrib3sARB);
  _(glVertexAttrib3svARB);
  _(glVertexAttrib4NbvARB);
  _(glVertexAttrib4NivARB);
  _(glVertexAttrib4NsvARB);
  _(glVertexAttrib4NubARB);
  _(glVertexAttrib4NubvARB);
  _(glVertexAttrib4NuivARB);
  _(glVertexAttrib4NusvARB);
  _(glVertexAttrib4bvARB);
  _(glVertexAttrib4dARB);
  _(glVertexAttrib4dvARB);
  _(glVertexAttrib4fARB);
  _(glVertexAttrib4fvARB);
  _(glVertexAttrib4ivARB);
  _(glVertexAttrib4sARB);
  _(glVertexAttrib4svARB);
  _(glVertexAttrib4ubvARB);
  _(glVertexAttrib4uivARB);
  _(glVertexAttrib4usvARB);
  _(glVertexAttribPointerARB);
  _(glEnableVertexAttribArrayARB);
  _(glDisableVertexAttribArrayARB);
  _(glBindAttribLocationARB);
  _(glGetActiveAttribARB);
  _(glGetAttribLocationARB);
  _(glGetVertexAttribdvARB);
  _(glGetVertexAttribfvARB);
  _(glGetVertexAttribivARB);
  _(glGetVertexAttribPointervARB);

  _(glBindBufferARB);
  _(glDeleteBuffersARB);
  _(glGenBuffersARB);
  _(glIsBufferARB);
  _(glBufferDataARB);
  _(glBufferSubDataARB);
  _(glGetBufferSubDataARB);
  _(glMapBufferARB);
  _(glUnmapBufferARB);
  _(glGetBufferParameterivARB);
  _(glGetBufferPointervARB);

  _(glDrawRangeElementsEXT);

  _(glClipControl);
  _(glDepthBoundsEXT);
#undef _

  //void MultiTexCoord(int level, GLfloat s, GLfloat t) { p_glMultiTexCoord2fARB(GLenum(GL_TEXTURE0_ARB + level), s, t); }

  inline void SelectTexture (int level) { p_glActiveTextureARB(GLenum(GL_TEXTURE0_ARB+level)); }

  static inline void SetColour (vuint32 c) {
    glColor4ub((vuint8)((c>>16)&255), (vuint8)((c>>8)&255), (vuint8)(c&255), (vuint8)((c>>24)&255));
  }

public:
  glFramebufferTexture2DFn glFramebufferTexture2D;
  glDeleteFramebuffersFn glDeleteFramebuffers;
  glGenFramebuffersFn glGenFramebuffers;
  glCheckFramebufferStatusFn glCheckFramebufferStatus;
  glBindFramebufferFn glBindFramebuffer;
};


#endif
