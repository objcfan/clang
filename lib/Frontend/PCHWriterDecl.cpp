//===--- PCHWriterDecl.cpp - Declaration Serialization --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements serialization for Declarations.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/PCHWriter.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Bitcode/BitstreamWriter.h"
#include "llvm/Support/ErrorHandling.h"
using namespace clang;

//===----------------------------------------------------------------------===//
// Declaration serialization
//===----------------------------------------------------------------------===//

namespace {
  class PCHDeclWriter : public DeclVisitor<PCHDeclWriter, void> {

    PCHWriter &Writer;
    ASTContext &Context;
    PCHWriter::RecordData &Record;

  public:
    pch::DeclCode Code;
    unsigned AbbrevToUse;

    PCHDeclWriter(PCHWriter &Writer, ASTContext &Context,
                  PCHWriter::RecordData &Record)
      : Writer(Writer), Context(Context), Record(Record) {
    }

    void VisitDecl(Decl *D);
    void VisitTranslationUnitDecl(TranslationUnitDecl *D);
    void VisitNamedDecl(NamedDecl *D);
    void VisitNamespaceDecl(NamespaceDecl *D);
    void VisitUsingDirectiveDecl(UsingDirectiveDecl *D);
    void VisitNamespaceAliasDecl(NamespaceAliasDecl *D);
    void VisitTypeDecl(TypeDecl *D);
    void VisitTypedefDecl(TypedefDecl *D);
    void VisitUnresolvedUsingTypename(UnresolvedUsingTypenameDecl *D);
    void VisitTagDecl(TagDecl *D);
    void VisitEnumDecl(EnumDecl *D);
    void VisitRecordDecl(RecordDecl *D);
    void VisitCXXRecordDecl(CXXRecordDecl *D);
    void VisitClassTemplateSpecializationDecl(
                                            ClassTemplateSpecializationDecl *D);
    void VisitClassTemplatePartialSpecializationDecl(
                                     ClassTemplatePartialSpecializationDecl *D);
    void VisitTemplateTypeParmDecl(TemplateTypeParmDecl *D);
    void VisitValueDecl(ValueDecl *D);
    void VisitEnumConstantDecl(EnumConstantDecl *D);
    void VisitUnresolvedUsingValue(UnresolvedUsingValueDecl *D);
    void VisitDeclaratorDecl(DeclaratorDecl *D);
    void VisitFunctionDecl(FunctionDecl *D);
    void VisitCXXMethodDecl(CXXMethodDecl *D);
    void VisitCXXConstructorDecl(CXXConstructorDecl *D);
    void VisitCXXDestructorDecl(CXXDestructorDecl *D);
    void VisitCXXConversionDecl(CXXConversionDecl *D);
    void VisitFieldDecl(FieldDecl *D);
    void VisitVarDecl(VarDecl *D);
    void VisitImplicitParamDecl(ImplicitParamDecl *D);
    void VisitParmVarDecl(ParmVarDecl *D);
    void VisitNonTypeTemplateParmDecl(NonTypeTemplateParmDecl *D);
    void VisitTemplateDecl(TemplateDecl *D);
    void VisitClassTemplateDecl(ClassTemplateDecl *D);
    void VisitFunctionTemplateDecl(FunctionTemplateDecl *D);
    void VisitTemplateTemplateParmDecl(TemplateTemplateParmDecl *D);
    void VisitUsingDecl(UsingDecl *D);
    void VisitUsingShadowDecl(UsingShadowDecl *D);
    void VisitLinkageSpecDecl(LinkageSpecDecl *D);
    void VisitFileScopeAsmDecl(FileScopeAsmDecl *D);
    void VisitAccessSpecDecl(AccessSpecDecl *D);
    void VisitFriendTemplateDecl(FriendTemplateDecl *D);
    void VisitStaticAssertDecl(StaticAssertDecl *D);
    void VisitBlockDecl(BlockDecl *D);

    void VisitDeclContext(DeclContext *DC, uint64_t LexicalOffset,
                          uint64_t VisibleOffset);


    // FIXME: Put in the same order is DeclNodes.td?
    void VisitObjCMethodDecl(ObjCMethodDecl *D);
    void VisitObjCContainerDecl(ObjCContainerDecl *D);
    void VisitObjCInterfaceDecl(ObjCInterfaceDecl *D);
    void VisitObjCIvarDecl(ObjCIvarDecl *D);
    void VisitObjCProtocolDecl(ObjCProtocolDecl *D);
    void VisitObjCAtDefsFieldDecl(ObjCAtDefsFieldDecl *D);
    void VisitObjCClassDecl(ObjCClassDecl *D);
    void VisitObjCForwardProtocolDecl(ObjCForwardProtocolDecl *D);
    void VisitObjCCategoryDecl(ObjCCategoryDecl *D);
    void VisitObjCImplDecl(ObjCImplDecl *D);
    void VisitObjCCategoryImplDecl(ObjCCategoryImplDecl *D);
    void VisitObjCImplementationDecl(ObjCImplementationDecl *D);
    void VisitObjCCompatibleAliasDecl(ObjCCompatibleAliasDecl *D);
    void VisitObjCPropertyDecl(ObjCPropertyDecl *D);
    void VisitObjCPropertyImplDecl(ObjCPropertyImplDecl *D);

    void WriteCXXBaseSpecifier(const CXXBaseSpecifier *Base);
  };
}

void PCHDeclWriter::VisitDecl(Decl *D) {
  Writer.AddDeclRef(cast_or_null<Decl>(D->getDeclContext()), Record);
  Writer.AddDeclRef(cast_or_null<Decl>(D->getLexicalDeclContext()), Record);
  Writer.AddSourceLocation(D->getLocation(), Record);
  Record.push_back(D->isInvalidDecl());
  Record.push_back(D->hasAttrs());
  Record.push_back(D->isImplicit());
  Record.push_back(D->isUsed(false));
  Record.push_back(D->getAccess());
  Record.push_back(D->getPCHLevel());
}

