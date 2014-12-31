/*
 *  Copyright 2014-2015 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "ragel.h"
#include "asm.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"
#include "ragel.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"
#include <sstream>

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

extern int numSplitPartitions;
extern bool noLineDirectives;
bool printStatistics = false;

void asmLineDirective( ostream &out, const char *fileName, int line )
{
#if 0
	if ( noLineDirectives )
		out << "/* ";

	/* Write the preprocessor line info for to the input file. */
	out << "#line " << line  << " \"";
	for ( const char *pc = fileName; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
	out << '"';

	if ( noLineDirectives )
		out << " */";

	out << '\n';
#endif
}

void AsmCodeGen::genAnalysis()
{
	/* For directly executable machines there is no required state
	 * ordering. Choose a depth-first ordering to increase the
	 * potential for fall-throughs. */
	redFsm->depthFirstOrdering();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Choose single. */
	redFsm->chooseSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;
	
	redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	analyzeMachine();
}

void AsmCodeGen::genLineDirective( ostream &out )
{
	std::streambuf *sbuf = out.rdbuf();
	output_filter *filter = static_cast<output_filter*>(sbuf);
	asmLineDirective( out, filter->fileName, filter->line + 1 );
}

/* Init code gen with in parameters. */
AsmCodeGen::AsmCodeGen( const CodeGenArgs &args )
:
	CodeGenData( args )
{
	static int gmn = 1;
	mn = gmn++;
}

unsigned int AsmCodeGen::arrayTypeSize( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	HostType *arrayType = keyOps->typeSubsumes( maxValLL );
	assert( arrayType != 0 );
	return arrayType->size;
}

string AsmCodeGen::ARRAY_TYPE( unsigned long maxVal )
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

HostType *AsmCodeGen::arrayType( unsigned long maxVal )
{
	long long maxValLL = (long long) maxVal;
	return keyOps->typeSubsumes( maxValLL );
}

/* Write out the fsm name. */
string AsmCodeGen::FSM_NAME()
{
	return fsmName;
}

/* Emit the offset of the start state as a decimal integer. */
string AsmCodeGen::START_STATE_ID()
{
	ostringstream ret;
	ret << redFsm->startState->id;
	return ret.str();
};

string AsmCodeGen::ACCESS()
{
	ostringstream ret;
	if ( accessExpr != 0 )
		INLINE_LIST( ret, accessExpr, 0, false, false );
	return ret.str();
}


