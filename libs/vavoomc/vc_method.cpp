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
#include "vc_local.h"
#include "vc_mcopt.cpp"


// ////////////////////////////////////////////////////////////////////////// //
FBuiltinInfo *FBuiltinInfo::Builtins;


//==========================================================================
//
//  FBuiltinInfo::FBuiltinInfo
//
//==========================================================================
FBuiltinInfo::FBuiltinInfo (const char *InName, VClass *InClass, builtin_t InFunc)
  : Name(InName)
  , OuterClass(InClass)
  , Func(InFunc)
{
  Next = Builtins;
  Builtins = this;
  //fprintf(stderr, "!!! <%s::%s>\n", *InClass->Name, InName);
}


//==========================================================================
//
//  PF_Fixme
//
//==========================================================================
/*
static void PF_Fixme () {
  VObject::VMDumpCallStack();
  Sys_Error("unimplemented bulitin");
}
*/


//==========================================================================
//
//  VMethodParam::VMethodParam
//
//==========================================================================
VMethodParam::VMethodParam () : TypeExpr(nullptr), Name(NAME_None) {
}


//==========================================================================
//
//  VMethodParam::~VMethodParam
//
//==========================================================================
VMethodParam::~VMethodParam () {
  delete TypeExpr; TypeExpr = nullptr;
}


//==========================================================================
//
//  VMethodParam::clear
//
//==========================================================================
void VMethodParam::clear () {
  delete TypeExpr; TypeExpr = nullptr;
}


//==========================================================================
//
//  VMethod::VMethod
//
//==========================================================================
VMethod::VMethod (VName AName, VMemberBase *AOuter, TLocation ALoc)
  : VMemberBase(MEMBER_Method, AName, AOuter, ALoc)
  , mPostLoaded(false)
  , NumLocals(0)
  , Flags(0)
  , ReturnType(TYPE_Void)
  , NumParams(0)
  , ParamsSize(0)
  , SuperMethod(nullptr)
  , ReplCond(nullptr)
  , ReturnTypeExpr(nullptr)
  , Statement(nullptr)
  //, SelfTypeName(NAME_None)
  , lmbCount(0)
  , printfFmtArgIdx(-1)
  , builtinOpc(-1)
  , Profile1(0)
  , Profile2(0)
  , NativeFunc(0)
  , VTableIndex(-666)
  , NetIndex(0)
  , NextNetMethod(0)
  , defineResult(-1)
  , emitCalled(false)
{
  memset(ParamFlags, 0, sizeof(ParamFlags));
}


//==========================================================================
//
//  VMethod::~VMethod
//
//==========================================================================
VMethod::~VMethod() {
  delete ReturnTypeExpr; ReturnTypeExpr = nullptr;
  delete Statement; Statement = nullptr;
}


//==========================================================================
//
//  VMethod::CompilerShutdown
//
//==========================================================================
void VMethod::CompilerShutdown () {
  VMemberBase::CompilerShutdown();
  for (int f = 0; f < NumParams; ++f) Params[f].clear();
  delete ReturnTypeExpr; ReturnTypeExpr = nullptr;
  delete Statement; Statement = nullptr;
}


//==========================================================================
//
//  VMethod::Serialise
//
//==========================================================================
void VMethod::Serialise (VStream &Strm) {
  VMemberBase::Serialise(Strm);
  vuint8 xver = 0; // current version is 0
  Strm << xver;
  Strm << SuperMethod
    << STRM_INDEX(NumLocals)
    << STRM_INDEX(Flags)
    << ReturnType
    << STRM_INDEX(NumParams)
    << STRM_INDEX(ParamsSize)
    << STRM_INDEX(lmbCount)
    << STRM_INDEX(printfFmtArgIdx)
    << STRM_INDEX(builtinOpc)
  ;
  for (int i = 0; i < NumParams; ++i) {
    // save param names too
    Strm << ParamTypes[i] << ParamFlags[i] << Params[i].Name;
  }
  Strm << ReplCond << Instructions;
}


