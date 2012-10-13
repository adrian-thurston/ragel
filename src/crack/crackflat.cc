/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
 *  Copyright 2007 Victor Hugo Borja <vic@rubyforge.org>
 *  Crack version (C) 11/11/11 Conrad Steenberg <conrad.steenberg@gmail.com>
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

#include "crackflat.h"
#include "ragel.h"
#include "redfsm.h"
#include "gendata.h"

using std::ostream;
using std::string;

namespace Crack {

std::ostream &CrackFlatCodeGen::TO_STATE_ACTION_SWITCH(const char *var)
{
  /* Walk the list of functions, printing the cases. */
  int i = 0;
  for ( GenActionList::Iter act = actionList; act.lte(); act++) {
    /* Write out referenced actions. */
    if ( act->numToStateRefs > 0 ) {
      /* Write the case label, the action and the case break */
      if (i > 0) out << "    else"; else out << "  ";
      out << "  if (" << var <<"  == " << act->actionId << ") { // TO_STATE_ACTION_SWITCH\n";
      ACTION( out, act, 0, false );
      out << "    }\n";
      i++;
    }
  }

  genLineDirective( out );
  return out;
}

std::ostream &CrackFlatCodeGen::FROM_STATE_ACTION_SWITCH(const char *var)
{
  /* Walk the list of functions, printing the cases. */
  int i = 0;
  for ( GenActionList::Iter act = actionList; act.lte(); act++) {
    /* Write out referenced actions. */
    if ( act->numFromStateRefs > 0 ) {
      /* Write the case label, the action and the case break */

      if (i > 0) out << "  else "; else out << "  ";
      out << "  if (" << var <<"  == " << act->actionId << ") { // FROM_STATE_ACTION_SWITCH\n";
      ACTION( out, act, 0, false );
      out << "    }\n";
      i++;
    }
  }

  genLineDirective( out );
  return out;
}

std::ostream &CrackFlatCodeGen::EOF_ACTION_SWITCH(const char *var)
{
  /* Walk the list of functions, printing the cases. */
  int i = 0;
  for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
    /* Write out referenced actions. */
    if ( act->numEofRefs > 0 ) {
      /* Write the case label, the action and the case break */
      if (i > 0) out << "    else"; else out << "  ";
      out << "  if (" << var <<"  == " << act->actionId << ") { // FROM_STATE_ACTION_SWITCH\n";
      ACTION( out, act, 0, true );
      out << "    }\n";
      i++;
    }
  }

  genLineDirective( out );
  return out;
}

std::ostream &CrackFlatCodeGen::ACTION_SWITCH(const char *var)
{
  /* Walk the list of functions, printing the cases. */
  int i = 0;
  for ( GenActionList::Iter act = actionList; act.lte(); act++) {
    /* Write out referenced actions. */
    if ( act->numTransRefs > 0 ) {
      /* Write the case label, the action and the case break */
      
      if (i > 0) out << "    else"; else out << "  ";
      out << "  if (" << var <<"  == " << act->actionId << ") { // FROM_STATE_ACTION_SWITCH\n";
      ACTION( out, act, 0, false );
      out << "    }\n";
      i++;
    }
  }

  genLineDirective( out );
  return out;
}