string AsmCodeGen::P()
{ 
	ostringstream ret;
	if ( pExpr == 0 )
		ret << "p";
	else {
		ret << "(";
		INLINE_LIST( ret, pExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::PE()
{
	ostringstream ret;
	if ( peExpr == 0 )
		ret << "pe";
	else {
		ret << "(";
		INLINE_LIST( ret, peExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::vEOF()
{
	ostringstream ret;
	if ( eofExpr == 0 )
		ret << "eof";
	else {
		ret << "(";
		INLINE_LIST( ret, eofExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::vCS()
{
	ostringstream ret;
	if ( csExpr == 0 )
		ret << ACCESS() << "cs";
	else {
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, csExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOP()
{
	ostringstream ret;
	if ( topExpr == 0 )
		ret << ACCESS() + "top";
	else {
		ret << "(";
		INLINE_LIST( ret, topExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::STACK()
{
	ostringstream ret;
	if ( stackExpr == 0 )
		ret << ACCESS() + "stack";
	else {
		ret << "(";
		INLINE_LIST( ret, stackExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::ACT()
{
	ostringstream ret;
	if ( actExpr == 0 )
		ret << ACCESS() + "act";
	else {
		ret << "(";
		INLINE_LIST( ret, actExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOKSTART()
{
	ostringstream ret;
	if ( tokstartExpr == 0 )
		ret << ACCESS() + "ts";
	else {
		ret << "(";
		INLINE_LIST( ret, tokstartExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::TOKEND()
{
	ostringstream ret;
	if ( tokendExpr == 0 )
		ret << ACCESS() + "te";
	else {
		ret << "(";
		INLINE_LIST( ret, tokendExpr, 0, false, false );
		ret << ")";
	}
	return ret.str();
}

string AsmCodeGen::GET_KEY()
{
	ostringstream ret;
	if ( getKeyExpr != 0 ) { 
		/* Emit the user supplied method of retrieving the key. */
		ret << "(";
		INLINE_LIST( ret, getKeyExpr, 0, false, false );
		ret << ")";
	}
	else {
		/* Expression for retrieving the key, use simple dereference. */
		ret << "(*" << P() << ")";
	}
	return ret.str();
}

/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string AsmCodeGen::TABS( int level )
{
	string result;
	while ( level-- > 0 )
		result += "\t";
	return result;
}

string AsmCodeGen::COND_KEY( CondKey key )
{
	ostringstream ret;
//	ostringstream ret;
//	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
//		ret << key.getVal();
//	else
//		ret << (unsigned long) key.getVal() << 'u';
	ret << "$" << key.getVal();
	return ret.str();
}


/* Write out a key from the fsm code gen. Depends on wether or not the key is
 * signed. */
string AsmCodeGen::KEY( Key key )
{
	ostringstream ret;
//	ostringstream ret;
//	if ( keyOps->isSigned || !hostLang->explicitUnsigned )
//		ret << key.getVal();
//	else
//		ret << (unsigned long) key.getVal() << 'u';
	ret << "$" << key.getVal();
	return ret.str();
}

bool AsmCodeGen::isAlphTypeSigned()
{
	return keyOps->isSigned;
}

bool AsmCodeGen::isWideAlphTypeSigned()
{
//	string ret;
//	if ( redFsm->maxKey <= keyOps->maxKey )
		return isAlphTypeSigned();
//	else {
//		long long maxKeyVal = redFsm->maxKey.getLongLong();
//		HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
//		return wideType->isSigned;
//	}
}

void AsmCodeGen::EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish )
{
	/* The parser gives fexec two children. The double brackets are for D
	 * code. If the inline list is a single word it will get interpreted as a
	 * C-style cast by the D compiler. */
	ret << "{" << P() << " = ((";
	INLINE_LIST( ret, item->children, targState, inFinish, false );
	ret << "))-1;}";
}

void AsmCodeGen::LM_SWITCH( ostream &ret, GenInlineItem *item, 
		int targState, int inFinish, bool csForced )
{
#if 0
	ret << 
		"	switch( " << ACT() << " ) {\n";

	bool haveDefault = false;
	for ( GenInlineList::Iter lma = *item->children; lma.lte(); lma++ ) {
		/* Write the case label, the action and the case break. */
		if ( lma->lmId < 0 ) {
			ret << "	default:\n";
			haveDefault = true;
		}
		else
			ret << "	case " << lma->lmId << ":\n";

		/* Write the block and close it off. */
		ret << "	{";
		INLINE_LIST( ret, lma->children, targState, inFinish, csForced );
		ret << "}\n";

		ret << "	break;\n";
	}

	if ( (hostLang->lang == HostLang::D || hostLang->lang == HostLang::D2) && !haveDefault )
		ret << "	default: break;";

	ret << 
		"	}\n"
		"\t";
#endif
}

void AsmCodeGen::SET_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = " << item->lmId << ";";
}

void AsmCodeGen::SET_TOKEND( ostream &ret, GenInlineItem *item )
{
	/* The tokend action sets tokend. */
	ret << TOKEND() << " = " << P();
	if ( item->offset != 0 ) 
		out << "+" << item->offset;
	out << ";";
}

void AsmCodeGen::GET_TOKEND( ostream &ret, GenInlineItem *item )
{
	ret << TOKEND();
}

void AsmCodeGen::INIT_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << NULL_ITEM() << ";";
}

void AsmCodeGen::INIT_ACT( ostream &ret, GenInlineItem *item )
{
	ret << ACT() << " = 0;";
}

void AsmCodeGen::SET_TOKSTART( ostream &ret, GenInlineItem *item )
{
	ret << TOKSTART() << " = " << P() << ";";
}

/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void AsmCodeGen::INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
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
		case GenInlineItem::Next:
			NEXT( ret, item->targState->id, inFinish );
			break;
		case GenInlineItem::Ret:
			RET( ret, inFinish );
			break;
		case GenInlineItem::PChar:
			ret << P();
			break;
		case GenInlineItem::Char:
			ret << GET_KEY();
			break;
		case GenInlineItem::Hold:
			ret << P() << "--;";
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
		case GenInlineItem::NextExpr:
			NEXT_EXPR( ret, item, inFinish );
			break;
		case GenInlineItem::LmSwitch:
			LM_SWITCH( ret, item, targState, inFinish, csForced );
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
		/* Stubbed. */
		case GenInlineItem::Ncall:
		case GenInlineItem::NcallExpr:
		case GenInlineItem::Nret:
		case GenInlineItem::Nbreak:
		case GenInlineItem::LmExec:
		case GenInlineItem::HostStmt:
		case GenInlineItem::HostExpr:
		case GenInlineItem::HostText:
		case GenInlineItem::GenStmt:
		case GenInlineItem::GenExpr:
		case GenInlineItem::LmCase:
		case GenInlineItem::LmHold:
			break;
		}
	}
}
/* Write out paths in line directives. Escapes any special characters. */
string AsmCodeGen::LDIR_PATH( char *path )
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

void AsmCodeGen::ACTION( ostream &ret, GenAction *action, int targState, 
		bool inFinish, bool csForced )
{
	/* Write the preprocessor line info for going into the source file. */
	asmLineDirective( ret, action->loc.fileName, action->loc.line );

	/* Write the block and close it off. */
	// ret << "\t{";
	INLINE_LIST( ret, action->inlineList, targState, inFinish, csForced );
	// ret << "}\n";
}

void AsmCodeGen::CONDITION( ostream &ret, GenAction *condition )
{
	ret << "\n";
	asmLineDirective( ret, condition->loc.fileName, condition->loc.line );
	INLINE_LIST( ret, condition->inlineList, 0, false, false );
}

string AsmCodeGen::ERROR_STATE()
{
	ostringstream ret;
	if ( redFsm->errState != 0 )
		ret << redFsm->errState->id;
	else
		ret << "-1";
	return ret.str();
}

string AsmCodeGen::FIRST_FINAL_STATE()
{
	ostringstream ret;
	if ( redFsm->firstFinState != 0 )
		ret << redFsm->firstFinState->id;
	else
		ret << redFsm->nextStateId;
	return ret.str();
}

void AsmCodeGen::writeInit()
{
//	out << "	{\n";

	if ( !noCS ) {
		// out << "\t" << vCS() << " = " << START() << ";\n";
		out <<
			"	movl		$" << redFsm->startState->id << ", cs(%rip)\n";
	}
	
//	/* If there are any calls, then the stack top needs initialization. */
//	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
//		out << "\t" << TOP() << " = 0;\n";

//	if ( hasLongestMatch ) {
//		out << 
//			"	" << TOKSTART() << " = " << NULL_ITEM() << ";\n"
//			"	" << TOKEND() << " = " << NULL_ITEM() << ";\n"
//			"	" << ACT() << " = 0;\n";
//	}
//	out << "	}\n";
}

string AsmCodeGen::DATA_PREFIX()
{
	if ( !noPrefix )
		return FSM_NAME() + "_";
	return "";
}

/* Emit the alphabet data type. */
string AsmCodeGen::ALPH_TYPE()
{
	string ret = keyOps->alphType->data1;
	if ( keyOps->alphType->data2 != 0 ) {
		ret += " ";
		ret += + keyOps->alphType->data2;
	}
	return ret;
}

void AsmCodeGen::STATIC_CONST_INT( const string &name, const string &value )
{
	out <<
		"	.align	4\n"
		"	.type	" << name << ", @object\n"
		"	.size	" << name << ", 4\n" <<
		name << ":\n"
		"	.long	" << value << "\n";
}

void AsmCodeGen::STATE_IDS()
{
	if ( redFsm->startState != 0 )
		STATIC_CONST_INT( START(), START_STATE_ID() );

	if ( !noFinal )
		STATIC_CONST_INT( FIRST_FINAL(), FIRST_FINAL_STATE() );

	if ( !noError )
		STATIC_CONST_INT( ERROR(), ERROR_STATE() );

	out << "\n";

	if ( entryPointNames.length() > 0 ) {
		for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
			ostringstream ret;
			ret << redFsm->startState->id;

			STATIC_CONST_INT( string( DATA_PREFIX() + "en_" + *en ),
					ret.str() );
		}
		out << "\n";
	}
}

void AsmCodeGen::writeStart()
{
	out << START_STATE_ID();
}

void AsmCodeGen::writeFirstFinal()
{
	out << FIRST_FINAL_STATE();
}

void AsmCodeGen::writeError()
{
	out << ERROR_STATE();
}

string AsmCodeGen::PTR_CONST()
{
	return "const ";
}

string AsmCodeGen::PTR_CONST_END()
{
	return "";
}

std::ostream &AsmCodeGen::OPEN_ARRAY( string type, string name )
{
	out << "static const " << type << " " << name << "[] = {\n";
	return out;
}

std::ostream &AsmCodeGen::CLOSE_ARRAY()
{
	return out << "};\n";
}

std::ostream &AsmCodeGen::STATIC_VAR( string type, string name )
{
	out << "static const " << type << " " << name;
	return out;
}

string AsmCodeGen::UINT( )
{
	return "unsigned int";
}

string AsmCodeGen::ARR_OFF( string ptr, string offset )
{
	return ptr + " + " + offset;
}

string AsmCodeGen::CAST( string type )
{
	return "(" + type + ")";
}

string AsmCodeGen::NULL_ITEM()
{
	return "0";
}

string AsmCodeGen::POINTER()
{
	return " *";
}

std::ostream &AsmCodeGen::SWITCH_DEFAULT()
{
	return out;
}

string AsmCodeGen::CTRL_FLOW()
{
	return "";
}

void AsmCodeGen::writeExports()
{
	if ( exportList.length() > 0 ) {
		for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
			out << "#define " << DATA_PREFIX() << "ex_" << ex->name << " " << 
					KEY(ex->key) << "\n";
		}
		out << "\n";
	}
}

void AsmCodeGen::finishRagelDef()
{
	/* For directly executable machines there is no required state
	 * ordering. Choose a depth-first ordering to increase the
	 * potential for fall-throughs. */
	redFsm->depthFirstOrdering();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
	redFsm->chooseSingle();
		
	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;
	
	redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among
	 * other things. We will use these in reporting the usage
	 * of fsm directives in action code. */
	analyzeMachine();

	/* Determine if we should use indicies. */
	calcIndexSize();
}

ostream &AsmCodeGen::source_warning( const InputLoc &loc )
{
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return cerr;
}

ostream &AsmCodeGen::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return cerr;
}

void AsmCodeGen::emitSingleIfElseIf( RedStateAp *state )
{
	/* Load up the singles. */
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;

	/* Write out the single indicies. */
	for ( int j = 0; j < numSingles; j++ ) {
		out <<
			"	cmpb	" << KEY( data[j].lowKey ) << ", %r14b\n"
			"	je	" << TRANS_GOTO_TARG( data[j].value ) << "\n";
	}
}

void AsmCodeGen::emitSingleJumpTable( RedStateAp *state, string def )
{
	static int l = 1;
	int table = l++;
	int failed = l++;
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;

	long long low = data[0].lowKey.getVal();
	long long high = data[numSingles-1].lowKey.getVal();

	if ( def.size() == 0 ) {
		std::stringstream s;
		s << ".L" << mn << "_sjt_" << failed;
		def = s.str();
	}

	out <<
		"	cmpb	$" << low << ", %r14b\n"
		"	jl	" << def << "\n"
		"	cmpb	$" << high << ", %r14b\n"
		"	jg	" << def << "\n"
		"	movzbq	%r14b, %rax\n"
		"	subq	$" << low << ", %rax\n"
		"	jmp	*.L" << mn << "_sjt_" << table << "(,%rax,8)\n"
		"	.section .rodata\n"
		"	.align 8\n"
		".L" << mn << "_sjt_" << table << ":\n";

	for ( long long j = 0; j < numSingles; j++ ) {
		/* Fill in gap between prev and this. */
		if ( j > 0 ){
			long long span = keyOps->span( data[j-1].lowKey, data[j].lowKey ) - 2;
			for ( long long k = 0; k < span; k++ ) {
				out <<
					"	.quad	" << def << "\n";
			}
		}

		out <<
			"	.quad	" << TRANS_GOTO_TARG( data[j].value ) << "\n";
	}

	out <<
		"	.text\n"
		".L" << mn << "_sjt_" << failed << ":\n";
}


void AsmCodeGen::emitRangeBSearch( RedStateAp *state, int level, int low, int high )
{
	static int nl = 1;

	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = keyOps->eq( data[mid].lowKey, keyOps->minKey );
	bool limitHigh = keyOps->eq( data[mid].highKey, keyOps->maxKey );
	
//	string nf = TRANS_GOTO_TARG( state->defTrans );

	/* For some reason the hop is faster and results in smaller code. Not sure
	 * why. */
	std::stringstream nfss;
	nfss << ".L" << mn << "_nf_" << state->id;
	string nf = nfss.str();

	if ( anyLower && anyHigher ) {
		int l1 = nl++;
		string targ = TRANS_GOTO_TARG( data[mid].value );

		/* Can go lower and higher than mid. */
		out <<
			"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
			"	jge	.L" << mn << "_nl" << l1 << "\n";
			
		
		emitRangeBSearch( state, level+1, low, mid-1 );

		out <<
			".L" << mn << "_nl" << l1 << ":\n";

		if ( !keyOps->eq( data[mid].lowKey, data[mid].highKey ) ) {
			out <<
				"	cmpb	" << KEY ( data[mid].highKey ) << ", %r14b\n";
		}

		out <<
			"	jle	" << targ << "\n";

		emitRangeBSearch( state, level+1, mid+1, high );
	}
	else if ( anyLower && !anyHigher ) {

		string targ;
		if ( limitHigh ) {
			targ = TRANS_GOTO_TARG( data[mid].value );
		}
		else {
			std::stringstream s;
			s << ".L" << mn << "_nl" << nl++;
			targ = s.str();
		}

		/* Can go lower than mid but not higher. */

		out <<
			"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
			"	jge	" << targ << "\n";

		emitRangeBSearch( state, level+1, low, mid-1 );

		/* If the higher is the highest in the alphabet then there is no sense
		 * testing it. */
		if ( !limitHigh ) {

			out <<
				targ << ":\n";

			if ( ! keyOps->eq( data[mid].lowKey, data[mid].highKey ) ) {
				out <<
					"	cmpb	" << KEY ( data[mid].highKey ) << ", %r14b\n";
			}

			out <<
				"	jg	" << nf << "\n";

			TRANS_GOTO( data[mid].value );
		}
	}
	else if ( !anyLower && anyHigher ) {
		string targ;
		if ( limitHigh ) {
			targ = TRANS_GOTO_TARG( data[mid].value );
		}
		else {
			std::stringstream s;
			s << ".L" << mn << "_nl" << nl++;
			targ = s.str();
		}

		/* Can go higher than mid but not lower. */

		out <<
			"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
			"	jle	" << targ << "\n";

		emitRangeBSearch( state, level+1, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( !limitLow ) {

			out <<
				targ << ":\n";

			if ( !keyOps->eq( data[mid].lowKey, data[mid].highKey ) ) {
				out <<
					"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n";
			}

			out <<
				"	jl	" << nf << "\n";

			TRANS_GOTO( data[mid].value );
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {

			if ( !keyOps->eq( data[mid].lowKey, data[mid].highKey ) ) {
				out <<
					"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
					"	jl	" << nf << "\n"
					"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
					"	jg	" << nf << "\n";
			}
			else {
				out <<
					"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
					"	jne	" << nf << "\n";
			}

			TRANS_GOTO( data[mid].value );
		}
		else if ( limitLow && !limitHigh ) {

			out <<
				"	cmpb	" << KEY( data[mid].highKey ) << ", %r14b\n"
				"	jg	" << nf << "\n";

			TRANS_GOTO( data[mid].value );
		}
		else if ( !limitLow && limitHigh ) {

			out <<
				"	cmpb	" << KEY( data[mid].lowKey ) << ", %r14b\n"
				"	jl	" << nf << "\n";

			TRANS_GOTO( data[mid].value );
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			TRANS_GOTO( data[mid].value );
		}
	}
}

std::ostream &AsmCodeGen::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );

			/* Load *p. */
			if ( st->outSingle.length() > 0 || st->outRange.length() > 0 )
				out << "	movb	(%r12), %r14b\n";

			/* Try singles. */
			if ( st->outSingle.length() > 0 ) {
				if ( st->outSingle.length() <= 4 )
					emitSingleIfElseIf( st );
				else {
					string def;
					if ( st->outRange.length() == 0 )
						def = TRANS_GOTO_TARG( st->defTrans );
					emitSingleJumpTable( st, def );
				}
			}

			/* Default case is to binary search for the ranges, if that fails then */
			if ( st->outRange.length() > 0 )
				emitRangeBSearch( st, 1, 0, st->outRange.length() - 1 );

			/* Write the default transition. */
			out << ".L" << mn << "_nf_" << st->id << ":\n";
			TRANS_GOTO( st->defTrans );
		}
	}
	return out;
}

unsigned int AsmCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

unsigned int AsmCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

unsigned int AsmCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}

bool AsmCodeGen::useAgainLabel()
{
	return redFsm->anyRegActionRets() || 
			redFsm->anyRegActionByValControl() || 
			redFsm->anyRegNextStmt();
}

void AsmCodeGen::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << CTRL_FLOW() << "goto st" << gotoDest << ";}";
}

void AsmCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << targState << 
			"; " << CTRL_FLOW() << "goto st" << callDest << ";}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void AsmCodeGen::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << targState << "; " << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void AsmCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "goto _again;}";
}

void AsmCodeGen::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void AsmCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void AsmCodeGen::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void AsmCodeGen::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void AsmCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << targState;
}

void AsmCodeGen::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << CTRL_FLOW() << "goto _out;}";
}

bool AsmCodeGen::IN_TRANS_ACTIONS( RedStateAp *state )
{
	bool anyWritten = false;
	static int skip = 1;

	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInCondTests; it++ ) {
		/* Write the label for the transition so it can be jumped to. */
		RedTransAp *trans = state->inCondTests[it];
		out << ".L" << mn << "_ctr_" << trans->id << ":\n";

		/* Using EBX. This is callee-save. */
		out << "	movl	$0, %ebx\n";

		for ( GenCondSet::Iter csi = trans->condSpace->condSet; csi.lte(); csi++ ) {
			int l = skip++;
			Size condValOffset = (1 << csi.pos());
			CONDITION( out, *csi );
			out << 
				"\n"
				"	cmpl	$0, %eax\n"
				"	je		.L" << mn << "_skip_" << l << "\n"
				"	movl	$" << condValOffset << ", %eax\n"
				"	addl	%eax, %ebx\n"
				".L" << mn << "_skip_" << l << ":\n";
		}

		for ( int c = 0; c < trans->numConds(); c++ ) {
			CondKey key = trans->outCondKey( c );
			RedCondPair *pair = trans->outCond( c );
			out <<
				"	cmpl	" << COND_KEY( key ) << ", %ebx\n"
				"	je	" << TRANS_GOTO_TARG( pair ) << "\n";

		}

		RedCondPair *err = trans->errCond();
		if ( err != 0 ) {
			out <<
				"	jmp	" << TRANS_GOTO_TARG( err ) << "\n";
		}
	}

	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInConds; it++ ) {
		RedCondPair *pair = state->inConds[it];
		if ( pair->action != 0 /* && pair->labelNeeded */ ) {
			/* Remember that we wrote an action so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;

			/* Write the label for the transition so it can be jumped to. */
			out << ".L" << mn << "_tr_" << pair->id << ":\n";

//			/* If the action contains a next, then we must preload the current
//			 * state since the action may or may not set it. */
//			if ( pair->action->anyNextStmt() )
//				out << "	" << vCS() << " = " << pair->targ->id << ";\n";

			/* Write each action in the list. */
			for ( GenActionTable::Iter item = pair->action->key; item.lte(); item++ ) {
				ACTION( out, item->value, pair->targ->id, false, 
						pair->action->anyNextStmt() );
				out << "\n";
			}


			/* If the action contains a next then we need to reload, otherwise
			 * jump directly to the target state. */
			if ( pair->action->anyNextStmt() )
				out << "	jmp .L" << mn << "_again\n";
			else
				out << "	jmp .L" << mn << "_st_" << pair->targ->id << "\n";
		}
	}

	return anyWritten;
}

void AsmCodeGen::GOTO_HEADER( RedStateAp *state )
{
	IN_TRANS_ACTIONS( state );

	if ( state->labelNeeded ) 
		out << ".L" << mn << "_st_" << state->id << ":\n";

	if ( state->toStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		for ( GenActionTable::Iter item = state->toStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false, 
					state->toStateAction->anyNextStmt() );
			out << "\n";
		}
	}

	/* Advance and test buffer pos. */
	if ( state->labelNeeded ) {
		out <<
			"	addq	$1, %r12\n";

		if ( !noEnd ) {
			out <<
				"	cmpq	%r12, %r13\n"
				"	je	.L" << mn << "_test_eof_" << state->id << "\n";
		}
	}

	/* This is the entry label for starting a run. */
	out << ".L" << mn << "_en_" << state->id << ":\n";

	if ( state->fromStateAction != 0 ) {
		/* Remember that we wrote an action. Write every action in the list. */
		for ( GenActionTable::Iter item = state->fromStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, state->id, false,
					state->fromStateAction->anyNextStmt() );
			out << "\n";
		}
	}

	/* Record the prev state if necessary. */
	if ( state->anyRegCurStateRef() )
		out << "	_ps = " << state->id << ";\n";
}

void AsmCodeGen::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedStateAp *state = redFsm->errState;
	IN_TRANS_ACTIONS( state );

	out << ".L" << mn << "_en_" << state->id << ":\n";
	if ( state->labelNeeded ) 
		out << ".L" << mn << "_st_" << state->id << ":\n";

	/* Break out here. */
	outLabelUsed = true;
	out << "	movl	$" << state->id << ", cs(%rip)\n";
	out << "	jmp .L" << mn << "_out\n";
}

std::string AsmCodeGen::TRANS_GOTO_TARG( RedCondPair *pair )
{
	std::stringstream s;
	if ( pair->action != 0 ) {
		/* Go to the transition which will go to the state. */
		// out << TABS(level) << "goto tr" << trans->id << ";";

		s << ".L" << mn << "_tr_" << pair->id;
	}
	else {
		/* Go directly to the target state. */
		//out << TABS(level) << "goto st" << trans->targ->id << ";";

		s << ".L" << mn << "_st_" << pair->targ->id;
	}
	return s.str();
}

std::string AsmCodeGen::TRANS_GOTO_TARG( RedTransAp *trans )
{
	if ( trans->condSpace != 0 ) {
		std::stringstream s;
		/* Need to jump to the trans since there are conditions. */
		s << ".L" << mn << "_ctr_" << trans->id;
		return s.str();
	}
	else {
		return TRANS_GOTO_TARG( &trans->p );
	}
}

/* Emit the goto to take for a given transition. */
std::ostream &AsmCodeGen::TRANS_GOTO( RedTransAp *trans )
{
	out << "	jmp	" << TRANS_GOTO_TARG( trans ) << "\n";
	return out;
}

std::ostream &AsmCodeGen::EXIT_STATES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->outNeeded ) {
			testEofUsed = true;

			// out << "_test_eof" << st->id << ": " << vCS() << " = " << 
			//		st->id << "; goto _test_eof; \n";

			out << 
				".L" << mn << "_test_eof_" << st->id << ":\n"
				"	movl	$" << st->id << ", cs(%rip)\n"
				"	jmp .L" << mn << "_test_eof\n";
		}
	}
	return out;
}

