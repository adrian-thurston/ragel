/*
 *  Copyright 2005-2006 Adrian Thurston <thurston@cs.queensu.ca>
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

%{

#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "rlcodegen.h"
#include "vector.h"
#include "xmlparse.h"
#include "gendata.h"

using std::cerr;
using std::endl;

char *sourceFileName;
char *attrKey;
char *attrValue;
int curAction;
int curActionTable;
int curTrans;
int curState;
int curCondSpace;
int curStateCond;

Key readKey( char *td, char **end );
long readOffsetPtr( char *td, char **end );
unsigned long readLength( char *td );

CodeGenMap codeGenMap;

%}

%pure-parser

%union {
	/* General data types. */
	char c;
	char *data;
	int integer;
	AttrList *attrList;

	/* Inline parse tree items. */
	InlineItem *ilitem;
	InlineList *illist;
}

%token TAG_unknown
%token TAG_ragel
%token TAG_ragel_def
%token TAG_host
%token TAG_state_list
%token TAG_state
%token TAG_trans_list
%token TAG_t
%token TAG_machine
%token TAG_start_state
%token TAG_action_list
%token TAG_action_table_list
%token TAG_action
%token TAG_action_table
%token TAG_alphtype
%token TAG_element
%token TAG_getkey
%token TAG_state_actions
%token TAG_entry_points
%token TAG_sub_action
%token TAG_cond_space_list
%token TAG_cond_space
%token TAG_cond_list
%token TAG_c

/* Inline block tokens. */
%token TAG_text
%token TAG_goto
%token TAG_call
%token TAG_next
%token TAG_goto_expr
%token TAG_call_expr
%token TAG_next_expr
%token TAG_ret
%token TAG_pchar
%token TAG_char
%token TAG_hold
%token TAG_exec
%token TAG_holdte
%token TAG_execte
%token TAG_curs
%token TAG_targs
%token TAG_entry
%token TAG_data
%token TAG_lm_switch
%token TAG_init_act
%token TAG_set_act
%token TAG_set_tokend
%token TAG_get_tokend
%token TAG_init_tokstart
%token TAG_set_tokstart
%token TAG_write
%token TAG_curstate
%token TAG_access
%token TAG_break
%token TAG_option

%token <data> XML_Word
%token <data> XML_Literal
%type <attrList> AttributeList

%type <illist> InlineList
%type <ilitem> InlineItem
%type <illist> LmActionList

%type <ilitem> TagText
%type <ilitem> TagGoto
%type <ilitem> TagCall
%type <ilitem> TagNext
%type <ilitem> TagGotoExpr
%type <ilitem> TagCallExpr
%type <ilitem> TagNextExpr
%type <ilitem> TagRet
%type <ilitem> TagBreak
%type <ilitem> TagPChar
%type <ilitem> TagChar
%type <ilitem> TagHold
%type <ilitem> TagExec
%type <ilitem> TagHoldTE
%type <ilitem> TagExecTE
%type <ilitem> TagCurs
%type <ilitem> TagTargs
%type <ilitem> TagIlEntry
%type <ilitem> TagLmSwitch
%type <ilitem> TagLmSetActId
%type <ilitem> TagLmGetTokEnd
%type <ilitem> TagLmSetTokEnd
%type <ilitem> TagLmInitTokStart
%type <ilitem> TagLmInitAct
%type <ilitem> TagLmSetTokStart
%type <ilitem> TagInlineAction
%type <ilitem> TagSubAction

%%

/* Input is any number of input sections. An empty file is accepted. */
input: 
	TagRagel |
	/* Nothing */ {
		/* Assume the frontend died if we get no input. It will emit an error.
		 * Cause us to return an error code. */
		gblErrorCount += 1;
	};

TagRagel: 
	TagRagelHead
	HostOrDefList
	'<' '/' TAG_ragel '>';

