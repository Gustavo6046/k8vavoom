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

#include "vcc.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	InitInfoTables
//
//==========================================================================

void InitInfoTables()
{
}

//==========================================================================
//
//	FindState
//
//==========================================================================

VState* CheckForState(VName StateName, VClass* InClass)
{
	for (TArray<VMemberBase*>::TIterator It(VMemberBase::GMembers); It; ++It)
	{
		if ((*It)->MemberType == MEMBER_State && (*It)->Name == StateName &&
			(*It)->Outer == InClass)
		{
			return (VState*)*It;
		}
	}
	if (InClass->ParentClass)
	{
		return CheckForState(StateName, InClass->ParentClass);
	}
	return NULL;
}

//==========================================================================
//
//	FindState
//
//==========================================================================

static VState* FindState(VName StateName, VClass* InClass)
{
	for (TArray<VMemberBase*>::TIterator It(VMemberBase::GMembers); It; ++It)
	{
		if ((*It)->MemberType == MEMBER_State && (*It)->Name == StateName &&
			(*It)->Outer == InClass)
		{
			return (VState*)*It;
		}
	}
	if (InClass->ParentClass)
	{
		return FindState(StateName, InClass->ParentClass);
	}
	ParseError("No such state %s", *StateName);
	return NULL;
}

//==========================================================================
//
//	ParseStates
//
//==========================================================================

void ParseStates(VClass* InClass)
{
	if (!InClass && TK_Check(PU_LPAREN))
	{
		InClass = CheckForClass();
		if (!InClass)
		{
			ParseError("Class name expected");
		}
		TK_Expect(PU_RPAREN, ERR_MISSING_RPAREN);
	}
	TK_Expect(PU_LBRACE, ERR_MISSING_LBRACE);
	while (!TK_Check(PU_RBRACE))
	{
		//	State identifier
		if (tk_Token != TK_IDENTIFIER)
		{
			ERR_Exit(ERR_INVALID_IDENTIFIER, true, NULL);
		}
		VState* s = new VState(tk_Name, InClass, tk_Location);
		InClass->AddState(s);
		TK_NextToken();
		TK_Expect(PU_LPAREN, ERR_MISSING_LPAREN);
		//	Sprite name
		if (tk_Token != TK_NAME)
		{
			ParseError(ERR_NONE, "Sprite name expected");
		}
		if (tk_Name != NAME_None && strlen(*tk_Name) != 4)
		{
			ParseError(ERR_NONE, "Invalid sprite name");
		}
		s->SpriteName = tk_Name;
		TK_NextToken();
		TK_Expect(PU_COMMA, ERR_NONE);
		//  Frame
		s->frame = EvalConstExpression(InClass, ev_int);
		TK_Expect(PU_COMMA, ERR_NONE);
		if (tk_Token == TK_NAME)
		{
			//	Model
			s->ModelName = tk_Name;
			TK_NextToken();
			TK_Expect(PU_COMMA, ERR_NONE);
			//  Frame
			s->model_frame = EvalConstExpression(InClass, ev_int);
			TK_Expect(PU_COMMA, ERR_NONE);
		}
		else
		{
			s->ModelName = NAME_None;
			s->model_frame = 0;
		}
		//  Tics
		s->time = ConstFloatExpression();
		TK_Expect(PU_COMMA, ERR_NONE);
		//  Next state
		if (tk_Token != TK_IDENTIFIER &&
			(tk_Token != TK_KEYWORD || tk_Keyword != KW_NONE))
		{
			ERR_Exit(ERR_NONE, true, NULL);
		}
		TK_NextToken();
		TK_Expect(PU_RPAREN, ERR_NONE);
		//	Code
		s->function = ParseStateCode(InClass, s);
	}
}

//==========================================================================
//
//	AddToMobjInfo
//
//==========================================================================

void AddToMobjInfo(int Index, VClass* Class)
{
	int i = mobj_info.Add();
	mobj_info[i].doomednum = Index;
	mobj_info[i].class_id = Class;
}

//==========================================================================
//
//	AddToScriptIds
//
//==========================================================================

void AddToScriptIds(int Index, VClass* Class)
{
	int i = script_ids.Add();
	script_ids[i].doomednum = Index;
	script_ids[i].class_id = Class;
}

//==========================================================================
//
//	SkipStates
//
//==========================================================================

