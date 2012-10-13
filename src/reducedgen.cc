/*
 *  Copyright 2005-2011 Adrian Thurston <thurston@complang.org>
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
#include "parsedata.h"
#include "fsmgraph.h"
#include "gendata.h"
#include "inputdata.h"
#include <string.h>
#include "rlparse.h"
#include "version.h"

/*
 * Code generators.
 */
#include "xml/xmlcodegen.h"

#include "cs/cstable.h"
#include "cs/csftable.h"
#include "cs/csflat.h"
#include "cs/csfflat.h"
#include "cs/csgoto.h"
#include "cs/csfgoto.h"
#include "cs/csipgoto.h"
#include "cs/cssplit.h"

#include "c/cdtable.h"
#include "c/cdftable.h"
#include "c/cdflat.h"
#include "c/cdfflat.h"
#include "c/cdgoto.h"
#include "c/cdfgoto.h"
#include "c/cdipgoto.h"
#include "c/cdsplit.h"

#include "dot/dotcodegen.h"

#include "java/javacodegen.h"

#include "go/gotable.h"
#include "go/goftable.h"
#include "go/goflat.h"
#include "go/gofflat.h"
#include "go/gogoto.h"
#include "go/gofgoto.h"
#include "go/goipgoto.h"

#include "ml/mltable.h"
#include "ml/mlftable.h"
#include "ml/mlflat.h"
#include "ml/mlfflat.h"
#include "ml/mlgoto.h"
#include "ml/mlfgoto.h"

#include "ruby/rubytable.h"
#include "ruby/rubyftable.h"
#include "ruby/rubyflat.h"
#include "ruby/rubyfflat.h"
#include "rbx/rbxgoto.h"

#include "crack/crackflat.h"

using std::cerr;
using std::endl;

GenBase::GenBase( char *fsmName, ParseData *pd, FsmAp *fsm )
:
	fsmName(fsmName),
	pd(pd),
	fsm(fsm),
	nextActionTableId(0)
{
}

void GenBase::appendTrans( TransListVect &outList, Key lowKey, 
		Key highKey, TransAp *trans )
{
	if ( trans->ctList.head->toState != 0 || trans->ctList.head->actionTable.length() > 0 )
		outList.append( TransEl( lowKey, highKey, trans ) );
}

void GenBase::reduceActionTables()
{
	/* Reduce the actions tables to a set. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		RedActionTable *actionTable = 0;

		/* Reduce To State Actions. */
		if ( st->toStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->toStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce From State Actions. */
		if ( st->fromStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->fromStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce EOF actions. */
		if ( st->eofActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->eofActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Loop the transitions and reduce their actions. */
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->ctList.head->actionTable.length() > 0 ) {
				if ( actionTableMap.insert( trans->ctList.head->actionTable, &actionTable ) )
					actionTable->id = nextActionTableId++;
			}
		}
	}
}

CodeGenData *makeCodeGen( const CodeGenArgs &args );
CodeGenData *makeCodeGen2( const CodeGenArgs &args )
{
	return makeCodeGen( args );
}

