/*
 * Copyright 2004-2018 Adrian Thurston <thurston@colm.net>
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
#include "flat.h"
#include "redfsm.h"
#include "gendata.h"

void Flat::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Do flat expand. */
	redFsm->makeFlatClass();

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

void Flat::tableDataPass()
{
	if ( type == Flat::Loop ) {
		if ( redFsm->anyActions() )
			taActions();
	}

	taKeys();
	taCharClass();
	taFlatIndexOffset();

	taIndices();
	taIndexDefaults();
	taTransCondSpaces();

	if ( red->condSpaceList.length() > 0 )
		taTransOffsets();

	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofConds();
	taEofActions();
	taEofTrans();
	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void Flat::writeData()
{
	if ( type == Flat::Loop ) {
		/* If there are any transtion functions then output the array. If there
		 * are none, don't bother emitting an empty array that won't be used. */
		if ( redFsm->anyActions() )
			taActions();
	}

	taKeys();
	taCharClass();
	taFlatIndexOffset();

	taIndices();
	taIndexDefaults();
	taTransCondSpaces();
	if ( red->condSpaceList.length() > 0 )
		taTransOffsets();
	taCondTargs();
	taCondActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	taEofConds();

	if ( redFsm->anyEofActions() )
		taEofActions();

	if ( redFsm->anyEofTrans() )
		taEofTrans();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();

	STATE_IDS();
}


void Flat::setKeyType()
{
	transKeys.setType( ALPH_TYPE(), alphType->size, alphType->isChar );
	transKeys.isSigned = keyOps->isSigned;
}

void Flat::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}

void Flat::taFlatIndexOffset()
{
	flatIndexOffset.start();

	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		flatIndexOffset.value( curIndOffset );
		
		/* Move the index offset ahead. */
		if ( st->transList != 0 )
			curIndOffset += ( st->high - st->low + 1 );
	}

	flatIndexOffset.finish();
}

void Flat::taCharClass()
{
	charClass.start();

	if ( redFsm->classMap != 0 ) {
		long long maxSpan = keyOps->span( redFsm->lowKey, redFsm->highKey );

		for ( long long pos = 0; pos < maxSpan; pos++ )
			charClass.value( redFsm->classMap[pos] );
	}

	charClass.finish();
}

void Flat::taToStateActions()
{
	toStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		TO_STATE_ACTION(st);
	}

	toStateActions.finish();
}

void Flat::taFromStateActions()
{
	fromStateActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		FROM_STATE_ACTION( st );
	}

	fromStateActions.finish();
}

void Flat::taEofActions()
{
	eofActions.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		EOF_ACTION( st );
	}

	eofActions.finish();
}

void Flat::taEofConds()
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

void Flat::taEofTrans()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	long *transPos = new long[redFsm->transSet.length()];
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		transPos[trans->id] = t;
	}

	eofTrans.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		long trans = 0;

		if ( st->eofTrans != 0 )
			trans = transPos[st->eofTrans->id] + 1;

		eofTrans.value( trans );
	}

	eofTrans.finish();

	delete[] transPtrs;
	delete[] transPos;
}

void Flat::taKeys()
{
	transKeys.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList ) {
			/* Emit just low key and high key. */
			transKeys.value( st->low );
			transKeys.value( st->high );
		}
		else {
			/* Emit an impossible range so the driver fails the lookup. */
			transKeys.value( 1 );
			transKeys.value( 0 );
		}
	}

	transKeys.finish();
}

void Flat::taIndices()
{
	indices.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			long long span = st->high - st->low + 1;
			for ( long long pos = 0; pos < span; pos++ )
				indices.value( st->transList[pos]->id );
		}
	}

	indices.finish();
}

void Flat::taIndexDefaults()
{
	indexDefaults.start();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			indexDefaults.value( st->defTrans->id );
		else
			indexDefaults.value( 0 );
	}

	indexDefaults.finish();
}


void Flat::taTransCondSpaces()
{
	transCondSpaces.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		transPtrs[trans->id] = trans;
	}

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		if ( trans->condSpace != 0 )
			transCondSpaces.value( trans->condSpace->condSpaceId );
		else
			transCondSpaces.value( -1 );
	}
	delete[] transPtrs;

	transCondSpaces.finish();
}

void Flat::taTransOffsets()
{
	transOffsets.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	int curOffset = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		transOffsets.value( curOffset );

		curOffset += trans->condFullSize();
	}

	delete[] transPtrs;

	transOffsets.finish();
}

void Flat::taCondTargs()
{
	condTargs.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		long fullSize = trans->condFullSize();
		RedCondPair **fullPairs = new RedCondPair*[fullSize];
		for ( long k = 0; k < fullSize; k++ )
			fullPairs[k] = trans->errCond();

		for ( int c = 0; c < trans->numConds(); c++ )
			fullPairs[trans->outCondKey( c ).getVal()] = trans->outCond( c );

		for ( int k = 0; k < fullSize; k++ ) {
			RedCondPair *cond = fullPairs[k];
			condTargs.value( cond->targ->id );
		}

		delete[] fullPairs;
	}
	delete[] transPtrs;

	condTargs.finish();
}

void Flat::taCondActions()
{
	condActions.start();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];

		long fullSize = trans->condFullSize();
		RedCondPair **fullPairs = new RedCondPair*[fullSize];
		for ( long k = 0; k < fullSize; k++ )
			fullPairs[k] = trans->errCond();

		for ( int c = 0; c < trans->numConds(); c++ )
			fullPairs[trans->outCondKey( c ).getVal()] = trans->outCond( c );

		for ( int k = 0; k < fullSize; k++ ) {
			RedCondPair *cond = fullPairs[k];
			COND_ACTION( cond );
		}
		delete[] fullPairs;
	}
	delete[] transPtrs;

	condActions.finish();
}

/* Write out the array of actions. */
void Flat::taActions()
{
	actions.start();

	/* Add in the the empty actions array. */
	actions.value( 0 );

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Length first. */
		actions.value( act->key.length() );

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
			actions.value( item->value->actionId );
	}

	actions.finish();
}

void Flat::taNfaTargs()
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
void Flat::taNfaPushActions()
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

void Flat::taNfaPopTrans()
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


void Flat::taNfaOffsets()
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








