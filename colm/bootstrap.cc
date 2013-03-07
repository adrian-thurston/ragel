/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
#include <errno.h>

#include "parser.h"
#include "config.h"
#include "lmparse.h"
#include "global.h"
#include "input.h"
#include "bootstrap.h"

#include "colm/colm.h"

void BootstrapBase::parseInput( StmtList *stmtList )
{
	NamespaceQual *nspaceQual = NamespaceQual::cons( namespaceStack.top() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, String("start"), RepeatNone );

	LangVarRef *varRef = LangVarRef::cons( internal, new QualItemVect, String("stdin") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::VarRefType, varRef ) );

	ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, expr );
	ConsItemList *list = ConsItemList::cons( consItem );

	ObjectField *objField = ObjectField::cons( internal, 0, String("P") );

	expr = parseCmd( internal, false, objField, typeRef, 0, list );
	LangStmt *stmt = LangStmt::cons( internal, LangStmt::ExprType, expr );
	stmtList->append( stmt );
}

void BootstrapBase::exportTree( StmtList *stmtList )
{
	QualItemVect *qual = new QualItemVect;
	qual->append( QualItem( internal, String( "P" ), QualItem::Dot ) );
	LangVarRef *varRef = LangVarRef::cons( internal, qual, String("tree") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::VarRefType, varRef ) );

	NamespaceQual *nspaceQual = NamespaceQual::cons( namespaceStack.top() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, String("start"), RepeatNone );
	ObjectField *program = ObjectField::cons( internal, typeRef, String("Colm0Tree") );
	LangStmt *programExport = exportStmt( program, LangStmt::AssignType, expr );
	stmtList->append( programExport );
}


