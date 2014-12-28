/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
 */

#include "codegen.h"
#include "ragel.h"
#include "redfsm.h"
#include "gendata.h"
#include "inputdata.h"
#include "parsedata.h"
#include <sstream>
#include <string>
#include <assert.h>


using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;
using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

TableArray::TableArray( const char *name, CodeGen &codeGen )
:
	state(InitialState),
	name(name),
	width(0),
	isSigned(true),
	isChar(false),
	values(0),

	/*
	 * Use zero for min and max because 
	 * we we null terminate every array.
	 */
	min(0),
	max(0),

	codeGen(codeGen),
	out(codeGen.out),
	ln(0)
{
	codeGen.arrayVector.append( this );
}

std::string TableArray::ref() const
{
	return string("_") + codeGen.DATA_PREFIX() + name;
}

long long TableArray::size()
{
	return width * values;
}

void TableArray::startAnalyze()
{
}

void TableArray::valueAnalyze( long long v )
{
	values += 1;
	if ( v < min )
		min = v;
	if ( v > max )
		max = v;
}

void TableArray::finishAnalyze()
{
	if ( directBackend ) {
		/* Calculate the type if it is not already set. */
		if ( type.empty() ) {
			if ( min >= S8BIT_MIN && max <= S8BIT_MAX ) {
				type = "char";
				width = sizeof(char);
			}
			else if ( min >= S16BIT_MIN && max <= S16BIT_MAX ) {
				type = "short";
				width = sizeof(short);
			}
			else if ( min >= S32BIT_MIN && max <= S32BIT_MAX ) {
				type = "int";
				width = sizeof(int);
			}
			else if ( min >= S64BIT_MAX && max <= S64BIT_MAX ) {
				type = "long";
				width = sizeof(long);
			}
			else  {
				type = "long long";
				width = sizeof(long long);
			}
		}
	}
	else {
		/* Calculate the type if it is not already set. */
		if ( type.empty() ) {
			if ( min >= S8BIT_MIN && max <= S8BIT_MAX ) {
				type = "s8";
				width = sizeof(char);
			}
			else if ( min >= S16BIT_MIN && max <= S16BIT_MAX ) {
				type = "s16";
				width = sizeof(short);
			}
			else if ( min >= S32BIT_MIN && max <= S32BIT_MAX ) {
				type = "s32";
				width = sizeof(int);
			}
			else if ( min >= S64BIT_MAX && max <= S64BIT_MAX ) {
				type = "s64";
				width = sizeof(long);
			}
			else  {
				type = "s128";
				width = sizeof(long long);
			}
		}
	}
}

void TableArray::startGenerate()
{
	if ( directBackend ) {
		out << "static const " << type << " " << 
			"_" << codeGen.DATA_PREFIX() << name << 
			"[] = {\n\t";
	}
	else {
		out << "array " << type << " " << 
			"_" << codeGen.DATA_PREFIX() << name << 
			"( " << min << ", " << max << " ) = { ";
	}
}

void TableArray::valueGenerate( long long v )
{
	if ( directBackend ) {
		if ( isChar )
			out << "c(" << v << ")";
		else if ( !isSigned )
			out << v << "u";
		else
			out << v;

		if ( ( ++ln % IALL ) == 0 ) {
			out << ",\n\t";
			ln = 0;
		}
		else {
			out << ", ";
		}
	}
	else {
		if ( isChar )
			out << "c(" << v << ")";
		else if ( !isSigned )
			out << "u(" << v << ")";
		else
			out << v;
		out << ", ";
	}
}

void TableArray::finishGenerate()
{
	if ( directBackend ) {
		if ( isChar )
			out << "c(0)\n};\n\n";
		else if ( !isSigned )
			out << "0u\n};\n\n";
		else
			out << "0\n};\n\n";
	}
	else {
		if ( isChar )
			out << "c(0) };\n\n";
		else if ( !isSigned )
			out << "u(0) };\n\n";
		else
			out << "0 };\n\n";
	}

	if ( codeGen.pd->id->printStatistics ) {
		cout << name << "\t" << values << "\t" <<
			size() << "\t" << endl;
	}

	codeGen.tableData += size();
}

void TableArray::start()
{
	switch ( state ) {
		case InitialState:
			break;
		case AnalyzePass:
			startAnalyze();
			break;
		case GeneratePass:
			startGenerate();
			break;
	}
}

