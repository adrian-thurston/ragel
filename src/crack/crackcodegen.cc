/*
 *  2007 Victor Hugo Borja <vic@rubyforge.org>
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
 *  Crack version  (C) 11/11/11 Conrad Steenberg <conrad.steenberg@gmail.com>
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

#include <iomanip>
#include <sstream>
#include <iostream>
#include "redfsm.h"
#include "gendata.h"
#include "ragel.h"
#include "crackcodegen.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "common.h"

#include "ragel.h"
#include "crackflat.h"

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

/* Target crack impl */

/* Target language and output style. */
extern CodeStyle codeStyle;

extern int numSplitPartitions;
extern bool noLineDirectives;

/*
 * Callbacks invoked by the XML data parser.
 */


void crackLineDirective( ostream &out, const char *fileName, int line )
{
  if ( noLineDirectives )
    return;

  /* Write a comment containing line info. */
  out << "# line " << line  << " \"";
  for ( const char *pc = fileName; *pc != 0; pc++ ) {
    if ( *pc == '\\' )
      out << "\\\\";
    else
      out << *pc;
  }
  out << "\" # end of line directive\n";
}

void CrackCodeGen::genLineDirective( ostream &out )
{
  std::streambuf *sbuf = out.rdbuf();
  if (!sbuf) std::cerr << "sbuf is NULL" << endl;
  output_filter *filter = static_cast<output_filter*>(sbuf);
  if (!filter) std::cerr << "filter is NULL" << endl;
  crackLineDirective( out, filter->fileName, filter->line + 1 );
}

string CrackCodeGen::DATA_PREFIX()
{
  if ( !noPrefix )
    return FSM_NAME() + "_";
  return "";
}

std::ostream &CrackCodeGen::STATIC_VAR( string type, string name )
{
  out << type << " " << name;
  return out;
}


std::ostream &CrackCodeGen::OPEN_ARRAY( string type, string name )
{
  out << "Array[uint] " << name << " = [\n";
  return out;
}

std::ostream &CrackCodeGen::CLOSE_ARRAY()
{
  out << "];\n";
  return out;
}


string CrackCodeGen::ARR_OFF( string ptr, string offset )
{
  return ptr + "[" + offset + "]";
}

string CrackCodeGen::NULL_ITEM()
{
  return "0";
}


