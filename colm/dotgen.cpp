/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
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


#include "dotgen.h"
#include "parsedata.h"
#include "pdacodegen.h"

using namespace std;
#if 0

std::ostream &GraphvizDotGen::KEY( Key key )
{
	if ( printPrintables && key.isPrintable() ) {
		// Output values as characters, ensuring we escape the quote (") character
		char cVal = (char) key.getVal();
		out << "'";
		switch ( cVal ) {
			case '"': case '\\':
				out << "\\" << cVal;
				break;
			default:	
				out << cVal;
				break;
		}
		out << "'";
	}
	else {
		if ( keyOps->isSigned )
			out << key.getVal();
		else
			out << (unsigned long) key.getVal();
	}

	return out;
}

std::ostream &GraphvizDotGen::TRANS_ACTION( RedState *fromState, RedTrans *trans )
{
	int n = 0;
	RedAction *actions[3];

	if ( fromState->fromStateAction != 0 )
		actions[n++] = fromState->fromStateAction;
	if ( trans->action != 0 )
		actions[n++] = trans->action;
	if ( trans->targ != 0 && trans->targ->toStateAction != 0 )
		actions[n++] = trans->targ->toStateAction;

	if ( n > 0 )
		out << " / ";
	
	/* Loop the existing actions and write out what's there. */
	for ( int a = 0; a < n; a++ ) {
		for ( GenActionTable::Iter actIt = actions[a]->key.first(); actIt.lte(); actIt++ ) {
			GenAction *action = actIt->value;
			out << action->nameOrLoc();
			if ( a < n-1 || !actIt.last() )
				out << ", ";
		}
	}
	return out;
}

std::ostream &GraphvizDotGen::ACTION( RedAction *action )
{
	/* The action. */
	out << " / ";
	for ( GenActionTable::Iter actIt = action->key.first(); actIt.lte(); actIt++ ) {
		GenAction *action = actIt->value;
		if ( action->name != 0 )
			out << action->name;
		else
			out << action->loc.line << ":" << action->loc.col;
		if ( !actIt.last() )
			out << ", ";
	}
	return out;
}

std::ostream &GraphvizDotGen::ONCHAR( Key lowKey, Key highKey )
{
	if ( lowKey > keyOps->maxKey ) {
		GenCondSpace *condSpace = redFsm->findCondSpace( lowKey, highKey );
		Key values = ( lowKey - condSpace->baseKey ) / keyOps->alphSize();

		lowKey = keyOps->minKey + 
			(lowKey - condSpace->baseKey - keyOps->alphSize() * values.getVal());
		highKey = keyOps->minKey + 
			(highKey - condSpace->baseKey - keyOps->alphSize() * values.getVal());
		KEY( lowKey );
		if ( lowKey != highKey ) {
			out << "..";
			KEY( highKey );
		}
		out << "(";

		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			bool set = values & (1 << csi.pos());
			if ( !set )
				out << "!";
			out << (*csi)->nameOrLoc();
			if ( !csi.last() )
				out << ", ";
		}
		out << ")";
	}
	else {
		/* Output the key. Possibly a range. */
		KEY( lowKey );
		if ( highKey != lowKey ) {
			out << "..";
			KEY( highKey );
		}
	}
	return out;
}

void GraphvizDotGen::writeTransList( RedState *state )
{
	/* Build the set of unique transitions out of this state. */
	RedTransPtrSet stTransSet;
	for ( RedTransList::Iter tel = state->outRange; tel.lte(); tel++ ) {
		/* If we haven't seen the transitions before, the move forward
		 * emitting all the transitions on the same character. */
		if ( stTransSet.insert( tel->value ) ) {
			/* Write out the from and to states. */
			out << "\t" << state->id << " -> ";

			if ( tel->value->targ == 0 )
				out << "err_" << state->id;
			else
				out << tel->value->targ->id;

			/* Begin the label. */
			out << " [ label = \""; 
			ONCHAR( tel->lowKey, tel->highKey );

			/* Walk the transition list, finding the same. */
			for ( RedTransList::Iter mtel = tel.next(); mtel.lte(); mtel++ ) {
				if ( mtel->value == tel->value ) {
					out << ", ";
					ONCHAR( mtel->lowKey, mtel->highKey );
				}
			}

			/* Write the action and close the transition. */
			TRANS_ACTION( state, tel->value );
			out << "\" ];\n";
		}
	}

	/* Write the default transition. */
	if ( state->defTrans != 0 ) {
		/* Write out the from and to states. */
		out << "\t" << state->id << " -> ";

		if ( state->defTrans->targ == 0 )
			out << "err_" << state->id;
		else
			out << state->defTrans->targ->id;

		/* Begin the label. */
		out << " [ label = \"DEF"; 

		/* Write the action and close the transition. */
		TRANS_ACTION( state, state->defTrans );
		out << "\" ];\n";
	}
}