void PCHDeclWriter::VisitTranslationUnitDecl(TranslationUnitDecl *D) {
  VisitDecl(D);
  Writer.AddDeclRef(D->getAnonymousNamespace(), Record);
  Code = pch::DECL_TRANSLATION_UNIT;
}

void PCHDeclWriter::VisitNamedDecl(NamedDecl *D) {
  VisitDecl(D);
  Writer.AddDeclarationName(D->getDeclName(), Record);
}

void PCHDeclWriter::VisitTypeDecl(TypeDecl *D) {
  VisitNamedDecl(D);
  if (isa<CXXRecordDecl>(D)) {
    // FIXME: Hack. To read a templated CXXRecordDecl from PCH, we need an
    // initialized CXXRecordDecl before creating an InjectedClassNameType.
    // Delay emitting/reading CXXRecordDecl's TypeForDecl until when we handle
    // CXXRecordDecl emitting/initialization.
    Writer.AddTypeRef(QualType(), Record);
  } else
    Writer.AddTypeRef(QualType(D->getTypeForDecl(), 0), Record);
}

void PCHDeclWriter::VisitTypedefDecl(TypedefDecl *D) {
  VisitTypeDecl(D);
  Writer.AddTypeSourceInfo(D->getTypeSourceInfo(), Record);
  Code = pch::DECL_TYPEDEF;
}

void PCHDeclWriter::VisitTagDecl(TagDecl *D) {
  VisitTypeDecl(D);
  Writer.AddDeclRef(D->getPreviousDeclaration(), Record);
  Record.push_back((unsigned)D->getTagKind()); // FIXME: stable encoding
  Record.push_back(D->isDefinition());
  Record.push_back(D->isEmbeddedInDeclarator());
  Writer.AddSourceLocation(D->getRBraceLoc(), Record);
  Writer.AddSourceLocation(D->getTagKeywordLoc(), Record);
  // FIXME: maybe write optional qualifier and its range.
  Writer.AddDeclRef(D->getTypedefForAnonDecl(), Record);
}

void PCHDeclWriter::VisitEnumDecl(EnumDecl *D) {
  VisitTagDecl(D);
  Writer.AddTypeRef(D->getIntegerType(), Record);
  Writer.AddTypeRef(D->getPromotionType(), Record);
  Record.push_back(D->getNumPositiveBits());
  Record.push_back(D->getNumNegativeBits());
  // FIXME: C++ InstantiatedFrom
  Code = pch::DECL_ENUM;
}

void PCHDeclWriter::VisitRecordDecl(RecordDecl *D) {
  VisitTagDecl(D);
  Record.push_back(D->hasFlexibleArrayMember());
  Record.push_back(D->isAnonymousStructOrUnion());
  Record.push_back(D->hasObjectMember());
  Code = pch::DECL_RECORD;
}

void PCHDeclWriter::VisitValueDecl(ValueDecl *D) {
  VisitNamedDecl(D);
  Writer.AddTypeRef(D->getType(), Record);
}

void PCHDeclWriter::VisitEnumConstantDecl(EnumConstantDecl *D) {
  VisitValueDecl(D);
  Record.push_back(D->getInitExpr()? 1 : 0);
  if (D->getInitExpr())
    Writer.AddStmt(D->getInitExpr());
  Writer.AddAPSInt(D->getInitVal(), Record);
  Code = pch::DECL_ENUM_CONSTANT;
}

void PCHDeclWriter::VisitDeclaratorDecl(DeclaratorDecl *D) {
  VisitValueDecl(D);
  Writer.AddTypeSourceInfo(D->getTypeSourceInfo(), Record);
  // FIXME: write optional qualifier and its range.
}

void PCHDeclWriter::VisitFunctionDecl(FunctionDecl *D) {
  VisitDeclaratorDecl(D);

  Record.push_back(D->isThisDeclarationADefinition());
  if (D->isThisDeclarationADefinition())
    Writer.AddStmt(D->getBody());

  Writer.AddDeclRef(D->getPreviousDeclaration(), Record);
  Record.push_back(D->getStorageClass()); // FIXME: stable encoding
  Record.push_back(D->getStorageClassAsWritten());
  Record.push_back(D->isInlineSpecified());
  Record.push_back(D->isVirtualAsWritten());
  Record.push_back(D->isPure());
  Record.push_back(D->hasInheritedPrototype());
  Record.push_back(D->hasWrittenPrototype());
  Record.push_back(D->isDeleted());
  Record.push_back(D->isTrivial());
  Record.push_back(D->isCopyAssignment());
  Record.push_back(D->hasImplicitReturnZero());
  Writer.AddSourceLocation(D->getLocEnd(), Record);
  
  Record.push_back(D->getTemplatedKind());
  switch (D->getTemplatedKind()) {
  case FunctionDecl::TK_NonTemplate:
    break;
  case FunctionDecl::TK_FunctionTemplate:
    Writer.AddDeclRef(D->getDescribedFunctionTemplate(), Record);
    break;
  case FunctionDecl::TK_MemberSpecialization: {
    MemberSpecializationInfo *MemberInfo = D->getMemberSpecializationInfo();
    Writer.AddDeclRef(MemberInfo->getInstantiatedFrom(), Record);
    Record.push_back(MemberInfo->getTemplateSpecializationKind());
    Writer.AddSourceLocation(MemberInfo->getPointOfInstantiation(), Record);
    break;
  }
  case FunctionDecl::TK_FunctionTemplateSpecialization: {
    FunctionTemplateSpecializationInfo *
      FTSInfo = D->getTemplateSpecializationInfo();
    Writer.AddDeclRef(FTSInfo->getTemplate(), Record);
    Record.push_back(FTSInfo->getTemplateSpecializationKind());
    
    // Template arguments.
    assert(FTSInfo->TemplateArguments && "No template args!");
    Record.push_back(FTSInfo->TemplateArguments->flat_size());
    for (int i=0, e = FTSInfo->TemplateArguments->flat_size(); i != e; ++i)
      Writer.AddTemplateArgument(FTSInfo->TemplateArguments->get(i), Record);
    
    // Template args as written.
    if (FTSInfo->TemplateArgumentsAsWritten) {
      Record.push_back(FTSInfo->TemplateArgumentsAsWritten->size());
      for (int i=0, e = FTSInfo->TemplateArgumentsAsWritten->size(); i!=e; ++i)
        Writer.AddTemplateArgumentLoc((*FTSInfo->TemplateArgumentsAsWritten)[i],
                                      Record);
      Writer.AddSourceLocation(FTSInfo->TemplateArgumentsAsWritten->getLAngleLoc(),
                               Record);
      Writer.AddSourceLocation(FTSInfo->TemplateArgumentsAsWritten->getRAngleLoc(),
                               Record);
    } else {
      Record.push_back(0);
    }
  }
  case FunctionDecl::TK_DependentFunctionTemplateSpecialization: {
    DependentFunctionTemplateSpecializationInfo *
      DFTSInfo = D->getDependentSpecializationInfo();
    
    // Templates.
    Record.push_back(DFTSInfo->getNumTemplates());
    for (int i=0, e = DFTSInfo->getNumTemplates(); i != e; ++i)
      Writer.AddDeclRef(DFTSInfo->getTemplate(i), Record);
    
    // Templates args.
    Record.push_back(DFTSInfo->getNumTemplateArgs());
    for (int i=0, e = DFTSInfo->getNumTemplateArgs(); i != e; ++i)
      Writer.AddTemplateArgumentLoc(DFTSInfo->getTemplateArg(i), Record);
  }
  }

  Record.push_back(D->param_size());
  for (FunctionDecl::param_iterator P = D->param_begin(), PEnd = D->param_end();
       P != PEnd; ++P)
    Writer.AddDeclRef(*P, Record);
  Code = pch::DECL_FUNCTION;
}