//==========================================================================
//
//  VMethod::Define
//
//==========================================================================
bool VMethod::Define () {
  if (emitCalled) Sys_Error("`Define()` after `Emit()` for %s", *GetFullName());
  if (defineResult >= 0) {
    GLog.Logf(NAME_Debug, "`Define()` for `%s` already called!", *GetFullName());
    return (defineResult > 0);
  }
  defineResult = 0;

  bool Ret = true;

  if (Flags&FUNC_Static) {
    if ((Flags&FUNC_Final) == 0) {
      ParseError(Loc, "Currently static methods must be final");
      Ret = false;
    }
  }

  if ((Flags&FUNC_VarArgs) != 0 && (Flags&FUNC_Native) == 0) {
    ParseError(Loc, "Only native methods can have varargs");
    Ret = false;
  }

  if ((Flags&(FUNC_Iterator|FUNC_Native)) == FUNC_Iterator) {
    ParseError(Loc, "Iterators can only be native");
    Ret = false;
  }

  VEmitContext ec(this);

  if (ReturnTypeExpr) ReturnTypeExpr = ReturnTypeExpr->ResolveAsType(ec);
  if (ReturnTypeExpr) {
    VFieldType t = ReturnTypeExpr->Type;
    if (t.Type != TYPE_Void) {
      // function's return type must be void, vector or with size 4
      t.CheckReturnable(ReturnTypeExpr->Loc);
    }
    ReturnType = t;
  } else {
    Ret = false;
  }

  // resolve parameters types
  ParamsSize = (Flags&FUNC_Static ? 0 : 1); // first is `self`
  for (int i = 0; i < NumParams; ++i) {
    VMethodParam &P = Params[i];

    if (P.TypeExpr) P.TypeExpr = P.TypeExpr->ResolveAsType(ec);
    if (!P.TypeExpr) { Ret = false; continue; }
    VFieldType type = P.TypeExpr->Type;

    if (type.Type == TYPE_Void) {
      ParseError(P.TypeExpr->Loc, "Bad variable type");
      Ret = false;
      continue;
    }

    ParamTypes[i] = type;
    //if ((ParamFlags[i]&FPARM_Optional) != 0 && (ParamFlags[i]&FPARM_Out) != 0) ParseError(P.Loc, "Modifiers `optional` and `out` are mutually exclusive");
    //if ((ParamFlags[i]&FPARM_Optional) != 0 && (ParamFlags[i]&FPARM_Ref) != 0) ParseError(P.Loc, "Modifiers `optional` and `ref` are mutually exclusive");
    if ((ParamFlags[i]&FPARM_Out) != 0 && (ParamFlags[i]&FPARM_Ref) != 0) ParseError(P.Loc, "Modifiers `out` and `ref` are mutually exclusive");

    if (ParamFlags[i]&(FPARM_Out|FPARM_Ref)) {
      ++ParamsSize;
    } else {
      if (!type.CheckPassable(P.TypeExpr->Loc, false)) ParseError(P.TypeExpr->Loc, "Invalid parameter #%d type '%s' in method '%s'", i+1, *type.GetName(), *GetFullName());
      ParamsSize += type.GetStackSize()/4;
    }
    if (ParamFlags[i]&FPARM_Optional) ++ParamsSize;
  }

  // if this is a overridden method, verify that return type and argument types match
  SuperMethod = nullptr;
  if (Outer->MemberType == MEMBER_Class && Name != NAME_None && ((VClass *)Outer)->ParentClass) {
    SuperMethod = ((VClass *)Outer)->ParentClass->FindMethod(Name);
  }

  if (SuperMethod) {
    if (VMemberBase::optDeprecatedLaxOverride || VMemberBase::koraxCompatibility) Flags |= FUNC_Override; // force `override`
    if ((Flags&FUNC_Override) == 0) {
      ParseError(Loc, "Overriding virtual method `%s` without `override` keyword", *GetFullName());
      Ret = false;
    }
    if (Ret && (SuperMethod->Flags&FUNC_Private) != 0) {
      ParseError(Loc, "Overriding private method `%s` is not allowed", *GetFullName());
      Ret = false;
    }
    if (Ret && (Flags&FUNC_Private) != 0) {
      ParseError(Loc, "Overriding with private method `%s` is not allowed", *GetFullName());
      Ret = false;
    }
    if (Ret && (SuperMethod->Flags&FUNC_Protected) != (Flags&FUNC_Protected)) {
      if ((SuperMethod->Flags&FUNC_Protected)) {
        ParseError(Loc, "Cannot override protected method `%s` with public one", *GetFullName());
        Ret = false;
      } else {
        //FIXME: not yet implemented
        ParseError(Loc, "Cannot override public method `%s` with protected one", *GetFullName());
        Ret = false;
      }
    }
    if (Ret && (SuperMethod->Flags&FUNC_Final)) {
      ParseError(Loc, "Method `%s` already has been declared as final and cannot be overridden", *GetFullName());
      Ret = false;
    }
    if (!SuperMethod->ReturnType.Equals(ReturnType)) {
      if (Ret) ParseError(Loc, "Method `%s` redefined with different return type", *GetFullName());
      Ret = false;
    } else if (SuperMethod->NumParams != NumParams) {
      if (Ret) ParseError(Loc, "Method `%s` redefined with different number of arguments", *GetFullName());
      Ret = false;
    } else {
      for (int i = 0; i < NumParams; ++i) {
        if (!SuperMethod->ParamTypes[i].Equals(ParamTypes[i])) {
          if (Ret) {
            ParseError(Loc, "Type of argument #%d differs from base class (expected `%s`, got `%s`)", i+1,
              *SuperMethod->ParamTypes[i].GetName(), *ParamTypes[i].GetName());
          }
          Ret = false;
        }
        if ((SuperMethod->ParamFlags[i]^ParamFlags[i])&(FPARM_Optional|FPARM_Out|FPARM_Ref|FPARM_Const)) {
          if (Ret) ParseError(Loc, "Modifiers of argument #%d differs from base class", i+1);
          Ret = false;
        }
      }
    }

    // inherit network flags
    Flags |= SuperMethod->Flags&FUNC_NetFlags;
  } else {
    if ((Flags&FUNC_Override) != 0) {
      ParseError(Loc, "Trying to override non-existing method `%s`", *GetFullName());
      Ret = false;
    }
  }

  if (Flags&FUNC_Spawner) {
    // verify that it's a valid spawner method
    if (NumParams < 1) {
      ParseError(Loc, "Spawner method must have at least 1 argument");
    } else if (ParamTypes[0].Type != TYPE_Class) {
      ParseError(Loc, "Spawner method must have class as first argument");
    } else if (ReturnType.Type != TYPE_Reference && ReturnType.Type != TYPE_Class) {
      ParseError(Loc, "Spawner method must return an object reference or class");
    } else if (ReturnType.Class != ParamTypes[0].Class) {
      // hack for `SpawnObject (class)`
      if (ParamTypes[0].Class || ReturnType.Class->Name != "Object") {
        ParseError(Loc, "Spawner method must return an object of the same type as class");
      }
    }
  }

  defineResult = (Ret ? 1 : 0);
  return Ret;
}