TagRagelHead:
	'<' TAG_ragel AttributeList '>' {
		Attribute *fileNameAttr = $3->find( "filename" );
		if ( fileNameAttr == 0 )
			xml_error(@2) << "tag <ragel> requires a filename attribute" << endl;
		else
			sourceFileName = fileNameAttr->value;

		Attribute *langAttr = $3->find( "lang" );
		if ( langAttr == 0 )
			xml_error(@2) << "tag <ragel> requires a lang attribute" << endl;
		else {
			if ( strcmp( langAttr->value, "C" ) == 0 ) {
				hostLangType = CCode;
				hostLang = &hostLangC;
			}
			else if ( strcmp( langAttr->value, "D" ) == 0 ) {
				hostLangType = DCode;
				hostLang = &hostLangD;
			}
			else if ( strcmp( langAttr->value, "Java" ) == 0 ) {
				hostLangType = JavaCode;
				hostLang = &hostLangJava;
			}
		}

		/* Eventually more types will be supported. */
		if ( hostLangType == JavaCode && codeStyle != GenTables ) {
			error() << "java: only the table code style -T0 is "
						"currently supported" << endl;
		}

		openOutput( sourceFileName );
	};

AttributeList:
	AttributeList Attribute {
		$$ = $1;
		$$->append( Attribute( attrKey, attrValue ) );
	} |
	/* Nothing */ {
		$$ = new AttrList;
	};

Attribute:
	XML_Word '=' XML_Literal {
		attrKey = $1;
		attrValue = $3;
	};
	
HostOrDefList:
	HostOrDefList HostOrDef |
	/* Nothing */;

HostOrDef: 
	TagHost | TagRagelDef;

TagHost:
	TagHostHead
	'<' '/' TAG_host '>' {
		if ( outputFormat == OutCode )
			*outStream << xmlData.data;
	};

TagHostHead:
	'<' TAG_host AttributeList '>' {
		Attribute *lineAttr = $3->find( "line" );
		if ( lineAttr == 0 )
			xml_error(@2) << "tag <host> requires a line attribute" << endl;
		else {
			int line = atoi( lineAttr->value );
			if ( outputFormat == OutCode )
				lineDirective( *outStream, sourceFileName, line );
		}
	};

TagRagelDef:
	RagelDefHead
	RagelDefItemList
	'<' '/' TAG_ragel_def '>' {
		if ( gblErrorCount == 0 )
			cgd->generate();
	};

RagelDefHead:
	'<' TAG_ragel_def AttributeList '>' {
		bool wantComplete = outputFormat != OutGraphvizDot;

		char *fsmName = 0;
		Attribute *nameAttr = $3->find( "name" );
		if ( nameAttr != 0 ) {
			fsmName = nameAttr->value;

			CodeGenMapEl *mapEl = codeGenMap.find( fsmName );
			if ( mapEl != 0 )
				cgd = mapEl->value;
			else {
				cgd = new CodeGenData( sourceFileName, fsmName, wantComplete );
				codeGenMap.insert( fsmName, cgd );
			}
		}
		else {
			cgd = new CodeGenData( sourceFileName, fsmName, wantComplete );
		}

		cgd->writeOps = 0;
		cgd->writeData = false;
		cgd->writeInit = false;
		cgd->writeExec = false;
		cgd->writeEOF = false;
		::keyOps = &cgd->thisKeyOps;
	};

RagelDefItemList:
	RagelDefItemList RagelDefItem |
	/* Nothing */;

RagelDefItem:
	TagAlphType |
	TagGetKeyExpr |
	TagAccessExpr |
	TagCurStateExpr |
	TagMachine |
	TagWrite;

TagWrite:
	'<' TAG_write AttributeList '>'
	OptionList
	'<' '/' TAG_write '>' {
		Attribute *what = $3->find( "what" );
		if ( what == 0 ) {
			xml_error(@2) << "tag <write> requires a what attribute" << endl;
		}
		else {
			if ( strcmp( what->value, "data" ) == 0 )
				cgd->writeData = true;
			else if ( strcmp( what->value, "init" ) == 0 )
				cgd->writeInit = true;
			else if ( strcmp( what->value, "exec" ) == 0 )
				cgd->writeExec = true;
			else if ( strcmp( what->value, "eof" ) == 0 )
				cgd->writeEOF = true;
		}
	};

OptionList:
	OptionList TagOption |
	/* Nothing */;