ReducedGen::ReducedGen( const CodeGenArgs &args )
:
	GenBase(args.fsmName, args.pd, args.fsm),
	cgd(0)
{
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *cdMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;
	switch ( hostLang->lang ) {
	case HostLang::C:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new C::CTabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new C::CFTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new C::CFlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new C::CFFlatCodeGen(args);
			break;
		case GenGoto:
			codeGen = new C::CGotoCodeGen(args);
			break;
		case GenFGoto:
			codeGen = new C::CFGotoCodeGen(args);
			break;
		case GenIpGoto:
			codeGen = new C::CIpGotoCodeGen(args);
			break;
		case GenSplit:
			codeGen = new C::CSplitCodeGen(args);
			break;
		}
		break;

	case HostLang::D:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new C::DTabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new C::DFTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new C::DFlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new C::DFFlatCodeGen(args);
			break;
		case GenGoto:
			codeGen = new C::DGotoCodeGen(args);
			break;
		case GenFGoto:
			codeGen = new C::DFGotoCodeGen(args);
			break;
		case GenIpGoto:
			codeGen = new C::DIpGotoCodeGen(args);
			break;
		case GenSplit:
			codeGen = new C::DSplitCodeGen(args);
			break;
		}
		break;

	case HostLang::D2:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new C::D2TabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new C::D2FTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new C::D2FlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new C::D2FFlatCodeGen(args);
			break;
		case GenGoto:
			codeGen = new C::D2GotoCodeGen(args);
			break;
		case GenFGoto:
			codeGen = new C::D2FGotoCodeGen(args);
			break;
		case GenIpGoto:
			codeGen = new C::D2IpGotoCodeGen(args);
			break;
		case GenSplit:
			codeGen = new C::D2SplitCodeGen(args);
			break;
		}
		break;

	default: break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *javaMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = new Java::JavaTabCodeGen(args);

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *goMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new Go::GoTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new Go::GoFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new Go::GoFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new Go::GoFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new Go::GoGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new Go::GoFGotoCodeGen(args);
		break;
	case GenIpGoto:
		codeGen = new Go::GoIpGotoCodeGen(args);
		break;
	default:
		cerr << "Invalid output style, only -T0, -T1, -F0, -F1, -G0, -G1 and -G2 are supported.\n";
		exit(1);
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *crackMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen;

	switch ( codeStyle ) {
	case GenFlat:
		codeGen = new Crack::CrackFlatCodeGen(args);
		break;
	default:
		cerr << "Invalid output style, only -F0 is supported. Please "
			"rerun ragel including this flag.\n";
		exit(1);
	}

	return codeGen;
}


/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *rubyMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;
	switch ( codeStyle ) {
		case GenTables: 
			codeGen = new Ruby::RubyTabCodeGen(args);
			break;
		case GenFTables:
			codeGen = new Ruby::RubyFTabCodeGen(args);
			break;
		case GenFlat:
			codeGen = new Ruby::RubyFlatCodeGen(args);
			break;
		case GenFFlat:
			codeGen = new Ruby::RubyFFlatCodeGen(args);
			break;
		case GenGoto:
			if ( rubyImpl == Rubinius ) {
				codeGen = new Rbx::RbxGotoCodeGen(args);
			} else {
				cerr << "Goto style is still _very_ experimental " 
					"and only supported using Rubinius.\n"
					"You may want to enable the --rbx flag "
					" to give it a try.\n";
				exit(1);
			}
			break;
		default:
			cerr << "Invalid code style\n";
			exit(1);
			break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *csharpMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new CSharp::CSharpTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new CSharp::CSharpFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new CSharp::CSharpFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new CSharp::CSharpFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new CSharp::CSharpGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new CSharp::CSharpFGotoCodeGen(args);
		break;
	case GenIpGoto:
		codeGen = new CSharp::CSharpIpGotoCodeGen(args);
		break;
	case GenSplit:
		codeGen = new CSharp::CSharpSplitCodeGen(args);
		break;
	}

	return codeGen;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *ocamlMakeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new OCaml::OCamlTabCodeGen(args);
		break;
	case GenFTables:
		codeGen = new OCaml::OCamlFTabCodeGen(args);
		break;
	case GenFlat:
		codeGen = new OCaml::OCamlFlatCodeGen(args);
		break;
	case GenFFlat:
		codeGen = new OCaml::OCamlFFlatCodeGen(args);
		break;
	case GenGoto:
		codeGen = new OCaml::OCamlGotoCodeGen(args);
		break;
	case GenFGoto:
		codeGen = new OCaml::OCamlFGotoCodeGen(args);
		break;
	default:
		cerr << "I only support the -T0 -T1 -F0 -F1 -G0 and -G1 output styles for OCaml.\n";
		exit(1);
	}

	return codeGen;
}