//==========================================================================
//
//  VMethod::Emit
//
//==========================================================================
void VMethod::Emit () {
  if (defineResult < 0) Sys_Error("`Emit()` before `Define()` for %s", *GetFullName());
  if (emitCalled) {
    GLog.Logf(NAME_Debug, "`Emit()` for `%s` already called!", *GetFullName());
    return;
  }
  emitCalled = true;

  if (Flags&FUNC_Native) {
    if (Statement) ParseError(Loc, "Native methods can't have a body");
    return;
  }

  if (Outer->MemberType == MEMBER_Field) return; // delegate

  if (!Statement) { ParseError(Loc, "Method body missing"); return; }

  // no need to do it here, as optimizer will do it for us
  /*
  if (ReturnTypeExpr && ReturnTypeExpr->Type.Type != TYPE_Void) {
    if (!Statement->IsEndsWithReturn()) {
      ParseError(Loc, "Missing `return` in one of the pathes of function `%s`", *GetFullName());
      return;
    }
  }
  */

  //fprintf(stderr, "*** EMIT000: <%s> (%s); ParamsSize=%d; NumLocals=%d; NumParams=%d\n", *GetFullName(), *Loc.toStringNoCol(), ParamsSize, NumLocals, NumParams);

  VEmitContext ec(this);

  ec.ClearLocalDefs();
  ec.localsofs = (Flags&FUNC_Static ? 0 : 1); // first is `self`
  if (Outer->MemberType == MEMBER_Class && this == ((VClass *)Outer)->DefaultProperties) {
    ec.InDefaultProperties = true;
  }

  for (int i = 0; i < NumParams; ++i) {
    VMethodParam &P = Params[i];
    if (P.Name != NAME_None) {
      auto oldlofs = ec.localsofs;
      if (ec.CheckForLocalVar(P.Name) != -1) ParseError(P.Loc, "Redefined argument `%s`", *P.Name);
      VLocalVarDef &L = ec.AllocLocal(P.Name, ParamTypes[i], P.Loc);
      ec.localsofs = oldlofs;
      L.Offset = ec.localsofs;
      L.Visible = true;
      L.ParamFlags = ParamFlags[i];
    }
    if (ParamFlags[i]&(FPARM_Out|FPARM_Ref)) {
      ++ec.localsofs;
    } else {
      ec.localsofs += ParamTypes[i].GetStackSize()/4;
    }
    if (ParamFlags[i]&FPARM_Optional) {
      if (P.Name != NAME_None) {
        auto oldlofs = ec.localsofs;
        VLocalVarDef &L = ec.AllocLocal(va("specified_%s", *P.Name), TYPE_Int, P.Loc);
        ec.localsofs = oldlofs;
        L.Offset = ec.localsofs;
        L.Visible = true;
        L.ParamFlags = 0;
      }
      ++ec.localsofs;
    }
  }

  for (int i = 0; i < ec.GetLocalDefCount(); ++i) {
    const VLocalVarDef &loc = ec.GetLocalByIndex(i);
    if (loc.Type.Type == TYPE_Vector && (ParamFlags[i]&(FPARM_Out|FPARM_Ref)) == 0) {
      ec.AddStatement(OPC_VFixParam, loc.Offset, Loc);
    }
  }

  if (!Statement->Resolve(ec)) {
    //ParseError(Loc, "Cannot resolve statements in `%s`", *GetFullName());
    //fprintf(stderr, "===\n%s\n===\n", /*Statement->toString()*/*shitppTypeNameObj(*Statement));
    return;
  }

  Statement->Emit(ec);

  if (ReturnType.Type == TYPE_Void) {
    ec.EmitFinalizers(); // just in case we have function-global finalizers
    ec.EmitLocalDtors(0, ec.GetLocalDefCount(), Loc);
    ec.AddStatement(OPC_Return, Loc);
  }
  NumLocals = ec.localsofs;
  ec.EndCode();

       if (VMemberBase::doAsmDump) DumpAsm();
  else if (VObject::cliAsmDumpMethods.has(VStr(Name))) DumpAsm();

  OptimizeInstructions();

  // and dump it again for optimized case
       if (VMemberBase::doAsmDump) DumpAsm();
  else if (VObject::cliAsmDumpMethods.has(VStr(Name))) DumpAsm();

  // do not clear statement list here (it will be done in `CompilerShutdown()`)
}


