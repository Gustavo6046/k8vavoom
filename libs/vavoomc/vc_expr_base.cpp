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
//**  Copyright (C) 2018-2021 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, version 3 of the License ONLY.
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
//#define VCC_DEBUG_COMPILER_LEAKS
//#define VCC_DEBUG_COMPILER_LEAKS_CHECKS

#include "vc_local.h"


#if defined(VCC_DEBUG_COMPILER_LEAKS) || 0
#include <string>
#include <cstdlib>
#include <cxxabi.h>

template<typename T> VStr shitppTypeName () {
  VStr tpn(typeid(T).name());
  char *dmn = abi::__cxa_demangle(*tpn, nullptr, nullptr, nullptr);
  if (dmn) {
    tpn = VStr(dmn);
    //Z_Free(dmn);
    // use `free()` here, because it is not allocated via zone allocator
    free(dmn);
  }
  return tpn;
}


template<class T> VStr shitppTypeNameObj (const T &o) {
  VStr tpn(typeid(o).name());
  char *dmn = abi::__cxa_demangle(*tpn, nullptr, nullptr, nullptr);
  if (dmn) {
    tpn = VStr(dmn);
    //Z_Free(dmn);
    // use `free()` here, because it is not allocated via zone allocator
    free(dmn);
  }
  return tpn;
}
# define GET_MY_TYPE()  (VStr("{")+shitppTypeNameObj(*this)+"}")
#else
# define GET_MY_TYPE()  VStr()
#endif


#if defined(VCC_DEBUG_COMPILER_LEAKS)
struct MemInfo {
  void *ptr;
  size_t size;
  MemInfo *prev, *next;
};

static MemInfo *allocInfoHead = nullptr;
static MemInfo *allocInfoTail = nullptr;


static void *AllocMem (size_t sz) {
  MemInfo *mi = (MemInfo *)Z_Malloc(sz+sizeof(MemInfo));
  mi->ptr = (vuint8 *)(mi+1)+sizeof(size_t);
  mi->size = sz-sizeof(size_t);
  mi->prev = allocInfoTail;
  mi->next = nullptr;
  if (allocInfoTail) allocInfoTail->next = mi; else allocInfoHead = mi;
  allocInfoTail = mi;
  return mi+1;
}


static void FreeMem (void *p) {
  if (!p) return;
  MemInfo *mi = ((MemInfo *)p)-1;
  if (mi->prev) mi->prev->next = mi->next; else allocInfoHead = mi->next;
  if (mi->next) mi->next->prev = mi->prev; else allocInfoTail = mi->prev;
  Z_Free(mi);
}


#if defined(VCC_DEBUG_COMPILER_LEAKS_CHECKS)
static size_t CalcMem () {
  size_t res = 0;
  for (MemInfo *mi = allocInfoHead; mi; mi = mi->next) res += mi->size;
  return res;
}


static void DumpAllocs () {
  fprintf(stderr, "=== ALLOCS ===\n");
  for (MemInfo *mi = allocInfoHead; mi; mi = mi->next) {
    fprintf(stderr, "address: %p; size: %u\n", mi->ptr, (unsigned)mi->size);
  }
  fprintf(stderr, "---\n");
}
#endif


void VExpression::ReportLeaks () {
  GLog.WriteLine("================================= LEAKS =================================");
  for (MemInfo *mi = allocInfoHead; mi; mi = mi->next) {
    GLog.WriteLine("address: %p", mi->ptr);
    auto e = (VExpression *)mi->ptr;
    GLog.WriteLine("  type: %s (loc: %s:%d)", *shitppTypeNameObj(*e), *e->Loc.GetSource(), e->Loc.GetLine());
  }
  GLog.WriteLine("---");
}


#else
# define AllocMem  malloc
# define FreeMem   free
void VExpression::ReportLeaks () {
}
#endif


//==========================================================================
//
//  VExpression::VExpression
//
//==========================================================================
VExpression::VExpression (const TLocation &ALoc)
  : Type(TYPE_Void)
  , RealType(TYPE_Void)
  , Flags(0)
  , Loc(ALoc)
{
}


