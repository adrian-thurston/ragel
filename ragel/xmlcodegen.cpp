/*
 *  Copyright 2005-2007 Adrian Thurston <thurston@cs.queensu.ca>
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
#include "xmlcodegen.h"
#include "parsedata.h"
#include "fsmgraph.h"
#include <string.h>

using namespace std;

XMLCodeGen::XMLCodeGen( char *fsmName, ParseData *pd, FsmAp *fsm, 
		std::ostream &out )
:
	fsmName(fsmName),
	pd(pd),
	fsm(fsm),
	out(out),
	nextActionTableId(0)
{
}


void XMLCodeGen::writeActionList()
{
	/* Determine which actions to write. */
	int nextActionId = 0;
	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->numRefs() > 0 || act->numCondRefs > 0 )
			act->actionId = nextActionId++;
	}

	/* Write the list. */
	out << "    <action_list length=\"" << nextActionId << "\">\n";
	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->actionId >= 0 )
			writeAction( act );
	}
	out << "    </action_list>\n";
}

void XMLCodeGen::writeActionTableList()
{
	/* Must first order the action tables based on their id. */
	int numTables = nextActionTableId;
	RedActionTable **tables = new RedActionTable*[numTables];
	for ( ActionTableMap::Iter at = actionTableMap; at.lte(); at++ )
		tables[at->id] = at;

	out << "    <action_table_list length=\"" << numTables << "\">\n";
	for ( int t = 0; t < numTables; t++ ) {
		out << "      <action_table id=\"" << t << "\" length=\"" << 
				tables[t]->key.length() << "\">";
		for ( ActionTable::Iter atel = tables[t]->key; atel.lte(); atel++ ) {
			out << atel->value->actionId;
			if ( ! atel.last() )
				out << " ";
		}
		out << "</action_table>\n";
	}
	out << "    </action_table_list>\n";

	delete[] tables;
}

void XMLCodeGen::reduceActionTables()
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
			if ( trans->actionTable.length() > 0 ) {
				if ( actionTableMap.insert( trans->actionTable, &actionTable ) )
					actionTable->id = nextActionTableId++;
			}
		}
	}
}

void XMLCodeGen::appendTrans( TransListVect &outList, Key lowKey, 
		Key highKey, TransAp *trans )
{
	if ( trans->toState != 0 || trans->actionTable.length() > 0 )
		outList.append( TransEl( lowKey, highKey, trans ) );
}

void XMLCodeGen::writeKey( Key key )
{
	if ( keyOps->isSigned )
		out << key.getVal();
	else
		out << (unsigned long) key.getVal();
}

void XMLCodeGen::writeTrans( Key lowKey, Key highKey, TransAp *trans )
{
	/* First reduce the action. */
	RedActionTable *actionTable = 0;
	if ( trans->actionTable.length() > 0 )
		actionTable = actionTableMap.find( trans->actionTable );

	/* Write the transition. */
	out << "        <t>";
	writeKey( lowKey );
	out << " ";
	writeKey( highKey );

	if ( trans->toState != 0 )
		out << " " << trans->toState->alg.stateNum;
	else
		out << " x";

	if ( actionTable != 0 )
		out << " " << actionTable->id;
	else
		out << " x";
	out << "</t>\n";
}

void XMLCodeGen::writeTransList( StateAp *state )
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

	out << "      <trans_list length=\"" << outList.length() << "\">\n";
	for ( TransListVect::Iter tvi = outList; tvi.lte(); tvi++ )
		writeTrans( tvi->lowKey, tvi->highKey, tvi->value );
	out << "      </trans_list>\n";
}

void XMLCodeGen::writeText( InlineItem *item )
{
	if ( item->prev == 0 || item->prev->type != InlineItem::Text )
		out << "<text>";
	xmlEscapeHost( out, item->data, strlen(item->data) );
	if ( item->next == 0 || item->next->type != InlineItem::Text )
		out << "</text>";
}

void XMLCodeGen::writeGoto( InlineItem *item )
{
	if ( pd->generatingSectionSubset )
		out << "<goto>-1</goto>";
	else {
		EntryMapEl *targ = fsm->entryPoints.find( item->nameTarg->id );
		out << "<goto>" << targ->value->alg.stateNum << "</goto>";
	}
}

void XMLCodeGen::writeCall( InlineItem *item )
{
	if ( pd->generatingSectionSubset )
		out << "<call>-1</call>";
	else {
		EntryMapEl *targ = fsm->entryPoints.find( item->nameTarg->id );
		out << "<call>" << targ->value->alg.stateNum << "</call>";
	}
}

