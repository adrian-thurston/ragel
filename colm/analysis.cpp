#include "bytecode.h"
#include "parsedata.h"
#include <iostream>
#include <assert.h>

using std::cout;

void TypeRef::analyze( ParseData *pd ) const
{
	/* Look for the production's associated region. */
	Namespace *nspace = nspaceQual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have namespace for resolving reference" << endp;
	
	/* Look up the language element in the region. */
	KlangEl *langEl = getKlangEl( pd, nspace, typeName );

	if ( repeatType == RepeatRepeat ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String repeatName( 128, "_repeat_%s", typeName.data );

	  	SymbolMapEl *inDict = nspace->symbolMap.find( repeatName );
	    if ( inDict == 0 )
			pd->makeRepeatProd( nspace, repeatName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatList ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String listName( 128, "_list_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( listName );
	    if ( inDict == 0 )
			pd->makeListProd( nspace, listName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatOpt ) {
		/* If the factor is an opt, create the opt element and link the factor
		 * to it. */
		String optName( 128, "_opt_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( optName );
	    if ( inDict == 0 )
			pd->makeOptProd( nspace, optName, nspaceQual, typeName );
	}
}

void LangTerm::analyze( ParseData *pd ) const
{
	switch ( type ) {
		case ConstructType: {
			typeRef->analyze( pd );
			break;
		}
	}
}

void LangVarRef::analyze( ParseData *pd ) const
{

}

void LangExpr::analyze( ParseData *pd ) const
{
	switch ( type ) {
		case BinaryType: {
			left->analyze( pd );
			right->analyze( pd );
			break;
		}
		case UnaryType: {
			right->analyze( pd );
			break;
		}
		case TermType: {
			term->analyze( pd );
			break;
		}
	}
}

void LangStmt::analyze( ParseData *pd ) const
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType: {
			/* Push the args backwards. */
			for ( ExprVect::Iter pex = exprPtrVect->last(); pex.gtb(); pex-- )
				(*pex)->analyze( pd );
			break;
		}
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. */
			expr->analyze( pd );
			break;
		}
		case IfType: {
			/* Evaluate the test. */
			expr->analyze( pd );

			/* Analyze the if true branch. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->analyze( pd );

			if ( elsePart != 0 ) {
				for ( StmtList::Iter stmt = *elsePart; stmt.lte(); stmt++ )
					stmt->analyze( pd );
			}

			break;
		}
		case RejectType:
			break;
		case WhileType: {
			expr->analyze( pd );

			/* Compute the while block. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->analyze( pd );
			break;
		}
		case AssignType: {
			/* Evaluate the exrepssion. */
			expr->analyze( pd );
			break;
		}
		case ForIterType: {
			/* Evaluate and push the arguments. */
			langTerm->analyze( pd );

			/* Compile the contents. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->analyze( pd );

			break;
		}
		case ReturnType: {
			/* Evaluate the exrepssion. */
			expr->analyze( pd );
			break;
		}
		case BreakType: {
			break;
		}
		case YieldType: {
			/* take a reference and yield it. Immediately reset the referece. */
			varRef->analyze( pd );
			break;
		}
	}
}

void CodeBlock::analyze( ParseData *pd ) const
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->analyze( pd );
}

void ParseData::analyzeFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->analyze( this );
}

void ParseData::analyzeUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->analyze( this );
}

void ParseData::analyzePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;
	block->analyze( this );
}

void ParseData::analyzeRootBlock()
{
	CodeBlock *block = rootCodeBlock;
	block->analyze( this );
}

void ParseData::analyzeTranslateBlock( KlangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;
	block->analyze( this );
}

void ParseData::analyzeReductionCode( Definition *prod )
{
	CodeBlock *block = prod->redBlock;
	block->analyze( this );
}

void ParseData::analyzeParseTree()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			analyzeUserIter( f );
		else
			analyzeFunction( f );
	}

	/* Compile the reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			analyzeReductionCode( prod );
	}

	/* Compile the token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			analyzeTranslateBlock( lel );
	}

	/* Compile preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			analyzePreEof( r );
	}

	/* Compile the init code */
	analyzeRootBlock( );
}