void TableArray::value( long long v )
{
	switch ( state ) {
		case InitialState:
			break;
		case AnalyzePass:
			valueAnalyze( v );
			break;
		case GeneratePass:
			valueGenerate( v );
			break;
	}
}

void TableArray::finish()
{
	switch ( state ) {
		case InitialState:
			break;
		case AnalyzePass:
			finishAnalyze();
			break;
		case GeneratePass:
			finishGenerate();
			break;
	}
}

/* Init code gen with in parameters. */
CodeGen::CodeGen( const CodeGenArgs &args )
:
	CodeGenData( args ),
	tableData( 0 )
{
}

void CodeGen::statsSummary()
{
	if ( pd->id->printStatistics )
		cout << "table-data\t" << tableData << endl << endl;
}

unsigned int CodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string CodeGen::ARRAY_TYPE( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );

	string ret = arrayType->data1;
	if ( arrayType->data2 != 0 ) {
		ret += " ";
		ret += arrayType->data2;
	}
	return ret;
}


/* Write out the fsm name. */
string CodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string CodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};


string CodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 ) {
		ret << OPEN_HOST_PLAIN();
		INLINE_LIST( ret, accessExpr, 0, false, false );
		ret << CLOSE_HOST_PLAIN();
		ret << ACCESS_OPER();
	}
	return ret.str();
}


string CodeGen::P()
{ 
	ostringstream ret;
	if ( pExpr == 0 )
		ret << "p";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, pExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::PE()
{
	ostringstream ret;
	if ( peExpr == 0 )
		ret << "pe";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, peExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::vEOF()
{
	ostringstream ret;
	if ( eofExpr == 0 )
		ret << "eof";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, eofExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::vCS()
{
	ostringstream ret;
	if ( csExpr == 0 )
		ret << ACCESS() << "cs";
	else {
		/* Emit the user supplied method of retrieving the key. */
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, csExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::TOP()
{
	ostringstream ret;
	if ( topExpr == 0 )
		ret << ACCESS() + "top";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, topExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::STACK()
{
	ostringstream ret;
	if ( stackExpr == 0 )
		ret << ACCESS() + "stack";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, stackExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::ACT()
{
	ostringstream ret;
	if ( actExpr == 0 )
		ret << ACCESS() + "act";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, actExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::TOKSTART()
{
	ostringstream ret;
	if ( tokstartExpr == 0 )
		ret << ACCESS() + "ts";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, tokstartExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::TOKEND()
{
	ostringstream ret;
	if ( tokendExpr == 0 )
		ret << ACCESS() + "te";
	else {
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, tokendExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	return ret.str();
}

string CodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, getKeyExpr, 0, false, false );
		ret << CLOSE_HOST_EXPR();
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "( " << DEREF( "data", P() ) << ")";
	}
	return ret.str();
}

/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string CodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

/* Write out a key from the fsm code gen. Depends on wether or not the key is
 * signed. */
string CodeGen::KEY( Key key )
{
	if ( directBackend ) {
		ostringstream ret;
		if ( keyOps->alphType->isChar )
			ret << "c(" << (unsigned long) key.getVal() << ")";
		else if ( keyOps->isSigned || !keyOps->hostLang->explicitUnsigned )
			ret << key.getVal();
		else
			ret << (unsigned long) key.getVal() << "u";
		return ret.str();
	}
	else {
		ostringstream ret;
		if ( keyOps->alphType->isChar )
			ret << "c(" << (unsigned long) key.getVal() << ")";
		else if ( keyOps->isSigned || !keyOps->hostLang->explicitUnsigned )
			ret << key.getVal();
		else
			ret << "u(" << (unsigned long) key.getVal() << ")";
		return ret.str();
	}
}

bool CodeGen::isAlphTypeSigned()
{
	return keyOps->isSigned;
}

bool CodeGen::isWideAlphTypeSigned()
{
	string ret;
	if ( keyOps->le( redFsm->maxKey, keyOps->maxKey ) )
		return isAlphTypeSigned();
	else {
		long long maxKeyVal = keyOps->getLongLong( redFsm->maxKey );
		HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
		return wideType->isSigned;
	}
}

void CodeGen::EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << OPEN_GEN_BLOCK() << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish, false );
	ret << "))-1;" << CLOSE_GEN_BLOCK() << "\n";
}

void CodeGen::LM_SWITCH( ostream &ret, GenInlineItem *item, 
		int targState, int inFinish, bool csForced )
{
	ret << 
		OPEN_GEN_BLOCK() << "switch( " << ACT() << " ) {\n";

	for ( GenInlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		if ( lma->lmId < 0 )
			ret << "	" << DEFAULT() << " {\n";
		else
			ret << "	" << CASE( STR(lma->lmId) ) << " {\n";

		/* Write the block and close it off. */
		INLINE_LIST( ret, lma->children, targState, inFinish, csForced );

		ret << CEND() << "}\n";
	}

	ret << 
		"	}" << CLOSE_GEN_BLOCK() << "\n"
		"\t";
}

void CodeGen::LM_EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. This should be in the D code generator. */
	ret << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish, false );
	ret << "))-1;\n";
}

void CodeGen::SET_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void CodeGen::SET_TOKEND( ostream &ret, GenInlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void CodeGen::GET_TOKEND( ostream &ret, GenInlineItem *item )
{
	ret << TOKEND();
}

void CodeGen::INIT_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << NIL() << ";";
}

void CodeGen::INIT_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = 0;";
}

void CodeGen::SET_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

void CodeGen::HOST_STMT( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << OPEN_HOST_BLOCK();
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << CLOSE_HOST_BLOCK();
	}
}

#if 0
void CodeGen::LM_CASE( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
	}
}
#endif