std::ostream &AsmCodeGen::AGAIN_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << 
			"		case " << st->id << ": goto st" << st->id << ";\n";
	}
	return out;
}

std::ostream &AsmCodeGen::FINISH_CASES()
{
	bool anyWritten = false;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 ) {
			if ( st->eofAction->eofRefs == 0 )
				st->eofAction->eofRefs = new IntSet;
			st->eofAction->eofRefs->insert( st->id );
		}
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 )
			out << "	case " << st->id << ": goto tr" << st->eofTrans->id << ";\n";
	}

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		if ( act->eofRefs != 0 ) {
			for ( IntSet::Iter pst = *act->eofRefs; pst.lte(); pst++ )
				out << "	case " << *pst << ": \n";

			/* Remember that we wrote a trans so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;

			/* Write each action in the eof action list. */
			for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
				ACTION( out, item->value, STATE_ERR_STATE, true, false );
			out << "\tbreak;\n";
		}
	}

	if ( anyWritten )
		genLineDirective( out );
	return out;
}

void AsmCodeGen::setLabelsNeeded( GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Goto: case GenInlineItem::Call: {
			/* Mark the target as needing a label. */
			item->targState->labelNeeded = true;
			break;
		}
		default: break;
		}

		if ( item->children != 0 )
			setLabelsNeeded( item->children );
	}
}