//==========================================================================
//
//  VExpression::~VExpression
//
//==========================================================================
VExpression::~VExpression () {
}


//==========================================================================
//
//  VExpression::DoRestSyntaxCopyTo
//
//==========================================================================
//#include <typeinfo>
void VExpression::DoSyntaxCopyTo (VExpression *e) {
  //fprintf(stderr, "  ***VExpression::DoSyntaxCopyTo for `%s`\n", typeid(*e).name());
  e->Type = Type;
  e->RealType = RealType;
  e->Flags = Flags;
  e->Loc = Loc;
}


//==========================================================================
//
//  VExpression::Resolve
//
//==========================================================================
VExpression *VExpression::Resolve (VEmitContext &ec) {
  VExpression *e = DoResolve(ec);
  return e;
}


//==========================================================================
//
//  VExpression::ResolveBoolean
//
//==========================================================================
VExpression *VExpression::ResolveBoolean (VEmitContext &ec) {
  VExpression *e = Resolve(ec);
  if (!e) return nullptr;
  return e->CoerceToBool(ec);
}


//==========================================================================
//
//  VExpression::CoerceToBool
//
//==========================================================================
VExpression *VExpression::CoerceToBool (VEmitContext &ec) {
  switch (Type.Type) {
    case TYPE_Int:
    case TYPE_Byte:
    case TYPE_Bool:
      break;
    case TYPE_Float:
      return (new VFloatToBool(this, true))->Resolve(ec);
    case TYPE_Name:
      return (new VNameToBool(this, true))->Resolve(ec);
    case TYPE_Pointer:
    case TYPE_Reference:
    case TYPE_Class:
    case TYPE_State:
      return (new VPointerToBool(this, true))->Resolve(ec);
    case TYPE_String:
      return (new VStringToBool(this, true))->Resolve(ec);
      break;
    case TYPE_Delegate:
      return (new VDelegateToBool(this, true))->Resolve(ec);
      break;
    case TYPE_Vector:
      return (new VVectorToBool(this, true))->Resolve(ec);
    case TYPE_DynamicArray:
      return (new VDynArrayToBool(this, true))->Resolve(ec);
    case TYPE_Dictionary:
      return (new VDictToBool(this, true))->Resolve(ec);
    case TYPE_SliceArray:
      ParseWarning(Loc, "Coercing slice to boolean is not tested yet");
      return (new VSliceToBool(this, true))->Resolve(ec);
    default:
      ParseError(Loc, "Expression type mismatch, boolean expression expected, got `%s`", *Type.GetName());
      delete this;
      return nullptr;
  }
  return this;
}


//==========================================================================
//
//  VExpression::IsBoolLiteral
//
//  returns -1 if cannot determine, or 0/1
//
//==========================================================================
int VExpression::IsBoolLiteral (VEmitContext &ec) const {
  if (IsIntConst()) return (GetIntConst() ? 1 : 0);
  if (IsFloatConst()) return (isZeroInfNaN(GetFloatConst()) ? 0 : 1); // so inf/nan will yield `false`
  if (IsNameConst()) return (GetNameConst() != NAME_None ? 1 : 0);
  if (IsStrConst()) return (GetStrConst(ec.Package).isEmpty() ? 0 : 1);
  if (IsNoneLiteral()) return 0;
  if (IsNoneDelegateLiteral()) return 0;
  if (IsNullLiteral()) return 0;
  if (IsConstVectorCtor()) return (((const VVectorExpr *)this)->GetConstValue().toBool() ? 1 : 0);
  return -1;
}


//==========================================================================
//
//  VExpression::ResolveFloat
//
//==========================================================================
VExpression *VExpression::ResolveFloat (VEmitContext &ec) {
  VExpression *e = Resolve(ec);
  if (!e) return nullptr;
  switch (e->Type.Type) {
    case TYPE_Int:
    case TYPE_Byte:
    //case TYPE_Bool:
      e = (new VScalarToFloat(e, true))->Resolve(ec);
      break;
    case TYPE_Float:
      break;
    default:
      ParseError(e->Loc, "Expression type mismatch, float expression expected");
      delete e;
      return nullptr;
  }
  return e;
}