void CodeGen::HOST_EXPR( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << OPEN_HOST_EXPR();
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << CLOSE_HOST_EXPR();
	}
}

void CodeGen::HOST_TEXT( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << OPEN_HOST_PLAIN();
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << CLOSE_HOST_PLAIN();
	}
}

void CodeGen::GEN_STMT( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << OPEN_GEN_BLOCK();
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << CLOSE_GEN_BLOCK();
	}
}

void CodeGen::GEN_EXPR( ostream &ret, GenInlineItem *item, 
		int targState, bool inFinish, bool csForced )
{
	if ( item->children->length() > 0 ) {
		/* Write the block and close it off. */
		ret << OPEN_GEN_EXPR();
		INLINE_LIST( ret, item->children, targState, inFinish, csForced );
		ret << CLOSE_GEN_EXPR();
	}
}

/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void CodeGen::INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
		int targState, bool inFinish, bool csForced )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Text:
			ret << item->data;
			break;
		case GenInlineItem::Goto:
			GOTO( ret, item->targState->id, inFinish );
			break;
		case GenInlineItem::Call:
			CALL( ret, item->targState->id, targState, inFinish );
			break;
		case GenInlineItem::Ncall:
			NCALL( ret, item->targState->id, targState, inFinish );
			break;
		case GenInlineItem::Next:
			NEXT( ret, item->targState->id, inFinish );
			break;
		case GenInlineItem::Ret:
			RET( ret, inFinish );
			break;
		case GenInlineItem::Nret:
			NRET( ret, inFinish );
			break;
		case GenInlineItem::PChar:
			ret << P();
			break;
		case GenInlineItem::Char:
			ret << OPEN_GEN_EXPR() << GET_KEY() << CLOSE_GEN_EXPR();
			break;
		case GenInlineItem::Hold:
			ret << OPEN_GEN_BLOCK() << P() << " = " << P() << " - 1; " << CLOSE_GEN_BLOCK();
			break;
		case GenInlineItem::LmHold:
			ret << P() << " = " << P() << " - 1;";
			break;
		case GenInlineItem::Exec:
			EXEC( ret, item, targState, inFinish );
			break;
		case GenInlineItem::Curs:
			CURS( ret, inFinish );
			break;
		case GenInlineItem::Targs:
			TARGS( ret, inFinish, targState );
			break;
		case GenInlineItem::Entry:
			ret << item->targState->id;
			break;
		case GenInlineItem::GotoExpr:
			GOTO_EXPR( ret, item, inFinish );
			break;
		case GenInlineItem::CallExpr:
			CALL_EXPR( ret, item, targState, inFinish );
			break;
		case GenInlineItem::NcallExpr:
			NCALL_EXPR( ret, item, targState, inFinish );
			break;
		case GenInlineItem::NextExpr:
			NEXT_EXPR( ret, item, inFinish );
			break;
		case GenInlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::LmExec:
			LM_EXEC( ret, item, targState, inFinish );
			break;
		case GenInlineItem::LmCase:
			/* Not encountered here, in the lm switch. */
			break;
		case GenInlineItem::LmSetActId:
			SET_ACT( ret, item );
			break;
		case GenInlineItem::LmSetTokEnd:
			SET_TOKEND( ret, item );
			break;
		case GenInlineItem::LmGetTokEnd:
			GET_TOKEND( ret, item );
			break;
		case GenInlineItem::LmInitTokStart:
			INIT_TOKSTART( ret, item );
			break;
		case GenInlineItem::LmInitAct:
			INIT_ACT( ret, item );
			break;
		case GenInlineItem::LmSetTokStart:
			SET_TOKSTART( ret, item );
			break;
		case GenInlineItem::Break:
			BREAK( ret, targState, csForced );
			break;
		case GenInlineItem::Nbreak:
			NBREAK( ret, targState, csForced );
			break;
		case GenInlineItem::HostStmt:
			HOST_STMT( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::HostExpr:
			HOST_EXPR( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::HostText:
			HOST_TEXT( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::GenStmt:
			GEN_STMT( ret, item, targState, inFinish, csForced );
			break;
		case GenInlineItem::GenExpr:
			GEN_EXPR( ret, item, targState, inFinish, csForced );
			break;
		}
	}
}
/* Write out paths in line directives. Escapes any special characters. */
string CodeGen::LDIR_PATH( char *path )
{
	ostringstream ret;
	for ( char *pc = path; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			ret << "\\\\";
		else
			ret << *pc;
	}
	return ret.str();
}