CodeGenData *makeCodeGen( const CodeGenArgs &args )
{
	CodeGenData *cgd = 0;
	if ( hostLang == &hostLangC )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangD )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangD2 )
		cgd = cdMakeCodeGen( args );
	else if ( hostLang == &hostLangGo )
		cgd = goMakeCodeGen( args );
	else if ( hostLang == &hostLangJava )
		cgd = javaMakeCodeGen( args );
	else if ( hostLang == &hostLangRuby )
		cgd = rubyMakeCodeGen( args );
	else if ( hostLang == &hostLangCSharp )
		cgd = csharpMakeCodeGen( args );
	else if ( hostLang == &hostLangOCaml )
		cgd = ocamlMakeCodeGen( args );
	else if ( hostLang == &hostLangCrack )
		cgd = crackMakeCodeGen( args );
	return cgd;
}


void ReducedGen::makeText( GenInlineList *outList, InlineItem *item )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::Text );
	inlineItem->data = item->data;

	outList->append( inlineItem );
}

void ReducedGen::makeTargetItem( GenInlineList *outList, NameInst *nameTarg, 
		GenInlineItem::Type type )
{
	long targetState;
	if ( pd->generatingSectionSubset )
		targetState = -1;
	else {
		EntryMapEl *targ = fsm->entryPoints.find( nameTarg->id );
		targetState = targ->value->alg.stateNum;
	}

	/* Make the item. */
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), type );
	inlineItem->targId = targetState;
	outList->append( inlineItem );
}

/* Make a sublist item with a given type. */
void ReducedGen::makeSubList( GenInlineList *outList, 
		InlineList *inlineList, GenInlineItem::Type type )
{
	/* Fill the sub list. */
	GenInlineList *subList = new GenInlineList;
	makeGenInlineList( subList, inlineList );

	/* Make the item. */
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), type );
	inlineItem->children = subList;
	outList->append( inlineItem );
}

void ReducedGen::makeLmOnLast( GenInlineList *outList, InlineItem *item )
{
	makeSetTokend( outList, 1 );

	if ( item->longestMatchPart->action != 0 ) {
		makeSubList( outList, 
				item->longestMatchPart->action->inlineList, 
				GenInlineItem::SubAction );
	}
}

void ReducedGen::makeLmOnNext( GenInlineList *outList, InlineItem *item )
{
	makeSetTokend( outList, 0 );
	outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Hold ) );

	if ( item->longestMatchPart->action != 0 ) {
		makeSubList( outList, 
			item->longestMatchPart->action->inlineList,
			GenInlineItem::SubAction );
	}
}

void ReducedGen::makeExecGetTokend( GenInlineList *outList )
{
	/* Make the Exec item. */
	GenInlineItem *execItem = new GenInlineItem( InputLoc(), GenInlineItem::Exec );
	execItem->children = new GenInlineList;

	/* Make the GetTokEnd */
	GenInlineItem *getTokend = new GenInlineItem( InputLoc(), GenInlineItem::LmGetTokEnd );
	execItem->children->append( getTokend );

	outList->append( execItem );
}

void ReducedGen::makeLmOnLagBehind( GenInlineList *outList, InlineItem *item )
{
	/* Jump to the tokend. */
	makeExecGetTokend( outList );

	if ( item->longestMatchPart->action != 0 ) {
		makeSubList( outList,
			item->longestMatchPart->action->inlineList,
			GenInlineItem::SubAction );
	}
}

