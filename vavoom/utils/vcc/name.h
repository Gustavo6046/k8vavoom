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
//**	Vavoom global name types.
//**
//**************************************************************************

// Maximum size of name.
enum {NAME_SIZE	= 64};

// Name index.
typedef int NAME_INDEX;

// Enumeration for finding name.
enum EFindName
{
	FNAME_Find,			// Find a name; return 0 if it doesn't exist.
	FNAME_Add,			// Find a name or add it if it doesn't exist.
};

//==========================================================================
//
//	FNameEntry
//
//==========================================================================

//
// A global name, as stored in the global name table.
//
struct FNameEntry
{
	// Variables.
	NAME_INDEX	Index;				// Index of name in hash.
	FNameEntry*	HashNext;			// Pointer to the next entry in this hash bin's linked list.

	// The name string.
	char		Name[NAME_SIZE];	// Name, variable-sized.

	// Functions.
	friend FNameEntry* AllocateNameEntry(const char* Name, dword Index, 
		FNameEntry* HashNext);
};

//==========================================================================
//
//	FName
//
//==========================================================================

//
// Public name, available to the world.  Names are stored as int indices
// into the name table and every name in Vavoom is stored once
// and only once in that table.  Names are case-sensitive.
//
class FName 
{
public:
	// Accessors.
	const char* operator*() const
	{
		return Names[Index]->Name;
	}
	NAME_INDEX GetIndex() const
	{
		return Index;
	}
	bool operator == (const FName& Other) const
	{
		return Index == Other.Index;
	}
	bool operator != (const FName& Other) const
	{
		return Index != Other.Index;
	}
	bool IsValid()
	{
		return Index >= 0 && Index < Names.Num() && Names[Index] != NULL;
	}

	// Constructors.
	FName(enum EName N) : Index(N)
	{}
	FName()
	{}
	FName(const char* Name, EFindName FindType = FNAME_Add);

	// Name subsystem.
	static void StaticInit();
	static void StaticExit();

	// Name subsystem accessors.
	static const char* SafeString(EName Index)
	{
		return Initialised ? Names[Index]->Name : "Uninitialised";
	}
	static int GetMaxNames()
	{
		return Names.Num();
	}
	static FNameEntry* GetEntry(int i)
	{
		return Names[i];
	}
	static bool GetInitialised()
	{
		return Initialised;
	}

private:
	// Name index.
	NAME_INDEX Index;

	// Static subsystem variables.
	static TArray<FNameEntry*>	Names;			 // Table of all names.
	static FNameEntry*			NameHash[4096];  // Hashed names.
	static bool					Initialised;	 // Subsystem initialised.
};
inline dword GetTypeHash(const FName N)
{
	return N.GetIndex();
}

//**************************************************************************
//
//	$Log$
//	Revision 1.2  2005/11/24 20:41:07  dj_jl
//	Cleaned up a bit.
//
//	Revision 1.1  2002/01/11 08:17:31  dj_jl
//	Added name subsystem, removed support for unsigned ints
//	
//**************************************************************************
