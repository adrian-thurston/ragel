/*
 *  2007 Victor Hugo Borja <vic@rubyforge.org>
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
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
#include "redfsm.h"
#include "gendata.h"
#include "rlgen-ruby.h"
#include "ruby-ftabcodegen.h"

using std::ostream;
using std::ostringstream;
using std::string;
using std::cerr;
using std::endl;

std::ostream &RubyFTabCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numToStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\twhen " << redAct->actListId+1 << "\n";

			/* Write each action in the list of action items. */
			for ( ActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false );

		}
	}

	genLineDirective( out );
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &RubyFTabCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numFromStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\twhen " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( ActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false );

		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &RubyFTabCodeGen::EOF_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numEofRefs > 0 ) {
			/* Write the entry label. */
			out << "\twhen " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( ActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, true );

		}
	}

	genLineDirective( out );
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &RubyFTabCodeGen::ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			/* Write the entry label. */
			out << "\twhen " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( ActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false );

		}
	}

	genLineDirective( out );
	return out;
}


int RubyFTabCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	return act;
}

int RubyFTabCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	return act;
}

int RubyFTabCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
        return act;
}


/* Write out the function for a transition. */
int RubyFTabCodeGen::TRANS_ACTION( RedTransAp *trans )
{
	int action = 0;
	if ( trans->action != 0 )
		action = trans->action->actListId+1;
	return action;
}

void RubyFTabCodeGen::writeData()
{

	if ( redFsm->anyConditions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondOffset), CO() );
		COND_OFFSETS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondLen), CL() );
		COND_LENS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( WIDE_ALPH_TYPE(), CK() );
		COND_KEYS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxCondSpaceId), C() );
		COND_SPACES();
		CLOSE_ARRAY() <<
		"\n";
	}

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxKeyOffset), KO() );
	KEY_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
	KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxSingleLen), SL() );
	SINGLE_LENS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxRangeLen), RL() );
	RANGE_LENS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset), IO() );
	INDEX_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	if ( useIndicies ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
		INDICIES();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TT() );
		TRANS_TARGS_WI();
		CLOSE_ARRAY() <<
		"\n";

		if ( redFsm->anyActions() ) {
			OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActListId), TA() );
			TRANS_ACTIONS_WI();
			CLOSE_ARRAY() <<
			"\n";
		}
	}
	else {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TT() );
		TRANS_TARGS();
		CLOSE_ARRAY() <<
		"\n";

		if ( redFsm->anyActions() ) {
			OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActListId), TA() );
			TRANS_ACTIONS();
			CLOSE_ARRAY() <<
			"\n";
		}
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
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActListId), EA() );
		EOF_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	STATE_IDS();
}

void RubyFTabCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	begin\n"
			"		case ( " << EA() << "[" << CS() << "] )\n";
			EOF_ACTION_SWITCH();
		out <<
			"		end\n"
			"	end\n"
			"\n";
	}
}

void RubyFTabCodeGen::writeExec()
{
	out << 
                "	begin # ragel ftab \n"
		"	  _klen, _trans, _keys";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";

	if ( redFsm->anyConditions() )
		out << ", _widec";

	out << " = nil\n";

	if ( hasEnd ) {
		out <<	"	if " << P() << " != " << PE() << " \n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << "	if  " << CS() << " != " << redFsm->errState->id << "\n";
	}

        /* Open the _resume loop. */
	out << "	loop do # _resume \n"
               "		_break_resume = false\n";

	/* Open the _again loop. */
	out << "	begin # _again loop \n"
	    << "		_break_again = false\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	case " << FSA() << "[" << CS() << "] \n";
			FROM_STATE_ACTION_SWITCH();
		out <<
			"	end # from state action switch \n"
                        "	break if _break_again\n"
			"\n";
	}

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();

	LOCATE_TRANS();

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << CS() << ";\n";

	if ( useIndicies )
		out << "	_trans = " << I() << "[_trans];\n";

	out <<
		"	" << CS() << " = " << TT() << "[_trans];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
                /* break _again */
		out << 
			"	break if " << TA() << "[_trans] == 0\n"
			"\n"
			"	case " << TA() << "[_trans] \n";
			ACTION_SWITCH();
                out <<
			"	end # action switch \n"
			"\n";
                /* Not necessary as long as there is no code between here and the
                 * end while false. */
                // "break if _break_again\n";

	}
        
        /* Close the again loop. */
        out << "	end while false # _again loop\n";
        out << "	break if _break_resume\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	case " << TSA() << "[" << CS() << "] \n";
			TO_STATE_ACTION_SWITCH();
		out <<
			"	end # to state action switch \n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		out << 
			"	break if  " << CS() << " == " << redFsm->errState->id << "\n";
	}

	out << "	" << P() << " += 1\n";

	if ( hasEnd ) {
		out << 
			"	break if "<< P() << " == " << PE() << " \n";
	}

	/* Close the resume loop. */
	out << "	end # close the resume loop \n";


	/* The if guarding on the error state. */
	if ( redFsm->errState != 0 ) 
		out << "	end # close if guarding error state \n";

	/* The if guarding on empty string. */
	if ( hasEnd ) 
		out << "	end # close if guarding empty string \n";

	/* Wrapping the execute block. */
	out << "	end # close execution block \n";
}


void RubyFTabCodeGen::calcIndexSize()
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
		sizeWithInds += arrayTypeSize(redFsm->maxActListId) * redFsm->transSet.length();

	/* Calculate the cost of not using indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithoutInds += arrayTypeSize(redFsm->maxState) * totalIndex;
		if ( redFsm->anyActions() )
			sizeWithoutInds += arrayTypeSize(redFsm->maxActListId) * totalIndex;
	}

	/* If using indicies reduces the size, use them. */
	useIndicies = sizeWithInds < sizeWithoutInds;
}

/*
 * Local Variables:
 * mode: c++
 * indent-tabs-mode: 1
 * c-file-style: "bsd"
 * End:
 */