std::ostream &CrackFlatCodeGen::KEYS()
{
  START_ARRAY_LINE();
  int totalTrans = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Emit just low key and high key. */
    ARRAY_ITEM( KEY( st->lowKey ), ++totalTrans, false );
    ARRAY_ITEM( KEY( st->highKey ), ++totalTrans, false );
    if ( ++totalTrans % IALL == 0 )
      out << "\n" << TABS(1);

  }

  /* Output one last number so we don't have to figure out when the last
   * entry is and avoid writing a comma. */
  ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::INDICIES()
{
  int totalTrans = 0;
  START_ARRAY_LINE();
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    if ( st->transList != 0 ) {
      /* Walk the singles. */
      unsigned long long span = keyOps->span( st->lowKey, st->highKey );
      for ( unsigned long long pos = 0; pos < span; pos++ ) {
        ARRAY_ITEM( KEY( st->transList[pos]->id ), ++totalTrans, false );
      }
    }

    /* The state's default index goes next. */
    if ( st->defTrans != 0 )
      ARRAY_ITEM( KEY( st->defTrans->id ), ++totalTrans, false );
  }

  /* Output one last number so we don't have to figure out when the last
   * entry is and avoid writing a comma. */
  ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::FLAT_INDEX_OFFSET()
{
  START_ARRAY_LINE();
  int totalStateNum = 0, curIndOffset = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write the index offset. */
    ARRAY_ITEM( INT( curIndOffset ), ++totalStateNum, st.last() );
    /* Move the index offset ahead. */
    if ( st->transList != 0 )
      curIndOffset += keyOps->span( st->lowKey, st->highKey );

    if ( st->defTrans != 0 )
      curIndOffset += 1;
  }

  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::KEY_SPANS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write singles length. */
    unsigned long long span = 0;
    if ( st->transList != 0 )
      span = keyOps->span( st->lowKey, st->highKey );
    ARRAY_ITEM( INT( span ), ++totalStateNum, st.last() );
  }
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::TO_STATE_ACTIONS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write any eof action. */
    ARRAY_ITEM( INT( TO_STATE_ACTION(st) ), ++totalStateNum, st.last() );
  }
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::FROM_STATE_ACTIONS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write any eof action. */
    ARRAY_ITEM( INT( FROM_STATE_ACTION(st) ), ++totalStateNum, st.last() );
  }
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::EOF_ACTIONS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write any eof action. */
    ARRAY_ITEM( INT( EOF_ACTION(st) ), ++totalStateNum, st.last() );
  }
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::EOF_TRANS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write any eof action. */
    long trans = 0;
    if ( st->eofTrans != 0 ) {
      assert( st->eofTrans->pos >= 0 );
      trans = st->eofTrans->pos+1;
    }

    /* Write any eof action. */
    ARRAY_ITEM( INT(trans), ++totalStateNum, st.last() );
  }
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::TRANS_TARGS()
{
  /* Transitions must be written ordered by their id. */
  RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
  for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
    transPtrs[trans->id] = trans;

  /* Keep a count of the num of items in the array written. */
  START_ARRAY_LINE();

  int totalStates = 0;
  for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
    /* Save the position. Needed for eofTargs. */
    RedTransAp *trans = transPtrs[t];
    trans->pos = t;

    /* Write out the target state. */
    ARRAY_ITEM( INT( trans->targ->id ), ++totalStates, t >= redFsm->transSet.length()-1  );
  }
  END_ARRAY_LINE();
  delete[] transPtrs;
  return out;
}


std::ostream &CrackFlatCodeGen::TRANS_ACTIONS()
{
  /* Transitions must be written ordered by their id. */
  RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
  for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
    transPtrs[trans->id] = trans;

  /* Keep a count of the num of items in the array written. */
  START_ARRAY_LINE();
  int totalAct = 0;
  for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
    /* Write the function for the transition. */
    RedTransAp *trans = transPtrs[t];
    ARRAY_ITEM( INT( TRANS_ACTION( trans ) ), ++totalAct, t >= redFsm->transSet.length()-1 );
  }
  END_ARRAY_LINE();
  delete[] transPtrs;
  return out;
}


void CrackFlatCodeGen::LOCATE_TRANS()
{
  out <<
    "      _keys = " << vCS() << " << 1; // LOCATE_TRANS\n"
    "      _inds = " << IO() << "[" << vCS() << "];\n"
    "      _slen = " << SP() << "[" << vCS() << "];\n\n"
    "      if (   _slen > 0 && \n"
    "         " << K() << "[_keys] <= " << GET_WIDE_KEY() << " && \n"
    "         " << GET_WIDE_KEY() << " <= " << K() << "[_keys + 1]) \n"
    "        _trans = " << I() << "[ _inds + " << GET_WIDE_KEY() << " - " << K() << "[_keys] ]; \n"
    "      else _trans =" << I() << "[ _inds + _slen ];\n\n";
}

std::ostream &CrackFlatCodeGen::COND_INDEX_OFFSET()
{
  START_ARRAY_LINE();
  int totalStateNum = 0, curIndOffset = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write the index offset. */
    ARRAY_ITEM( INT( curIndOffset ), ++totalStateNum, st.last() );
    /* Move the index offset ahead. */
    if ( st->condList != 0 )
      curIndOffset += keyOps->span( st->condLowKey, st->condHighKey );
  }
  END_ARRAY_LINE();
  return out;
}