void PCHDeclWriter::VisitObjCMethodDecl(ObjCMethodDecl *D) {
  VisitNamedDecl(D);
  // FIXME: convert to LazyStmtPtr?
  // Unlike C/C++, method bodies will never be in header files.
  Record.push_back(D->getBody() != 0);
  if (D->getBody() != 0) {
    Writer.AddStmt(D->getBody());
    Writer.AddDeclRef(D->getSelfDecl(), Record);
    Writer.AddDeclRef(D->getCmdDecl(), Record);
  }
  Record.push_back(D->isInstanceMethod());
  Record.push_back(D->isVariadic());
  Record.push_back(D->isSynthesized());
  // FIXME: stable encoding for @required/@optional
  Record.push_back(D->getImplementationControl());
  // FIXME: stable encoding for in/out/inout/bycopy/byref/oneway
  Record.push_back(D->getObjCDeclQualifier());
  Record.push_back(D->getNumSelectorArgs());
  Writer.AddTypeRef(D->getResultType(), Record);
  Writer.AddTypeSourceInfo(D->getResultTypeSourceInfo(), Record);
  Writer.AddSourceLocation(D->getLocEnd(), Record);
  Record.push_back(D->param_size());
  for (ObjCMethodDecl::param_iterator P = D->param_begin(),
                                   PEnd = D->param_end(); P != PEnd; ++P)
    Writer.AddDeclRef(*P, Record);
  Code = pch::DECL_OBJC_METHOD;
}

void PCHDeclWriter::VisitObjCContainerDecl(ObjCContainerDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceRange(D->getAtEndRange(), Record);
  // Abstract class (no need to define a stable pch::DECL code).
}

void PCHDeclWriter::VisitObjCInterfaceDecl(ObjCInterfaceDecl *D) {
  VisitObjCContainerDecl(D);
  Writer.AddTypeRef(QualType(D->getTypeForDecl(), 0), Record);
  Writer.AddDeclRef(D->getSuperClass(), Record);
  Record.push_back(D->protocol_size());
  for (ObjCInterfaceDecl::protocol_iterator P = D->protocol_begin(),
         PEnd = D->protocol_end();
       P != PEnd; ++P)
    Writer.AddDeclRef(*P, Record);
  for (ObjCInterfaceDecl::protocol_loc_iterator PL = D->protocol_loc_begin(),
         PLEnd = D->protocol_loc_end();
       PL != PLEnd; ++PL)
    Writer.AddSourceLocation(*PL, Record);
  Record.push_back(D->ivar_size());
  for (ObjCInterfaceDecl::ivar_iterator I = D->ivar_begin(),
                                     IEnd = D->ivar_end(); I != IEnd; ++I)
    Writer.AddDeclRef(*I, Record);
  Writer.AddDeclRef(D->getCategoryList(), Record);
  Record.push_back(D->isForwardDecl());
  Record.push_back(D->isImplicitInterfaceDecl());
  Writer.AddSourceLocation(D->getClassLoc(), Record);
  Writer.AddSourceLocation(D->getSuperClassLoc(), Record);
  Writer.AddSourceLocation(D->getLocEnd(), Record);
  Code = pch::DECL_OBJC_INTERFACE;
}

void PCHDeclWriter::VisitObjCIvarDecl(ObjCIvarDecl *D) {
  VisitFieldDecl(D);
  // FIXME: stable encoding for @public/@private/@protected/@package
  Record.push_back(D->getAccessControl());
  Code = pch::DECL_OBJC_IVAR;
}

