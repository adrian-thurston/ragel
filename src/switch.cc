/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ragel.h"
#include "switch.h"
#include "redfsm.h"
#include "gendata.h"

#include <assert.h>

std::ostream &Switch::TRANS_GOTO( int off, RedTransAp *trans )
{
	out << "_trans = " << off << ";\n";
	return out;
}

void Switch::RANGE_B_SEARCH( RedStateAp *state, Key lower, Key upper, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = keyOps->eq( data[mid].lowKey, lower );
	bool limitHigh = keyOps->eq( data[mid].highKey, upper );

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );
		out << "} else if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );
		out << "} else {\n";
		TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value ) << "\n";
		out << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << "} else {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";
		}
		else {
			out << "} else if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << "if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << "} else {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";
		}
		else {
			out << "} else if ( " << GET_KEY() << " >= " << 
					KEY(data[mid].lowKey) << " ) {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " && " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << "if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " ) {\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";

			out << "else {\n";
			DEFAULT( state );
			out << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			out << "{\n";
			TRANS_GOTO(transBase + state->outSingle.length() + (mid), data[mid].value) << "\n";
			out << "}\n";
		}
	}
}

void Switch::SINGLE_SWITCH( RedStateAp *st )
{
	/* Load up the singles. */
	int numSingles = st->outSingle.length();
	RedTransEl *data = st->outSingle.data;

	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif ( " << GET_KEY() << " == " << 
				KEY(data[0].lowKey) << " ) {\n\t\t"; 

		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(transBase, data[0].value) << "\n";
		out << "\t}\n";
	
		out << "else {\n";
		NOT_SINGLE( st );
		out << "}\n";
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch( " << GET_KEY() << " ) {\n";

		/* Write out the single indices. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << CASE( KEY(data[j].lowKey) ) << " {\n";
			TRANS_GOTO(transBase + j, data[j].value) << "\n";
			out << CEND() << "\n}\n";
		}

		out << CodeGen::DEFAULT() << " {\n";
		NOT_SINGLE( st );
		out << CEND() << "\n}\n";
		
		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void Switch::DEFAULT( RedStateAp *st )
{
	if ( st->defTrans != 0 ) {
		TRANS_GOTO( transBase + st->outSingle.length() + st->outRange.length(), st->defTrans ) << "\n";
	}
}

void Switch::NOT_SINGLE( RedStateAp *st )
{
	if ( st->outRange.length() > 0 ) {
		RANGE_B_SEARCH( st, keyOps->minKey, keyOps->maxKey,
				0, st->outRange.length() - 1 );
	}
	else {
		DEFAULT( st );
	}
}

void Switch::LOCATE_TRANS()
{
	transBase = 0;

	out <<
		"	switch ( " << vCS() << " ) {\n";

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState ) {
			out << CASE( STR( st->id ) ) << " {\n";
			out << CEND() << "\n}\n";
		}
		else {
			/* Label the state. */
			out << CASE( STR( st->id ) ) << " {\n";

			/* Try singles. */
			if ( st->outSingle.length() > 0 ) {
				SINGLE_SWITCH( st );
			}
			else {
				NOT_SINGLE( st );
			}

			out << CEND() << "\n}\n";
		}

		transBase += st->outSingle.length() +
				st->outRange.length() +
				( st->defTrans != 0 ? 1 : 0 );
	}

	out <<
		"	}\n"
		"\n";
}

void Switch::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Choose the singles. */
	redFsm->moveSelectTransToSingle();

	if ( redFsm->errState != 0 )
		redFsm->getErrorCond();

	/* If any errors have occured in the input file then don't write anything. */
	if ( red->id->errorCount > 0 )
		return;

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	red->analyzeMachine();

	setKeyType();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}


void Switch::tableDataPass()
{
	if ( type == Loop )
		taActions();

	taKeyOffsets();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();
	taIndices();

	taTransCondSpacesWi();
	taTransOffsetsWi();
	taTransLengthsWi();

	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();

	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();
	taEofConds();
	taEofTrans();

	taKeys();
	taCondKeys();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void Switch::writeData()
{
	if ( type == Loop ) {
		/* If there are any transtion functions then output the array. If there
		 * are none, don't bother emitting an empty array that won't be used. */
		if ( redFsm->anyActions() )
			taActions();
	}

	taKeyOffsets();
	taKeys();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();

	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();

	taCondKeys();
	taCondTargs();
	taCondActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	taEofConds();

	if ( redFsm->anyEofTrans() )
		taEofTrans();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();

	STATE_IDS();
}


void Switch::setKeyType()
{
	transKeys.setType( ALPH_TYPE(), alphType->size, alphType->isChar );
	transKeys.isSigned = keyOps->isSigned;
}

void Switch::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}