void CrackFlatCodeGen::COND_TRANSLATE()
{
  out << 
    "    _widec = " << GET_KEY() << "; // COND_TRANSLATE\n"
    "    _keys = " << vCS() << " << 1;\n"
    "    _conds = " << CO() << "[" << vCS() << "];\n"
    "    _slen = " << CSP() << "[" << vCS() << "];\n"
    "    if ( _slen > 0 && \n" 
    "      " << CK() << "[_keys] <= " << GET_WIDE_KEY() << " &&\n" 
    "      " << GET_WIDE_KEY() << " <= " << CK() << "[_keys + 1]\n"
    "     ) _cond = \n"
    "     " << C() << "[ _conds + " << GET_WIDE_KEY() << " - " << CK() << "[_keys]" << " ];\n"
    "     else _cond = 0;\n\n";

  for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
    GenCondSpace *condSpace = csi;
    out << "  if (_cond == " << condSpace->condSpaceId + 1 << ") {\n";
    out << TABS(2) << "_widec = " << "(" <<
        KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
        " - " << KEY(keyOps->minKey) << "))\n";

    for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
      out << TABS(2) << "  if ( ";
      CONDITION( out, *csi );
      Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
      out << 
        "   ) { \n" <<
        TABS(3) << "  _widec += " << condValOffset << "\n"
        "    }\n";
    }
  }

  out <<
    " } # _cond switch \n";
}

std::ostream &CrackFlatCodeGen::CONDS()
{
  int totalTrans = 0;
  START_ARRAY_LINE();
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    if ( st->condList != 0 ) {
      /* Walk the singles. */
      unsigned long long span = keyOps->span( st->condLowKey, st->condHighKey );
      for ( unsigned long long pos = 0; pos < span; pos++ ) {
        if ( st->condList[pos] != 0 )
          ARRAY_ITEM( INT( st->condList[pos]->condSpaceId + 1 ), ++totalTrans, false );
        else
          ARRAY_ITEM( INT( 0 ), ++totalTrans, false );
      }
    }
  }

  /* Output one last number so we don't have to figure out when the last
   * entry is and avoid writing a comma. */
  ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::COND_KEYS()
{
  START_ARRAY_LINE();
  int totalTrans = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Emit just cond low key and cond high key. */
    ARRAY_ITEM( KEY( st->condLowKey ), ++totalTrans, false );
    ARRAY_ITEM( KEY( st->condHighKey ), ++totalTrans, false );
  }

  /* Output one last number so we don't have to figure out when the last
   * entry is and avoid writing a comma. */
  ARRAY_ITEM( INT( 0 ), ++totalTrans, true );
  END_ARRAY_LINE();
  return out;
}

std::ostream &CrackFlatCodeGen::COND_KEY_SPANS()
{
  START_ARRAY_LINE();
  int totalStateNum = 0;
  for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
    /* Write singles length. */
    unsigned long long span = 0;
    if ( st->condList != 0 )
      span = keyOps->span( st->condLowKey, st->condHighKey );
    ARRAY_ITEM( INT( span ), ++totalStateNum, false );
  }
  END_ARRAY_LINE();
  return out;
}


void CrackFlatCodeGen::GOTO( ostream &out, int gotoDest, bool inFinish )
{
  out << 

    "    " << vCS() << " = " << gotoDest << ";// GOTO\n"
    "    _trigger_goto = true;\n"
    "    _goto_level = _again;\n"
    "    break;\n\n";
}

void CrackFlatCodeGen::CALL( ostream &out, int callDest, int targState, bool inFinish )
{
  if ( prePushExpr != 0 ) {
    out << "{\n";
    INLINE_LIST( out, prePushExpr, 0, false );
  }

  out <<
    "    " << STACK() << "[" << TOP() << "] = " << vCS() << "; // CALL\n"
    "    " << TOP() << "+= 1;\n"
    "    " << vCS() << " = " << callDest << ";\n"
    "    _trigger_goto = true;\n"
    "    _goto_level = _again;\n"
    "    break;\n\n";

  if ( prePushExpr != 0 )
    out << "  }\n";
}