void XMLCodeGen::writeNext( InlineItem *item )
{
	if ( pd->generatingSectionSubset )
		out << "<next>-1</next>";
	else {
		EntryMapEl *targ = fsm->entryPoints.find( item->nameTarg->id );
		out << "<next>" << targ->value->alg.stateNum << "</next>";
	}
}

void XMLCodeGen::writeGotoExpr( InlineItem *item )
{
	out << "<goto_expr>";
	writeInlineList( item->children );
	out << "</goto_expr>";
}

void XMLCodeGen::writeCallExpr( InlineItem *item )
{
	out << "<call_expr>";
	writeInlineList( item->children );
	out << "</call_expr>";
}

void XMLCodeGen::writeNextExpr( InlineItem *item )
{
	out << "<next_expr>";
	writeInlineList( item->children );
	out << "</next_expr>";
}

void XMLCodeGen::writeEntry( InlineItem *item )
{
	if ( pd->generatingSectionSubset )
		out << "<entry>-1</entry>";
	else {
		EntryMapEl *targ = fsm->entryPoints.find( item->nameTarg->id );
		out << "<entry>" << targ->value->alg.stateNum << "</entry>";
	}
}

void XMLCodeGen::writeActionExec( InlineItem *item )
{
	out << "<exec>";
	writeInlineList( item->children );
	out << "</exec>";
}

void XMLCodeGen::writeLmOnLast( InlineItem *item )
{
	out << "<set_tokend>1</set_tokend>";

	if ( item->longestMatchPart->action != 0 ) {
		out << "<sub_action>";
		writeInlineList( item->longestMatchPart->action->inlineList );
		out << "</sub_action>";
	}
}

void XMLCodeGen::writeLmOnNext( InlineItem *item )
{
	out << "<set_tokend>0</set_tokend>";
	out << "<hold></hold>";

	if ( item->longestMatchPart->action != 0 ) {
		out << "<sub_action>";
		writeInlineList( item->longestMatchPart->action->inlineList );
		out << "</sub_action>";
	}
}

void XMLCodeGen::writeLmOnLagBehind( InlineItem *item )
{
	out << "<exec><get_tokend></get_tokend></exec>";

	if ( item->longestMatchPart->action != 0 ) {
		out << "<sub_action>";
		writeInlineList( item->longestMatchPart->action->inlineList );
		out << "</sub_action>";
	}
}

void XMLCodeGen::writeLmSwitch( InlineItem *item )
{
	LongestMatch *longestMatch = item->longestMatch;
	out << "<lm_switch>\n";

	if ( longestMatch->lmSwitchHandlesError ) {
		/* If the switch handles error then we should have also forced the
		 * error state. */
		assert( fsm->errState != 0 );

		out << "        <sub_action id=\"0\">";
		out << "<goto>" << fsm->errState->alg.stateNum << "</goto>";
		out << "</sub_action>\n";
	}
	
	for ( LmPartList::Iter lmi = *longestMatch->longestMatchList; lmi.lte(); lmi++ ) {
		if ( lmi->inLmSelect && lmi->action != 0 ) {
			/* Open the action. Write it with the context that sets up _p 
			 * when doing control flow changes from inside the machine. */
			out << "        <sub_action id=\"" << lmi->longestMatchId << "\">";
			out << "<exec><get_tokend></get_tokend></exec>";
			writeInlineList( lmi->action->inlineList );
			out << "</sub_action>\n";
		}
	}

	out << "    </lm_switch>";
}

void XMLCodeGen::writeInlineList( InlineList *inlineList )
{
	for ( InlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			writeText( item );
			break;
		case InlineItem::Goto:
			writeGoto( item );
			break;
		case InlineItem::GotoExpr:
			writeGotoExpr( item );
			break;
		case InlineItem::Call:
			writeCall( item );
			break;
		case InlineItem::CallExpr:
			writeCallExpr( item );
			break;
		case InlineItem::Next:
			writeNext( item );
			break;
		case InlineItem::NextExpr:
			writeNextExpr( item );
			break;
		case InlineItem::Break:
			out << "<break></break>";
			break;
		case InlineItem::Ret: 
			out << "<ret></ret>";
			break;
		case InlineItem::PChar:
			out << "<pchar></pchar>";
			break;
		case InlineItem::Char: 
			out << "<char></char>";
			break;
		case InlineItem::Curs: 
			out << "<curs></curs>";
			break;
		case InlineItem::Targs: 
			out << "<targs></targs>";
			break;
		case InlineItem::Entry:
			writeEntry( item );
			break;

		case InlineItem::Hold:
			out << "<hold></hold>";
			break;
		case InlineItem::Exec:
			writeActionExec( item );
			break;

		case InlineItem::LmSetActId:
			out << "<set_act>" << 
					item->longestMatchPart->longestMatchId << 
					"</set_act>";
			break;
		case InlineItem::LmSetTokEnd:
			out << "<set_tokend>1</set_tokend>";
			break;

		case InlineItem::LmOnLast:
			writeLmOnLast( item );
			break;
		case InlineItem::LmOnNext:
			writeLmOnNext( item );
			break;
		case InlineItem::LmOnLagBehind:
			writeLmOnLagBehind( item );
			break;
		case InlineItem::LmSwitch: 
			writeLmSwitch( item );
			break;

		case InlineItem::LmInitAct:
			out << "<init_act></init_act>";
			break;
		case InlineItem::LmInitTokStart:
			out << "<init_tokstart></init_tokstart>";
			break;
		case InlineItem::LmSetTokStart:
			out << "<set_tokstart></set_tokstart>";
			break;
		}
	}
}

