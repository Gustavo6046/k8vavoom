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

#ifdef _MSC_VER
// Disable some unwanted warnings
#pragma warning(disable : 4127) // Conditional expression is constant
#pragma warning(disable : 4244) // conversion to float, possible loss of data
#pragma warning(disable : 4284) // return type is not a UDT or reference to a UDT
#pragma warning(disable : 4291) // no matching operator delete found
#pragma warning(disable : 4305) // truncation from 'const double' to 'float'
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4702) // unreachable code in inline expanded function
#pragma warning(disable : 4710) // inline function not expanded
#endif

#ifndef __GNUC__
#define __attribute__(whatever)
#endif

#if defined __unix__ && !defined DJGPP
#undef stricmp	//	Allegro defines them
#undef strnicmp
#define stricmp		strcasecmp
#define strnicmp	strncasecmp
#endif

//==========================================================================
//
//	Basic types
//
//==========================================================================

#define MINCHAR		((char)0x80)
#define MINSHORT    ((short)0x8000)
#define MININT      ((int)0x80000000L)
#define MINLONG     ((long)0x80000000L)

#define MAXCHAR		((char)0x7f)
#define MAXSHORT    ((short)0x7fff)
#define MAXINT      ((int)0x7fffffff)
#define MAXLONG     ((long)0x7fffffff)

typedef int					boolean;	//	Must be 4 bytes long
typedef unsigned char 		byte;
typedef unsigned short	 	word;
typedef unsigned long	 	dword;

//==========================================================================
//
//	Standard macros
//
//==========================================================================

//	Number of elements in an array.
#define ARRAY_COUNT(array)				(sizeof(array) / sizeof((array)[0]))

//	Offset of a struct member.
#define STRUCT_OFFSET(struc, member)	((int)&((struc *)NULL)->member)

//==========================================================================
//
//	Guard macros
//
//==========================================================================

#ifdef DO_GUARD
#define guard(name)		static const char *__FUNC_NAME__ = #name; try {
#define unguard			} catch (RecoverableError &e) { throw e; } \
	catch (...) { Host_CoreDump(__FUNC_NAME__); throw; }
#define unguardf(msg)	} catch (RecoverableError &e) { throw e; } \
	catch (...) { Host_CoreDump(__FUNC_NAME__); Host_CoreDump msg; throw; }
#else
#define guard(name)		static const char *__FUNC_NAME__ = #name; {
#define unguard			}
#define unguardf(msg)	}
#endif

#ifdef PARANOID
#define guardSlow(name)		guard(name)
#define unguardSlow			unguard
#define unguardfSlow(msg)	unguardf(msg)
#else
#define guardSlow(name)		{
#define unguardSlow			}
#define unguardfSlow(msg)	}
#endif

//==========================================================================
//
//	Errors
//
//==========================================================================

#define MAX_ERROR_TEXT_SIZE		1024

class VavoomError
{
public:
	char message[MAX_ERROR_TEXT_SIZE];

	explicit VavoomError(const char *text)
	{
		strncpy(message, text, MAX_ERROR_TEXT_SIZE - 1);
		message[MAX_ERROR_TEXT_SIZE - 1] = 0;
	}
};

class RecoverableError:public VavoomError
{
public:
	explicit RecoverableError(const char *text) : VavoomError(text) { }
};

//**************************************************************************
//
//	$Log$
//	Revision 1.8  2002/01/21 18:23:56  dj_jl
//	Disabled some MSVC warnings
//
//	Revision 1.7  2002/01/07 12:16:41  dj_jl
//	Changed copyright year
//	
//	Revision 1.6  2002/01/03 18:38:25  dj_jl
//	Added guard macros and core dumps
//	
//	Revision 1.5  2001/12/12 19:27:08  dj_jl
//	Added some macros
//	
//	Revision 1.4  2001/10/08 17:26:17  dj_jl
//	Started to use exceptions
//	
//	Revision 1.3  2001/07/31 17:16:30  dj_jl
//	Just moved Log to the end of file
//	
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