TagOption:
	'<' TAG_option '>'
	'<' '/' TAG_option '>' {
		if ( strcmp( xmlData.data, "noend" ) == 0 )
			cgd->writeOps |= WO_NOEND;
	 	else if ( strcmp( xmlData.data, "noerror" ) == 0 )
			cgd->writeOps |= WO_NOERROR;
		else if ( strcmp( xmlData.data, "noprefix" ) == 0 )
			cgd->writeOps |= WO_NOPREFIX;
		else if ( strcmp( xmlData.data, "nofinal" ) == 0 )
			cgd->writeOps |= WO_NOFF;
		else {
			warning() << "unrecognized write option" << endl;
		}
	};


TagAlphType:
	'<' TAG_alphtype '>'
	'<' '/' TAG_alphtype '>' {
		if ( ! cgd->setAlphType( xmlData.data ) )
			xml_error(@2) << "tag <alphtype> specifies unknown alphabet type" << endl;
	};

TagGetKeyExpr:
	'<' TAG_getkey '>'
	InlineList
	'<' '/' TAG_getkey '>' {
		cgd->getKeyExpr = $4;
	};

TagAccessExpr:
	'<' TAG_access '>'
	InlineList
	'<' '/' TAG_access '>' {
		cgd->accessExpr = $4;
	};

TagCurStateExpr:
	'<' TAG_curstate '>'
	InlineList
	'<' '/' TAG_curstate '>' {
		cgd->curStateExpr = $4;
	};

TagMachine:
	TagMachineHead
	MachineItemList
	'<' '/' TAG_machine '>' {
		cgd->finishMachine();
	};

TagMachineHead:
	'<' TAG_machine '>' {
		cgd->createMachine();
	};

MachineItemList:
	MachineItemList MachineItem |
	/* Nothing */;

MachineItem:
	TagStartState |
	TagEntryPoints |
	TagStateList |
	TagActionList |
	TagActionTableList |
	TagCondSpaceList;

TagStartState:
	'<' TAG_start_state '>'
	'<' '/' TAG_start_state '>' {
		unsigned long startState = strtoul( xmlData.data, 0, 10 );
		cgd->setStartState( startState );
	};

TagEntryPoints:
	'<' TAG_entry_points AttributeList '>'
	EntryPointList
	'<' '/' TAG_entry_points '>' {
		Attribute *errorAttr = $3->find( "error" );
		if ( errorAttr != 0 )
			cgd->setForcedErrorState();
	};

EntryPointList:
	EntryPointList TagEntry |
	/* Nothing */;

TagEntry:
	'<' TAG_entry AttributeList '>'
	'<' '/' TAG_entry '>' {
		Attribute *nameAttr = $3->find( "name" );
		if ( nameAttr == 0 )
			xml_error(@2) << "tag <entry_points>::<entry> requires a name attribute" << endl;
		else {
			char *data = xmlData.data;
			unsigned long entry = strtoul( data, &data, 10 );
			cgd->addEntryPoint( nameAttr->value, entry );
		}
	};

TagStateList:
	TagStateListHead
	StateList
	'<' '/' TAG_state_list '>';

TagStateListHead:
	'<' TAG_state_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <state_list> requires a length attribute" << endl;
	 	else {
			unsigned long length = strtoul( lengthAttr->value, 0, 10 );
			cgd->initStateList( length );
			curState = 0;
		}
	};

StateList:
	StateList TagState |
	/* Nothing */;

TagState:
	TagStateHead
	StateItemList
	'<' '/' TAG_state '>' {
		curState += 1;
	};

TagStateHead:
	'<' TAG_state AttributeList '>' {
		Attribute *lengthAttr = $3->find( "final" );
		if ( lengthAttr != 0 )
			cgd->setFinal( curState );
	};

StateItemList:
	StateItemList StateItem |
	/* Nothing */;

StateItem:
	TagStateActions |
	TagStateCondList |
	TagTransList;

TagStateActions:
	'<' TAG_state_actions '>'
	'<' '/' TAG_state_actions '>' {
		char *ad = xmlData.data;

		long toStateAction = readOffsetPtr( ad, &ad );
		long fromStateAction = readOffsetPtr( ad, &ad );
		long eofAction = readOffsetPtr( ad, &ad );

		cgd->setStateActions( curState, toStateAction,
				fromStateAction, eofAction );
	};

TagStateCondList:
	TagStateCondListHead
	StateCondList
	'<' '/' TAG_cond_list '>';