void PCHDeclWriter::VisitObjCProtocolDecl(ObjCProtocolDecl *D) {
  VisitObjCContainerDecl(D);
  Record.push_back(D->isForwardDecl());
  Writer.AddSourceLocation(D->getLocEnd(), Record);
  Record.push_back(D->protocol_size());
  for (ObjCProtocolDecl::protocol_iterator
       I = D->protocol_begin(), IEnd = D->protocol_end(); I != IEnd; ++I)
    Writer.AddDeclRef(*I, Record);
  for (ObjCProtocolDecl::protocol_loc_iterator PL = D->protocol_loc_begin(),
         PLEnd = D->protocol_loc_end();
       PL != PLEnd; ++PL)
    Writer.AddSourceLocation(*PL, Record);
  Code = pch::DECL_OBJC_PROTOCOL;
}

void PCHDeclWriter::VisitObjCAtDefsFieldDecl(ObjCAtDefsFieldDecl *D) {
  VisitFieldDecl(D);
  Code = pch::DECL_OBJC_AT_DEFS_FIELD;
}

void PCHDeclWriter::VisitObjCClassDecl(ObjCClassDecl *D) {
  VisitDecl(D);
  Record.push_back(D->size());
  for (ObjCClassDecl::iterator I = D->begin(), IEnd = D->end(); I != IEnd; ++I)
    Writer.AddDeclRef(I->getInterface(), Record);
  for (ObjCClassDecl::iterator I = D->begin(), IEnd = D->end(); I != IEnd; ++I)
    Writer.AddSourceLocation(I->getLocation(), Record);
  Code = pch::DECL_OBJC_CLASS;
}

void PCHDeclWriter::VisitObjCForwardProtocolDecl(ObjCForwardProtocolDecl *D) {
  VisitDecl(D);
  Record.push_back(D->protocol_size());
  for (ObjCForwardProtocolDecl::protocol_iterator
       I = D->protocol_begin(), IEnd = D->protocol_end(); I != IEnd; ++I)
    Writer.AddDeclRef(*I, Record);
  for (ObjCForwardProtocolDecl::protocol_loc_iterator 
         PL = D->protocol_loc_begin(), PLEnd = D->protocol_loc_end();
       PL != PLEnd; ++PL)
    Writer.AddSourceLocation(*PL, Record);
  Code = pch::DECL_OBJC_FORWARD_PROTOCOL;
}

void PCHDeclWriter::VisitObjCCategoryDecl(ObjCCategoryDecl *D) {
  VisitObjCContainerDecl(D);
  Writer.AddDeclRef(D->getClassInterface(), Record);
  Record.push_back(D->protocol_size());
  for (ObjCCategoryDecl::protocol_iterator
       I = D->protocol_begin(), IEnd = D->protocol_end(); I != IEnd; ++I)
    Writer.AddDeclRef(*I, Record);
  for (ObjCCategoryDecl::protocol_loc_iterator 
         PL = D->protocol_loc_begin(), PLEnd = D->protocol_loc_end();
       PL != PLEnd; ++PL)
    Writer.AddSourceLocation(*PL, Record);
  Writer.AddDeclRef(D->getNextClassCategory(), Record);
  Writer.AddSourceLocation(D->getAtLoc(), Record);
  Writer.AddSourceLocation(D->getCategoryNameLoc(), Record);
  Code = pch::DECL_OBJC_CATEGORY;
}

void PCHDeclWriter::VisitObjCCompatibleAliasDecl(ObjCCompatibleAliasDecl *D) {
  VisitNamedDecl(D);
  Writer.AddDeclRef(D->getClassInterface(), Record);
  Code = pch::DECL_OBJC_COMPATIBLE_ALIAS;
}

void PCHDeclWriter::VisitObjCPropertyDecl(ObjCPropertyDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceLocation(D->getAtLoc(), Record);
  Writer.AddTypeSourceInfo(D->getTypeSourceInfo(), Record);
  // FIXME: stable encoding
  Record.push_back((unsigned)D->getPropertyAttributes());
  Record.push_back((unsigned)D->getPropertyAttributesAsWritten());
  // FIXME: stable encoding
  Record.push_back((unsigned)D->getPropertyImplementation());
  Writer.AddDeclarationName(D->getGetterName(), Record);
  Writer.AddDeclarationName(D->getSetterName(), Record);
  Writer.AddDeclRef(D->getGetterMethodDecl(), Record);
  Writer.AddDeclRef(D->getSetterMethodDecl(), Record);
  Writer.AddDeclRef(D->getPropertyIvarDecl(), Record);
  Code = pch::DECL_OBJC_PROPERTY;
}

void PCHDeclWriter::VisitObjCImplDecl(ObjCImplDecl *D) {
  VisitObjCContainerDecl(D);
  Writer.AddDeclRef(D->getClassInterface(), Record);
  // Abstract class (no need to define a stable pch::DECL code).
}

void PCHDeclWriter::VisitObjCCategoryImplDecl(ObjCCategoryImplDecl *D) {
  VisitObjCImplDecl(D);
  Writer.AddIdentifierRef(D->getIdentifier(), Record);
  Code = pch::DECL_OBJC_CATEGORY_IMPL;
}

void PCHDeclWriter::VisitObjCImplementationDecl(ObjCImplementationDecl *D) {
  VisitObjCImplDecl(D);
  Writer.AddDeclRef(D->getSuperClass(), Record);
  // FIXME add writing of IvarInitializers and NumIvarInitializers.
  Code = pch::DECL_OBJC_IMPLEMENTATION;
}

void PCHDeclWriter::VisitObjCPropertyImplDecl(ObjCPropertyImplDecl *D) {
  VisitDecl(D);
  Writer.AddSourceLocation(D->getLocStart(), Record);
  Writer.AddDeclRef(D->getPropertyDecl(), Record);
  Writer.AddDeclRef(D->getPropertyIvarDecl(), Record);
  // FIXME. write GetterCXXConstructor and SetterCXXAssignment.
  Code = pch::DECL_OBJC_PROPERTY_IMPL;
}

void PCHDeclWriter::VisitFieldDecl(FieldDecl *D) {
  VisitDeclaratorDecl(D);
  Record.push_back(D->isMutable());
  Record.push_back(D->getBitWidth()? 1 : 0);
  if (D->getBitWidth())
    Writer.AddStmt(D->getBitWidth());
  Code = pch::DECL_FIELD;
}