void SkipStates(VClass* InClass)
{
	if (!InClass && TK_Check(PU_LPAREN))
	{
		InClass = CheckForClass();
		if (!InClass)
		{
			ParseError("Class name expected");
		}
		TK_Expect(PU_RPAREN, ERR_MISSING_RPAREN);
	}
	TK_Expect(PU_LBRACE, ERR_MISSING_LBRACE);
	while (!TK_Check(PU_RBRACE))
	{
		VState* s = FindState(tk_Name, InClass);

		//	State identifier
		if (tk_Token != TK_IDENTIFIER)
		{
			ERR_Exit(ERR_INVALID_IDENTIFIER, true, NULL);
		}
		TK_NextToken();
		TK_Expect(PU_LPAREN, ERR_MISSING_LPAREN);
		//	Sprite name
		TK_NextToken();
		TK_Expect(PU_COMMA, ERR_NONE);
		//  Frame
		EvalConstExpression(InClass, ev_int);
		TK_Expect(PU_COMMA, ERR_NONE);
		if (tk_Token == TK_NAME)
		{
			TK_NextToken();
			TK_Expect(PU_COMMA, ERR_NONE);
			//  Frame
			EvalConstExpression(InClass, ev_int);
			TK_Expect(PU_COMMA, ERR_NONE);
		}
		//  Tics
		ConstFloatExpression();
		TK_Expect(PU_COMMA, ERR_NONE);
		//  Next state
		if (tk_Token == TK_KEYWORD && tk_Keyword == KW_NONE)
		{
			s->nextstate = NULL;
		}
		else
		{
			s->nextstate = FindState(tk_Name, InClass);
		}
		TK_NextToken();
		TK_Expect(PU_RPAREN, ERR_NONE);
		//	Code
		CompileStateCode(InClass, s->function);
	}
}

//**************************************************************************
//
//	$Log$
//	Revision 1.35  2006/03/23 18:30:54  dj_jl
//	Use single list of all members, members tree.
//
//	Revision 1.34  2006/03/12 20:04:50  dj_jl
//	States as objects, added state variable type.
//	
//	Revision 1.33  2006/03/10 19:31:55  dj_jl
//	Use serialisation for progs files.
//	
//	Revision 1.32  2006/02/28 19:17:20  dj_jl
//	Added support for constants.
//	
//	Revision 1.31  2006/02/27 21:23:54  dj_jl
//	Rewrote names class.
//	
//	Revision 1.30  2006/02/15 23:27:06  dj_jl
//	Added script ID class attribute.
//	
//	Revision 1.29  2006/01/10 19:29:10  dj_jl
//	Fixed states belonging to a class.
//	
//	Revision 1.28  2005/12/14 20:53:23  dj_jl
//	State names belong to a class.
//	Structs and enums defined in a class.
//	
//	Revision 1.27  2005/12/12 20:58:47  dj_jl
//	Removed compiler limitations.
//	
//	Revision 1.26  2005/12/07 22:52:55  dj_jl
//	Moved compiler generated data out of globals.
//	
//	Revision 1.25  2005/11/29 19:31:43  dj_jl
//	Class and struct classes, removed namespaces, beautification.
//	
//	Revision 1.24  2005/11/24 20:42:05  dj_jl
//	Renamed opcodes, cleanup and improvements.
//	
//	Revision 1.23  2005/04/28 07:00:40  dj_jl
//	Temporary fix for crash with optimisations.
//	
//	Revision 1.22  2003/10/22 06:42:55  dj_jl
//	Added function name
//	
//	Revision 1.21  2003/03/08 12:47:52  dj_jl
//	Code cleanup.
//	
//	Revision 1.20  2002/08/24 14:45:38  dj_jl
//	2 pass compiling.
//	
//	Revision 1.19  2002/07/20 14:53:34  dj_jl
//	Switched to dynamic arrays.
//	
//	Revision 1.18  2002/06/14 15:33:45  dj_jl
//	Some fixes.
//	
//	Revision 1.17  2002/02/22 18:11:53  dj_jl
//	Removed misc fields from states.
//	
//	Revision 1.16  2002/02/16 16:28:36  dj_jl
//	Added support for bool variables
//	
//	Revision 1.15  2002/01/17 18:19:52  dj_jl
//	New style of adding to mobjinfo, some fixes
//	
//	Revision 1.14  2002/01/12 18:06:34  dj_jl
//	New style of state functions, some other changes
//	
//	Revision 1.13  2002/01/11 18:21:49  dj_jl
//	Started to use names in progs
//	
//	Revision 1.12  2002/01/11 08:17:31  dj_jl
//	Added name subsystem, removed support for unsigned ints
//	
//	Revision 1.11  2002/01/07 12:31:36  dj_jl
//	Changed copyright year
//	
//	Revision 1.10  2001/12/18 19:09:41  dj_jl
//	Some extra info in progs and other small changes
//	
//	Revision 1.9  2001/12/12 19:22:22  dj_jl
//	Support for method usage as state functions, dynamic cast
//	Added dynamic arrays
//	
//	Revision 1.8  2001/12/01 18:17:09  dj_jl
//	Fixed calling of parent method, speedup
//	
//	Revision 1.7  2001/11/09 14:42:28  dj_jl
//	References, beautification
//	
//	Revision 1.6  2001/10/22 17:28:02  dj_jl
//	Removed mobjinfo index constants
//	
//	Revision 1.5  2001/10/02 17:44:52  dj_jl
//	Some optimizations
//	
//	Revision 1.4  2001/09/27 17:05:57  dj_jl
//	Removed spawn functions, added mobj classes
//	
//	Revision 1.3  2001/08/21 17:52:54  dj_jl
//	Added support for real string pointers, beautification
//	
//	Revision 1.2  2001/07/27 14:27:56  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