void ReducedGen::makeLmSwitch( GenInlineList *outList, InlineItem *item )
{
	GenInlineItem *lmSwitch = new GenInlineItem( InputLoc(), GenInlineItem::LmSwitch );
	GenInlineList *lmList = lmSwitch->children = new GenInlineList;
	LongestMatch *longestMatch = item->longestMatch;

	/* We can't put the <exec> here because we may need to handle the error
	 * case and in that case p should not be changed. Instead use a default
	 * label in the switch to adjust p when user actions are not set. An id of
	 * -1 indicates the default. */

	if ( longestMatch->lmSwitchHandlesError ) {
		/* If the switch handles error then we should have also forced the
		 * error state. */
		assert( fsm->errState != 0 );

		GenInlineItem *errCase = new GenInlineItem( InputLoc(), GenInlineItem::SubAction );
		errCase->lmId = 0;
		errCase->children = new GenInlineList;

		/* Make the item. */
		GenInlineItem *gotoItem = new GenInlineItem( InputLoc(), GenInlineItem::Goto );
		gotoItem->targId = fsm->errState->alg.stateNum;
		errCase->children->append( gotoItem );

		lmList->append( errCase );
	}
	
	bool needDefault = false;
	for ( LmPartList::Iter lmi = *longestMatch->longestMatchList; lmi.lte(); lmi++ ) {
		if ( lmi->inLmSelect ) {
			if ( lmi->action == 0 )
				needDefault = true;
			else {
				/* Open the action. Write it with the context that sets up _p 
				 * when doing control flow changes from inside the machine. */
				GenInlineItem *lmCase = new GenInlineItem( InputLoc(), 
						GenInlineItem::SubAction );
				lmCase->lmId = lmi->longestMatchId;
				lmCase->children = new GenInlineList;

				makeExecGetTokend( lmCase->children );
				makeGenInlineList( lmCase->children, lmi->action->inlineList );

				lmList->append( lmCase );
			}
		}
	}

	if ( needDefault ) {
		GenInlineItem *defCase = new GenInlineItem( InputLoc(), 
				GenInlineItem::SubAction );
		defCase->lmId = -1;
		defCase->children = new GenInlineList;

		makeExecGetTokend( defCase->children );

		lmList->append( defCase );
	}

	outList->append( lmSwitch );
}

void ReducedGen::makeSetTokend( GenInlineList *outList, long offset )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::LmSetTokEnd );
	inlineItem->offset = offset;
	outList->append( inlineItem );
}

void ReducedGen::makeSetAct( GenInlineList *outList, long lmId )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::LmSetActId );
	inlineItem->lmId = lmId;
	outList->append( inlineItem );
}

void ReducedGen::makeGenInlineList( GenInlineList *outList, InlineList *inList )
{
	for ( InlineList::Iter item = *inList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			makeText( outList, item );
			break;
		case InlineItem::Goto:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Goto );
			break;
		case InlineItem::GotoExpr:
			makeSubList( outList, item->children, GenInlineItem::GotoExpr );
			break;
		case InlineItem::Call:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Call );
			break;
		case InlineItem::CallExpr:
			makeSubList( outList, item->children, GenInlineItem::CallExpr );
			break;
		case InlineItem::Next:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Next );
			break;
		case InlineItem::NextExpr:
			makeSubList( outList, item->children, GenInlineItem::NextExpr );
			break;
		case InlineItem::Break:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Break ) );
			break;
		case InlineItem::Ret: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Ret ) );
			break;
		case InlineItem::PChar:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::PChar ) );
			break;
		case InlineItem::Char: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Char ) );
			break;
		case InlineItem::Curs: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Curs ) );
			break;
		case InlineItem::Targs: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Targs ) );
			break;
		case InlineItem::Entry:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Entry );
			break;

		case InlineItem::Hold:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Hold ) );
			break;
		case InlineItem::Exec:
			makeSubList( outList, item->children, GenInlineItem::Exec );
			break;

		case InlineItem::LmSetActId:
			makeSetAct( outList, item->longestMatchPart->longestMatchId );
			break;
		case InlineItem::LmSetTokEnd:
			makeSetTokend( outList, 1 );
			break;

		case InlineItem::LmOnLast:
			makeLmOnLast( outList, item );
			break;
		case InlineItem::LmOnNext:
			makeLmOnNext( outList, item );
			break;
		case InlineItem::LmOnLagBehind:
			makeLmOnLagBehind( outList, item );
			break;
		case InlineItem::LmSwitch: 
			makeLmSwitch( outList, item );
			break;

		case InlineItem::LmInitAct:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmInitAct ) );
			break;
		case InlineItem::LmInitTokStart:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmInitTokStart ) );
			break;
		case InlineItem::LmSetTokStart:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmSetTokStart ) );
			cgd->hasLongestMatch = true;
			break;
		}
	}
}