void CodeGen::ACTION( ostream &ret, GenAction *action, IlOpts opts )
{
	ret << '\t';
	ret << OPEN_HOST_BLOCK( action->loc.fileName, action->loc.line );
	INLINE_LIST( ret, action->inlineList, opts.targState, opts.inFinish, opts.csForced );
	ret << CLOSE_HOST_BLOCK();
}

void CodeGen::CONDITION( ostream &ret, GenAction *condition )
{
	ret << OPEN_HOST_EXPR( condition->loc.fileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false, false );
	ret << CLOSE_HOST_EXPR();
}

string CodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string CodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void CodeGen::writeInit()
{
	out << "	{\n";

	if ( !noCS )
		out << "\t" << vCS() << " = " << START() << ";\n";
	
	/* If there are any calls, then the stack top needs initialization. */
	if ( redFsm->anyActionCalls() || redFsm->anyActionNcalls() || redFsm->anyActionRets() || redFsm->anyActionNrets() )
		out << "\t" << TOP() << " = 0;\n";

	if ( hasLongestMatch ) {
		out << 
			"	" << TOKSTART() << " = " << NIL() << ";\n"
			"	" << TOKEND() << " = " << NIL() << ";\n";

		if ( redFsm->usingAct() ) {
			out << 
				"	" << ACT() << " = 0;\n";
		}
	}
	out << "	}\n";
}

string CodeGen::DATA_PREFIX()
{
	if ( !noPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string CodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

void CodeGen::VALUE( string type, string name, string value )
{
	if ( directBackend )
		out << "static const " << type << " " << name << " = " << value << ";\n";
	else
		out << "value " << type << " " << name << " = " << value << ";\n";
}

string CodeGen::STR( int v )
{
	ostringstream s;
	s << v;
	return s.str();
}

void CodeGen::STATE_IDS()
{
	if ( redFsm->startState != 0 )
		VALUE( "int",  START(), START_STATE_ID() );

	if ( !noFinal )
		VALUE( "int", FIRST_FINAL(), FIRST_FINAL_STATE() );

	if ( !noError )
		VALUE( "int", ERROR(), ERROR_STATE() );

	out << "\n";

	if ( entryPointNames.length() > 0 ) {
		for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
			string name = DATA_PREFIX() + "en_" + *en;
			VALUE( "int", name, STR( entryPointIds[en.pos()] ) );
		}
		out << "\n";
	}
}

void CodeGen::writeStart()
{
	out << START_STATE_ID();
}

void CodeGen::writeFirstFinal()
{
	out << FIRST_FINAL_STATE();
}

void CodeGen::writeError()
{
	out << ERROR_STATE();
}

void CodeGen::writeExports()
{
	if ( exportList.length() > 0 ) {
		for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
			out << EXPORT( ALPH_TYPE(), 
				DATA_PREFIX() + "ex_" + ex->name, KEY(ex->key) ) << "\n";
		}
		out << "\n";
	}
}

ostream &CodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &CodeGen::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}