void GraphvizDotGen::writeDotFile( )
{
	out << 
		"digraph " << fsmName << " {\n"
		"	rankdir=LR;\n";
	
	/* Define the psuedo states. Transitions will be done after the states
	 * have been defined as either final or not final. */
	out << "	node [ shape = point ];\n";
	out << "	ENTRY;\n";

	/* Psuedo states for entry points in the entry map. */
	for ( EntryIdVect::Iter en = redFsm->entryPointIds; en.lte(); en++ ) {
		RedState *state = redFsm->allStates + *en;
		out << "	en_" << state->id << ";\n";
	}

	/* Psuedo states for final states with eof actions. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 )
			out << "	eof_" << st->id << ";\n";
	}

	out << "	node [ shape = circle, height = 0.2 ];\n";

	/* Psuedo states for states whose default actions go to error. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		bool needsErr = false;
		if ( st->defTrans != 0 && st->defTrans->targ == 0 )
			needsErr = true;
		else {
			for ( RedTransList::Iter tel = st->outRange; tel.lte(); tel++ ) {
				if ( tel->value->targ == 0 ) {
					needsErr = true;
					break;
				}
			}
		}

		if ( needsErr )
			out << "	err_" << st->id << " [ label=\"\"];\n";
	}

	/* Attributes common to all nodes, plus double circle for final states. */
	out << "	node [ fixedsize = true, height = 0.65, shape = doublecircle ];\n";

	/* List Final states. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->isFinal )
			out << "	" << st->id << ";\n";
	}

	/* List transitions. */
	out << "	node [ shape = circle ];\n";

	/* Walk the states. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		writeTransList( st );

	/* Transitions into the start state. */
	out << "	ENTRY -> " << redFsm->startState->id << " [ label = \"IN";
	out << "\" ];\n";

	/* Transitions into the entry points. */
	for ( EntryIdVect::Iter en = redFsm->entryPointIds; en.lte(); en++ ) {
		RedState *state = redFsm->allStates + *en;
		char *name = redFsm->entryPointNames[en.pos()];
		out << "	en_" << state->id << " -> " << state->id <<
				" [ label = \"" << name << "\" ];\n";
	}

	/* Out action transitions. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 ) {
			out << "	" << st->id << " -> eof_" << 
					st->id << " [ label = \"EOF"; 
			ACTION( st->eofAction ) << "\" ];\n";
		}
	}

	out <<
		"}\n";
}

void GraphvizDotGen::finishRagelDef()
{
	if ( !graphvizDone ) {
		graphvizDone = true;

		/* For dot file generation we want to pick default transitions. */
		redFsm->chooseDefaultSpan();

		/* Write out with it. */
		writeDotFile();
	}
}


void PdaCodeGen::writeTransList( PdaState *state )
{
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		/* Write out the from and to states. */
		out << "\t" << state->stateNum << " -> " << trans->value->toState->stateNum;

		/* Begin the label. */
		out << " [ label = \""; 
		long key = trans->key;  
		KlangEl *lel = pd->langElIndex[key];
		if ( lel != 0 )
			out << lel->data;
		else
			out << (char)key;

		if ( trans->value->actions.length() > 0 ) {
			out << " / ";
			for ( ActDataList::Iter act = trans->value->actions; act.lte(); act++ ) {
				switch ( *act & 0x3 ) {
				case 1: 
					out << "S(" << trans->value->actOrds[act.pos()] << ")";
					break;
				case 2: {
					out << "R(" << pd->prodIdIndex[(*act >> 2)]->data <<
							", " << trans->value->actOrds[act.pos()] << ")";
					break;
				}
				case 3: {
					out << "SR(" << pd->prodIdIndex[(*act >> 2)]->data << 
							", " << trans->value->actOrds[act.pos()] << ")";
					break;
				}}
				if ( ! act.last() )
					out << ", ";
			}
		}

		out << "\" ];\n";
	}
}

void PdaCodeGen::writeDotFile( PdaGraph *graph )
{
	out << 
		"digraph " << parserName << " {\n"
		"	rankdir=LR;\n";
	
	/* Define the psuedo states. Transitions will be done after the states
	 * have been defined as either final or not final. */
	out << "	node [ shape = point ];\n";
	out << "	ENTRY;\n";

	out << "	node [ shape = circle, height = 0.2 ];\n";

	/* Attributes common to all nodes, plus double circle for final states. */
	out << "	node [ fixedsize = true, height = 0.65, shape = doublecircle ];\n";

	/* List Final states. */
	for ( PdaStateSet::Iter st = graph->finStateSet; st.lte(); st++ )
		out << "	" << (*st)->stateNum << ";\n";

	/* List transitions. */
	out << "	node [ shape = circle ];\n";

	/* Walk the states. */
	for ( PdaStateList::Iter st = graph->stateList; st.lte(); st++ )
		writeTransList( st );

	/* Transitions into the start state. */
	out << "	ENTRY -> " << graph->startState->stateNum << " [ label = \"START\" ];\n";

	out <<
		"}\n";
}

void PdaCodeGen::writeDotFile( )
{
	writeDotFile( pd->pdaGraph );
}

#endif