void XMLCodeGen::writeAction( Action *action )
{
	out << "      <action id=\"" << action->actionId << "\"";
	if ( action->name != 0 ) 
		out << " name=\"" << action->name << "\"";
	out << " line=\"" << action->loc.line << "\" col=\"" << action->loc.col << "\">";
	writeInlineList( action->inlineList );
	out << "</action>\n";
}

void xmlEscapeHost( std::ostream &out, char *data, long len )
{
	char *end = data + len;
	while ( data != end ) {
		switch ( *data ) {
		case '<': out << "&lt;"; break;
		case '>': out << "&gt;"; break;
		case '&': out << "&amp;"; break;
		default: out << *data; break;
		}
		data += 1;
	}
}

void XMLCodeGen::writeStateActions( StateAp *state )
{
	RedActionTable *toStateActions = 0;
	if ( state->toStateActionTable.length() > 0 )
		toStateActions = actionTableMap.find( state->toStateActionTable );

	RedActionTable *fromStateActions = 0;
	if ( state->fromStateActionTable.length() > 0 )
		fromStateActions = actionTableMap.find( state->fromStateActionTable );

	RedActionTable *eofActions = 0;
	if ( state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	if ( toStateActions != 0 || fromStateActions != 0 || eofActions != 0 ) {
		out << "      <state_actions>";
		if ( toStateActions != 0 )
			out << toStateActions->id;
		else
			out << "x";

		if ( fromStateActions != 0 )
			out << " " << fromStateActions->id;
		else
			out << " x";

		if ( eofActions != 0 )
			out << " " << eofActions->id;
		else
			out << " x"; out << "</state_actions>\n";
	}
}

void XMLCodeGen::writeStateConditions( StateAp *state )
{
	if ( state->stateCondList.length() > 0 ) {
		out << "      <cond_list length=\"" << state->stateCondList.length() << "\">\n";
		for ( StateCondList::Iter scdi = state->stateCondList; scdi.lte(); scdi++ ) {
			out << "        <c>";
			writeKey( scdi->lowKey );
			out << " ";
			writeKey( scdi->highKey );
			out << " ";
			out << scdi->condSpace->condSpaceId;
			out << "</c>\n";
		}
		out << "      </cond_list>\n";
	}
}

void XMLCodeGen::writeStateList()
{
	/* Write the list of states. */
	out << "    <state_list length=\"" << fsm->stateList.length() << "\">\n";
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		out << "      <state id=\"" << st->alg.stateNum << "\"";
		if ( st->isFinState() )
			out << " final=\"t\"";
		out << ">\n";

		writeStateActions( st );
		writeStateConditions( st );
		writeTransList( st );

		out << "      </state>\n";

		if ( !st.last() )
			out << "\n";
	}
	out << "    </state_list>\n";
}

bool XMLCodeGen::writeNameInst( NameInst *nameInst )
{
	bool written = false;
	if ( nameInst->parent != 0 )
		written = writeNameInst( nameInst->parent );
	
	if ( nameInst->name != 0 ) {
		if ( written )
			out << '_';
		out << nameInst->name;
		written = true;
	}

	return written;
}

void XMLCodeGen::writeEntryPoints()
{
	/* List of entry points other than start state. */
	if ( fsm->entryPoints.length() > 0 || pd->lmRequiresErrorState ) {
		out << "    <entry_points";
		if ( pd->lmRequiresErrorState )
			out << " error=\"t\"";
		out << ">\n";
		for ( EntryMap::Iter en = fsm->entryPoints; en.lte(); en++ ) {
			/* Get the name instantiation from nameIndex. */
			NameInst *nameInst = pd->nameIndex[en->key];
			StateAp *state = en->value;
			out << "      <entry name=\"";
			writeNameInst( nameInst );
			out << "\">" << state->alg.stateNum << "</entry>\n";
		}
		out << "    </entry_points>\n";
	}
}