void AsmCodeGen::setLabelsNeeded( RedCondPair *pair )
{
	/* If there is no action with a next statement, then the label will be
	 * needed. */
	if ( pair->action == 0 || !pair->action->anyNextStmt() )
		pair->targ->labelNeeded = true;

	/* Need labels for states that have goto or calls in action code
	 * invoked on characters (ie, not from out action code). */
	if ( pair->action != 0 ) {
		/* Loop the actions. */
		for ( GenActionTable::Iter act = pair->action->key; act.lte(); act++ ) {
			/* Get the action and walk it's tree. */
			setLabelsNeeded( act->value->inlineList );
		}
	}
}

/* Set up labelNeeded flag for each state. */
void AsmCodeGen::setLabelsNeeded()
{
	/* If we use the _again label, then we the _again switch, which uses all
	 * labels. */
	if ( useAgainLabel() ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = true;
	}
	else {
		/* Do not use all labels by default, init all labelNeeded vars to false. */
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = false;

		for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
			if ( trans->condSpace == 0 )
				setLabelsNeeded( &trans->p );
		}

		for ( CondApSet::Iter cond = redFsm->condSet; cond.lte(); cond++ )
			setLabelsNeeded( &cond->p );
	}

	if ( !noEnd ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st != redFsm->errState )
				st->outNeeded = st->labelNeeded;
		}
	}
}