void CrackFlatCodeGen::CALL_EXPR(ostream &out, GenInlineItem *ilItem, int targState, bool inFinish )
{
  if ( prePushExpr != 0 ) {
    out << "{ \n";
    INLINE_LIST( out, prePushExpr, 0, false );
  }

  out <<
    "    " << STACK() << "[" << TOP() << "] = " << vCS() << "; // CALL_EXPR\n"
    "    " << TOP() << " += 1;\n"
    "    " << vCS() << " = (";
  INLINE_LIST( out, ilItem->children, targState, inFinish );
  out << ");\n";

  out << 
    "    _trigger_goto = true;\n"
    "    _goto_level = _again;\n"
    "    break;\n\n";

  if ( prePushExpr != 0 )
    out << "}\n";
}

void CrackFlatCodeGen::RET( ostream &out, bool inFinish )
{
  out <<
    "    " << TOP() << " -= 1; // RET\n"
    "    " << vCS() << " = " << STACK() << "[" << TOP() << "];\n";

  if ( postPopExpr != 0 ) {
    out << "{\n";
    INLINE_LIST( out, postPopExpr, 0, false );
    out << "}\n";
  }

  out <<
    "    _trigger_goto = true;\n"
    "    _goto_level = _again;\n"
    "    break;\n";
}

void CrackFlatCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
  ret << vCS() << " = " << nextDest << ";";
}

void CrackFlatCodeGen::GOTO_EXPR( ostream &out, GenInlineItem *ilItem, bool inFinish )
{
  out << 
    "    " << vCS() << " = (";
  INLINE_LIST( out, ilItem->children, 0, inFinish );
  out << ");\n";
  out <<
    "    _trigger_goto = true;\n"
    "    _goto_level = _again;\n"
    "    break;\n\n";
}

void CrackFlatCodeGen::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
  ret << vCS() << " = (";
  INLINE_LIST( ret, ilItem->children, 0, inFinish );
  ret << ");";
}


void CrackFlatCodeGen::CURS( ostream &ret, bool inFinish )
{
  ret << "(_ps)";
}

void CrackFlatCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
  ret << "(" << vCS() << ")";
}

void CrackFlatCodeGen::BREAK( ostream &out, int targState )
{
  out << 
    "    " << P() << " += 1;\n"
    "    _trigger_goto = true;\n"
    "    _goto_level = _out;\n"
    "    break;\n\n";
}

int CrackFlatCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
  int act = 0;
  if ( state->toStateAction != 0 )
    act = state->toStateAction->location+1;
  return act;
}

int CrackFlatCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
  int act = 0;
  if ( state->fromStateAction != 0 )
    act = state->fromStateAction->location+1;
  return act;
}

int CrackFlatCodeGen::EOF_ACTION( RedStateAp *state )
{
  int act = 0;
  if ( state->eofAction != 0 )
    act = state->eofAction->location+1;
  return act;
}

int CrackFlatCodeGen::TRANS_ACTION( RedTransAp *trans )
{
  /* If there are actions, emit them. Otherwise emit zero. */
  int act = 0;
  if ( trans->action != 0 )
    act = trans->action->location+1;
  return act;
}

