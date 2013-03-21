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

#include <iostream>
#include "avltree.h"
#include "parsedata.h"
#include "parser.h"

struct lex_factor;
struct lex_factor_neg;
struct lex_factor_rep;
struct lex_term;
struct lex_expr;
struct token_list;
struct prod_el_list;
struct prod_list;
struct root_item;
struct region_def;
struct cfl_def;
struct statement;
struct print_stmt;
struct expr_stmt;
struct var_ref;
struct code_expr;
struct _repeat_code_expr;
struct qual;
struct region_qual;
struct opt_repeat;
struct type_ref;
struct root_item;
struct _repeat_root_item;
struct namespace_def;
struct var_def;
struct block_or_single;
struct iter_call;
struct lang_stmt_list;
struct elsif_list;
struct elsif_clause;
struct optional_else;
struct code_factor;
struct reg_or_char;
struct reg_or_data;
struct literal_item;
struct literal_list;
struct literal_def;
struct token_def;
struct ignore_def;
struct context_def;
struct context_item;
struct context_var_def;
struct opt_reduce;
struct field_init;
struct opt_field_init;

struct LoadSource
:
	public BaseParser
{
	LoadSource( Compiler *pd, const char *inputFileName )
	:
		BaseParser( pd ),
		inputFileName( inputFileName )
	{}

	const char *inputFileName;

	ObjectField *walkVarDef( var_def varDef );
	NamespaceQual *walkRegionQual( region_qual regionQual );
	RepeatType walkOptRepeat( opt_repeat OptRepeat );
	TypeRef *walkTypeRef( type_ref typeRef );

	ReOrItem *walkRegOrChar( reg_or_char regOrChar );
	ReOrBlock *walkRegOrData( reg_or_data regOrData );

	LexFactor *walkLexFactor( lex_factor &LexFactorTree );
	LexFactorNeg *walkLexFactorNeg( lex_factor_neg &LexFactorNegTree );
	LexFactorRep *walkLexFactorRep( lex_factor_rep &LexFactorRepTree );
	LexFactorAug *walkLexFactorAug( lex_factor_rep &LexFactorRepTree );
	LexTerm *walkLexTerm( lex_term &LexTerm );
	LexExpression *walkLexExpr( lex_expr &LexExpr );
	ExprVect *walkCodeExprList( _repeat_code_expr codeExprList );
	LangExpr *walkCodeExpr( code_expr codeExpr );
	void walkTokenList( token_list &TokenList );
	void walkLexRegion( region_def regionDef );
	void walkProdElList( ProdElList *list, prod_el_list ProdElList );
	void walkProdList( LelDefList *lelDefList, prod_list &ProdList );
	void walkCflDef( cfl_def cflDef );
	LangTerm *walkIterCall( iter_call IterCall );
	LangStmt *walkOptionalElse( optional_else optionalElse );
	LangStmt *walkElsifClause( elsif_clause elsifClause );
	LangStmt *walkElsifList( elsif_list elsifList );
	LangStmt *walkStatement( statement Statement );
	LangStmt *walkPrintStmt( print_stmt &PrintStmt );
	LangExpr *walkCodeFactor( code_factor codeFactor );
	LangStmt *walkExprStmt( expr_stmt &ExprStmt );
	QualItemVect *walkQual( qual &Qual );
	LangVarRef *walkVarRef( var_ref varRef );
	void walkRootItem( root_item &rootItem, StmtList *stmtList );
	StmtList *walkRootItemList( _repeat_root_item rootItemList );
	void walkNamespaceDef( namespace_def NamespaceDef );
	StmtList *walkLangStmtList( lang_stmt_list LangStmtList );
	StmtList *walkBlockOrSingle( block_or_single blockOrSingle );

	void walkLiteralItem( literal_item literalItem );
	void walkLiteralList( literal_list literalList );
	void walkLiteralDef( literal_def literalDef );

	void walkTokenDef( token_def TokenDef );
	void walkIgnoreDef( ignore_def IgnoreDef );
	void walkContextDef( context_def contextDef );
	void walkContextItem( context_item contextItem );
	void walkContextVarDef( context_var_def contextVarDef );
	CodeBlock *walkOptReduce( opt_reduce optReduce );

	void walkFieldInit( FieldInitVect *list, field_init fieldInit );
	FieldInitVect *walkOptFieldInit( opt_field_init optFieldInit );
	void go();
};