void PCHDeclWriter::VisitVarDecl(VarDecl *D) {
  VisitDeclaratorDecl(D);
  Record.push_back(D->getStorageClass()); // FIXME: stable encoding
  Record.push_back(D->getStorageClassAsWritten());
  Record.push_back(D->isThreadSpecified());
  Record.push_back(D->hasCXXDirectInitializer());
  Record.push_back(D->isDeclaredInCondition());
  Record.push_back(D->isExceptionVariable());
  Record.push_back(D->isNRVOVariable());
  Writer.AddDeclRef(D->getPreviousDeclaration(), Record);
  Record.push_back(D->getInit() ? 1 : 0);
  if (D->getInit())
    Writer.AddStmt(D->getInit());
  Code = pch::DECL_VAR;
}

void PCHDeclWriter::VisitImplicitParamDecl(ImplicitParamDecl *D) {
  VisitVarDecl(D);
  Code = pch::DECL_IMPLICIT_PARAM;
}

void PCHDeclWriter::VisitParmVarDecl(ParmVarDecl *D) {
  VisitVarDecl(D);
  Record.push_back(D->getObjCDeclQualifier()); // FIXME: stable encoding
  Record.push_back(D->hasInheritedDefaultArg());
  Code = pch::DECL_PARM_VAR;

  // If the assumptions about the DECL_PARM_VAR abbrev are true, use it.  Here
  // we dynamically check for the properties that we optimize for, but don't
  // know are true of all PARM_VAR_DECLs.
  if (!D->getTypeSourceInfo() &&
      !D->hasAttrs() &&
      !D->isImplicit() &&
      !D->isUsed(false) &&
      D->getAccess() == AS_none &&
      D->getPCHLevel() == 0 &&
      D->getStorageClass() == 0 &&
      !D->hasCXXDirectInitializer() && // Can params have this ever?
      D->getObjCDeclQualifier() == 0 &&
      !D->hasInheritedDefaultArg() &&
      D->getInit() == 0)  // No default expr.
    AbbrevToUse = Writer.getParmVarDeclAbbrev();

  // Check things we know are true of *every* PARM_VAR_DECL, which is more than
  // just us assuming it.
  assert(!D->isInvalidDecl() && "Shouldn't emit invalid decls");
  assert(!D->isThreadSpecified() && "PARM_VAR_DECL can't be __thread");
  assert(D->getAccess() == AS_none && "PARM_VAR_DECL can't be public/private");
  assert(!D->isDeclaredInCondition() && "PARM_VAR_DECL can't be in condition");
  assert(!D->isExceptionVariable() && "PARM_VAR_DECL can't be exception var");
  assert(D->getPreviousDeclaration() == 0 && "PARM_VAR_DECL can't be redecl");
}

void PCHDeclWriter::VisitFileScopeAsmDecl(FileScopeAsmDecl *D) {
  VisitDecl(D);
  Writer.AddStmt(D->getAsmString());
  Code = pch::DECL_FILE_SCOPE_ASM;
}

void PCHDeclWriter::VisitBlockDecl(BlockDecl *D) {
  VisitDecl(D);
  Writer.AddStmt(D->getBody());
  Writer.AddTypeSourceInfo(D->getSignatureAsWritten(), Record);
  Record.push_back(D->param_size());
  for (FunctionDecl::param_iterator P = D->param_begin(), PEnd = D->param_end();
       P != PEnd; ++P)
    Writer.AddDeclRef(*P, Record);
  Code = pch::DECL_BLOCK;
}

void PCHDeclWriter::VisitLinkageSpecDecl(LinkageSpecDecl *D) {
  VisitDecl(D);
  // FIXME: It might be nice to serialize the brace locations for this
  // declaration, which don't seem to be readily available in the AST.
  Record.push_back(D->getLanguage());
  Record.push_back(D->hasBraces());
  Code = pch::DECL_LINKAGE_SPEC;
}

void PCHDeclWriter::VisitNamespaceDecl(NamespaceDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceLocation(D->getLBracLoc(), Record);
  Writer.AddSourceLocation(D->getRBracLoc(), Record);
  Writer.AddDeclRef(D->getNextNamespace(), Record);

  // Only write one reference--original or anonymous
  Record.push_back(D->isOriginalNamespace());
  if (D->isOriginalNamespace())
    Writer.AddDeclRef(D->getAnonymousNamespace(), Record);
  else
    Writer.AddDeclRef(D->getOriginalNamespace(), Record);
  Code = pch::DECL_NAMESPACE;
}

void PCHDeclWriter::VisitNamespaceAliasDecl(NamespaceAliasDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceLocation(D->getAliasLoc(), Record);
  Writer.AddSourceRange(D->getQualifierRange(), Record);
  Writer.AddNestedNameSpecifier(D->getQualifier(), Record);
  Writer.AddSourceLocation(D->getTargetNameLoc(), Record);
  Writer.AddDeclRef(D->getNamespace(), Record);
  Code = pch::DECL_NAMESPACE_ALIAS;
}

void PCHDeclWriter::VisitUsingDecl(UsingDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceRange(D->getNestedNameRange(), Record);
  Writer.AddSourceLocation(D->getUsingLocation(), Record);
  Writer.AddNestedNameSpecifier(D->getTargetNestedNameDecl(), Record);
  Record.push_back(D->getNumShadowDecls());
  for (UsingDecl::shadow_iterator P = D->shadow_begin(),
       PEnd = D->shadow_end(); P != PEnd; ++P)
    Writer.AddDeclRef(*P, Record);
  Record.push_back(D->isTypeName());
  Code = pch::DECL_USING;
}