void XMLCodeGen::writeMachine()
{
	/* Open the machine. */
	out << "  <machine>\n"; 
	
	/* Action tables. */
	reduceActionTables();

	writeActionList();
	writeActionTableList();
	writeConditions();

	/* Start state. */
	out << "    <start_state>" << fsm->startState->alg.stateNum << 
			"</start_state>\n";
	
	/* Error state. */
	if ( fsm->errState != 0 ) {
		out << "    <error_state>" << fsm->errState->alg.stateNum << 
			"</error_state>\n";
	}

	writeEntryPoints();
	writeStateList();

	out << "  </machine>\n";
}


void XMLCodeGen::writeConditions()
{
	if ( condData->condSpaceMap.length() > 0 ) {
		long nextCondSpaceId = 0;
		for ( CondSpaceMap::Iter cs = condData->condSpaceMap; cs.lte(); cs++ )
			cs->condSpaceId = nextCondSpaceId++;

		out << "    <cond_space_list length=\"" << condData->condSpaceMap.length() << "\">\n";
		for ( CondSpaceMap::Iter cs = condData->condSpaceMap; cs.lte(); cs++ ) {
			out << "      <cond_space id=\"" << cs->condSpaceId << 
				"\" length=\"" << cs->condSet.length() << "\">";
			writeKey( cs->baseKey );
			for ( CondSet::Iter csi = cs->condSet; csi.lte(); csi++ )
				out << " " << (*csi)->actionId;
			out << "</cond_space>\n";
		}
		out << "    </cond_space_list>\n";
	}
}

void XMLCodeGen::writeExports()
{
	if ( pd->exportList.length() > 0 ) {
		out << "  <exports>\n";
		for ( ExportList::Iter exp = pd->exportList; exp.lte(); exp++ ) {
			out << "    <ex name=\"" << exp->name << "\">";
			writeKey( exp->key );
			out << "</ex>\n";
		}
		out << "  </exports>\n";
	}
}

void XMLCodeGen::writeXML()
{
	/* Open the definition. */
	out << "<ragel_def name=\"" << fsmName << "\">\n";

	/* Alphabet type. */
	out << "  <alphtype>" << keyOps->alphType->internalName << "</alphtype>\n";
	
	/* Getkey expression. */
	if ( pd->getKeyExpr != 0 ) {
		out << "  <getkey>";
		writeInlineList( pd->getKeyExpr );
		out << "</getkey>\n";
	}

	/* Access expression. */
	if ( pd->accessExpr != 0 ) {
		out << "  <access>";
		writeInlineList( pd->accessExpr );
		out << "</access>\n";
	}

	/* PrePush expression. */
	if ( pd->prePushExpr != 0 ) {
		out << "  <prepush>";
		writeInlineList( pd->prePushExpr );
		out << "</prepush>\n";
	}

	/* PostPop expression. */
	if ( pd->postPopExpr != 0 ) {
		out << "  <postpop>";
		writeInlineList( pd->postPopExpr );
		out << "</postpop>\n";
	}

	/*
	 * Variable expressions.
	 */

	if ( pd->pExpr != 0 ) {
		out << "  <p_expr>";
		writeInlineList( pd->pExpr );
		out << "</p_expr>\n";
	}
	
	if ( pd->peExpr != 0 ) {
		out << "  <pe_expr>";
		writeInlineList( pd->peExpr );
		out << "</pe_expr>\n";
	}
	
	if ( pd->csExpr != 0 ) {
		out << "  <cs_expr>";
		writeInlineList( pd->csExpr );
		out << "</cs_expr>\n";
	}
	
	if ( pd->topExpr != 0 ) {
		out << "  <top_expr>";
		writeInlineList( pd->topExpr );
		out << "</top_expr>\n";
	}
	
	if ( pd->stackExpr != 0 ) {
		out << "  <stack_expr>";
		writeInlineList( pd->stackExpr );
		out << "</stack_expr>\n";
	}
	
	if ( pd->actExpr != 0 ) {
		out << "  <act_expr>";
		writeInlineList( pd->actExpr );
		out << "</act_expr>\n";
	}
	
	if ( pd->tokstartExpr != 0 ) {
		out << "  <tokstart_expr>";
		writeInlineList( pd->tokstartExpr );
		out << "</tokstart_expr>\n";
	}
	
	if ( pd->tokendExpr != 0 ) {
		out << "  <tokend_expr>";
		writeInlineList( pd->tokendExpr );
		out << "</tokend_expr>\n";
	}
	
	if ( pd->dataExpr != 0 ) {
		out << "  <data_expr>";
		writeInlineList( pd->dataExpr );
		out << "</data_expr>\n";
	}
	
	writeExports();
	
	writeMachine();

	out <<
		"</ragel_def>\n";
}

