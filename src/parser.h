/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include "avltree.h"
#include "compiler.h"
#include "parser.h"

#define PROPERTY_REDUCE_FIRST 0x1

struct BaseParser
{
	BaseParser( Compiler *pd )
		: pd(pd), enterRl(false)
	{}

	virtual ~BaseParser() {}

	Compiler *pd;

	RegionSetVect regionStack;
	NamespaceVect namespaceStack;
	ReductionVect reductionStack;
	StructStack structStack;
	ObjectDef *localFrameTop;
	NameScope *scopeTop;

	bool enterRl;

	bool insideRegion()
		{ return regionStack.length() > 0; }

	StructDef *curStruct()
		{ return structStack.length() == 0 ? 0 : structStack.top(); }

	Namespace *curNspace()
		{ return namespaceStack.top(); }
	
	NameScope *curScope()
		{ return scopeTop; }
	
	ObjectDef *curLocalFrame()
		{ return localFrameTop; }
	
	Reduction *curReduction()
		{ return reductionStack.top(); }

	/* Lexical feedback. */

	void listElDef( String name );
	void mapElDef( String name, TypeRef *keyType );

	void argvDecl();
	void init();
	void addRegularDef( const InputLoc &loc, Namespace *nspace, 
			const String &name, LexJoin *join );
	TokenRegion *createRegion( const InputLoc &loc, RegionImpl *impl );
	Namespace *createRootNamespace();
	Namespace *createNamespace( const InputLoc &loc, const String &name );
	void pushRegionSet( const InputLoc &loc );
	void popRegionSet();
	void addProduction( const InputLoc &loc, const String &name, 
			ProdElList *prodElList, bool commit,
			CodeBlock *redBlock, LangEl *predOf );
	void addArgvList();
	LexJoin *literalJoin( const InputLoc &loc, const String &data );

	Reduction *createReduction( const InputLoc loc, const String &name );

	void defineToken( const InputLoc &loc, String name, LexJoin *join,
			ObjectDef *objectDef, CodeBlock *transBlock,
			bool ignore, bool noPreIgnore, bool noPostIgnore );

	void zeroDef( const InputLoc &loc, const String &name );
	void literalDef( const InputLoc &loc, const String &data,
			bool noPreIgnore, bool noPostIgnore );

	ObjectDef *blockOpen();
	void blockClose();

	void inHostDef( const String &hostCall, ObjectDef *localFrame,
			ParameterList *paramList, TypeRef *typeRef,
			const String &name, bool exprt );
	void functionDef( StmtList *stmtList, ObjectDef *localFrame,
			ParameterList *paramList, TypeRef *typeRef,
			const String &name, bool exprt );

	void iterDef( StmtList *stmtList, ObjectDef *localFrame,
			ParameterList *paramList, const String &name );
	LangStmt *globalDef( ObjectField *objField, LangExpr *expr,
			LangStmt::Type assignType );
	void cflDef( NtDef *ntDef, ObjectDef *objectDef, LelDefList *defList );
	ReOrBlock *lexRegularExprData( ReOrBlock *reOrBlock, ReOrItem *reOrItem );

	int lexFactorRepNum( const InputLoc &loc, const String &data );
	LexFactor *lexRlFactorName( const String &data, const InputLoc &loc );
	LexFactorAug *lexFactorLabel( const InputLoc &loc, const String &data,
			LexFactorAug *factorAug );
	LexJoin *lexOptJoin( LexJoin *join, LexJoin *context );
	LangExpr *send( const InputLoc &loc, LangVarRef *varRef,
			ConsItemList *list, bool eof );
	LangExpr *sendTree( const InputLoc &loc, LangVarRef *varRef,
			ConsItemList *list, bool eof );
	LangExpr *parseCmd( const InputLoc &loc, bool tree, bool stop, ObjectField *objField,
			TypeRef *typeRef, FieldInitVect *fieldInitVect, ConsItemList *list,
			bool used, bool reduce, const String &reducer );
	PatternItemList *consPatternEl( LangVarRef *varRef, PatternItemList *list );
	PatternItemList *patternElNamed( const InputLoc &loc, LangVarRef *varRef,
			NamespaceQual *nspaceQual, const String &data, RepeatType repeatType );
	PatternItemList *patternElType( const InputLoc &loc, LangVarRef *varRef,
			NamespaceQual *nspaceQual, const String &data, RepeatType repeatType );
	PatternItemList *patListConcat( PatternItemList *list1, PatternItemList *list2 );
	ConsItemList *consListConcat( ConsItemList *list1, ConsItemList *list2 );
	LangStmt *forScope( const InputLoc &loc, const String &data,
			NameScope *scope, TypeRef *typeRef, IterCall *iterCall, StmtList *stmtList );
	void preEof( const InputLoc &loc, StmtList *stmtList, ObjectDef *localFrame );

	ProdEl *prodElName( const InputLoc &loc, const String &data,
			NamespaceQual *nspaceQual, ObjectField *objField, RepeatType repeatType,
			bool commit );
	ProdEl *prodElLiteral( const InputLoc &loc, const String &data,
			NamespaceQual *nspaceQual, ObjectField *objField, RepeatType repeatType,
			bool commit );
	ConsItemList *consElLiteral( const InputLoc &loc, TypeRef *consTypeRef,
			const String &data, NamespaceQual *nspaceQual );
	Production *production( const InputLoc &loc, ProdElList *prodElList,
			String name, bool commit, CodeBlock *codeBlock, LangEl *predOf );
	void objVarDef( ObjectDef *objectDef, ObjectField *objField );
	LelDefList *prodAppend( LelDefList *defList, Production *definition );

	LangExpr *construct( const InputLoc &loc, ObjectField *objField,
			ConsItemList *list, TypeRef *typeRef, FieldInitVect *fieldInitVect );
	LangExpr *match( const InputLoc &loc, LangVarRef *varRef,
			PatternItemList *list );
	LangStmt *varDef( ObjectField *objField,
			LangExpr *expr, LangStmt::Type assignType );
	LangStmt *exportStmt( ObjectField *objField, LangStmt::Type assignType, LangExpr *expr );

	LangExpr *require( const InputLoc &loc, LangVarRef *varRef, PatternItemList *list );
	void structVarDef( const InputLoc &loc, ObjectField *objField );
	void structHead( const InputLoc &loc, Namespace *inNspace,
			const String &data, ObjectDef::Type objectType );
	StmtList *appendStatement( StmtList *stmtList, LangStmt *stmt );
	ParameterList *appendParam( ParameterList *paramList, ObjectField *objField );
	ObjectField *addParam( const InputLoc &loc,
			ObjectField::Type type, TypeRef *typeRef, const String &name );
	PredDecl *predTokenName( const InputLoc &loc, NamespaceQual *qual, const String &data );
	PredDecl *predTokenLit( const InputLoc &loc, const String &data,
			NamespaceQual *nspaceQual );
	void alias( const InputLoc &loc, const String &data, TypeRef *typeRef );
	void precedenceStmt( PredType predType, PredDeclList *predDeclList );
	ProdElList *appendProdEl( ProdElList *prodElList, ProdEl *prodEl );

	void pushScope();
	void popScope();

	virtual void go( long activeRealm ) = 0;

	BstSet<String, CmpStr> genericElDefined;

	NamespaceQual *emptyNspaceQual()
	{
		return NamespaceQual::cons( curNspace() );
	}

};

#endif