void Switch::taKeyOffsets()
{
	keyOffsets.start();

	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		keyOffsets.value( curKeyOffset );
		curKeyOffset += st->outSingle.length() + st->outRange.length() * 2;
	}

	keyOffsets.finish();
}


void Switch::taSingleLens()
{
	singleLens.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		singleLens.value( st->outSingle.length() );

	singleLens.finish();
}


void Switch::taRangeLens()
{
	rangeLens.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		rangeLens.value( st->outRange.length() );

	rangeLens.finish();
}

void Switch::taIndexOffsets()
{
	indexOffsets.start();

	int curIndOffset = 0;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		indexOffsets.value( curIndOffset );

		/* Move the index offset ahead. */
		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	indexOffsets.finish();
}

void Switch::taToStateActions()
{
	toStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		TO_STATE_ACTION(st);

	toStateActions.finish();
}

void Switch::taFromStateActions()
{
	fromStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		FROM_STATE_ACTION(st);

	fromStateActions.finish();
}

void Switch::taEofActions()
{
	eofActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		EOF_ACTION( st );

	eofActions.finish();
}

void Switch::taEofConds()
{
	/*
	 * EOF Cond Spaces
	 */
	eofCondSpaces.start();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->outCondSpace != 0 )
			eofCondSpaces.value( st->outCondSpace->condSpaceId );
		else
			eofCondSpaces.value( -1 );
	}
	eofCondSpaces.finish();

	/*
	 * EOF Cond Key Indixes
	 */
	eofCondKeyOffs.start();

	int curOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long off = 0;
		if ( st->outCondSpace != 0 ) {
			off = curOffset;
			curOffset += st->outCondKeys.length();
		}
		eofCondKeyOffs.value( off );
	}

	eofCondKeyOffs.finish();

	/*
	 * EOF Cond Key Lengths.
	 */
	eofCondKeyLens.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long len = 0;
		if ( st->outCondSpace != 0 )
			len = st->outCondKeys.length();
		eofCondKeyLens.value( len );
	}

	eofCondKeyLens.finish();

	/*
	 * EOF Cond Keys
	 */
	eofCondKeys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->outCondSpace != 0 ) {
			for ( int c = 0; c < st->outCondKeys.length(); c++ ) {
				CondKey key = st->outCondKeys[c];
				eofCondKeys.value( key.getVal() );
			}
		}
	}

	eofCondKeys.finish();
}

void Switch::taEofTrans()
{
	eofTrans.start();

	/* Need to compute transition positions. */
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		totalTrans += st->outSingle.length();
		totalTrans += st->outRange.length();
		if ( st->defTrans != 0 )
			totalTrans += 1;
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long trans = 0;
		if ( st->eofTrans != 0 ) {
			trans = totalTrans + 1;
			totalTrans += 1;
		}

		eofTrans.value( trans );
	}

	eofTrans.finish();
}

void Switch::taKeys()
{
	transKeys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			transKeys.value( stel->lowKey.getVal() );
		}

		/* Loop the state's transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			/* Lower key. */
			transKeys.value( rtel->lowKey.getVal() );

			/* Upper key. */
			transKeys.value( rtel->highKey.getVal() );
		}
	}

	transKeys.finish();
}

void Switch::taIndices()
{
	indices.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ )
			indices.value( stel->value->id );

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ )
			indices.value( rtel->value->id );

		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			indices.value( st->defTrans->id );
	}

	indices.finish();
}

void Switch::taTransCondSpaces()
{
	transCondSpaces.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			if ( trans->condSpace != 0 )
				transCondSpaces.value( trans->condSpace->condSpaceId );
			else
				transCondSpaces.value( -1 );
		}
	}

	transCondSpaces.finish();
}

void Switch::taTransOffsets()
{
	transOffsets.start();

	int curOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			transOffsets.value( curOffset );
			curOffset += trans->numConds();
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			transOffsets.value( curOffset );
			curOffset += trans->numConds();
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			transOffsets.value( curOffset );
			curOffset += trans->numConds();
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			transOffsets.value( curOffset );
			curOffset += trans->numConds();
		}
	}

	errCondOffset = curOffset;

	transOffsets.finish();
}