void CrackFlatCodeGen::writeData()
{
  /* If there are any transtion functions then output the array. If there
   * are none, don't bother emitting an empty array that won't be used. */
  if ( redFsm->anyActions() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActArrItem), A() );
    ACTIONS_ARRAY();
    CLOSE_ARRAY() <<
    "\n";
  }

  if ( redFsm->anyConditions() ) {
    OPEN_ARRAY( WIDE_ALPH_TYPE(), CK() );
    COND_KEYS();
    CLOSE_ARRAY() <<
    "\n";

    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondSpan), CSP() );
    COND_KEY_SPANS();
    CLOSE_ARRAY() <<
    "\n";

    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCond), C() );
    CONDS();
    CLOSE_ARRAY() <<
    "\n";

    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondIndexOffset), CO() );
    COND_INDEX_OFFSET();
    CLOSE_ARRAY() <<
    "\n";
  }

  OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
  KEYS();
  CLOSE_ARRAY() <<
  "\n";

  OPEN_ARRAY( ARRAY_TYPE(redFsm->maxSpan), SP() );
  KEY_SPANS();
  CLOSE_ARRAY() <<
  "\n";

  OPEN_ARRAY( ARRAY_TYPE(redFsm->maxFlatIndexOffset), IO() );
  FLAT_INDEX_OFFSET();
  CLOSE_ARRAY() <<
  "\n";

  OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
  INDICIES();
  CLOSE_ARRAY() <<
  "\n";

  OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TT() );
  TRANS_TARGS();
  CLOSE_ARRAY() <<
  "\n";

  if ( redFsm->anyActions() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TA() );
    TRANS_ACTIONS();
    CLOSE_ARRAY() <<
    "\n";
  }

  if ( redFsm->anyToStateActions() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TSA() );
    TO_STATE_ACTIONS();
    CLOSE_ARRAY() <<
    "\n";
  }

  if ( redFsm->anyFromStateActions() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), FSA() );
    FROM_STATE_ACTIONS();
    CLOSE_ARRAY() <<
    "\n";
  }

  if ( redFsm->anyEofActions() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), EA() );
    EOF_ACTIONS();
    CLOSE_ARRAY() <<
    "\n";
  }

  if ( redFsm->anyEofTrans() ) {
    OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset+1), ET() );
    EOF_TRANS();
    CLOSE_ARRAY() <<
    "\n";
  }
  
  STATE_IDS();

}

