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
//**	Copyright (C) 1999-2002 J�nis Legzdi��
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

#include "gamedefs.h"
#include "cl_local.h"
#include "../utils/glbsp/glbsp.h"

// MACROS ------------------------------------------------------------------

#define MESSAGE1	"VAVOOM IS NOW CREATING THE GWA FILE..."
#define MESSAGE2	"THIS ONLY HAS TO BE DONE ONCE FOR THIS WAD"

#define BARX		32
#define BAR1Y		96
#define BAR2Y		160
#define BARW		(320 - 2 * BARX)
#define BARH		8

#define BARTEXTX	32
#define BARTEXT1Y	64
#define BARTEXT2Y	140

// TYPES -------------------------------------------------------------------

struct gb_bar_t
{
	float x;
	float w;
	float y1;
	float y2;

	int limit;
	float position;

	char text[64];
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char message_buf[1024];

static gb_bar_t bars[2];

static float barborderw;
static float barborderh;

static displaytype_e CurrentDisplay;
static double lastprog;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	GLBSP_Draw
//
//==========================================================================

static void GLBSP_Draw(void)
{
	Drawer->StartUpdate();

	Drawer->FillRect(0, 0, ScreenWidth, ScreenHeight, 0xff000000);

	T_SetFont(font_small);
	T_SetAlign(hcenter, vcenter);
	T_DrawText(160, 16, MESSAGE1);
	T_DrawText(160, 32, MESSAGE2);
	T_SetAlign(hleft, vtop);

	int i;

	int num_bars;

	switch (CurrentDisplay)
	{
	case DIS_BUILDPROGRESS:
		num_bars = 2;
		break;

	case DIS_FILEPROGRESS:
		num_bars = 1;
		break;

	default:
		return;
	}

	for (i = 0; i < num_bars; i++)
	{
		gb_bar_t &b = bars[i];
		Drawer->FillRect(b.x - barborderw, b.y1 - barborderh,
			b.x + b.w + barborderw, b.y2 + barborderh, 0xffff0000);
		Drawer->FillRect(b.x, b.y1, b.x + b.w, b.y2, 0xff000000);
		Drawer->FillRect(b.x, b.y1, b.x + b.w * b.position, b.y2, 0xff00ff00);
	}

	T_DrawText(BARTEXTX, BARTEXT1Y, bars[0].text);
	if (num_bars > 1)
	{
		T_DrawText(BARTEXTX, BARTEXT2Y, bars[1].text);
	}

	Drawer->Update();
}

//==========================================================================
//
//	GLBSP_PrintMsg
//
//==========================================================================

static void GLBSP_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	GCon->Logf("GB: %s", message_buf);
}

//==========================================================================
//
//	GLBSP_FatalError
//
//	Terminates the program reporting an error.
//
//==========================================================================

static void GLBSP_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	Sys_Error("Builing nodes failed: %s\n", message_buf);
}

//==========================================================================
//
//	GLBSP_Ticker
//
//==========================================================================

static void GLBSP_Ticker(void)
{
}

//==========================================================================
//
// GLBSP_DisplayOpen
//
//==========================================================================

static boolean_g GLBSP_DisplayOpen(displaytype_e type)
{
	CurrentDisplay = type;
	GLBSP_Draw();
	return true;
}

//==========================================================================
//
//	GLBSP_DisplaySetTitle
//
//==========================================================================

static void GLBSP_DisplaySetTitle(const char *)
{
	// does nothing
}

//==========================================================================
//
//	GLBSP_DisplaySetBarText
//
//==========================================================================

static void GLBSP_DisplaySetBarText(int barnum, const char *str)
{
	gb_bar_t &b = bars[barnum - 1];
	strcpy(b.text, str);
	b.position = 0;
	GLBSP_Draw();
}

//==========================================================================
//
//	GLBSP_DisplaySetBarLimit
//
//==========================================================================

static void GLBSP_DisplaySetBarLimit(int barnum, int limit)
{
	bars[barnum - 1].limit = limit;
}

//==========================================================================
//
//	GLBSP_DisplaySetBar
//
//==========================================================================

static void GLBSP_DisplaySetBar(int barnum, int count)
{
	gb_bar_t &b = bars[barnum - 1];
	b.position = float(count) / float(b.limit);
	if (barnum == 1 && count > 0 && count < b.limit &&
		Sys_Time() - lastprog < 0.2)
	{
		return;
	}
	Drawer->BeginDirectUpdate();
	Drawer->FillRect(b.x, b.y1, b.x + b.w * b.position, b.y2, 0xff00ff00);
	Drawer->EndDirectUpdate();
	if (barnum == 1)
	{
		lastprog = Sys_Time();
	}
}