void PCHDeclWriter::VisitUsingShadowDecl(UsingShadowDecl *D) {
  VisitNamedDecl(D);
  Writer.AddDeclRef(D->getTargetDecl(), Record);
  Writer.AddDeclRef(D->getUsingDecl(), Record);
  Code = pch::DECL_USING_SHADOW;
}

void PCHDeclWriter::VisitUsingDirectiveDecl(UsingDirectiveDecl *D) {
  VisitNamedDecl(D);
  Writer.AddSourceLocation(D->getNamespaceKeyLocation(), Record);
  Writer.AddSourceRange(D->getQualifierRange(), Record);
  Writer.AddNestedNameSpecifier(D->getQualifier(), Record);
  Writer.AddSourceLocation(D->getIdentLocation(), Record);
  Writer.AddDeclRef(D->getNominatedNamespace(), Record);
  Writer.AddDeclRef(dyn_cast<Decl>(D->getCommonAncestor()), Record);
  Code = pch::DECL_USING_DIRECTIVE;
}

void PCHDeclWriter::VisitUnresolvedUsingValue(UnresolvedUsingValueDecl *D) {
  VisitValueDecl(D);
  Writer.AddSourceRange(D->getTargetNestedNameRange(), Record);
  Writer.AddSourceLocation(D->getUsingLoc(), Record);
  Writer.AddNestedNameSpecifier(D->getTargetNestedNameSpecifier(), Record);
  Code = pch::DECL_UNRESOLVED_USING_VALUE;
}

void PCHDeclWriter::VisitUnresolvedUsingTypename(
                                               UnresolvedUsingTypenameDecl *D) {
  VisitTypeDecl(D);
  Writer.AddSourceRange(D->getTargetNestedNameRange(), Record);
  Writer.AddSourceLocation(D->getUsingLoc(), Record);
  Writer.AddSourceLocation(D->getTypenameLoc(), Record);
  Writer.AddNestedNameSpecifier(D->getTargetNestedNameSpecifier(), Record);
  Code = pch::DECL_UNRESOLVED_USING_TYPENAME;
}

void PCHDeclWriter::WriteCXXBaseSpecifier(const CXXBaseSpecifier *Base) {
  Record.push_back(Base->isVirtual());
  Record.push_back(Base->isBaseOfClass());
  Record.push_back(Base->getAccessSpecifierAsWritten());
  Writer.AddTypeRef(Base->getType(), Record);
  Writer.AddSourceRange(Base->getSourceRange(), Record);
}

void PCHDeclWriter::VisitCXXRecordDecl(CXXRecordDecl *D) {
  // assert(false && "cannot write CXXRecordDecl");
  VisitRecordDecl(D);

  enum {
    CXXRecNotTemplate = 0, CXXRecTemplate, CXXRecMemberSpecialization
  };
  if (ClassTemplateDecl *TemplD = D->getDescribedClassTemplate()) {
    Record.push_back(CXXRecTemplate);
    Writer.AddDeclRef(TemplD, Record);
  } else if (MemberSpecializationInfo *MSInfo
               = D->getMemberSpecializationInfo()) {
    Record.push_back(CXXRecMemberSpecialization);
    Writer.AddDeclRef(MSInfo->getInstantiatedFrom(), Record);
    Record.push_back(MSInfo->getTemplateSpecializationKind());
    Writer.AddSourceLocation(MSInfo->getPointOfInstantiation(), Record);
  } else {
    Record.push_back(CXXRecNotTemplate);
  }

  // FIXME: Hack. See PCHDeclWriter::VisitTypeDecl.
  Writer.AddTypeRef(QualType(D->getTypeForDecl(), 0), Record);

  if (D->isDefinition()) {
    unsigned NumBases = D->getNumBases();
    Record.push_back(NumBases);
    for (CXXRecordDecl::base_class_iterator I = D->bases_begin(),
           E = D->bases_end(); I != E; ++I)
      WriteCXXBaseSpecifier(&*I);
  }
  Code = pch::DECL_CXX_RECORD;
}

void PCHDeclWriter::VisitCXXMethodDecl(CXXMethodDecl *D) {
  // assert(false && "cannot write CXXMethodDecl");
  VisitFunctionDecl(D);
  Code = pch::DECL_CXX_METHOD;
}

void PCHDeclWriter::VisitCXXConstructorDecl(CXXConstructorDecl *D) {
  // assert(false && "cannot write CXXConstructorDecl");
  VisitCXXMethodDecl(D);
  Code = pch::DECL_CXX_CONSTRUCTOR;
}

void PCHDeclWriter::VisitCXXDestructorDecl(CXXDestructorDecl *D) {
  // assert(false && "cannot write CXXDestructorDecl");
  VisitCXXMethodDecl(D);
  Code = pch::DECL_CXX_DESTRUCTOR;
}

void PCHDeclWriter::VisitCXXConversionDecl(CXXConversionDecl *D) {
  // assert(false && "cannot write CXXConversionDecl");
  VisitCXXMethodDecl(D);
  Code = pch::DECL_CXX_CONVERSION;
}

void PCHDeclWriter::VisitAccessSpecDecl(AccessSpecDecl *D) {
  VisitDecl(D);
  Writer.AddSourceLocation(D->getColonLoc(), Record);
  Code = pch::DECL_ACCESS_SPEC;
}

void PCHDeclWriter::VisitFriendTemplateDecl(FriendTemplateDecl *D) {
  assert(false && "cannot write FriendTemplateDecl");
}

void PCHDeclWriter::VisitTemplateDecl(TemplateDecl *D) {
  VisitNamedDecl(D);

  Writer.AddDeclRef(D->getTemplatedDecl(), Record);
  {
    // TemplateParams.
    TemplateParameterList *TPL = D->getTemplateParameters();
    assert(TPL && "No TemplateParameters!");
    Writer.AddSourceLocation(TPL->getTemplateLoc(), Record);
    Writer.AddSourceLocation(TPL->getLAngleLoc(), Record);
    Writer.AddSourceLocation(TPL->getRAngleLoc(), Record);
    Record.push_back(TPL->size());
    for (TemplateParameterList::iterator P = TPL->begin(), PEnd = TPL->end();
           P != PEnd; ++P)
      Writer.AddDeclRef(*P, Record);
  }
}