TagStateCondListHead:
	'<' TAG_cond_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <cond_list> requires a length attribute" << endl;
	 	else {
			ulong length = readLength( lengthAttr->value );
			cgd->initStateCondList( curState, length );
			curStateCond = 0;
		}
	}

StateCondList:
	StateCondList StateCond |
	/* Empty */;

StateCond:
	'<' TAG_c '>'
	'<' '/' TAG_c '>' {
		char *td = xmlData.data;
		Key lowKey = readKey( td, &td );
		Key highKey = readKey( td, &td );
		long condId = readOffsetPtr( td, &td );
		cgd->addStateCond( curState, lowKey, highKey, condId );
	}
	
TagTransList:
	TagTransListHead
	TransList
	'<' '/' TAG_trans_list '>' {
		cgd->finishTransList( curState );
	};

TagTransListHead:
	'<' TAG_trans_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <trans_list> requires a length attribute" << endl;
	 	else {
			unsigned long length = strtoul( lengthAttr->value, 0, 10 );
			cgd->initTransList( curState, length );
			curTrans = 0;
		}
	};

TransList:
	TransList TagTrans |
	/* Nothing */;

TagTrans:
	'<' TAG_t AttributeList '>'
	'<' '/' TAG_t '>' {
		char *td = xmlData.data;
		Key lowKey = readKey( td, &td );
		Key highKey = readKey( td, &td );
		long targ = readOffsetPtr( td, &td );
		long action = readOffsetPtr( td, &td );

		cgd->newTrans( curState, curTrans++, lowKey, highKey, targ, action );
	};

TagActionList:
	TagActionListHead
	ActionList
	'<' '/' TAG_action_list '>';

TagActionListHead:
	'<' TAG_action_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <action_list> requires a length attribute" << endl;
	 	else {
			unsigned long length = strtoul( lengthAttr->value, 0, 10 );
			cgd->initActionList( length );
			curAction = 0;
		}
	};

ActionList:
	ActionList TagAction |
	/* Nothing */;

TagAction:
	'<' TAG_action AttributeList '>'
	InlineList
	'<' '/' TAG_action '>' {
		Attribute *lineAttr = $3->find( "line" );
		Attribute *colAttr = $3->find( "col" );
		Attribute *nameAttr = $3->find( "name" );
		if ( lineAttr == 0 || colAttr == 0)
			xml_error(@2) << "tag <action> requires a line and col attributes" << endl;
	 	else {
			unsigned long line = strtoul( lineAttr->value, 0, 10 );
			unsigned long col = strtoul( colAttr->value, 0, 10 );

			char *name = 0;
			if ( nameAttr != 0 )
				name = nameAttr->value;

			cgd->newAction( curAction++, name, line, col, $5 );
		}
	};

InlineList:
	InlineList InlineItem {
		/* Append the item to the list, return the list. */
		$1->append( $2 );
		$$ = $1;
	} |
	/* Nothing */ {
		/* Start with empty list. */
		$$ = new InlineList;
	};

InlineItem:
	TagText |
	TagGoto |
	TagCall |
	TagNext |
	TagGotoExpr |
	TagCallExpr |
	TagNextExpr |
	TagRet |
	TagBreak |
	TagPChar |
	TagChar |
	TagHold |
	TagExec |
	TagHoldTE |
	TagExecTE |
	TagCurs |
	TagTargs |
	TagIlEntry |
	TagLmSwitch |
	TagLmSetActId |
	TagLmSetTokEnd |
	TagLmGetTokEnd |
	TagSubAction |
	TagLmInitTokStart |
	TagLmInitAct |
	TagLmSetTokStart;

TagText:
	'<' TAG_text AttributeList '>'
	'<' '/' TAG_text '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Text );
		$$->data = strdup(xmlData.data);
	};

TagGoto:
	'<' TAG_goto '>'
	'<' '/' TAG_goto '>' {
		int targ = strtol( xmlData.data, 0, 10 );
		$$ = new InlineItem( InputLoc(), InlineItem::Goto );
		$$->targId = targ;
	};
	
TagCall:
	'<' TAG_call '>'
	'<' '/' TAG_call '>' {
		int targ = strtol( xmlData.data, 0, 10 );
		$$ = new InlineItem( InputLoc(), InlineItem::Call );
		$$->targId = targ;
	};