void CrackFlatCodeGen::writeExec()
{
  out << 
    "#  ragel flat exec\n\n"
    "  bool testEof = false;\n"
    "  uint _slen = 0;\n"
    "  uint _trans = 0;\n" 
    "  uint _keys = 0;\n"
    "  uint _inds = 0;\n";
  if ( redFsm->anyRegCurStateRef() )
    out << "  uint _ps = 0;\n";
  if ( redFsm->anyConditions() )
    out << "  uint _cond = 0;\n"
           "  uint _conds = 0;\n"
           "  uint _widec = 0;\n";
  if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
      || redFsm->anyFromStateActions() )
    out << "  uint _acts = 0;\n"
           "  uint _nacts = 0;\n";

  out << 
    "  uint _tempval = 0;\n"
    "  uint _goto_level = 0;\n"
    "  uint _resume = 10;\n"
    "  uint _eof_trans = 15;\n"
    "  uint _again = 20;\n"
    "  uint _test_eof = 30;\n"
    "  uint _out = 40;\n\n";

  out << 
    "  while(true) { # goto loop\n"
    "    bool _trigger_goto = false;\n"
    "    if (_goto_level <= 0) {\n";
  
  if ( !noEnd ) {
    out <<  
     "\n# noEnd\n"
     "      if (" << P() << " == " << PE() << "){\n"
     "        _goto_level = _test_eof;\n"
     "        continue;\n"
     "      }\n\n";
  }

  if ( redFsm->errState != 0 ) {
    out <<  "\n# errState != 0\n"
     "      if (" << vCS() << " == " << redFsm->errState->id << "){\n"
     "        _goto_level = _out;\n"
     "       continue;\n"
     "      }\n";
  }
  
  /* The resume label. */
  out << 
   "    } # _goto_level <= 0\n\n "
   "    if (_goto_level <= _resume){\n";

  if ( redFsm->anyFromStateActions() ) {
    out << 
     "      _acts = " << FSA() << "[" << vCS() << "];\n"
     "      _nacts = " << A() << "[_acts];\n"
     "      _acts += 1;\n\n"
     "      while (_nacts > 0) {\n"
     "        _nacts -= 1;\n"
     "        _acts += 1;\n"
     "        _tempval = " << A() << "[_acts - 1];\n\n"
     "      # start from state action switch\n";
    FROM_STATE_ACTION_SWITCH("_tempval");
    out <<
     "      # end from state action switch\n"
     "      }\n\n"
     "      if (_trigger_goto) continue;\n";
  }

  if ( redFsm->anyConditions() )
    COND_TRANSLATE();
  
  LOCATE_TRANS();

  if ( redFsm->anyEofTrans() ) {
    out << 
      "      } # _goto_level <= _resume\n\n"
      "      if (_goto_level <= _eof_trans) {\n";
  }

  if ( redFsm->anyRegCurStateRef() )
    out << "      _ps = " << vCS() << ";\n";

  out << "    " << vCS() << " = " << TT() << "[_trans];\n\n";

  if ( redFsm->anyRegActions() ) {
    out << 
     "    if (" << TA() << "[_trans] != 0) {\n"
     "      _acts = " << TA() << "[_trans];\n"
     "      _nacts = " << A() << "[_acts];\n"
     "      _acts += 1;\n\n"
     "      while (_nacts > 0) {\n"
     "        _nacts -= 1;\n"
     "        _acts += 1;\n"
     "        _tempval = " << A() << "[_acts - 1];\n\n"
     "     # start action switch\n";
    ACTION_SWITCH("_tempval");
    out <<
     "    # end action switch\n"
     "      } # while _nacts\n"
     "    }\n\n"
     "    if (_trigger_goto) continue;\n";
  }
  
  /* The again label. */
  out <<
   "    } # endif \n\n"
   "    if (_goto_level <= _again) {\n";

  if ( redFsm->anyToStateActions() ) {
    out <<
     "      _acts = " << TSA() << "["  << vCS() << "];\n"
     "      _nacts = " << A() << "[_acts];\n"
     "      _acts += 1;\n"
     "      while (_nacts > 0) {\n"
     "        _nacts -= 1;\n"
     "        _acts += 1;\n"
     "        _tempval = " << A() << "[_acts - 1];\n\n"
     "      # start to state action switch\n";
     TO_STATE_ACTION_SWITCH("_tempval") <<
     "      # end to state action switch\n"
     "      }\n\n"
     "      if (_trigger_goto) continue;\n";
  }

  if ( redFsm->errState != 0 ) {
    out << 
     "      if (" << vCS() << " == " << redFsm->errState->id << ") {\n"
     "        _goto_level = _out;\n"
     "        continue;\n"
     "      }\n";
  }

  out << "      " << P() << " += 1;\n";

  if ( !noEnd ) {
    out << 
     "      if (" << P() << " != " << PE() << ") {\n"
     "        _goto_level = _resume;\n"
     "        continue;\n"
     "      }\n";
  }
  else {
    out <<
     "      _goto_level = _resume;\n"
     "      continue;\n";
  }

  /* The test_eof label. */
  out <<
   "    } # _goto_level <= _again\n\n"
   "    if (_goto_level <= _test_eof) {\n";

  if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
    out << 
     "    if (" << P() << " == " << vEOF() << ") {\n";

    if ( redFsm->anyEofTrans() ) {
      out << "# anyEofTrans\n"
       "    if (" << ET() << "[" << vCS() << "] > 0) {\n"
       "      _trans = " << ET() << "[" << vCS() << "] - 1;\n"
       "      _goto_level = _eof_trans;\n"
       "      continue;\n"
       "    }\n";
    }

    if ( redFsm->anyEofActions() ) {
      out <<
       "# anyEofActions\n"
       "    __acts = " << EA() << "[" << vCS() << "];\n"
       "    __nacts = " << A() << "[__acts];\n" << 
       "    __acts += 1;\n"
       "    while ( __nacts > 0 ) {\n"
       "      __nacts -= 1;\n"
       "      __acts += 1;\n"
       "      _tempval = " << A() << "[__acts - 1];\n"
       "    # start eof action switch\n";
       EOF_ACTION_SWITCH("_tempvalue") <<
       "    # end eof action switch\n\n"
       "    } # while __nacts \n\n"
       "    if (_trigger_goto) continue;\n";
    }

    out <<
     "    } # endif\n";
  }

  out << 
   "    } # _goto_level <= _test_eof\n\n"
   "    if (_goto_level <= _out) break;\n";

  /* The loop for faking goto. */
  out <<
   "    } # endif _goto_level <= out\n\n";

  /* Wrapping the execute block. */
  out << 
   "  # end of execute block";
}

}

/*
 * Local Variables:
 * mode: c++
 * indent-tabs-mode: 1
 * c-file-style: "bsd"
 * End:
 */
