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


#include "global.h"
#include "compiler.h"

using namespace std;


void Compiler::writeTransList( PdaState *state )
{
	ostream &out = *outStream;
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		/* Write out the from and to states. */
		out << "\t" << state->stateNum << " -> " << trans->value->toState->stateNum;

		/* Begin the label. */
		out << " [ label = \""; 
		long key = trans->key;  
		LangEl *lel = langElIndex[key];
		if ( lel != 0 )
			out << lel->name;
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
					out << "R(" << prodIdIndex[(*act >> 2)]->data <<
							", " << trans->value->actOrds[act.pos()] << ")";
					break;
				}
				case 3: {
					out << "SR(" << prodIdIndex[(*act >> 2)]->data << 
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

void Compiler::writeDotFile( PdaGraph *graph )
{
	ostream &out = *outStream;
	out << 
		"digraph " << parserName << " {\n"
		"	rankdir=LR;\n"
		"	ranksep=\"0\"\n"
		"	nodesep=\"0.25\"\n"
		"\n";
	
	/* Define the psuedo states. Transitions will be done after the states
	 * have been defined as either final or not final. */
	out << 
		"	node [ shape = point ];\n";
	
	for ( int i = 0; i < graph->entryStateSet.length(); i++ )
		out << "\tENTRY" << i <<  " [ label = \"\" ];\n";

	out << 
		"\n"
		"	node [ shape = circle, fixedsize = true, height = 0.6 ];\n";

	/* Walk the states. */
	for ( PdaStateList::Iter st = graph->stateList; st.lte(); st++ )
		out << "	" << st->stateNum << " [ label = \"" << st->stateNum << "\" ];\n";

	out << "\n";

	/* Walk the states. */
	for ( PdaStateList::Iter st = graph->stateList; st.lte(); st++ )
		writeTransList( st );

	/* Start state and other entry points. */
	for ( PdaStateSet::Iter st = graph->entryStateSet; st.lte(); st++ )
		out << "\tENTRY" << st.pos() << " -> " << (*st)->stateNum << " [ label = \"\" ];\n";

	out <<
		"}\n";
}

void Compiler::writeDotFile()
{
	writeDotFile( pdaGraph );
}