void ReducedGen::makeExports()
{
	for ( ExportList::Iter exp = pd->exportList; exp.lte(); exp++ )
		cgd->exportList.append( new Export( exp->name, exp->key ) );
}

void ReducedGen::makeAction( Action *action )
{
	GenInlineList *genList = new GenInlineList;
	makeGenInlineList( genList, action->inlineList );

	cgd->newAction( curAction++, action->name, action->loc, genList );
}


void ReducedGen::makeActionList()
{
	/* Determine which actions to write. */
	int nextActionId = 0;
	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->numRefs() > 0 || act->numCondRefs > 0 )
			act->actionId = nextActionId++;
	}

	/* Write the list. */
	cgd->initActionList( nextActionId );
	curAction = 0;

	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->actionId >= 0 )
			makeAction( act );
	}
}

void ReducedGen::makeActionTableList()
{
	/* Must first order the action tables based on their id. */
	int numTables = nextActionTableId;
	RedActionTable **tables = new RedActionTable*[numTables];
	for ( ActionTableMap::Iter at = actionTableMap; at.lte(); at++ )
		tables[at->id] = at;

	cgd->initActionTableList( numTables );
	curActionTable = 0;

	for ( int t = 0; t < numTables; t++ ) {
		long length = tables[t]->key.length();

		/* Collect the action table. */
		RedAction *redAct = cgd->allActionTables + curActionTable;
		redAct->actListId = curActionTable;
		redAct->key.setAsNew( length );

		for ( ActionTable::Iter atel = tables[t]->key; atel.lte(); atel++ ) {
			redAct->key[atel.pos()].key = 0;
			redAct->key[atel.pos()].value = cgd->allActions + 
					atel->value->actionId;
		}

		/* Insert into the action table map. */
		cgd->redFsm->actionMap.insert( redAct );

		curActionTable += 1;
	}

	delete[] tables;
}

void ReducedGen::makeConditions()
{
	if ( condData->condSpaceMap.length() > 0 ) {
		long nextCondSpaceId = 0;
		for ( CondSpaceMap::Iter cs = condData->condSpaceMap; cs.lte(); cs++ )
			cs->condSpaceId = nextCondSpaceId++;

		long listLength = condData->condSpaceMap.length();
		cgd->initCondSpaceList( listLength );
		curCondSpace = 0;

		for ( CondSpaceMap::Iter cs = condData->condSpaceMap; cs.lte(); cs++ ) {
			long id = cs->condSpaceId;
			cgd->newCondSpace( curCondSpace, id, cs->baseKey );
			for ( CondSet::Iter csi = cs->condSet; csi.lte(); csi++ )
				cgd->condSpaceItem( curCondSpace, (*csi)->actionId );
			curCondSpace += 1;
		}
	}
}

bool ReducedGen::makeNameInst( std::string &res, NameInst *nameInst )
{
	bool written = false;
	if ( nameInst->parent != 0 )
		written = makeNameInst( res, nameInst->parent );
	
	if ( nameInst->name != 0 ) {
		if ( written )
			res += '_';
		res += nameInst->name;
		written = true;
	}

	return written;
}

void ReducedGen::makeEntryPoints()
{
	/* List of entry points other than start state. */
	if ( fsm->entryPoints.length() > 0 || pd->lmRequiresErrorState ) {
		if ( pd->lmRequiresErrorState )
			cgd->setForcedErrorState();

		for ( EntryMap::Iter en = fsm->entryPoints; en.lte(); en++ ) {
			/* Get the name instantiation from nameIndex. */
			NameInst *nameInst = pd->nameIndex[en->key];
			std::string name;
			makeNameInst( name, nameInst );
			StateAp *state = en->value;
			cgd->addEntryPoint( strdup(name.c_str()), state->alg.stateNum );
		}
	}
}