//==========================================================================
//
//  VMethod::FindArgByName
//
//  <0: not found
//
//==========================================================================
int VMethod::FindArgByName (VName aname) const {
  if (aname == NAME_None) return -1;
  for (int f = 0; f < NumParams; ++f) {
    if (Params[f].Name == aname) return f;
  }
  return -1;
}


//==========================================================================
//
//  VMethod::DumpAsm
//
//  disassembles a method
//
//==========================================================================
void VMethod::DumpAsm () {
  VMemberBase *PM = Outer;
  while (PM->MemberType != MEMBER_Package) PM = PM->Outer;
  VPackage *Package = (VPackage *)PM;

  GLog.Logf(NAME_Debug, "--------------------------------------------");
  GLog.Logf(NAME_Debug, "Dump ASM function %s.%s (%d instructions)", *Outer->Name, *Name, (Flags&FUNC_Native ? 0 : Instructions.Num()));
  if (Flags&FUNC_Native) {
    //  Builtin function
    GLog.Logf(NAME_Debug, "*** Builtin function.");
    return;
  }
  for (int s = 0; s < Instructions.Num(); ++s) {
    // opcode
    VStr disstr;
    int st = Instructions[s].Opcode;
    disstr += va("%6d: %s", s, StatementInfo[st].name);
    switch (StatementInfo[st].Args) {
      case OPCARGS_None:
        break;
      case OPCARGS_Member:
        // name of the object
        disstr += va(" %s", *Instructions[s].Member->GetFullName());
        break;
      case OPCARGS_BranchTargetB:
      case OPCARGS_BranchTargetNB:
      //case OPCARGS_BranchTargetS:
      case OPCARGS_BranchTarget:
        disstr += va(" %6d", Instructions[s].Arg1);
        break;
      case OPCARGS_ByteBranchTarget:
      case OPCARGS_ShortBranchTarget:
      case OPCARGS_IntBranchTarget:
        disstr += va(" %6d, %6d", Instructions[s].Arg1, Instructions[s].Arg2);
        break;
      case OPCARGS_NameBranchTarget:
        disstr += va(" '%s', %6d", *VName(EName(Instructions[s].Arg1)), Instructions[s].Arg2);
        break;
      case OPCARGS_Byte:
      case OPCARGS_Short:
        disstr += va(" %6d (%x)", Instructions[s].Arg1, Instructions[s].Arg1);
        break;
      case OPCARGS_Int:
        if (Instructions[s].Arg1IsFloat) {
          disstr += va(" %f", *(const float *)&Instructions[s].Arg1);
        } else {
          disstr += va(" %6d (%x)", Instructions[s].Arg1, Instructions[s].Arg1);
        }
        break;
      case OPCARGS_Name:
        // name
        disstr += va(" '%s'", *Instructions[s].NameArg);
        break;
      case OPCARGS_String:
        // string
        disstr += va(" %s", *Package->GetStringByIndex(Instructions[s].Arg1).quote());
        break;
      case OPCARGS_FieldOffset:
        if (Instructions[s].Member) disstr += va(" %s", *Instructions[s].Member->Name); else disstr += va(" (0)");
        break;
      case OPCARGS_VTableIndex:
        disstr += va(" %s", *Instructions[s].Member->Name);
        break;
      case OPCARGS_VTableIndex_Byte:
      case OPCARGS_FieldOffset_Byte:
      case OPCARGS_FieldOffsetS_Byte:
        if (Instructions[s].Member) disstr += va(" %s %d", *Instructions[s].Member->Name, Instructions[s].Arg2); else disstr += va(" (0)%d", Instructions[s].Arg2);
        break;
      case OPCARGS_TypeSize:
      case OPCARGS_Type:
      case OPCARGS_A2DDimsAndSize:
        disstr += va(" %s", *Instructions[s].TypeArg.GetName());
        break;
      case OPCARGS_TypeDD:
        disstr += va(" %s!(%s,%s)", StatementDictDispatchInfo[Instructions[s].Arg2].name, *Instructions[s].TypeArg.GetName(), *Instructions[s].TypeArg1.GetName());
        break;
      case OPCARGS_TypeAD:
        disstr += va(" %s!(%s)", StatementDynArrayDispatchInfo[Instructions[s].Arg2].name, *Instructions[s].TypeArg.GetName());
        break;
      case OPCARGS_Builtin:
        disstr += va(" %s", StatementBuiltinInfo[Instructions[s].Arg1].name);
        break;
      case OPCARGS_Member_Int:
        disstr += va(" %s (%d)", *Instructions[s].Member->GetFullName(), Instructions[s].Arg2);
        break;
      case OPCARGS_Type_Int:
        disstr += va(" %s (%d)", *Instructions[s].TypeArg.GetName(), Instructions[s].Arg2);
        break;
      case OPCARGS_ArrElemType_Int:
        disstr += va(" %s (%d)", *Instructions[s].TypeArg.GetName(), Instructions[s].Arg2);
        break;
    }
    GLog.Logf(NAME_Debug, "%s", *disstr);
  }
}