void Switch::taTransLengths()
{
	transLengths.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			transLengths.value( trans->numConds() );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			transLengths.value( trans->numConds() );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			transLengths.value( trans->numConds() );
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			transLengths.value( trans->numConds() );
		}
	}

	transLengths.finish();
}

void Switch::taTransCondSpacesWi()
{
	transCondSpacesWi.start();

	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		/* Cond Space id. */
		if ( trans->condSpace != 0 )
			transCondSpacesWi.value( trans->condSpace->condSpaceId );
		else
			transCondSpacesWi.value( -1 );
	}

	transCondSpacesWi.finish();
}

void Switch::taTransOffsetsWi()
{
	transOffsetsWi.start();

	int curOffset = 0;
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		transOffsetsWi.value( curOffset );

		TransApSet::Iter next = trans;
		next.increment();

		curOffset += trans->numConds();
	}

	transOffsetsWi.finish();
}

void Switch::taTransLengthsWi()
{
	transLengthsWi.start();

	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		transLengthsWi.value( trans->numConds() );

		TransApSet::Iter next = trans;
		next.increment();
	}

	transLengthsWi.finish();
}

void Switch::taCondKeys()
{
	condKeys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				CondKey key = trans->outCondKey( c );
				condKeys.value( key.getVal() );
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				CondKey key = trans->outCondKey( c );
				condKeys.value( key.getVal() );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				CondKey key = trans->outCondKey( c );
				condKeys.value( key.getVal() );
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				CondKey key = trans->outCondKey( c );
				condKeys.value( key.getVal() );
			}
		}
	}

	condKeys.finish();
}

void Switch::taCondTargs()
{
	condTargs.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				condTargs.value( cond->targ->id );
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				condTargs.value( cond->targ->id );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				condTargs.value( cond->targ->id );
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				condTargs.value( cond->targ->id );
			}
		}
	}

	if ( redFsm->errCond != 0 ) {
		RedCondPair *cond = &redFsm->errCond->p;
		condTargs.value( cond->targ->id );
	}

	condTargs.finish();
}

void Switch::taCondActions()
{
	condActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				COND_ACTION( cond );
			}
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond( c );
				COND_ACTION( cond );
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond(c);
				COND_ACTION( cond );
			}
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond(c);
				COND_ACTION( cond );
			}
		}
	}

	if ( redFsm->errCond != 0 ) {
		RedCondPair *cond = &redFsm->errCond->p;
		COND_ACTION( cond );
	}

	condActions.finish();
}

void Switch::taNfaTargs()
{
	nfaTargs.start();

	/* Offset of zero means no NFA targs, put a filler there. */
	nfaTargs.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaTargs.value( st->nfaTargs->length() );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				nfaTargs.value( targ->state->id );
		}
	}

	nfaTargs.finish();
}

/* These need to mirror nfa targs. */
void Switch::taNfaPushActions()
{
	nfaPushActions.start();

	nfaPushActions.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaPushActions.value( 0 );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_PUSH_ACTION( targ );
		}
	}

	nfaPushActions.finish();
}

void Switch::taNfaPopTrans()
{
	nfaPopTrans.start();

	nfaPopTrans.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {

			nfaPopTrans.value( 0 );

			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_POP_TEST( targ );
		}
	}

	nfaPopTrans.finish();
}

void Switch::taNfaOffsets()
{
	nfaOffsets.start();

	/* Offset of zero means no NFA targs, real targs start at 1. */
	long offset = 1;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs == 0 ) {
			nfaOffsets.value( 0 );
		}
		else {
			nfaOffsets.value( offset );
			offset += 1 + st->nfaTargs->length();
		}
	}

	nfaOffsets.finish();
}


/* Write out the array of actions. */
std::ostream &Switch::ACTIONS_ARRAY()
{
	out << "\t0, ";
	int totalActions = 1;
	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		out << act->key.length() << ", ";
		/* Put in a line break every 8 */
		if ( totalActions++ % 8 == 7 )
			out << "\n\t";

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ ) {
			out << item->value->actionId;
			if ( ! (act.last() && item.last()) )
				out << ", ";

			/* Put in a line break every 8 */
			if ( totalActions++ % 8 == 7 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

void Switch::taActions()
{
	actions.start();

	/* Put "no-action" at the beginning. */
	actions.value( 0 );

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		actions.value( act->key.length() );

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
			actions.value( item->value->actionId );
	}

	actions.finish();
}