void ReducedGen::makeStateActions( StateAp *state )
{
	RedActionTable *toStateActions = 0;
	if ( state->toStateActionTable.length() > 0 )
		toStateActions = actionTableMap.find( state->toStateActionTable );

	RedActionTable *fromStateActions = 0;
	if ( state->fromStateActionTable.length() > 0 )
		fromStateActions = actionTableMap.find( state->fromStateActionTable );

	/* EOF actions go out here only if the state has no eof target. If it has
	 * an eof target then an eof transition will be used instead. */
	RedActionTable *eofActions = 0;
	if ( state->eofTarget == 0 && state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	if ( toStateActions != 0 || fromStateActions != 0 || eofActions != 0 ) {
		long to = -1;
		if ( toStateActions != 0 )
			to = toStateActions->id;

		long from = -1;
		if ( fromStateActions != 0 )
			from = fromStateActions->id;

		long eof = -1;
		if ( eofActions != 0 )
			eof = eofActions->id;

		cgd->setStateActions( curState, to, from, eof );
	}
}

void ReducedGen::makeEofTrans( StateAp *state )
{
	RedActionTable *eofActions = 0;
	if ( state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	/* The EOF trans is used when there is an eof target, otherwise the eof
	 * action goes into state actions. */
	if ( state->eofTarget != 0 ) {
		long targ = state->eofTarget->alg.stateNum;
		long action = -1;
		if ( eofActions != 0 )
			action = eofActions->id;

		cgd->setEofTrans( curState, targ, action );
	}
}

void ReducedGen::makeStateConditions( StateAp *state )
{
	if ( state->stateCondList.length() > 0 ) {
		long length = state->stateCondList.length();
		cgd->initStateCondList( curState, length );
		curStateCond = 0;

		for ( StateCondList::Iter scdi = state->stateCondList; scdi.lte(); scdi++ ) {
			cgd->addStateCond( curState, scdi->lowKey, scdi->highKey, 
					scdi->condSpace->condSpaceId );
		}
	}
}

void ReducedGen::makeTrans( Key lowKey, Key highKey, TransAp *trans )
{
	RedCondList redCondList;
		
	long targ = -1;
	long action = -1;

	for ( CondTransList::Iter cti = trans->ctList; cti.lte(); cti++ ) {
		/* First reduce the action. */
		RedActionTable *actionTable = 0;
		if ( cti->actionTable.length() > 0 )
			actionTable = actionTableMap.find( cti->actionTable );

		if ( cti->toState != 0 )
			targ = cti->toState->alg.stateNum;

		if ( actionTable != 0 )
			action = actionTable->id;

		cgd->newCondTrans( redCondList, curState, /*curTrans++*/ 0, cti->key, targ, action );
	}

	cgd->newTrans( curState, curTrans++, lowKey, highKey, targ, action );
}

void ReducedGen::makeTransList( StateAp *state )
{
	TransListVect outList;

	/* If there is only are no ranges the task is simple. */
	if ( state->outList.length() > 0 ) {
		/* Loop each source range. */
		for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
			/* Reduce the transition. If it reduced to anything then add it. */
			appendTrans( outList, trans->lowKey, trans->highKey, trans );
		}
	}

	cgd->initTransList( curState, outList.length() );
	curTrans = 0;

	for ( TransListVect::Iter tvi = outList; tvi.lte(); tvi++ )
		makeTrans( tvi->lowKey, tvi->highKey, tvi->value );

	cgd->finishTransList( curState );
}


void ReducedGen::makeStateList()
{
	/* Write the list of states. */
	long length = fsm->stateList.length();
	cgd->initStateList( length );
	curState = 0;
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		makeStateActions( st );
		makeEofTrans( st );
		makeStateConditions( st );
		makeTransList( st );

		long id = st->alg.stateNum;
		cgd->setId( curState, id );

		if ( st->isFinState() )
			cgd->setFinal( curState );

		curState += 1;
	}
}