//==========================================================================
//
//  VMethod::PostLoad
//
//==========================================================================
void VMethod::PostLoad () {
  //k8: it should be called only once, but let's play safe here
  if (mPostLoaded) {
    //GLog.Logf(NAME_Debug, "method `%s` was already postloaded", *GetFullName());
    return;
  }

  if (defineResult < 0) Sys_Error("`Define()` not called for `%s`", *GetFullName());
  if (!emitCalled) {
    // delegate declarations creates methods without a code, and without a name; tolerate those!
    if (Name != NAME_None || Instructions.length() != 0) {
      Sys_Error("`Emit()` not called before `PostLoad()` for `%s`", *GetFullName());
    }
    emitCalled = true; // why not?
  }

  // check if owning class correctly postloaded
  // but don't do this for anonymous methods -- they're prolly created for delegates (FIXME: change this!)
  if (VTableIndex < -1) {
    if (Name != NAME_None) {
      VMemberBase *origClass = Outer;
      while (origClass) {
        if (origClass->isClassMember()) {
          // check for a valid class
          Sys_Error("owning class `%s` for `%s` wasn't correctly postloaded", origClass->GetName(), *GetFullName());
        } else if (origClass->isStateMember()) {
          // it belongs to a state, so it is a wrapper
          if ((Flags&FUNC_Final) == 0) Sys_Error("state method `%s` is not final", *GetFullName());
          VTableIndex = -1;
          break;
        }
        origClass = origClass->Outer;
      }
      if (!origClass) {
        if ((Flags&FUNC_Final) == 0) Sys_Error("owner-less method `%s` is not final", *GetFullName());
        VTableIndex = -1; // dunno, something strange here
      }
    } else {
      // delegate dummy method (never called anyway)
      VTableIndex = -1; // dunno, something strange here
    }
  }

#if !defined(IN_VCC)
  //GLog.Logf(NAME_Debug, "*** %s: %s", *Loc.toString(), *GetFullName());
  // set up builtins
  if (NumParams > VMethod::MAX_PARAMS) Sys_Error("Function has more than %i params", VMethod::MAX_PARAMS);
  for (FBuiltinInfo *B = FBuiltinInfo::Builtins; B; B = B->Next) {
    if (Outer == B->OuterClass && !VStr::Cmp(*Name, B->Name)) {
      if ((Flags&FUNC_Native) != 0 && builtinOpc < 0) {
        NativeFunc = B->Func;
        break;
      } else {
        Sys_Error("PR_LoadProgs: Builtin %s redefined", B->Name);
      }
    }
  }
  if (builtinOpc < 0 && !NativeFunc && (Flags&FUNC_Native) != 0) {
    // default builtin
    /*
    NativeFunc = PF_Fixme;
    // don't abort with error, because it will be done, when this
    // function will be called (if it will be called)
    GLog.Logf(NAME_Error, "Builtin `%s` not found!", *GetFullName());
    */
    // k8: actually, abort. define all builtins.
    /*
    for (FBuiltinInfo *B = FBuiltinInfo::Builtins; B; B = B->Next) {
      GLog.Logf(NAME_Debug, "BUILTIN: `%s::%s`", *B->OuterClass->Name, B->Name);
    }
    */
    Sys_Error("Builtin `%s` not implemented!", *GetFullName());
  }
#endif

  GenerateCode();

  mPostLoaded = true;
}


//==========================================================================
//
//  VMethod::WriteType
//
//==========================================================================
void VMethod::WriteType (const VFieldType &tp) {
  vuint8 tbuf[VFieldType::MemSize*2];
  vuint8 *ptr = tbuf;
  tp.WriteTypeMem(ptr);
  vassert((ptrdiff_t)(ptr-tbuf) == VFieldType::MemSize);
  for (vuint8 *p = tbuf; p != ptr; ++p) Statements.append(*p);
}