//==========================================================================
//
//  VExpression::CoerceToFloat
//
//  Expression MUST be already resolved here.
//
//==========================================================================
VExpression *VExpression::CoerceToFloat (VEmitContext &ec) {
  if (Type.Type == TYPE_Float) return this; // nothing to do
  if (Type.Type == TYPE_Int || Type.Type == TYPE_Byte) {
    if (IsIntConst()) {
      VExpression *e = new VFloatLiteral((float)GetIntConst(), Loc);
      delete this;
      return e->Resolve(ec);
    }
    return (new VScalarToFloat(this, true))->Resolve(ec);
  }
  ParseError(Loc, "Expression type mismatch, float expression expected");
  delete this;
  return nullptr;
}


//==========================================================================
//
//  workerCoerceOp1None
//
//==========================================================================
static bool workerCoerceOp1None (VEmitContext &ec, VExpression *&op1, VExpression *&op2, bool coerceNoneDelegate) {
  if (!op1 || !op2) return false;
  if (op1->IsNoneLiteral() && !op2->IsNoneLiteral() && !op2->IsNullLiteral()) {
    switch (op2->Type.Type) {
      case TYPE_Reference:
      case TYPE_Class:
      case TYPE_State:
        op1->Type = op2->Type;
        return true;
      //k8: delegate coercing requires turning `none` to two different types:
      //    `none class/object` and `none delegate`
      case TYPE_Delegate:
        if (coerceNoneDelegate) {
          VNoneDelegateLiteral *nl = new VNoneDelegateLiteral(op1->Loc);
          delete op1;
          op1 = nl->Resolve(ec); //???
          if (!op1) return false;
        }
        return true;
    }
    return true;
  }
  return false;
}


//==========================================================================
//
//  workerCoerceOp1Null
//
//==========================================================================
static bool workerCoerceOp1Null (VEmitContext &ec, VExpression *&op1, VExpression *&op2) {
  if (!op1 || !op2) return false;
  if (op1->IsNullLiteral() && !op2->IsNoneLiteral() && !op2->IsNullLiteral()) {
    switch (op2->Type.Type) {
      case TYPE_Pointer:
        op1->Type = op2->Type;
        return true;
    }
    return true;
  }
  return false;
}


//==========================================================================
//
//  VExpression::CoerceTypes
//
//  this coerces ints to floats, and fixes `none`/`nullptr` type
//
//==========================================================================
void VExpression::CoerceTypes (VEmitContext &ec, VExpression *&op1, VExpression *&op2, bool coerceNoneDelegate) {
  if (!op1 || !op2) return; // oops
  // if one operand is vector, and other operand is integer, coerce integer to float
  // this is required for float vs scalar operators
  if (op1->Type.Type == TYPE_Vector && (op2->Type.Type == TYPE_Int || op2->Type.Type == TYPE_Byte)) {
    op2 = op2->CoerceToFloat(ec);
    return;
  }
  if (op2->Type.Type == TYPE_Vector && (op1->Type.Type == TYPE_Int || op1->Type.Type == TYPE_Byte)) {
    op1 = op1->CoerceToFloat(ec);
    return;
  }
  // coerce to float
  if ((op1->Type.Type == TYPE_Float || op2->Type.Type == TYPE_Float) &&
      (op1->Type.Type == TYPE_Int || op2->Type.Type == TYPE_Int ||
       op1->Type.Type == TYPE_Byte || op2->Type.Type == TYPE_Byte))
  {
    op1 = op1->CoerceToFloat(ec);
    op2 = op2->CoerceToFloat(ec);
    return;
  }
  // coerce `none`
  if (workerCoerceOp1None(ec, op1, op2, coerceNoneDelegate)) return;
  if (workerCoerceOp1None(ec, op2, op1, coerceNoneDelegate)) return;
  // coerce `nullptr`
  if (workerCoerceOp1Null(ec, op1, op2)) return;
  if (workerCoerceOp1Null(ec, op2, op1)) return;
}


