//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2001 J�nis Legzdi��
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <allegro.h>
#include <allegro/aintern.h>
#include "d_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

BEGIN_COLOR_DEPTH_LIST
	COLOR_DEPTH_8
	COLOR_DEPTH_15
	COLOR_DEPTH_16
	COLOR_DEPTH_32
END_COLOR_DEPTH_LIST

dword				mmx_mask4 = 0;
dword				mmx_mask8 = 0;
dword				mmx_mask16 = 0;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static BITMAP		*gamebitmap = NULL;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	TSoftwareDrawer::Init
//
//==========================================================================

void TSoftwareDrawer::Init(void)
{
	if (cpu_mmx)
	{
		mmx_mask4 = ~3;
		mmx_mask8 = ~7;
		mmx_mask16 = ~15;
	}
}

//==========================================================================
//
//	my_create_bitmap_ex
//
//	Creates a new memory bitmap in the specified color_depth
//
//==========================================================================

static BITMAP *my_create_bitmap_ex(int color_depth, int width, int height)
{
	GFX_VTABLE *vtable;
	BITMAP *bitmap;
	int i;

	vtable = _get_vtable(color_depth);
	if (!vtable)
		return NULL;

	bitmap = (BITMAP*)Z_Malloc(sizeof(BITMAP) + (sizeof(char *) * height), PU_VIDEO, 0);
	if (!bitmap)
		return NULL;

	bitmap->dat = scrn;

	bitmap->w = bitmap->cr = width;
	bitmap->h = bitmap->cb = height;
	bitmap->clip = TRUE;
	bitmap->cl = bitmap->ct = 0;
	bitmap->vtable = vtable;
	bitmap->write_bank = bitmap->read_bank = (void*)_stub_bank_switch;
	bitmap->id = 0;
	bitmap->extra = NULL;
	bitmap->x_ofs = 0;
	bitmap->y_ofs = 0;
	bitmap->seg = _default_ds();

	bitmap->line[0] = (byte*)bitmap->dat;
	for (i=1; i<height; i++)
		bitmap->line[i] = bitmap->line[i-1] + width * BYTES_PER_PIXEL(color_depth);

	if (system_driver->created_bitmap)
		system_driver->created_bitmap(bitmap);

	return bitmap;
}

//==========================================================================
//
// 	TSoftwareDrawer::SetResolution
//
// 	Set up the video mode
//
//==========================================================================

bool TSoftwareDrawer::SetResolution(int Width, int Height, int bpp)
{
	if (!Width || !Height)
	{
		//	Set defaults
		Width = 320;
		Height = 200;
		bpp = 8;
	}

	if (gamebitmap)
    {
		Z_Free(gamebitmap);
        gamebitmap = NULL;
    }
	FreeMemory();

    set_color_depth(bpp);
    if (set_gfx_mode(GFX_AUTODETECT, Width, Height, 0, 0))
    {
		con << "Failed to set video mode:\n" << allegro_error << endl;
    	return false;
    }

	if (!AllocMemory(SCREEN_W, SCREEN_H, bpp))
	{
		return false;
	}
    gamebitmap = my_create_bitmap_ex(bpp, SCREEN_W, SCREEN_H);
	if (!gamebitmap)
	{
		con << "Failed to create game bitmap:\n";
		return false;
	}

	ScreenWidth = SCREEN_W;
    ScreenHeight = SCREEN_H;
	ScreenBPP = bpp;

    memset(scrn, 0, SCREEN_W * SCREEN_H * ((bpp + 7) >> 3));

	if (ScreenBPP == 15)
	{
		rshift = _rgb_r_shift_15;
		gshift = _rgb_g_shift_15;
		bshift = _rgb_b_shift_15;
	}
	else if (ScreenBPP == 16)
	{
		rshift = _rgb_r_shift_16;
		gshift = _rgb_g_shift_16;
		bshift = _rgb_b_shift_16;
	}
	else if (ScreenBPP == 32)
	{
		rshift = _rgb_r_shift_32;
		gshift = _rgb_g_shift_32;
		bshift = _rgb_b_shift_32;
	}

	return true;
}

//==========================================================================
//
// 	TSoftwareDrawer::SetPalette8
//
// 	Takes full 8 bit values.
//
//==========================================================================

void TSoftwareDrawer::SetPalette8(byte *palette)
{
  	int 	i;
	byte	*table;

	if (ScreenBPP != 8)
		return;

	table = gammatable[usegamma];

#if defined DJGPP

	//	Wait for vertical retrace
	while ((inportb(0x3da) & 8) != 8);
	while ((inportb(0x3da) & 8) == 8);

  	outportb(0x3c8, 0);
  	for (i = 0; i < 768; i++)
    	outportb(0x3c9, table[*palette++] >> 2);

#else

	PALETTE		pal;
	for (i = 0; i < 256; i++)
	{
		pal[i].r = table[*palette++] >> 2;
		pal[i].g = table[*palette++] >> 2;
		pal[i].b = table[*palette++] >> 2;
	}
	set_palette(pal);

#endif
}

//==========================================================================
//
//	TSoftwareDrawer::Update
//
// 	Blit to the screen / Flip surfaces
//
//==========================================================================

#ifdef DJGPP
static TCvarI d_blt_func("d_blt_func", "0", CVAR_ARCHIVE);

static void Blit_LBL(void)
{
  int i;
  unsigned int temppointer;
  int pitch = ScreenWidth * PixelBytes;
  int mcnt = pitch >> 2;

  temppointer = (unsigned int)scrn;
  for (i = 0; i < ScreenHeight; i++, temppointer += pitch)
  {
    _movedatal(_my_ds(), temppointer, screen->seg, (unsigned int)screen->line[i], mcnt);
  }
}

static void Blit_Banked(void)
{
  int i;
  unsigned long temppointer, destpointer;
  int pitch = ScreenWidth * PixelBytes;
  int mcnt = pitch >> 2;

  temppointer = (unsigned long)scrn;
  for (i = 0; i < ScreenHeight; i++)
  {
    destpointer = bmp_write_line(screen, i);
    _movedatal(_my_ds(), temppointer, screen->seg, destpointer, mcnt);
    temppointer += pitch;
  }
}
#endif

void TSoftwareDrawer::Update(void)
{
#ifdef DJGPP
	if (is_linear_bitmap(screen))
	{
		if (d_blt_func == 1)
		{
			Blit_LBL();
			return;
		}
		if (d_blt_func == 2)
		{
			Blit_Banked();
			return;
		}
	}
#endif
	blit(gamebitmap, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}

//==========================================================================
//
// 	TSoftwareDrawer::Shutdown
//
// 	Restore text mode
//
//==========================================================================

void TSoftwareDrawer::Shutdown(void)
{
    set_gfx_mode(GFX_TEXT, 80, 25, 0, 0);
}

//**************************************************************************
//
//	$Log$
//	Revision 1.6  2001/10/27 07:47:52  dj_jl
//	Public gamma variables
//
//	Revision 1.5  2001/09/12 17:32:10  dj_jl
//	Made my_create_bitmap static
//	
//	Revision 1.4  2001/08/17 17:43:40  dj_jl
//	LINUX fixes
//	
//	Revision 1.3  2001/07/31 17:16:30  dj_jl
//	Just moved Log to the end of file
//	
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