void PCHDeclWriter::VisitClassTemplateDecl(ClassTemplateDecl *D) {
  VisitTemplateDecl(D);

  Writer.AddDeclRef(D->getPreviousDeclaration(), Record);
  if (D->getPreviousDeclaration() == 0) {
    // This ClassTemplateDecl owns the CommonPtr; write it.

    typedef llvm::FoldingSet<ClassTemplateSpecializationDecl> CTSDSetTy;
    CTSDSetTy &CTSDSet = D->getSpecializations();
    Record.push_back(CTSDSet.size());
    for (CTSDSetTy::iterator I=CTSDSet.begin(), E = CTSDSet.end(); I!=E; ++I)
      Writer.AddDeclRef(&*I, Record);

    typedef llvm::FoldingSet<ClassTemplatePartialSpecializationDecl> CTPSDSetTy;
    CTPSDSetTy &CTPSDSet = D->getPartialSpecializations();
    Record.push_back(CTPSDSet.size());
    for (CTPSDSetTy::iterator I=CTPSDSet.begin(), E = CTPSDSet.end(); I!=E; ++I)
      Writer.AddDeclRef(&*I, Record);

    // InjectedClassNameType is computed, no need to write it.

    Writer.AddDeclRef(D->getInstantiatedFromMemberTemplate(), Record);
    if (D->getInstantiatedFromMemberTemplate())
      Record.push_back(D->isMemberSpecialization());
  }
  Code = pch::DECL_CLASS_TEMPLATE;
}

void PCHDeclWriter::VisitClassTemplateSpecializationDecl(
                                           ClassTemplateSpecializationDecl *D) {
  assert(false && "cannot write ClassTemplateSpecializationDecl");
}

void PCHDeclWriter::VisitClassTemplatePartialSpecializationDecl(
                                    ClassTemplatePartialSpecializationDecl *D) {
  assert(false && "cannot write ClassTemplatePartialSpecializationDecl");
}

void PCHDeclWriter::VisitFunctionTemplateDecl(FunctionTemplateDecl *D) {
  VisitTemplateDecl(D);

  Writer.AddDeclRef(D->getPreviousDeclaration(), Record);
  if (D->getPreviousDeclaration() == 0) {
    // This FunctionTemplateDecl owns the CommonPtr; write it.

    // FunctionTemplateSpecializationInfos are filled through the
    // templated FunctionDecl's setFunctionTemplateSpecialization, no need to
    // write them here.

    Writer.AddDeclRef(D->getInstantiatedFromMemberTemplate(), Record);
    if (D->getInstantiatedFromMemberTemplate())
      Record.push_back(D->isMemberSpecialization());
  }
  Code = pch::DECL_FUNCTION_TEMPLATE;
}

void PCHDeclWriter::VisitTemplateTypeParmDecl(TemplateTypeParmDecl *D) {
  VisitTypeDecl(D);

  Record.push_back(D->wasDeclaredWithTypename());
  Record.push_back(D->isParameterPack());
  Record.push_back(D->defaultArgumentWasInherited());
  Writer.AddTypeSourceInfo(D->getDefaultArgumentInfo(), Record);

  Code = pch::DECL_TEMPLATE_TYPE_PARM;
}

void PCHDeclWriter::VisitNonTypeTemplateParmDecl(NonTypeTemplateParmDecl *D) {
  assert(false && "cannot write NonTypeTemplateParmDecl");
}

void PCHDeclWriter::VisitTemplateTemplateParmDecl(TemplateTemplateParmDecl *D) {
  assert(false && "cannot write TemplateTemplateParmDecl");
}

void PCHDeclWriter::VisitStaticAssertDecl(StaticAssertDecl *D) {
  assert(false && "cannot write StaticAssertDecl");
}

/// \brief Emit the DeclContext part of a declaration context decl.
///
/// \param LexicalOffset the offset at which the DECL_CONTEXT_LEXICAL
/// block for this declaration context is stored. May be 0 to indicate
/// that there are no declarations stored within this context.
///
/// \param VisibleOffset the offset at which the DECL_CONTEXT_VISIBLE
/// block for this declaration context is stored. May be 0 to indicate
/// that there are no declarations visible from this context. Note
/// that this value will not be emitted for non-primary declaration
/// contexts.
void PCHDeclWriter::VisitDeclContext(DeclContext *DC, uint64_t LexicalOffset,
                                     uint64_t VisibleOffset) {
  Record.push_back(LexicalOffset);
  Record.push_back(VisibleOffset);
}


//===----------------------------------------------------------------------===//
// PCHWriter Implementation
//===----------------------------------------------------------------------===//