//==========================================================================
//
//  VExpression::ResolveAsType
//
//==========================================================================
VTypeExpr *VExpression::ResolveAsType (VEmitContext &) {
  ParseError(Loc, "Invalid type expression");
  delete this;
  return nullptr;
}


//==========================================================================
//
//  VExpression::ResolveAssignmentTarget
//
//==========================================================================
VExpression *VExpression::ResolveAssignmentTarget (VEmitContext &ec) {
  return Resolve(ec);
}


//==========================================================================
//
//  VExpression::ResolveAssignmentValue
//
//==========================================================================
VExpression *VExpression::ResolveAssignmentValue (VEmitContext &ec) {
  return Resolve(ec);
}


//==========================================================================
//
//  VExpression::ResolveIterator
//
//==========================================================================
VExpression *VExpression::ResolveIterator (VEmitContext &) {
  ParseError(Loc, "Iterator method expected");
  delete this;
  return nullptr;
}


//==========================================================================
//
//  VExpression::ResolveCompleteAssign
//
//==========================================================================
VExpression *VExpression::ResolveCompleteAssign (VEmitContext &ec, VExpression *val, bool &resolved) {
  // do nothing
  return this;
}


//==========================================================================
//
//  VExpression::RequestAddressOf
//
//==========================================================================
void VExpression::RequestAddressOf () {
  ParseError(Loc, "Bad address operation");
}


//==========================================================================
//
//  VExpression::RequestAddressOfForAssign
//
//==========================================================================
void VExpression::RequestAddressOfForAssign () {
  RequestAddressOf();
}


//==========================================================================
//
//  VExpression::EmitBranchable
//
//==========================================================================
void VExpression::EmitBranchable (VEmitContext &ec, VLabel Lbl, bool OnTrue) {
  Emit(ec);
  if (OnTrue) {
    ec.AddStatement(OPC_IfGoto, Lbl, Loc);
  } else {
    ec.AddStatement(OPC_IfNotGoto, Lbl, Loc);
  }
}


//==========================================================================
//
//  VExpression::EmitPushPointedCode
//
//==========================================================================
void VExpression::EmitPushPointedCode (VFieldType type, VEmitContext &ec) {
  ec.EmitPushPointedCode(type, Loc);
}


//==========================================================================
//
//  VExpression::ResolveToIntLiteralEx
//
//  this resolves one-char strings and names to int literals too
//
//==========================================================================
VExpression *VExpression::ResolveToIntLiteralEx (VEmitContext &ec, bool allowFloatTrunc) {
  VExpression *res = Resolve(ec);
  if (!res) return nullptr; // we are dead anyway

  // easy case
  if (res->IsIntConst()) return res;

  // truncate floats?
  if (allowFloatTrunc && res->IsFloatConst()) {
    VExpression *e = new VIntLiteral((int)res->GetFloatConst(), res->Loc);
    delete res;
    return e->Resolve(ec);
  }

  // one-char string?
  if (res->IsStrConst()) {
    VStr s = res->GetStrConst(ec.Package);
    if (s.length() == 1) {
      VExpression *e = new VIntLiteral((vuint8)s[0], res->Loc);
      delete res;
      return e->Resolve(ec);
    }
  }

  // one-char name?
  if (res->IsNameConst()) {
    const char *s = *res->GetNameConst();
    if (s && s[0] && !s[1]) {
      VExpression *e = new VIntLiteral((vuint8)s[0], res->Loc);
      delete res;
      return e->Resolve(ec);
    }
  }

  ParseError(res->Loc, "Integer constant expected");
  delete res;
  return nullptr;
}