TagNext:
	'<' TAG_next '>'
	'<' '/' TAG_next '>' {
		int targ = strtol( xmlData.data, 0, 10 );
		$$ = new InlineItem( InputLoc(), InlineItem::Next );
		$$->targId = targ;
	};

TagGotoExpr:
	'<' TAG_goto_expr '>'
	InlineList
	'<' '/' TAG_goto_expr '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::GotoExpr );
		$$->children = $4;
	};
	
TagCallExpr:
	'<' TAG_call_expr '>'
	InlineList
	'<' '/' TAG_call_expr '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::CallExpr );
		$$->children = $4;
	};

TagNextExpr:
	'<' TAG_next_expr '>'
	InlineList
	'<' '/' TAG_next_expr '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::NextExpr );
		$$->children = $4;
	};

TagRet:
	'<' TAG_ret '>'
	'<' '/' TAG_ret '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Ret );
	};
	
TagPChar:
	'<' TAG_pchar '>'
	'<' '/' TAG_pchar '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::PChar );
	};

TagChar:
	'<' TAG_char '>'
	'<' '/' TAG_char '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Char );
	};

TagHold:
	'<' TAG_hold '>'
	'<' '/' TAG_hold '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Hold );
	};

TagExec:
	'<' TAG_exec '>'
	InlineList
	'<' '/' TAG_exec '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Exec );
		$$->children = $4;
	};

TagHoldTE:
	'<' TAG_holdte '>'
	'<' '/' TAG_holdte '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::HoldTE );
	};

TagExecTE:
	'<' TAG_execte '>'
	InlineList
	'<' '/' TAG_execte '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::ExecTE );
		$$->children = $4;
	};

TagCurs:
	'<' TAG_curs '>'
	'<' '/' TAG_curs '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Curs );
	};

TagTargs:
	'<' TAG_targs '>'
	'<' '/' TAG_targs '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Targs );
	};

TagIlEntry:
	'<' TAG_entry '>'
	'<' '/' TAG_entry '>' {
		int targ = strtol( xmlData.data, 0, 10 );
		$$ = new InlineItem( InputLoc(), InlineItem::Entry );
		$$->targId = targ;
	};

TagBreak:
	'<' TAG_break '>'
	'<' '/' TAG_break '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::Break );
	};
	

TagLmSwitch:
	'<' TAG_lm_switch AttributeList '>'
	LmActionList
	'<' '/' TAG_lm_switch '>' {
		bool handlesError = false;
		Attribute *handlesErrorAttr = $3->find( "handles_error" );
		if ( handlesErrorAttr != 0 )
			handlesError = true;

		$$ = new InlineItem( InputLoc(), InlineItem::LmSwitch );
		$$->children = $5;
		$$->handlesError = handlesError;
	};

LmActionList:
	LmActionList TagInlineAction {
		$$ = $1;
		$$->append( $2 );
	} |
	/* Nothing */ {
		$$ = new InlineList;
	};

TagInlineAction:
	'<' TAG_sub_action AttributeList '>'
	InlineList
	'<' '/' TAG_sub_action '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::SubAction );
		$$->children = $5;

		Attribute *idAttr = $3->find( "id" );
		if ( idAttr != 0 ) {
			unsigned long id = strtoul( idAttr->value, 0, 10 );
			$$->lmId = id;
		}
	};

TagLmSetActId:
	'<' TAG_set_act '>'
	'<' '/' TAG_set_act '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmSetActId );
		$$->lmId = strtol( xmlData.data, 0, 10 );
	};

TagLmGetTokEnd:
	'<' TAG_get_tokend '>'
	'<' '/' TAG_get_tokend '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmGetTokEnd );
	};

TagLmSetTokEnd:
	'<' TAG_set_tokend '>'
	'<' '/' TAG_set_tokend '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmSetTokEnd );
		$$->offset = strtol( xmlData.data, 0, 10 );
	};

TagSubAction:
	'<' TAG_sub_action '>'
	InlineList
	'<' '/' TAG_sub_action '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::SubAction );
		$$->children = $4;
	};

TagLmInitTokStart:
	'<' TAG_init_tokstart '>'
	'<' '/' TAG_init_tokstart '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmInitTokStart );
	};