#define WriteUInt8(p)  Statements.Append(p)
#define WriteInt16(p)  Statements.SetNum(Statements.Num()+2); *(vint16 *)&Statements[Statements.Num()-2] = (p)
#define WriteInt32(p)  Statements.SetNum(Statements.Num()+4); *(vint32 *)&Statements[Statements.Num()-4] = (p)
#define WritePtr(p)    Statements.SetNum(Statements.Num()+sizeof(void *)); *(void **)&Statements[Statements.Num()-sizeof(void *)] = (p)


//==========================================================================
//
//  VMethod::GenerateCode
//
//  this generates VM (or other) executable code (to `Statements`)
//  from IR `Instructions`
//==========================================================================
void VMethod::GenerateCode () {
  Statements.Clear();
  if (!Instructions.Num()) return;

  TArray<int> iaddr; // addresses of all generated instructions
  iaddr.resize(Instructions.length()); // we know the size beforehand

  // generate VM bytecode
  for (int i = 0; i < Instructions.Num()-1; ++i) {
    //Instructions[i].Address = Statements.Num();
    vassert(iaddr.length() == i);
    iaddr.append(Statements.length());
    Statements.Append(Instructions[i].Opcode);
    switch (StatementInfo[Instructions[i].Opcode].Args) {
      case OPCARGS_None: break;
      case OPCARGS_Member: WritePtr(Instructions[i].Member); break;
      case OPCARGS_BranchTargetB: WriteUInt8(0); break;
      case OPCARGS_BranchTargetNB: WriteUInt8(0); break;
      //case OPCARGS_BranchTargetS: WriteInt16(0); break;
      case OPCARGS_BranchTarget: WriteInt32(0); break;
      case OPCARGS_ByteBranchTarget: WriteUInt8(Instructions[i].Arg1); WriteInt16(0); break;
      case OPCARGS_ShortBranchTarget: WriteInt16(Instructions[i].Arg1); WriteInt16(0); break;
      case OPCARGS_IntBranchTarget: WriteInt32(Instructions[i].Arg1); WriteInt16(0); break;
      case OPCARGS_NameBranchTarget: WriteInt32(Instructions[i].Arg1); WriteInt16(0); break;
      case OPCARGS_Byte: WriteUInt8(Instructions[i].Arg1); break;
      case OPCARGS_Short: WriteInt16(Instructions[i].Arg1); break;
      case OPCARGS_Int: WriteInt32(Instructions[i].Arg1); break;
      case OPCARGS_Name: WriteInt32(Instructions[i].NameArg.GetIndex()); break;
      case OPCARGS_NameS: WriteInt16(Instructions[i].NameArg.GetIndex()); break;
      case OPCARGS_String: WritePtr((void *)&(GetPackage()->GetStringByIndex(Instructions[i].Arg1))); break;
      case OPCARGS_FieldOffset:
        // make sure struct / class field offsets have been calculated
        if (Instructions[i].Member) {
          Instructions[i].Member->Outer->PostLoad();
          WriteInt32(((VField *)Instructions[i].Member)->Ofs);
        } else {
          WriteInt32(0);
        }
        break;
      case OPCARGS_FieldOffsetS:
        // make sure struct / class field offsets have been calculated
        if (Instructions[i].Member) {
          Instructions[i].Member->Outer->PostLoad();
          WriteInt16(((VField *)Instructions[i].Member)->Ofs);
        } else {
          WriteInt16(0);
        }
        break;
      case OPCARGS_VTableIndex:
        // make sure class virtual table has been calculated
        Instructions[i].Member->Outer->PostLoad();
        WriteInt16(((VMethod *)Instructions[i].Member)->VTableIndex);
        break;
      case OPCARGS_VTableIndexB:
        // make sure class virtual table has been calculated
        Instructions[i].Member->Outer->PostLoad();
        WriteUInt8(((VMethod *)Instructions[i].Member)->VTableIndex);
        break;
      case OPCARGS_VTableIndex_Byte:
        // make sure class virtual table has been calculated
        Instructions[i].Member->Outer->PostLoad();
        WriteInt16(((VMethod *)Instructions[i].Member)->VTableIndex);
        WriteUInt8(Instructions[i].Arg2);
        break;
      case OPCARGS_VTableIndexB_Byte:
        // make sure class virtual table has been calculated
        Instructions[i].Member->Outer->PostLoad();
        WriteUInt8(((VMethod *)Instructions[i].Member)->VTableIndex);
        WriteUInt8(Instructions[i].Arg2);
        break;
      case OPCARGS_FieldOffset_Byte:
        // make sure struct / class field offsets have been calculated
        if (Instructions[i].Member) {
          Instructions[i].Member->Outer->PostLoad();
          WriteInt32(((VField *)Instructions[i].Member)->Ofs);
        } else {
          WriteInt32(0);
        }
        WriteUInt8(Instructions[i].Arg2);
        break;
      case OPCARGS_FieldOffsetS_Byte:
        // make sure struct / class field offsets have been calculated
        if (Instructions[i].Member) {
          Instructions[i].Member->Outer->PostLoad();
          WriteInt16(((VField *)Instructions[i].Member)->Ofs);
        } else {
          WriteInt16(0);
        }
        WriteUInt8(Instructions[i].Arg2);
        break;
      case OPCARGS_TypeSize: WriteInt32(Instructions[i].TypeArg.GetSize()); break;
      case OPCARGS_TypeSizeB: WriteUInt8(Instructions[i].TypeArg.GetSize()); break;
      case OPCARGS_Type: WriteType(Instructions[i].TypeArg); break;
      case OPCARGS_TypeDD:
        WriteType(Instructions[i].TypeArg);
        WriteType(Instructions[i].TypeArg1);
        WriteUInt8(Instructions[i].Arg2);
        break;
      case OPCARGS_A2DDimsAndSize:
        {
          // we got an array type here, but we need size of one array element, so DIY it
          // sorry for code duplication from `VArrayElement::InternalResolve()`
          VFieldType arr = Instructions[i].TypeArg;
          VFieldType itt = arr.GetArrayInnerType();
          if (itt.Type == TYPE_Byte || itt.Type == TYPE_Bool) itt = VFieldType(TYPE_Int);
          WriteInt16(arr.GetFirstDim());
          WriteInt16(arr.GetSecondDim());
          WriteInt32(itt.GetSize());
          //fprintf(stderr, "d0=%d; d1=%d; dim=%d; isz=%d; size=%d\n", arr.GetFirstDim(), arr.GetSecondDim(), arr.GetArrayDim(), itt.GetSize(), arr.GetSize());
        }
        break;
      case OPCARGS_Builtin: WriteUInt8(Instructions[i].Arg1); break;
      case OPCARGS_Member_Int: WritePtr(Instructions[i].Member); break; // int is not emited
      case OPCARGS_Type_Int: WriteInt32(Instructions[i].Arg2); break; // type is not emited
      case OPCARGS_ArrElemType_Int: WriteType(Instructions[i].TypeArg); WriteInt32(Instructions[i].Arg2); break;
      case OPCARGS_TypeAD: WriteType(Instructions[i].TypeArg); WriteUInt8(Instructions[i].Arg2); break;
    }
    while (StatLocs.length() < Statements.length()) StatLocs.Append(Instructions[i].loc);
  }
  //Instructions[Instructions.Num()-1].Address = Statements.Num();
  vassert(iaddr.length() == Instructions.length()-1);
  iaddr.append(Statements.length());

  // fix jump destinations
  for (int i = 0; i < Instructions.Num()-1; ++i) {
    switch (StatementInfo[Instructions[i].Opcode].Args) {
      case OPCARGS_BranchTargetB:
        Statements[iaddr[i]+1] = iaddr[Instructions[i].Arg1]-iaddr[i];
        break;
      case OPCARGS_BranchTargetNB:
        Statements[iaddr[i]+1] = iaddr[i]-iaddr[Instructions[i].Arg1];
        break;
      //case OPCARGS_BranchTargetS:
      //  *(vint16 *)&Statements[iaddr[i]+1] = iaddr[Instructions[i].Arg1]-iaddr[i];
      //  break;
      case OPCARGS_BranchTarget:
        *(vint32 *)&Statements[iaddr[i]+1] = iaddr[Instructions[i].Arg1]-iaddr[i];
        break;
      case OPCARGS_ByteBranchTarget:
        *(vint16 *)&Statements[iaddr[i]+2] = iaddr[Instructions[i].Arg2]-iaddr[i];
        break;
      case OPCARGS_ShortBranchTarget:
        *(vint16 *)&Statements[iaddr[i]+3] = iaddr[Instructions[i].Arg2]-iaddr[i];
        break;
      case OPCARGS_IntBranchTarget:
      case OPCARGS_NameBranchTarget:
        *(vint16 *)&Statements[iaddr[i]+5] = iaddr[Instructions[i].Arg2]-iaddr[i];
        break;
    }
  }

  // we don't need instructions anymore
  Instructions.Clear();
  Statements.condense();
}