// ////////////////////////////////////////////////////////////////////////// //
// IsXXX
VExpression *VExpression::AddDropResult () { return nullptr; } // not processed
bool VExpression::IsValidTypeExpression () const { return false; }
bool VExpression::IsIntConst () const { return false; }
bool VExpression::IsFloatConst () const { return false; }
bool VExpression::IsStrConst () const { return false; }
bool VExpression::IsNameConst () const { return false; }
bool VExpression::IsClassNameConst () const { return false; }
vint32 VExpression::GetIntConst () const { ParseError(Loc, "Integer constant expected"); return 0; }
float VExpression::GetFloatConst () const { ParseError(Loc, "Float constant expected"); return 0.0f; }
const VStr &VExpression::GetStrConst (VPackage *) const { ParseError(Loc, "String constant expected"); return VStr::EmptyString; }
VName VExpression::GetNameConst () const { ParseError(Loc, "Name constant expected"); return NAME_None; }
bool VExpression::IsNoneLiteral () const { return false; }
bool VExpression::IsNoneDelegateLiteral () const { return false; }
bool VExpression::IsNullLiteral () const { return false; }
bool VExpression::IsDefaultObject () const { return false; }
bool VExpression::IsPropertyAssign () const { return false; }
bool VExpression::IsDynArraySetNum () const { return false; }
bool VExpression::IsDecorateSingleName () const { return false; }
bool VExpression::IsDecorateUserVar () const { return false; }
bool VExpression::IsLocalVarDecl () const { return false; }
bool VExpression::IsLocalVarExpr () const { return false; }
bool VExpression::IsAssignExpr () const { return false; }
bool VExpression::IsParens () const { return false; }
bool VExpression::IsUnaryMath () const { return false; }
bool VExpression::IsUnaryMutator () const { return false; }
bool VExpression::IsBinaryMath () const { return false; }
bool VExpression::IsBinaryLogical () const { return false; }
bool VExpression::IsTernary () const { return false; }
bool VExpression::IsSingleName () const { return false; }
bool VExpression::IsDoubleName () const { return false; }
bool VExpression::IsDotField () const { return false; }
bool VExpression::IsMarshallArg () const { return false; }
bool VExpression::IsRefArg () const { return false; }
bool VExpression::IsOutArg () const { return false; }
bool VExpression::IsOptMarshallArg () const { return false; }
bool VExpression::IsDefaultArg () const { return false; }
bool VExpression::IsNamedArg () const { return false; }
VName VExpression::GetArgName () const { return NAME_None; }
bool VExpression::IsAnyInvocation () const { return false; }
bool VExpression::IsLLInvocation () const { return false; }
bool VExpression::IsTypeExpr () const { return false; }
bool VExpression::IsAutoTypeExpr () const { return false; }
bool VExpression::IsSimpleType () const { return false; }
bool VExpression::IsReferenceType () const { return false; }
bool VExpression::IsClassType () const { return false; }
bool VExpression::IsPointerType () const { return false; }
bool VExpression::IsAnyArrayType () const { return false; }
bool VExpression::IsDictType () const { return false; }
bool VExpression::IsStaticArrayType () const { return false; }
bool VExpression::IsDynamicArrayType () const { return false; }
bool VExpression::IsDelegateType () const { return false; }
bool VExpression::IsSliceType () const { return false; }
bool VExpression::IsVectorCtor () const { return false; }
bool VExpression::IsConstVectorCtor () const { return false; }
bool VExpression::IsComma () const { return false; }
bool VExpression::IsCommaRetOp0 () const { return false; }
bool VExpression::IsDropResult () const { return false; }
//VStr VExpression::toString () const { return VStr("<VExpression:")+GET_MY_TYPE()+":no-toString>"; }
VStr VExpression::GetMyTypeName () const { return GET_MY_TYPE(); }


// ////////////////////////////////////////////////////////////////////////// //
// memory allocation
vuint32 VExpression::TotalMemoryUsed = 0;
vuint32 VExpression::CurrMemoryUsed = 0;
vuint32 VExpression::PeakMemoryUsed = 0;
vuint32 VExpression::TotalMemoryFreed = 0;
bool VExpression::InCompilerCleanup = false;