string CrackCodeGen::P()
{ 
  ostringstream ret;
  if ( pExpr == 0 )
    ret << "p";
  else {
    //ret << "(";
    INLINE_LIST( ret, pExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::PE()
{
  ostringstream ret;
  if ( peExpr == 0 )
    ret << "pe";
  else {
    //ret << "(";
    INLINE_LIST( ret, peExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::vEOF()
{
  ostringstream ret;
  if ( eofExpr == 0 )
    ret << "eof";
  else {
    //ret << "(";
    INLINE_LIST( ret, eofExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::vCS()
{
  ostringstream ret;
  if ( csExpr == 0 )
    ret << ACCESS() << "cs";
  else {
    //ret << "(";
    INLINE_LIST( ret, csExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::TOP()
{
  ostringstream ret;
  if ( topExpr == 0 )
    ret << ACCESS() + "top";
  else {
    //ret << "(";
    INLINE_LIST( ret, topExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::STACK()
{
  ostringstream ret;
  if ( stackExpr == 0 )
    ret << ACCESS() + "stack";
  else {
    //ret << "(";
    INLINE_LIST( ret, stackExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::ACT()
{
  ostringstream ret;
  if ( actExpr == 0 )
    ret << ACCESS() + "act";
  else {
    //ret << "(";
    INLINE_LIST( ret, actExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::TOKSTART()
{
  ostringstream ret;
  if ( tokstartExpr == 0 )
    ret << ACCESS() + "ts";
  else {
    //ret << "(";
    INLINE_LIST( ret, tokstartExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::TOKEND()
{
  ostringstream ret;
  if ( tokendExpr == 0 )
    ret << ACCESS() + "te";
  else {
    //ret << "(";
    INLINE_LIST( ret, tokendExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

string CrackCodeGen::DATA()
{
  ostringstream ret;
  if ( dataExpr == 0 )
    ret << ACCESS() + "data";
  else {
    //ret << "(";
    INLINE_LIST( ret, dataExpr, 0, false );
    //ret << ")";
  }
  return ret.str();
}

/* Write out the fsm name. */
string CrackCodeGen::FSM_NAME()
{
  return fsmName;
}


void CrackCodeGen::ACTION( ostream &ret, GenAction *action, int targState, bool inFinish )
{
  /* Write the preprocessor line info for going into the source file. */
  crackLineDirective( ret, action->loc.fileName, action->loc.line );

  /* Write the block and close it off. */
  ret << "    ";
  INLINE_LIST( ret, action->inlineList, targState, inFinish );
  ret << "    // ACTION\n";
}



string CrackCodeGen::GET_WIDE_KEY()
{
  if ( redFsm->anyConditions() ) 
    return "_widec";
  else
    return GET_KEY();
}

string CrackCodeGen::GET_WIDE_KEY( RedStateAp *state )
{
  if ( state->stateCondList.length() > 0 )
    return "_widec";
  else
    return GET_KEY();
}

string CrackCodeGen::GET_KEY()
{
  ostringstream ret;
  if ( getKeyExpr != 0 ) { 
    /* Emit the user supplied method of retrieving the key. */
    ret << "(";
    INLINE_LIST( ret, getKeyExpr, 0, false );
    ret << ")";
  }
  else {
    /* Expression for retrieving the key, use dereference and read ordinal */
    ret << DATA() << "[" << P() << "]";
  }
  return ret.str();
}

string CrackCodeGen::KEY( Key key )
{
  ostringstream ret;
  if ( keyOps->isSigned || !hostLang->explicitUnsigned )
    ret << key.getVal();
  else
    ret << (unsigned long) key.getVal();
  return ret.str();
}


/* Write out level number of tabs. Makes the nested binary search nice
 * looking. */
string CrackCodeGen::TABS( int level )
{
  string result;
  while ( level-- > 0 )
    result += "  ";
  return result;
}

string CrackCodeGen::INT( int i )
{
  ostringstream ret;
  ret << i;
  return ret.str();
}

void CrackCodeGen::CONDITION( ostream &ret, GenAction *condition )
{
  ret << "// CONDITION\n";
  crackLineDirective( ret, condition->loc.fileName, condition->loc.line );
  INLINE_LIST( ret, condition->inlineList, 0, false );
}

/* Emit the alphabet data type. */
string CrackCodeGen::ALPH_TYPE()
{
  string ret = keyOps->alphType->data1;
  if ( keyOps->alphType->data2 != 0 ) {
    ret += " ";
    ret += + keyOps->alphType->data2;
  }
  return ret;
}

/* Emit the alphabet data type. */
string CrackCodeGen::WIDE_ALPH_TYPE()
{
  string ret;
  if ( redFsm->maxKey <= keyOps->maxKey )
    ret = ALPH_TYPE();
  else {
    long long maxKeyVal = redFsm->maxKey.getLongLong();
    HostType *wideType = keyOps->typeSubsumes( keyOps->isSigned, maxKeyVal );
    assert( wideType != 0 );

    ret = wideType->data1;
    if ( wideType->data2 != 0 ) {
      ret += " ";
      ret += wideType->data2;
    }
  }
  return ret;
}


string CrackCodeGen::ARRAY_TYPE( unsigned long maxVal )
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

/* Write out the array of actions. */
std::ostream &CrackCodeGen::ACTIONS_ARRAY()
{
  START_ARRAY_LINE();
  int totalActions = 0;
  ARRAY_ITEM( INT(0), ++totalActions, false );
  for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
    /* Write out the length, which will never be the last character. */
    ARRAY_ITEM( INT(act->key.length()), ++totalActions, false );

    for ( GenActionTable::Iter item = act->key; item.lte(); item++ ) {
      ARRAY_ITEM( INT(item->value->actionId), ++totalActions, (act.last() && item.last()) );
    }
  }
  END_ARRAY_LINE();
  return out;
}

void CrackCodeGen::STATE_IDS()
{
  if ( redFsm->startState != 0 )
    STATIC_VAR( "uint", START() ) << " = " << START_STATE_ID() << ";\n";

  if ( !noFinal )
    STATIC_VAR( "uint" , FIRST_FINAL() ) << " = " << FIRST_FINAL_STATE() << ";\n";

  if ( !noError )
    STATIC_VAR( "uint", ERROR() ) << " = " << ERROR_STATE() << ";\n";

  out << "\n";

  if ( entryPointNames.length() > 0 ) {
    for ( EntryNameVect::Iter en = entryPointNames; en.lte(); en++ ) {
      STATIC_VAR( "uint ", DATA_PREFIX() + "en_" + *en ) << 
          " = " << entryPointIds[en.pos()] << ";\n";
    }
    out << "\n";
  }
}

std::ostream &CrackCodeGen::START_ARRAY_LINE()
{
  out << "  ";
  return out;
}

std::ostream &CrackCodeGen::ARRAY_ITEM( string item, int count, bool last )
{
  out << item;
  if ( !last )
  {
    out << ", ";
    if ( count % IALL == 0 )
    {
      END_ARRAY_LINE();
      START_ARRAY_LINE();
    }
  }
  return out;
}

std::ostream &CrackCodeGen::END_ARRAY_LINE()
{
  out << "\n";
  return out;
}

/* Emit the offset of the start state as a decimal integer. */
string CrackCodeGen::START_STATE_ID()
{
  ostringstream ret;
  ret << redFsm->startState->id;
  return ret.str();
};

string CrackCodeGen::ERROR_STATE()
{
  ostringstream ret;
  if ( redFsm->errState != 0 )
    ret << redFsm->errState->id;
  else
    ret << "0xffffffff";
  return ret.str();
}

string CrackCodeGen::FIRST_FINAL_STATE()
{
  ostringstream ret;
  if ( redFsm->firstFinState != 0 )
    ret << redFsm->firstFinState->id;
  else
    ret << redFsm->nextStateId;
  return ret.str();
}

string CrackCodeGen::ACCESS()
{
  ostringstream ret;
  if ( accessExpr != 0 )
    INLINE_LIST( ret, accessExpr, 0, false );
  return ret.str();
}

/* Write out an inline tree structure. Walks the list and possibly calls out
 * to virtual functions than handle language specific items in the tree. */
void CrackCodeGen::INLINE_LIST( ostream &ret, GenInlineList *inlineList, 
    int targState, bool inFinish )
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
      ret << P() << " = " << P() << " - 1;";
      break;
    case GenInlineItem::Exec:
      EXEC( ret, item, targState, inFinish );
      break;
    case GenInlineItem::Curs:
      ret << "(_ps)";
      break;
    case GenInlineItem::Targs:
      ret << "(" << vCS() << ")";
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
      LM_SWITCH( ret, item, targState, inFinish );
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
    case GenInlineItem::SubAction:
      SUB_ACTION( ret, item, targState, inFinish );
      break;
    case GenInlineItem::Break:
      BREAK( ret, targState );
      break;
    }
  }
}


void CrackCodeGen::EXEC( ostream &ret, GenInlineItem *item, int targState, int inFinish )
{
  /* The parser gives fexec two children. The double brackets are for D
   * code. If the inline list is a single word it will get interpreted as a
   * C-style cast by the D compiler. */
  ret <<  P() << " = (";
  INLINE_LIST( ret, item->children, targState, inFinish );
  ret << ") - 1; //EXEC\n";
}

void CrackCodeGen::LM_SWITCH( ostream &ret, GenInlineItem *item, 
    int targState, int inFinish )
{
  int i = 0;

  for ( GenInlineList::Iter lma = *item->children; lma.lte(); lma++, i++ ) {
    /* Write the case label, the action and the case break. */
    if ( lma->lmId < 0 )
      ret << "  else ";
    else {

      if (i == 0)
        ret << " if (" ;
      else if (i > 0)
        ret << " else if (" ;

      ret << ACT()  << " ==  " << lma->lmId << " )";
    }
    
    /* Write the block and close it off. */
    ret << "  {";
    INLINE_LIST( ret, lma->children, targState, inFinish );
    ret << "  }\n";
  }

  ret << " // end LM_SWITCH\n";
}

void CrackCodeGen::SET_ACT( ostream &ret, GenInlineItem *item )
{
  ret << ACT() << " = " << item->lmId << "; // SET_ACT";
}

void CrackCodeGen::INIT_TOKSTART( ostream &ret, GenInlineItem *item )
{
  ret << TOKSTART() << " = " << NULL_ITEM() << ";  //INIT_TOKSTART";
}

void CrackCodeGen::INIT_ACT( ostream &ret, GenInlineItem *item )
{
  ret << ACT() << " = 0;\n";
}

void CrackCodeGen::SET_TOKSTART( ostream &ret, GenInlineItem *item )
{
  ret << TOKSTART() << " = " << P() << ";\n";
}

void CrackCodeGen::SET_TOKEND( ostream &ret, GenInlineItem *item )
{
  /* The tokend action sets tokend. */
  ret << TOKEND() << " = " << P();
  if ( item->offset != 0 ) 
    out << "+" << item->offset;
  out << ";\n";
}

void CrackCodeGen::GET_TOKEND( ostream &ret, GenInlineItem *item )
{
  ret << TOKEND();
}

void CrackCodeGen::SUB_ACTION( ostream &ret, GenInlineItem *item, 
    int targState, bool inFinish )
{
  if ( item->children->length() > 0 ) {
    /* Write the block and close it off. */
    ret << "    ";
    INLINE_LIST( ret, item->children, targState, inFinish );
    ret << "    // SUB_ACTION\n";
  }
}

int CrackCodeGen::TRANS_ACTION( RedTransAp *trans )
{
  /* If there are actions, emit them. Otherwise emit zero. */
  int act = 0;
  if ( trans->action != 0 )
    act = trans->action->location+1;
  return act;
}

ostream &CrackCodeGen::source_warning( const InputLoc &loc )
{
  cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
  return cerr;
}

ostream &CrackCodeGen::source_error( const InputLoc &loc )
{
  gblErrorCount += 1;
  assert( sourceFileName != 0 );
  cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
  return cerr;
}

void CrackCodeGen::finishRagelDef()
{
  if ( codeStyle == GenGoto || codeStyle == GenFGoto || 
      codeStyle == GenIpGoto || codeStyle == GenSplit )
  {
    /* For directly executable machines there is no required state
     * ordering. Choose a depth-first ordering to increase the
     * potential for fall-throughs. */
    redFsm->depthFirstOrdering();
  }
  else {
    /* The frontend will do this for us, but it may be a good idea to
     * force it if the intermediate file is edited. */
    redFsm->sortByStateId();
  }

  /* Choose default transitions and the single transition. */
  redFsm->chooseDefaultSpan();
    
  /* Maybe do flat expand, otherwise choose single. */
  if ( codeStyle == GenFlat || codeStyle == GenFFlat )
    redFsm->makeFlat();
  else
    redFsm->chooseSingle();

  /* If any errors have occured in the input file then don't write anything. */
  if ( gblErrorCount > 0 )
    return;
  
  if ( codeStyle == GenSplit )
    redFsm->partitionFsm( numSplitPartitions );

  if ( codeStyle == GenIpGoto || codeStyle == GenSplit )
    redFsm->setInTrans();

  /* Anlayze Machine will find the final action reference counts, among
   * other things. We will use these in reporting the usage
   * of fsm directives in action code. */
  analyzeMachine();

  /* Determine if we should use indicies. */
  calcIndexSize();
}


/* Determine if we should use indicies or not. */
void CrackCodeGen::calcIndexSize()
{
  int sizeWithInds = 0, sizeWithoutInds = 0;

  /* Calculate cost of using with indicies. */
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    int totalIndex = st->outSingle.length() + st->outRange.length() + 
        (st->defTrans == 0 ? 0 : 1);
    sizeWithInds += arrayTypeSize(redFsm->maxIndex) * totalIndex;
  }
  sizeWithInds += arrayTypeSize(redFsm->maxState) * redFsm->transSet.length();
  if ( redFsm->anyActions() )
    sizeWithInds += arrayTypeSize(redFsm->maxActionLoc) * redFsm->transSet.length();

  /* Calculate the cost of not using indicies. */
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    int totalIndex = st->outSingle.length() + st->outRange.length() + 
        (st->defTrans == 0 ? 0 : 1);
    sizeWithoutInds += arrayTypeSize(redFsm->maxState) * totalIndex;
    if ( redFsm->anyActions() )
      sizeWithoutInds += arrayTypeSize(redFsm->maxActionLoc) * totalIndex;
  }

  /* If using indicies reduces the size, use them. */
  useIndicies = sizeWithInds < sizeWithoutInds;
}

unsigned int CrackCodeGen::arrayTypeSize( unsigned long maxVal )
{
  long long maxValLL = (long long) maxVal;
  HostType *arrayType = keyOps->typeSubsumes( maxValLL );
  assert( arrayType != 0 );
  return arrayType->size;
}

void CrackCodeGen::writeInit()
{

  if ( !noCS )
    out << "  " << vCS() << " = " << START() << ";\n";

  /* If there are any calls, then the stack top needs initialization. */
  if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
    out << "writeInit::anyActionCalls" "  " << TOP() << " = 0;\n";

  if ( hasLongestMatch ) {
    out <<
      "  " << TOKSTART() << " = " << NULL_ITEM() << ";\n"
      "  " << TOKEND() << " = " << NULL_ITEM() << ";\n"
      "  " << ACT() << " = 0;\n";
  }

}

void CrackCodeGen::writeExports()
{
  if ( exportList.length() > 0 ) {
    for ( ExportList::Iter ex = exportList; ex.lte(); ex++ ) {
      STATIC_VAR( ALPH_TYPE(), DATA_PREFIX() + "ex_" + ex->name ) 
          << " = " << KEY(ex->key) << ";\n";
    }
    out << "\n";
  }
}

void CrackCodeGen::writeStart()
{
  out << START_STATE_ID();
}

void CrackCodeGen::writeFirstFinal()
{
  out << FIRST_FINAL_STATE();
}

void CrackCodeGen::writeError()
{
  out << ERROR_STATE();
}


/*
 * Local Variables:
 * mode: c++
 * indent-tabs-mode: 1
 * c-file-style: "bsd"
 * End:
 */