//==========================================================================
//
//  VMethod::OptimizeInstructions
//
//==========================================================================
void VMethod::OptimizeInstructions () {
  VMCOptimizer opt(this, Instructions);
  opt.optimizeAll();
  if (vcErrorCount == 0) {
    // do this last, as optimizer can remove some dead code
    opt.checkReturns();
    // this should be done as a last step
    opt.shortenInstructions();
  }
  opt.finish(); // this will copy result back to `Instructions`
}


//==========================================================================
//
//  VMethod::FindPCLocation
//
//==========================================================================
TLocation VMethod::FindPCLocation (const vuint8 *pc) {
  if (!pc || Statements.length() == 0) return TLocation();
  if (pc < Statements.Ptr()) return TLocation();
  size_t stidx = (size_t)(pc-Statements.Ptr());
  if (stidx >= (size_t)Statements.length()) return TLocation();
  if (stidx >= (size_t)StatLocs.length()) return TLocation(); // just in case
  return StatLocs[(int)stidx];
}


//==========================================================================
//
//  VMethod::CleanupParams
//
//  this can be called in `ExecuteNetMethod()` to do cleanup after RPC
//
//==========================================================================
void VMethod::CleanupParams () const {
#if !defined(IN_VCC)
  VStack *Param = pr_stackPtr-ParamsSize+(Flags&FUNC_Static ? 0 : 1); // skip self too
  for (int i = 0; i < NumParams; ++i) {
    switch (ParamTypes[i].Type) {
      case TYPE_Int:
      case TYPE_Byte:
      case TYPE_Bool:
      case TYPE_Float:
      case TYPE_Name:
      case TYPE_Pointer:
      case TYPE_Reference:
      case TYPE_Class:
      case TYPE_State:
        ++Param;
        break;
      case TYPE_String:
        ((VStr *)&Param->p)->clear();
        ++Param;
        break;
      case TYPE_Vector:
        Param += 3;
        break;
      default:
        Sys_Error("Bad method argument type `%s`", *ParamTypes[i].GetName());
    }
    if (ParamFlags[i]&FPARM_Optional) ++Param;
  }
  pr_stackPtr -= ParamsSize;

  // push null return value
  switch (ReturnType.Type) {
    case TYPE_Void:
      break;
    case TYPE_Int:
    case TYPE_Byte:
    case TYPE_Bool:
    case TYPE_Name:
      PR_Push(0);
      break;
    case TYPE_Float:
      PR_Pushf(0);
      break;
    case TYPE_String:
      PR_PushStr(VStr());
      break;
    case TYPE_Pointer:
    case TYPE_Reference:
    case TYPE_Class:
    case TYPE_State:
      PR_PushPtr(nullptr);
      break;
    case TYPE_Vector:
      PR_Pushf(0);
      PR_Pushf(0);
      PR_Pushf(0);
      break;
    default:
      Sys_Error("Bad return value type `%s`", *ReturnType.GetName());
  }
#endif
}