//==========================================================================
//
//  VExpression::operator new
//
//==========================================================================
void *VExpression::operator new (size_t size) {
  size_t *res = (size_t *)AllocMem(size+sizeof(size_t));
  if (!res) VCFatalError("OUT OF MEMORY!");
  *res = size;
  ++res;
  if (size) memset(res, 0, size);
  TotalMemoryUsed += (vuint32)size;
  CurrMemoryUsed += (vuint32)size;
  if (PeakMemoryUsed < CurrMemoryUsed) PeakMemoryUsed = CurrMemoryUsed;
  //fprintf(stderr, "* new: %u (%p)\n", (unsigned)size, res);
#if defined(VCC_DEBUG_COMPILER_LEAKS) && defined(VCC_DEBUG_COMPILER_LEAKS_CHECKS)
  if (CalcMem() != CurrMemoryUsed) {
    fprintf(stderr, "NEW CALC: %u\nNEW CURR: %u\n", (unsigned)CalcMem(), (unsigned)CurrMemoryUsed);
    DumpAllocs();
    abort();
  }
#endif
  return res;
}


//==========================================================================
//
//  VExpression::operator new[]
//
//==========================================================================
void *VExpression::operator new[] (size_t size) {
  size_t *res = (size_t *)AllocMem(size+sizeof(size_t));
  if (!res) VCFatalError("OUT OF MEMORY!");
  *res = size;
  ++res;
  if (size) memset(res, 0, size);
  TotalMemoryUsed += (vuint32)size;
  CurrMemoryUsed += (vuint32)size;
  if (PeakMemoryUsed < CurrMemoryUsed) PeakMemoryUsed = CurrMemoryUsed;
  //fprintf(stderr, "* new[]: %u (%p)\n", (unsigned)size, res);
#if defined(VCC_DEBUG_COMPILER_LEAKS) && defined(VCC_DEBUG_COMPILER_LEAKS_CHECKS)
  if (CalcMem() != CurrMemoryUsed) {
    fprintf(stderr, "NEW[] CALC: %u\nNEW[] CURR: %u\n", (unsigned)CalcMem(), (unsigned)CurrMemoryUsed);
    DumpAllocs();
    abort();
  }
#endif
  return res;
}


//==========================================================================
//
//  VExpression::operator delete
//
//==========================================================================
void VExpression::operator delete (void *p) {
  if (p) {
    //fprintf(stderr, "* del: %u (%p)\n", (vuint32)*((size_t *)p-1), p);
    if (InCompilerCleanup) TotalMemoryFreed += (vuint32)*((size_t *)p-1);
    CurrMemoryUsed -= (vuint32)*((size_t *)p-1);
    FreeMem(((size_t *)p-1));
#if defined(VCC_DEBUG_COMPILER_LEAKS) && defined(VCC_DEBUG_COMPILER_LEAKS_CHECKS)
    if (CalcMem() != CurrMemoryUsed) {
      fprintf(stderr, "DEL CALC: %u\nDEL CURR: %u\n", (unsigned)CalcMem(), (unsigned)CurrMemoryUsed);
      DumpAllocs();
      abort();
    }
#endif
  }
}


//==========================================================================
//
//  VExpression::operator delete[]
//
//==========================================================================
void VExpression::operator delete[] (void *p) {
  if (p) {
    //fprintf(stderr, "* del[]: %u (%p)\n", (vuint32)*((size_t *)p-1), p);
    if (InCompilerCleanup) TotalMemoryFreed += (vuint32)*((size_t *)p-1);
    CurrMemoryUsed -= (vuint32)*((size_t *)p-1);
    FreeMem(((size_t *)p-1));
#if defined(VCC_DEBUG_COMPILER_LEAKS) && defined(VCC_DEBUG_COMPILER_LEAKS_CHECKS)
    if (CalcMem() != CurrMemoryUsed) {
      fprintf(stderr, "DEL[] CALC: %u\nDEL[] CURR: %u\n", (unsigned)CalcMem(), (unsigned)CurrMemoryUsed);
      DumpAllocs();
      abort();
    }
#endif
  }
}
