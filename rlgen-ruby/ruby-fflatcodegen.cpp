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

#include "ruby-fflatcodegen.h"

int RubyFFlatCodeGen::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	return act;
}

int RubyFFlatCodeGen::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	return act;
}

int RubyFFlatCodeGen::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
	return act;
}

/* Write out the function for a transition. */
int RubyFFlatCodeGen::TRANS_ACTION( RedTransAp *trans )
{
	int action = 0;
	if ( trans->action != 0 )
		action = trans->action->actListId+1;
	return action;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &RubyFFlatCodeGen::TO_STATE_ACTION_SWITCH()
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
std::ostream &RubyFFlatCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numFromStateRefs > 0 ) {
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

std::ostream &RubyFFlatCodeGen::EOF_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numEofRefs > 0 ) {
			/* Write the entry label. */
			out << "\twhen " << redAct->actListId+1 << "\n";

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
std::ostream &RubyFFlatCodeGen::ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( ActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
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

void RubyFFlatCodeGen::writeData()
{
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
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActListId), TA() );
		TRANS_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyToStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc),  TSA() );
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

void RubyFFlatCodeGen::writeExec()
{
	out << 
		"begin # ragel fflat\n"
		"	_slen, _trans, _keys, _inds";
	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";
	if ( redFsm->anyConditions() )
		out << ", _cond, _conds, _widec";
	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
		out << ", _acts, _nacts";
	
	out << " = nil\n";
	
	if ( hasEnd ) 
		out << "	if " << P() << " != " << PE() << " # pe guard\n";

	if ( redFsm->errState != 0 ) 
		out << "	if " << CS() << " != " << redFsm->errState->id << " # errstate guard\n";


	
	out << /* Open the _resume loop. */
		"	while true # _resume loop \n"
		"		_break_resume = false\n";	
	out << /* Open the _again loop. */
		"	begin\n"
		"		_break_again = false\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	case " << FSA() << "[" << CS() << "] \n";
		FROM_STATE_ACTION_SWITCH();
		out << 
			"	end # from state action switch \n"
			"	break if _break_again\n";
	}

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();
	
	LOCATE_TRANS();

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << CS() << "\n";

	out << "	" << CS() << " = " << TT() << "[_trans]\n";

	if ( redFsm->anyRegActions() ) {
		/* break _again */
		out << 
			"	break if " << TA() << "[_trans] == 0\n"
			"	case " << TA() << "[_trans]" << "\n";
		ACTION_SWITCH();
		out << 
			"	end # action switch\n";
		/* Not necessary as long as there is no code between here and the
		 * end while false. */
		// "break if _break_again\n";
	}
	
	out << /* Close the _again loop. */
		"	end while false # _again loop\n"
		"	break if _break_resume\n";
	
	if ( redFsm->anyToStateActions() ) {
		out <<
			"	case " << TSA() << "[" << CS() << "] \n";
		TO_STATE_ACTION_SWITCH();
		out <<
			"	end # to state action switch \n"
			"\n";
	}

	if ( redFsm->errState != 0 ) 
		out << "	break if " << CS() << " == " << redFsm->errState->id << "\n";

	out << "	" << P() << " += 1\n";

	if ( hasEnd )
		out << "	break if " << P() << " == " << PE() << "\n";

	
	out << /* Close the _resume loop. */
		"	end # _resume loop\n";

	if ( redFsm->errState != 0 ) 
		out << "	end # errstate guard\n";

	
	if ( hasEnd )
		out << "	end # pe guard\n";

	out << "end # ragel fflat";

}


void RubyFFlatCodeGen::writeEOF()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	  case " << EA() << "[" << CS() << "]\n";
		EOF_ACTION_SWITCH();
		out <<
			"	  end # eof action switch \n"
			"\n";
	}
}


/*
 * Local Variables:
 * mode: c++
 * indent-tabs-mode: 1
 * c-file-style: "bsd"
 * End:
 */

