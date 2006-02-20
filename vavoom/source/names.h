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
//**
//**	Header file registering global hardcoded Vavoom names.
//**
//**************************************************************************

// Macros ------------------------------------------------------------------

// Define a message as an enumeration.
#ifndef REGISTER_NAME
	#define REGISTER_NAME(name)	NAME_##name,
	#define REGISTERING_ENUM
	enum EName {
#endif

// Hardcoded names ---------------------------------------------------------

// Special zero value, meaning no name.
REGISTER_NAME(None)

// Log messages.
REGISTER_NAME(Log)
REGISTER_NAME(Init)
REGISTER_NAME(Dev)
REGISTER_NAME(DevNet)

//	Native class names.
REGISTER_NAME(Object)
REGISTER_NAME(Thinker)
REGISTER_NAME(LevelInfo)
REGISTER_NAME(GameInfo)
REGISTER_NAME(Entity)
REGISTER_NAME(ViewEntity)
REGISTER_NAME(ACS)
REGISTER_NAME(Level)
REGISTER_NAME(GC)
REGISTER_NAME(Window)
REGISTER_NAME(ModalWindow)
REGISTER_NAME(RootWindow)
REGISTER_NAME(ClientGameBase)
REGISTER_NAME(ClientState)

// Closing -----------------------------------------------------------------

#ifdef REGISTERING_ENUM
		NUM_HARDCODED_NAMES
	};
	#undef REGISTER_NAME
	#undef REGISTERING_ENUM
#endif

//**************************************************************************
//
//	$Log$
//	Revision 1.10  2006/02/20 22:52:56  dj_jl
//	Changed client state to a class.
//
//	Revision 1.9  2006/02/09 22:35:54  dj_jl
//	Moved all client game code to classes.
//	
//	Revision 1.8  2005/12/27 22:24:00  dj_jl
//	Created level info class, moved action special handling to it.
//	
//	Revision 1.7  2005/11/22 19:10:38  dj_jl
//	Cleaned up a bit.
//	
//	Revision 1.6  2004/08/21 19:10:44  dj_jl
//	Changed sound driver declaration.
//	
//	Revision 1.5  2004/08/21 17:22:15  dj_jl
//	Changed rendering driver declaration.
//	
//	Revision 1.4  2004/08/21 15:03:07  dj_jl
//	Remade VClass to be standalone class.
//	
//	Revision 1.3  2002/05/18 16:56:34  dj_jl
//	Added FArchive and FOutputDevice classes.
//	
//	Revision 1.2  2002/01/07 12:16:42  dj_jl
//	Changed copyright year
//	
//	Revision 1.1  2001/12/18 18:57:11  dj_jl
//	Added global name subsystem
//	
//**************************************************************************
