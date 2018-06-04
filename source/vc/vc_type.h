//**************************************************************************
//**
//**  ##   ##    ##    ##   ##   ####     ####   ###     ###
//**  ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**   ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**   ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**    ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**     #    ##    ##    #      ####     ####   ##       ##
//**
//**  $Id$
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
//**
//**  This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**  This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************


// ////////////////////////////////////////////////////////////////////////// //
enum EType {
  TYPE_Void,
  TYPE_Int,
  TYPE_Byte,
  TYPE_Bool,
  TYPE_Float,
  TYPE_Name,
  TYPE_String,
  TYPE_Pointer,
  TYPE_Reference,
  TYPE_Class,
  TYPE_State,
  TYPE_Delegate,
  TYPE_Struct,
  TYPE_Vector,
  TYPE_Array,
  TYPE_DynamicArray,
  TYPE_Unknown,
  TYPE_Automatic, // this is valid only for variable declarations, and will be resolved to actual type

  NUM_BASIC_TYPES
};


// ////////////////////////////////////////////////////////////////////////// //
class VFieldType {
public:
  vuint8 Type;
  vuint8 InnerType; // for pointers
  vuint8 ArrayInnerType; // for arrays
  vuint8 PtrLevel;
  vint32 ArrayDim;
  union {
    vuint32 BitMask;
    VClass *Class; // class of the reference
    VStruct *Struct; // struct data
    VMethod *Function; // function of the delegate type
  };

  VFieldType ();
  VFieldType (EType Atype);
  explicit VFieldType (VClass *InClass);
  explicit VFieldType (VStruct *InStruct);

  friend VStream &operator << (VStream &, VFieldType &);

  bool Equals (const VFieldType &) const;
  VFieldType MakePointerType () const;
  VFieldType GetPointerInnerType () const;
  VFieldType MakeArrayType (int, const TLocation &) const;
  VFieldType MakeDynamicArrayType (const TLocation &) const;
  VFieldType GetArrayInnerType () const;
  int GetStackSize () const;
  int GetSize () const;
  int GetAlignment () const;
  void CheckPassable (const TLocation &) const;
  bool CheckMatch (const TLocation &loc, const VFieldType &, bool raiseError=true) const;
  VStr GetName () const;

  bool IsReusingDisabled () const;
};


// ////////////////////////////////////////////////////////////////////////// //
struct VObjectDelegate {
  VObject *Obj;
  VMethod *Func;
};


// ////////////////////////////////////////////////////////////////////////// //
// dynamic array object, used in script executor
class VScriptArray {
private:
  int ArrNum;
  int ArrSize;
  vuint8 *ArrData;

public:
#if defined(VCC_STANDALONE_EXECUTOR)
  VScriptArray (const TArray<VStr> &xarr);
#endif

  inline int Num () const { return ArrNum; }
  inline vuint8 *Ptr () { return ArrData; }
  void Clear (VFieldType &Type);
  void Resize (int NewSize, VFieldType &Type);
  void SetNum (int NewNum, VFieldType &Type);
  void SetNumMinus (int NewNum, VFieldType &Type);
  void SetNumPlus (int NewNum, VFieldType &Type);
  void Insert (int Index, int Count, VFieldType &Type);
  void Remove (int Index, int Count, VFieldType &Type);
};


// ////////////////////////////////////////////////////////////////////////// //
struct FReplacedString {
  int Index;
  bool Replaced;
  VStr Old;
  VStr New;
};