void AsmCodeGen::writeData()
{
	STATE_IDS();
}

void AsmCodeGen::writeExec()
{
	out << "# WRITE EXEC BEGIN\n";

	/* Must set labels immediately before writing because we may depend on the
	 * noend write option. */
	setLabelsNeeded();
	testEofUsed = false;
	outLabelUsed = false;

	/* Data arrives in %rdi, length in %esi */

	/* p  : %r12 */
	/* pe : %r13 */
	/* pc : %r14b */

	out << 
		"	push	%r12\n"
		"	push	%r13\n"
		"	push	%r14\n"
		"	movq	%rdi, %r12\n"
		"	movslq	%esi, %r13\n"
		"	addq	%rdi, %r13\n"
	;

#if 0
	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps = 0;\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto _test_eof;\n";
	}

	if ( useAgainLabel() ) {
		out << 
			"	goto _resume;\n"
			"\n"
			"_again:\n"
			"	switch ( " << vCS() << " ) {\n";
			AGAIN_CASES() <<
			"	default: break;\n"
			"	}\n"
			"\n";

		if ( !noEnd ) {
			testEofUsed = true;
			out << 
				"	if ( ++" << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";
		}
		else {
			out << 
				"	" << P() << " += 1;\n";
		}

		out << "_resume:\n";
	}

#endif

	/* Jump into the machine based on the curren state. */
	out <<
		"	movq	cs(%rip), %rax\n"
		"	jmp		*.L" << mn << "_entry_jmp(,%rax,8)\n"
		"	.section .rodata\n"
		"	.align 8\n"
		".L" << mn << "_entry_jmp:\n";

	for ( int stId = 0; stId < redFsm->stateList.length(); stId++ ) {
		out <<
			"	.quad	.L" << mn << "_en_" << stId << "\n";
	}

	out <<
		"	.text\n";

	STATE_GOTOS();
	EXIT_STATES() << "\n";

	if ( testEofUsed ) 
		out << ".L" << mn << "_test_eof:\n";

#if 0
	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n"
			"	switch ( " << vCS() << " ) {\n";
			FINISH_CASES();
			SWITCH_DEFAULT() <<
			"	}\n"
			"	}\n"
			"\n";
	}

#endif

	if ( outLabelUsed ) 
		out << ".L" << mn << "_out:\n";

	out << 
		"	pop	%r14\n"
		"	pop	%r13\n"
		"	pop	%r12\n"
	;

	out << "# WRITE EXEC END\n";
}