void ReducedGen::makeMachine()
{
	cgd->createMachine();

	/* Action tables. */
	reduceActionTables();

	makeActionList();
	makeActionTableList();
	makeConditions();

	/* Start State. */
	cgd->setStartState( fsm->startState->alg.stateNum );

	/* Error state. */
	if ( fsm->errState != 0 )
		cgd->setErrorState( fsm->errState->alg.stateNum );

	makeEntryPoints();
	makeStateList();

	cgd->closeMachine();
}

void ReducedGen::finishGen()
{
	/* Do this before distributing transitions out to singles and defaults
	 * makes life easier. */
	cgd->redFsm->maxKey = cgd->findMaxKey();

	cgd->redFsm->assignActionLocs();

	/* Find the first final state (The final state with the lowest id). */
	cgd->redFsm->findFirstFinState();

	/* Call the user's callback. */
	cgd->finishRagelDef();
}


CodeGenData *ReducedGen::make()
{
	/* Alphabet type. */
	cgd->setAlphType( keyOps->alphType->internalName );
	
	/* Getkey expression. */
	if ( pd->getKeyExpr != 0 ) {
		cgd->getKeyExpr = new GenInlineList;
		makeGenInlineList( cgd->getKeyExpr, pd->getKeyExpr );
	}

	/* Access expression. */
	if ( pd->accessExpr != 0 ) {
		cgd->accessExpr = new GenInlineList;
		makeGenInlineList( cgd->accessExpr, pd->accessExpr );
	}

	/* PrePush expression. */
	if ( pd->prePushExpr != 0 ) {
		cgd->prePushExpr = new GenInlineList;
		makeGenInlineList( cgd->prePushExpr, pd->prePushExpr );
	}

	/* PostPop expression. */
	if ( pd->postPopExpr != 0 ) {
		cgd->postPopExpr = new GenInlineList;
		makeGenInlineList( cgd->postPopExpr, pd->postPopExpr );
	}

	/*
	 * Variable expressions.
	 */

	if ( pd->pExpr != 0 ) {
		cgd->pExpr = new GenInlineList;
		makeGenInlineList( cgd->pExpr, pd->pExpr );
	}
	
	if ( pd->peExpr != 0 ) {
		cgd->peExpr = new GenInlineList;
		makeGenInlineList( cgd->peExpr, pd->peExpr );
	}

	if ( pd->eofExpr != 0 ) {
		cgd->eofExpr = new GenInlineList;
		makeGenInlineList( cgd->eofExpr, pd->eofExpr );
	}
	
	if ( pd->csExpr != 0 ) {
		cgd->csExpr = new GenInlineList;
		makeGenInlineList( cgd->csExpr, pd->csExpr );
	}
	
	if ( pd->topExpr != 0 ) {
		cgd->topExpr = new GenInlineList;
		makeGenInlineList( cgd->topExpr, pd->topExpr );
	}
	
	if ( pd->stackExpr != 0 ) {
		cgd->stackExpr = new GenInlineList;
		makeGenInlineList( cgd->stackExpr, pd->stackExpr );
	}
	
	if ( pd->actExpr != 0 ) {
		cgd->actExpr = new GenInlineList;
		makeGenInlineList( cgd->actExpr, pd->actExpr );
	}
	
	if ( pd->tokstartExpr != 0 ) {
		cgd->tokstartExpr = new GenInlineList;
		makeGenInlineList( cgd->tokstartExpr, pd->tokstartExpr );
	}
	
	if ( pd->tokendExpr != 0 ) {
		cgd->tokendExpr = new GenInlineList;
		makeGenInlineList( cgd->tokendExpr, pd->tokendExpr );
	}
	
	if ( pd->dataExpr != 0 ) {
		cgd->dataExpr = new GenInlineList;
		makeGenInlineList( cgd->dataExpr, pd->dataExpr );
	}
	
	makeExports();
	makeMachine();

	finishGen();

	return cgd;
}