//==========================================================================
//
//  operator <<
//
//==========================================================================
VStream &operator << (VStream &Strm, FInstruction &Instr) {
  vuint8 Opc;
  if (Strm.IsLoading()) {
    Strm << Opc;
    Instr.Opcode = Opc;
  } else {
    Opc = Instr.Opcode;
    Strm << Opc;
  }
  //Strm << Instr.loc;
  switch (StatementInfo[Opc].Args) {
    case OPCARGS_None:
      break;
    case OPCARGS_Member:
    case OPCARGS_FieldOffset:
    case OPCARGS_VTableIndex:
      Strm << Instr.Member;
      break;
    case OPCARGS_VTableIndex_Byte:
    case OPCARGS_FieldOffset_Byte:
      Strm << Instr.Member;
      Strm << STRM_INDEX(Instr.Arg2);
      break;
    case OPCARGS_BranchTarget:
      Strm << Instr.Arg1;
      break;
    case OPCARGS_ByteBranchTarget:
    case OPCARGS_ShortBranchTarget:
    case OPCARGS_IntBranchTarget:
      Strm << STRM_INDEX(Instr.Arg1);
      Strm << Instr.Arg2;
      break;
    case OPCARGS_NameBranchTarget:
      if (Strm.IsLoading()) {
        VName n = NAME_None;
        Strm << n;
        Instr.Arg1 = n.GetIndex();
      } else {
        VName n = VName(EName(Instr.Arg1));
        Strm << n;
      }
      Strm << Instr.Arg2;
      break;
    case OPCARGS_Byte:
    case OPCARGS_Short:
    case OPCARGS_Int:
      Strm << STRM_INDEX(Instr.Arg1);
      break;
    case OPCARGS_Name:
      Strm << Instr.NameArg;
      break;
    case OPCARGS_String:
      Strm << Instr.Arg1;
      break;
    case OPCARGS_TypeSize:
    case OPCARGS_Type:
    case OPCARGS_A2DDimsAndSize:
      Strm << Instr.TypeArg;
      break;
    case OPCARGS_TypeDD:
      Strm << Instr.TypeArg;
      Strm << Instr.TypeArg1;
      Strm << STRM_INDEX(Instr.Arg2);
      break;
    case OPCARGS_Builtin:
      Strm << STRM_INDEX(Instr.Arg1);
      break;
    case OPCARGS_Member_Int:
      Strm << Instr.Member;
      Strm << STRM_INDEX(Instr.Arg2);
      break;
    case OPCARGS_Type_Int:
    case OPCARGS_ArrElemType_Int:
    case OPCARGS_TypeAD:
      Strm << Instr.TypeArg;
      Strm << STRM_INDEX(Instr.Arg2);
      break;
  }
  return Strm;
}