void PCHWriter::WriteDeclsBlockAbbrevs() {
  using namespace llvm;
  // Abbreviation for DECL_PARM_VAR.
  BitCodeAbbrev *Abv = new BitCodeAbbrev();
  Abv->Add(BitCodeAbbrevOp(pch::DECL_PARM_VAR));

  // Decl
  Abv->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // DeclContext
  Abv->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // LexicalDeclContext
  Abv->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Location
  Abv->Add(BitCodeAbbrevOp(0));                       // isInvalidDecl (!?)
  Abv->Add(BitCodeAbbrevOp(0));                       // HasAttrs
  Abv->Add(BitCodeAbbrevOp(0));                       // isImplicit
  Abv->Add(BitCodeAbbrevOp(0));                       // isUsed
  Abv->Add(BitCodeAbbrevOp(AS_none));                 // C++ AccessSpecifier
  Abv->Add(BitCodeAbbrevOp(0));                       // PCH level

  // NamedDecl
  Abv->Add(BitCodeAbbrevOp(0));                       // NameKind = Identifier
  Abv->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Name
  // ValueDecl
  Abv->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::VBR, 6)); // Type
  // DeclaratorDecl
  Abv->Add(BitCodeAbbrevOp(pch::PREDEF_TYPE_NULL_ID)); // InfoType
  // VarDecl
  Abv->Add(BitCodeAbbrevOp(0));                       // StorageClass
  Abv->Add(BitCodeAbbrevOp(0));                       // StorageClassAsWritten
  Abv->Add(BitCodeAbbrevOp(0));                       // isThreadSpecified
  Abv->Add(BitCodeAbbrevOp(0));                       // hasCXXDirectInitializer
  Abv->Add(BitCodeAbbrevOp(0));                       // isDeclaredInCondition
  Abv->Add(BitCodeAbbrevOp(0));                       // isExceptionVariable
  Abv->Add(BitCodeAbbrevOp(0));                       // isNRVOVariable
  Abv->Add(BitCodeAbbrevOp(0));                       // PrevDecl
  Abv->Add(BitCodeAbbrevOp(0));                       // HasInit
  // ParmVarDecl
  Abv->Add(BitCodeAbbrevOp(0));                       // ObjCDeclQualifier
  Abv->Add(BitCodeAbbrevOp(0));                       // HasInheritedDefaultArg

  ParmVarDeclAbbrev = Stream.EmitAbbrev(Abv);
}

/// isRequiredDecl - Check if this is a "required" Decl, which must be seen by
/// consumers of the AST.
///
/// Such decls will always be deserialized from the PCH file, so we would like
/// this to be as restrictive as possible. Currently the predicate is driven by
/// code generation requirements, if other clients have a different notion of
/// what is "required" then we may have to consider an alternate scheme where
/// clients can iterate over the top-level decls and get information on them,
/// without necessary deserializing them. We could explicitly require such
/// clients to use a separate API call to "realize" the decl. This should be
/// relatively painless since they would presumably only do it for top-level
/// decls.
//
// FIXME: This predicate is essentially IRgen's predicate to determine whether a
// declaration can be deferred. Merge them somehow.
static bool isRequiredDecl(const Decl *D, ASTContext &Context) {
  // File scoped assembly must be seen.
  if (isa<FileScopeAsmDecl>(D))
    return true;

  // Otherwise if this isn't a function or a file scoped variable it doesn't
  // need to be seen.
  if (const VarDecl *VD = dyn_cast<VarDecl>(D)) {
    if (!VD->isFileVarDecl())
      return false;
  } else if (!isa<FunctionDecl>(D))
    return false;

  // Aliases and used decls must be seen.
  if (D->hasAttr<AliasAttr>() || D->hasAttr<UsedAttr>())
    return true;

  if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    // Forward declarations don't need to be seen.
    if (!FD->isThisDeclarationADefinition())
      return false;

    // Constructors and destructors must be seen.
    if (FD->hasAttr<ConstructorAttr>() || FD->hasAttr<DestructorAttr>())
      return true;

    // Otherwise, this is required unless it is static.
    //
    // FIXME: Inlines.
    return FD->getStorageClass() != FunctionDecl::Static;
  } else {
    const VarDecl *VD = cast<VarDecl>(D);

    // In C++, this doesn't need to be seen if it is marked "extern".
    if (Context.getLangOptions().CPlusPlus && !VD->getInit() &&
        (VD->getStorageClass() == VarDecl::Extern ||
         VD->isExternC()))
      return false;

    // In C, this doesn't need to be seen unless it is a definition.
    if (!Context.getLangOptions().CPlusPlus && !VD->getInit())
      return false;

    // Otherwise, this is required unless it is static.
    return VD->getStorageClass() != VarDecl::Static;
  }
}

void PCHWriter::WriteDecl(ASTContext &Context, Decl *D) {
  RecordData Record;
  PCHDeclWriter W(*this, Context, Record);

  // If this declaration is also a DeclContext, write blocks for the
  // declarations that lexically stored inside its context and those
  // declarations that are visible from its context. These blocks
  // are written before the declaration itself so that we can put
  // their offsets into the record for the declaration.
  uint64_t LexicalOffset = 0;
  uint64_t VisibleOffset = 0;
  DeclContext *DC = dyn_cast<DeclContext>(D);
  if (DC) {
    LexicalOffset = WriteDeclContextLexicalBlock(Context, DC);
    VisibleOffset = WriteDeclContextVisibleBlock(Context, DC);
  }

  // Determine the ID for this declaration
  pch::DeclID &ID = DeclIDs[D];
  if (ID == 0)
    ID = DeclIDs.size();

  unsigned Index = ID - 1;

  // Record the offset for this declaration
  if (DeclOffsets.size() == Index)
    DeclOffsets.push_back(Stream.GetCurrentBitNo());
  else if (DeclOffsets.size() < Index) {
    DeclOffsets.resize(Index+1);
    DeclOffsets[Index] = Stream.GetCurrentBitNo();
  }

  // Build and emit a record for this declaration
  Record.clear();
  W.Code = (pch::DeclCode)0;
  W.AbbrevToUse = 0;
  W.Visit(D);
  if (DC) W.VisitDeclContext(DC, LexicalOffset, VisibleOffset);

  if (!W.Code)
    llvm::report_fatal_error(llvm::StringRef("unexpected declaration kind '") +
                            D->getDeclKindName() + "'");
  Stream.EmitRecord(W.Code, Record, W.AbbrevToUse);

  // If the declaration had any attributes, write them now.
  if (D->hasAttrs())
    WriteAttributeRecord(D->getAttrs());

  // Flush any expressions that were written as part of this declaration.
  FlushStmts();

  // Note "external" declarations so that we can add them to a record in the
  // PCH file later.
  //
  // FIXME: This should be renamed, the predicate is much more complicated.
  if (isRequiredDecl(D, Context))
    ExternalDefinitions.push_back(Index + 1);
}