TagLmInitAct:
	'<' TAG_init_act '>'
	'<' '/' TAG_init_act '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmInitAct );
	};

TagLmSetTokStart:
	'<' TAG_set_tokstart '>'
	'<' '/' TAG_set_tokstart '>' {
		$$ = new InlineItem( InputLoc(), InlineItem::LmSetTokStart );
		cgd->hasLongestMatch = true;
	};

TagActionTableList:
	TagActionTableListHead
	ActionTableList
	'<' '/' TAG_action_table_list '>';

TagActionTableListHead:
	'<' TAG_action_table_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <action_table_list> requires a length attribute" << endl;
	 	else {
			unsigned long length = strtoul( lengthAttr->value, 0, 10 );
			cgd->initActionTableList( length );
			curActionTable = 0;
		}
	};

ActionTableList:
	ActionTableList TagActionTable |
	/* Nothing */;

TagActionTable:
	'<' TAG_action_table AttributeList '>'
	'<' '/' TAG_action_table '>' {
		/* Find the length of the action table. */
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <at> requires a length attribute" << endl;
	 	else {
			unsigned long length = strtoul( lengthAttr->value, 0, 10 );

			/* Collect the action table. */
			RedAction *redAct = cgd->allActionTables + curActionTable;
			redAct->actListId = curActionTable;
			redAct->key.setAsNew( length );
			char *ptr = xmlData.data;
			int pos = 0;
			while ( *ptr != 0 ) {
				unsigned long actionId = strtoul( ptr, &ptr, 10 );
				redAct->key[pos].key = 0;
				redAct->key[pos].value = cgd->allActions+actionId;
				pos += 1;
			}

			/* Insert into the action table map. */
			cgd->redFsm->actionMap.insert( redAct );
		}

		curActionTable += 1;
	};

TagCondSpaceList:
	TagCondSpaceListHead
	CondSpaceList
	'<' '/' TAG_cond_space_list '>';

TagCondSpaceListHead:
	'<' TAG_cond_space_list AttributeList '>' {
		Attribute *lengthAttr = $3->find( "length" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <cond_space_list> requires a length attribute" << endl;
	 	else {
			ulong length = readLength( lengthAttr->value );
			cgd->initCondSpaceList( length );
			curCondSpace = 0;
		}
	};

CondSpaceList: 
	CondSpaceList TagCondSpace |
	TagCondSpace;

TagCondSpace:
	'<' TAG_cond_space AttributeList '>'
	'<' '/' TAG_cond_space '>' {
		Attribute *lengthAttr = $3->find( "length" );
		Attribute *idAttr = $3->find( "id" );
		if ( lengthAttr == 0 )
			xml_error(@2) << "tag <cond_space> requires a length attribute" << endl;
	 	else {
			if ( lengthAttr == 0 )
				xml_error(@2) << "tag <cond_space> requires an id attribute" << endl;
		 	else {
				unsigned long condSpaceId = strtoul( idAttr->value, 0, 10 );
				ulong length = readLength( lengthAttr->value );

				char *td = xmlData.data;
				Key baseKey = readKey( td, &td );

				cgd->newCondSpace( curCondSpace, condSpaceId, baseKey );
				for ( ulong a = 0; a < length; a++ ) {
					long actionOffset = readOffsetPtr( td, &td );
					cgd->condSpaceItem( curCondSpace, actionOffset );
				}
				curCondSpace += 1;
			}
		}
	};

%%

unsigned long readLength( char *td )
{
	return strtoul( td, 0, 10 );
}

Key readKey( char *td, char **end )
{
	if ( keyOps->isSigned )
		return Key( strtol( td, end, 10 ) );
	else
		return Key( strtoul( td, end, 10 ) );
}

long readOffsetPtr( char *td, char **end )
{
	while ( *td == ' ' || *td == '\t' )
		td++;

	if ( *td == 'x' ) {
		if ( end != 0 )
			*end = td + 1;
		return -1;
	}

	return strtol( td, end, 10 );
}

void yyerror( char *err )
{
	/* Bison won't give us the location, but in the last call to the scanner we
	 * saved a pointer to the locationn variable. Use that. instead. */
	error(::yylloc->first_line, ::yylloc->first_column) << err << endl;
}