//==========================================================================
//
//	GLBSP_DisplayClose
//
//==========================================================================

static void GLBSP_DisplayClose(void)
{
	// does nothing
}

const nodebuildfuncs_t edge_build_funcs =
{
	GLBSP_FatalError,
	GLBSP_PrintMsg,
	GLBSP_Ticker,

	GLBSP_DisplayOpen,
	GLBSP_DisplaySetTitle,
	GLBSP_DisplaySetBar,
	GLBSP_DisplaySetBarLimit,
	GLBSP_DisplaySetBarText,
	GLBSP_DisplayClose
};

//==========================================================================
//
//	GLBSP_BuildNodes
//
//	Attempt to build nodes for the WAD file containing the given
// map_lump (a lump number from w_wad for the start marker, e.g.
// "MAP01").  Returns true if successful, false if it failed.
//
//==========================================================================

bool GLBSP_BuildNodes(const char *name)
{
	nodebuildinfo_t nb_info;
	nodebuildcomms_t nb_comms;
	glbsp_ret_e ret;

	nb_info = default_buildinfo;
	nb_comms = default_buildcomms;

	nb_info.input_file = name;

	// FIXME: check parm "-node-factor"

	bars[0].x = BARX * fScaleX;
	bars[0].w = BARW * fScaleX;
	bars[0].y1 = BAR1Y * fScaleY;
	bars[0].y2 = (BAR1Y + BARH) * fScaleY;
	bars[1].x = BARX * fScaleX;
	bars[1].w = BARW * fScaleX;
	bars[1].y1 = BAR2Y * fScaleY;
	bars[1].y2 = (BAR2Y + BARH) * fScaleY;
	barborderw = 2 * fScaleX;
	barborderh = 2 * fScaleY;

	if (GLBSP_E_OK != GlbspCheckInfo(&nb_info, &nb_comms))
		return false;

	ret = GlbspBuildNodes(&nb_info, &edge_build_funcs, &nb_comms);

	if (ret != GLBSP_E_OK)
		return false;

	return true;
}

//==========================================================================
//
//	COMMAND glBSP
//
//==========================================================================

COMMAND(glBSP)
{
	if (Argc() > 1)
	{
		nodebuildinfo_t nb_info;
		nodebuildcomms_t nb_comms;

		nb_info = default_buildinfo;
		nb_comms = default_buildcomms;

		if (GlbspParseArgs(&nb_info, &nb_comms, (const char**)Cmd_Argv() + 1,
			Argc() - 1) == GLBSP_E_OK)
		{
			bars[0].x = BARX * fScaleX;
			bars[0].w = BARW * fScaleX;
			bars[0].y1 = BAR1Y * fScaleY;
			bars[0].y2 = (BAR1Y + BARH) * fScaleY;
			bars[1].x = BARX * fScaleX;
			bars[1].w = BARW * fScaleX;
			bars[1].y1 = BAR2Y * fScaleY;
			bars[1].y2 = (BAR2Y + BARH) * fScaleY;
			barborderw = 2 * fScaleX;
			barborderh = 2 * fScaleY;

			if (GlbspCheckInfo(&nb_info, &nb_comms) == GLBSP_E_OK)
			{
				GlbspBuildNodes(&nb_info, &edge_build_funcs, &nb_comms);
			}
			else
			{
				GCon->Log("Check info failed");
			}
		}
		else
		{
			GCon->Log("Bad arguments");
		}
	}
}

//**************************************************************************
//
//	$Log$
//	Revision 1.6  2003/10/23 06:36:15  dj_jl
//	Parsing all args
//
//	Revision 1.5  2002/07/23 16:29:56  dj_jl
//	Replaced console streams with output device class.
//	
//	Revision 1.4  2002/01/07 12:16:42  dj_jl
//	Changed copyright year
//	
//	Revision 1.3  2001/09/20 16:23:40  dj_jl
//	Beautification
//	
//	Revision 1.2  2001/09/14 16:52:14  dj_jl
//	Added dynamic build of GWA file
//	
//	Revision 1.1  2001/09/12 17:37:47  dj_jl
//	Added glBSP and glVIS plugins
//	
//**************************************************************************
