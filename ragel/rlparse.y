/*
 *  Copyright 2001-2005 Adrian Thurston <thurston@cs.queensu.ca>
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
#include "ragel.h"
#include "parsetree.h"
#include "rlparse.h"

using std::cerr;
using std::endl;

InputData *id = 0;
int includeDepth = 0;

extern bool inlineWhitespace;

/* These come from the scanner and point back into the parser. We will borrow
 * them for error reporting. */
extern YYSTYPE *yylval;
extern YYLTYPE *yylloc;

/* The include stack pointer from the scanner. Used to determine if we are
 * currently processing an included file. */
extern int inc_stack_ptr;

/* Try to do a definition, common to assignment and instantiation. */
void tryMachineDef( const YYLTYPE &loc, char *name, 
		JoinOrLm *joinOrLm, bool isInstance );
void beginOutsideCode();
void doInclude( const InputLoc &loc, char *sectionName, char *inputFile );
int yylex( YYSTYPE *yylval, YYLTYPE *yylloc );

bool sectionOpened;
void openSection();

#define WO_NOEND 0x01

%}

%pure-parser

%union {
	/* General data types. */
	char c;
	TokenData data;
	int integer;
	Literal *literal;

	/* Tree nodes. */
	Term *term;
	FactorWithAug *factorWithAug;
	FactorWithRep *factorWithRep;
	FactorWithNeg *factorWithNeg;
	Factor *factor;
	Expression *expression;
	Join *join;
	JoinOrLm *joinOrLm;
	LmPartList *longestMatchList;
	LongestMatchPart *longestMatchPart;

	/* Priorities and actions. */
	AugType augType;
	StateAugType stateAugType;
	Action *action;
	PriorDesc *priorDesc;

	/* Regular expression items. */
	RegExpr *regExp;
	ReItem *reItem;
	ReOrBlock *reOrBlock;
	ReOrItem *reOrItem;

	/* Inline parse tree items. */
	InlineItem *ilitem;
	InlineList *illist;
}

%token TK_Section
%token TK_SectionNL

/* General tokens. */
%token <data> TK_UInt
%token <data> TK_Hex
%token <data> TK_Word
%token <data> TK_Literal
%token <data> TK_CiLiteral
%token <data> TK_BaseClause
%token TK_DotDot
%token TK_ColonGt
%token TK_ColonGtGt
%token TK_LtColon
%token TK_Arrow
%token TK_DoubleArrow
%token TK_StarStar
%token TK_ColonEquals
%token TK_NameSep
%token TK_BarStar
%token TK_RepOpOpen
%token TK_DashDash

%token TK_StartCond
%token TK_AllCond
%token TK_LeavingCond

%token TK_Middle

/* Global error actions. */
%token TK_StartGblError
%token TK_AllGblError
%token TK_FinalGblError
%token TK_NotFinalGblError
%token TK_NotStartGblError
%token TK_MiddleGblError

/* Local error actions. */
%token TK_StartLocalError
%token TK_AllLocalError
%token TK_FinalLocalError
%token TK_NotFinalLocalError
%token TK_NotStartLocalError
%token TK_MiddleLocalError

/* EOF Action embedding. */
%token TK_StartEOF
%token TK_AllEOF
%token TK_FinalEOF
%token TK_NotFinalEOF
%token TK_NotStartEOF
%token TK_MiddleEOF

/* To State Actions. */
%token TK_StartToState
%token TK_AllToState
%token TK_FinalToState
%token TK_NotFinalToState
%token TK_NotStartToState
%token TK_MiddleToState

/* In State Actions. */
%token TK_StartFromState
%token TK_AllFromState
%token TK_FinalFromState
%token TK_NotFinalFromState
%token TK_NotStartFromState
%token TK_MiddleFromState

/* Regular expression tokens. */
%token <data> RE_Slash
%token RE_SqOpen
%token RE_SqOpenNeg
%token RE_SqClose
%token RE_Dot
%token RE_Star
%token RE_Dash
%token <data> RE_Char

/* Tokens specific to inline code. */
%token <data> IL_WhiteSpace
%token <data> IL_Comment
%token <data> IL_Literal
%token <data> IL_Symbol

/* Keywords. */
%token KW_Action
%token KW_AlphType
%token KW_Range
%token KW_GetKey
%token KW_Include
%token KW_Write
%token KW_Machine
%token KW_When
%token KW_Eof
%token KW_Err
%token KW_Lerr
%token KW_To
%token KW_From

/* Specials in code blocks. */
%token KW_Break
%token KW_Exec
%token KW_Hold
%token KW_PChar
%token KW_Char
%token KW_Goto
%token KW_Call
%token KW_Ret
%token KW_CurState
%token KW_TargState
%token KW_Entry
%token KW_Next
%token KW_Exec
%token<data> KW_Variable
%token KW_Access

/* Special token for terminating semi-terminated code blocks. Needed because
 * semi is sent as a token in the code block rather than as a generic symbol. */
%token TK_Semi

/* Symbols. In ragel lexical space, the scanner does not pass 
 * any data along with the symbols, in inline code lexical 
 * space it does. */
%token '*' '?' '+' '!' '^' '(' ')' ';' ',' '=' 
%token ':' '@' '%' '$' '-' '|' '&' '.' '>'

/* Precedence information. Lower is a higher precedence. We need only two
 * precedence groups. Shifting the minus sign in front of a literal number
 * conflicts with the reduction of Expression and the subsequent shifting of a
 * subtraction operator when a '-' is seen. Since we want subtraction to take
 * precedence, we give EXPR_MINUS the higher priority. */
%nonassoc '-'
%nonassoc EXPR_MINUS

%type <augType> AugTypeBase
%type <augType> AugTypeGblError
%type <augType> AugTypeLocalError
%type <augType> AugTypeEOF
%type <augType> AugTypeToState
%type <augType> AugTypeFromState
%type <augType> AugTypeCond
%type <integer> PriorityAug
%type <data> PriorityAugNum
%type <action> ActionEmbed
%type <action> ActionEmbedWord
%type <action> ActionEmbedBlock
%type <action> OptLmPartAction
%type <longestMatchList> LmPartList
%type <longestMatchPart> LongestMatchPart
%type <join> Join
%type <joinOrLm> JoinOrLm
%type <expression> Expression
%type <term> Term
%type <factorWithAug> FactorWithLabel
%type <factorWithAug> FactorWithEp
%type <factorWithAug> FactorWithAug
%type <factorWithAug> FactorWithTransAction
%type <factorWithAug> FactorWithPriority
%type <factorWithAug> FactorWithCond
%type <factorWithAug> FactorWithToStateAction
%type <factorWithAug> FactorWithFromStateAction
%type <factorWithAug> FactorWithEOFAction
%type <factorWithAug> FactorWithGblErrorAction
%type <factorWithAug> FactorWithLocalErrorAction
%type <factorWithRep> FactorWithRep
%type <integer> FactorRepNum
%type <factorWithNeg> FactorWithNeg
%type <factor> Factor
%type <literal> RangeLit
%type <data> AlphabetNum
%type <data> MachineName
%type <integer> PriorityName
%type <integer> LocalErrName
%type <data> SectionName
%type <data> OptSection
%type <data> OptFileName
%type <integer> EndSection

%type <illist> InlineBlock
%type <ilitem> InlineBlockItem
%type <ilitem> InlineBlockInterpret
%type <data> InlineBlockAny
%type <data> InlineBlockSymbol

%type <illist> InlineExpr
%type <ilitem> InlineExprItem
%type <ilitem> InlineExprInterpret
%type <data> InlineExprSymbol
%type <data> InlineExprAny

%type <regExp> RegularExpr
%type <reItem> RegularExprItem
%type <reItem> RegularExprChar
%type <reOrBlock> RegularExprOrData
%type <reOrItem> RegularExprOrChar

%%

/* Input is any number of input sections. An empty file is accepted. */
input: FsmSpecList;
FsmSpecList: 		
	FsmSpecList FsmSpec |
	/* Nothing */;

/* Fsm Specification. Fsms are begin with '%%' and may be a {} delimited
 * list of Fsm statements or may be a single statement. If no name is
 * given the last name given in a machine is used. */
FsmSpec: 
	StartSection SectionName StatementList EndSection {
		if ( includeDepth == 0 ) {
			if ( sectionOpened )
				*outStream << "</ragel_def>\n";

			if ( machineSpec == 0 && machineName == 0 ) {
				/* The end section may include a newline on the end, so
				 * we use the last line, which will count the newline. */
				*outStream << "<host line=\"" << $4 << "\">";
			}
		}
	};

StartSection:
	TK_Section {
		id->sectionLoc = InputLoc(@1);

		if ( includeDepth == 0 ) {
			if ( machineSpec == 0 && machineName == 0 )
				*outStream << "</host>\n";
			sectionOpened = false;
		}
	};

SectionName: 
	KW_Machine TK_Word ';' {
		/* By default active until found not active. */
		id->active = true;
		id->sectionName = $2.data;

		if ( id->includeSpec != 0 ) {
			if ( strcmp( id->sectionName, id->includeSpec ) == 0 )
				id->sectionName = id->includeTo;
			else
				id->active = false;
		}

		/* Lookup the parse data, if it is not there then create it. */
		SectionMapEl *sectionMapEl = sectionMap.find( id->sectionName );
		if ( sectionMapEl == 0 ) {
			ParseData *newPd = new ParseData( id->fileName, id->sectionName, 
					id->sectionLoc );
			sectionMapEl = sectionMap.insert( id->sectionName, newPd );
		}
		id->pd = sectionMapEl->value;
	} |
	/* Empty */ {
		/* No machine name. Just use the previous section setup. Report an
		 * error if there is no previous section */
		if ( id->pd == 0 ) {
			error(id->sectionLoc) << "the first ragel section does not have a name" << endl;
			id->pd = new ParseData( id->fileName, "<DUMMY>", id->sectionLoc );
		}
	};

EndSection: 
	TK_Section { $$ = @1.last_line; } |
	TK_SectionNL { $$ = @1.last_line + 1; };

/* A NonEmpty list of statements in a fsm. */
StatementList:
	StatementList Statement |
	/* Nothing */;

/* The differnt types of statements in a fsm spec. */
Statement:	
	Assignment |
	Instantiation |
	ActionSpec |
	AlphSpec |
	GetKeySpec |
	RangeSpec |
	Include |
	Write |
	Access |
	Variable;

/* Garble up to the next ; */
Statement: error ';' { yyerrok; };

/* Allow the user to create a named fsm action that can be referenced when
 * building a machine. */
ActionSpec:	
	KW_Action TK_Word '{' InlineBlock '}' {
		if ( id->active ) {
			if ( id->pd->actionDict.find( $2.data ) ) {
				/* Recover by just ignoring the duplicate. */
				error(@2) << "action \"" << $2.data << "\" already defined" << endl;
			}
			else {
				/* Add the action to the list of actions. */
				Action *newAction = new Action( InputLoc(@3), $2.data, $4, id->nameRefList );

				/* Insert to list and dict. */
				id->pd->actionList.append( newAction );
				id->pd->actionDict.insert( newAction );
			}
		}
	};

/* Specifies the data type of the input alphabet. One or two words 
 * followed by a semi-colon. */
AlphSpec:
	KW_AlphType TK_Word TK_Word TK_Semi {
		if ( id->active ) {
			if ( ! id->pd->setAlphType( $2.data, $3.data ) ) {
				// Recover by ignoring the alphtype statement.
				error(@2) << "\"" << $2.data << 
					" " << $3.data << "\" is not a valid alphabet type" << endl;
			}
		}
	} |
	KW_AlphType TK_Word TK_Semi {
		if ( id->active ) {
			if ( ! id->pd->setAlphType( $2.data ) ) {
				// Recover by ignoring the alphtype statement.
				error(@2) << "\"" << $2.data << "\" is not a valid alphabet type" << endl;
			}
		}
	};

GetKeySpec:
	KW_GetKey InlineBlock TK_Semi {
		if ( id->active )
			id->pd->getKeyExpr = $2;
	};

/* Specifies a range to assume that the input characters will fall into. */
RangeSpec:
	KW_Range AlphabetNum AlphabetNum ';' {
		if ( id->active ) {
			// Save the upper and lower ends of the range and emit the line number.
			id->pd->lowerNum = $2.data;
			id->pd->upperNum = $3.data;
			id->pd->rangeLowLoc = InputLoc(@2);
			id->pd->rangeHighLoc = InputLoc(@3);
		}
	};


Write:
	WriteOpen WriteOptions ';' {
		if ( id->active )
			*outStream << "</write>\n";
	};

WriteOpen:
	KW_Write TK_Word {
		if ( id->active ) {
			openSection();
			if ( strcmp( $2.data, "data" ) != 0 &&
					strcmp( $2.data, "init" ) != 0 &&
					strcmp( $2.data, "exec" ) != 0 &&
					strcmp( $2.data, "eof" ) != 0 )
			{
				error( @2 ) << "unknown write command" << endl;
			}
			*outStream << "  <write what=\"" << $2.data << "\">";
		}
	};

WriteOptions:
	WriteOptions TK_Word {
		if ( id->active )
			*outStream << "<option>" << $2.data << "</option>";
	} |
	/* Nothing */;

Access:
	KW_Access InlineBlock TK_Semi {
		if ( id->active )
			id->pd->accessExpr = $2;
	};

Variable:
	KW_Variable InlineBlock TK_Semi {
		if ( id->active ) {
			if ( strcmp( $1.data, "curstate" ) == 0 ) {
				id->pd->curStateExpr = $2;
			}
		}
	};

/* Include statements are processed by both the scanner and the parser. */
Include:
	IncludeKeyword OptSection OptFileName ';' {
		if ( id->active )
			doInclude( @1, $2.data, $3.data );
	};

IncludeKeyword: 
	KW_Include {
		/* Do this immediately so that the scanner has a correct sense of the
		 * value in id->active when it reaches the end of the statement before
		 * the above action executes. */
		//getParseData( @1 );
	};

OptSection: TK_Word { $$ = $1; } | { $$.data = 0; $$.length = 0; };
OptFileName: TK_Literal { $$ = $1; } | { $$.data = 0; $$.length = 0; };

/* An assignement statement. Assigns the definition of a machine to a variable name. */
Assignment:
	MachineName '=' Join ';' {
		if ( id->active ) {
			/* Main machine must be an instance. */
			bool isInstance = false;
			if ( strcmp($1.data, machineMain) == 0 ) {
				warning(@1) << "main machine will be implicitly instantiated" << endl;
				isInstance = true;
			}

			/* Generic creation of machine for instantiation and assignment. */
			JoinOrLm *joinOrLm = new JoinOrLm( $3 );
			tryMachineDef( @1, $1.data, joinOrLm, isInstance );
		}
	};

/* An instantiation statement. Instantiates a machine and assigns it to a
 * variable name. */
Instantiation:
	MachineName TK_ColonEquals JoinOrLm ';' {
		/* Generic creation of machine for instantiation and assignment. */
		if ( id->active )
			tryMachineDef( @1, $1.data, $3, true );
	};

/* Capture the machine name for making the machine's priority name. */
MachineName:
	TK_Word {
		if ( id->active ) {
			/* Make/get the priority key. The name may have already been referenced
			 * and therefore exist. */
			PriorDictEl *priorDictEl;
			if ( id->pd->priorDict.insert( $1.data, id->pd->nextPriorKey, &priorDictEl ) )
				id->pd->nextPriorKey += 1;
			id->pd->curDefPriorKey = priorDictEl->value;

			/* Make/get the local error key. */
			LocalErrDictEl *localErrDictEl;
			if ( id->pd->localErrDict.insert( $1.data, id->pd->nextLocalErrKey, &localErrDictEl ) )
				id->pd->nextLocalErrKey += 1;
			id->pd->curDefLocalErrKey = localErrDictEl->value;
		}
	};

JoinOrLm: 
	Join {
		$$ = new JoinOrLm( $1 );
	} |
	TK_BarStar LmPartList '*' '|' {
		/* Create a new factor going to a longest match structure. Record
		 * in the parse data that we have a longest match. */
		LongestMatch *lm = new LongestMatch( @1, $2 );
		if ( id->active )
			id->pd->lmList.append( lm );
		for ( LmPartList::Iter lmp = *($2); lmp.lte(); lmp++ )
			lmp->longestMatch = lm;
		$$ = new JoinOrLm( lm );
	};

Join: 
	Join ',' Expression {
		/* Append the expression to the list and return it. */
		$1->exprList.append( $3 );
		$$ = $1;
	} |
	Expression {
		/* Create the expression list with the intial expression. */
		$$ = new Join( InputLoc(@1), $1 );
	};

/* Top level production in the parse of a fsm. The lowest precedence
 * is the '|' (or), '&' (intersection), and '-' (subtraction) operators. */
Expression:
	Expression '|' Term {
		$$ = new Expression( $1, $3, Expression::OrType );
	} %prec EXPR_MINUS |
	Expression '&' Term {
		$$ = new Expression( $1, $3, Expression::IntersectType );
	} %prec EXPR_MINUS |
	Expression '-' Term {
		$$ = new Expression( $1, $3, Expression::SubtractType );
	} %prec EXPR_MINUS |
	Expression TK_DashDash Term {
		$$ = new Expression( $1, $3, Expression::StrongSubtractType );
	} %prec EXPR_MINUS |
	Term {
		$$ = new Expression( $1 );
	} %prec EXPR_MINUS;

Term:
	Term FactorWithLabel {
		$$ = new Term( $1, $2 );
	} |
	Term '.' FactorWithLabel {
		$$ = new Term( $1, $3 );
	} |
	Term TK_ColonGt FactorWithLabel {
		$$ = new Term( $1, $3, Term::RightStartType );
	} |
	Term TK_ColonGtGt FactorWithLabel {
		$$ = new Term( $1, $3, Term::RightFinishType );
	} |
	Term TK_LtColon FactorWithLabel {
		$$ = new Term( $1, $3, Term::LeftType );
	} |
	FactorWithLabel {
		$$ = new Term( $1 );
	};

FactorWithLabel:
	TK_Word ':' FactorWithLabel { 
		/* Add the label to the list and pass the factor up. */
		$3->labels.prepend( Label(@1, $1.data) );
		$$ = $3; 
	} |
	FactorWithEp;

FactorWithEp:
	FactorWithEp TK_Arrow LocalStateRef { 
		/* Add the target to the list and return the factor object. */
		$1->epsilonLinks.append( EpsilonLink( InputLoc(@2), id->nameRef ) );
		$$ = $1; 
	} |
	FactorWithAug;

/* A local state reference. Qualified name witout :: prefix. */
LocalStateRef:
	NoNameSep StateRefNames;

/* Clear the name ref structure. */
NoNameSep:
	/* Nothing */ {
		id->nameRef.empty();
	};

/* A qualified state reference. */
StateRef:
	OptNameSep StateRefNames;

/* Optional leading name separator. */
OptNameSep:
	TK_NameSep {
		/* Insert an inition null pointer val to indicate the existence of the
		 * initial name seperator. */
		id->nameRef.setAs( 0 );
	} |
	/* Nothing. */ {
		id->nameRef.empty();
	};

/* List of names separated by :: */
StateRefNames:
	StateRefNames TK_NameSep TK_Word {
		id->nameRef.append( $3.data );
	} |
	TK_Word {
		id->nameRef.append( $1.data );
	};

/* Third group up in precedence. Allow users to embed actions and priorities */
FactorWithAug:
	FactorWithTransAction |
	FactorWithPriority |
	FactorWithCond |
	FactorWithToStateAction |
	FactorWithFromStateAction |
	FactorWithEOFAction |
	FactorWithGblErrorAction |
	FactorWithLocalErrorAction |
	FactorWithRep {
		$$ = new FactorWithAug( $1 );
	};

FactorWithTransAction:
	FactorWithAug AugTypeBase ActionEmbed {
		/* Append the action to the factorWithAug, record the refernce from 
		 * factorWithAug to the action and pass up the factorWithAug. */
		$1->actions.append( ParserAction( @2, $2, 0, $3 ) );
		$$ = $1;
	};

FactorWithPriority:
	FactorWithAug AugTypeBase PriorityAug {
		if ( id->active ) {
			/* Append the named priority to the factorWithAug and pass it up. */
			$1->priorityAugs.append( PriorityAug( $2, id->pd->curDefPriorKey, $3 ) );
		}
		$$ = $1;
	} |
	FactorWithAug AugTypeBase '(' PriorityName ',' PriorityAug ')' {
		/* Append the priority using a default name. */
		$1->priorityAugs.append( PriorityAug( $2, $4, $6 ) );
		$$ = $1;
	};

FactorWithCond:
	FactorWithAug AugTypeCond ActionEmbed {
		$$->conditions.append( ParserAction( @2, $2, 0, $3 ) );
		$$ = $1;
	};

AugTypeCond:
	TK_StartCond { $$ = at_start; } |
	'>' KW_When { $$ = at_start; } |
	TK_AllCond { $$ = at_all; } |
	'$' KW_When { $$ = at_all; } |
	TK_LeavingCond { $$ = at_leave; } |
	'%' KW_When { $$ = at_all; } |
	KW_When { $$ = at_all; };

FactorWithToStateAction:
	FactorWithAug AugTypeToState ActionEmbed {
		/* Append the action, pass it up. */
		$1->actions.append( ParserAction( @2, $2, 0, $3 ) );
		$$ = $1;
	};

FactorWithFromStateAction:
	FactorWithAug AugTypeFromState ActionEmbed {
		/* Append the action, pass it up. */
		$1->actions.append( ParserAction( @2, $2, 0, $3 ) );
		$$ = $1;
	};

FactorWithEOFAction:
	FactorWithAug AugTypeEOF ActionEmbed {
		/* Append the action, pass it up. */
		$1->actions.append( ParserAction( @2, $2, 0, $3 ) );
		$$ = $1;
	};

FactorWithGblErrorAction:
	FactorWithAug AugTypeGblError ActionEmbed {
		if ( id->active ) {
			/* Append the action to the factorWithAug, record the refernce from 
			 * factorWithAug to the action and pass up the factorWithAug. */
			$1->actions.append( ParserAction( @2, $2, id->pd->curDefLocalErrKey, $3 ) );
		}
		$$ = $1;
	};

FactorWithLocalErrorAction:
	FactorWithAug AugTypeLocalError ActionEmbed {
		if ( id->active ) {
			/* Append the action to the factorWithAug, record the refernce from 
			 * factorWithAug to the action and pass up the factorWithAug. */
			$1->actions.append( ParserAction( @2, $2, id->pd->curDefLocalErrKey, $3 ) );
		}
		$$ = $1;
	} |
	FactorWithAug AugTypeLocalError '(' LocalErrName ',' ActionEmbed ')' {
		/* Append the action to the factorWithAug, record the refernce from
		 * factorWithAug to the action and pass up the factorWithAug. */
		$1->actions.append( ParserAction( @2, $2, $4, $6 ) );
		$$ = $1;
	};

/* A specified priority name. Looks up the name in the current priority
 * dictionary. */
PriorityName:
	TK_Word {
		if ( id->active ) {
			// Lookup/create the priority key.
			PriorDictEl *priorDictEl;
			if ( id->pd->priorDict.insert( $1.data, id->pd->nextPriorKey, &priorDictEl ) )
				id->pd->nextPriorKey += 1;

			// Use the inserted/found priority key.
			$$ = priorDictEl->value;
		}
	};

LocalErrName:
	TK_Word {
		if ( id->active ) {
			/* Lookup/create the priority key. */
			LocalErrDictEl *localErrDictEl;
			if ( id->pd->localErrDict.insert( $1.data, id->pd->nextLocalErrKey, &localErrDictEl ) )
				id->pd->nextLocalErrKey += 1;

			/* Use the inserted/found priority key. */
			$$ = localErrDictEl->value;
		}
	};

/* Priority change specs. */
PriorityAug: 
	PriorityAugNum {
		// Convert the priority number to a long. Check for overflow.
		errno = 0;
		int aug = strtol( $1.data, 0, 10 );
		if ( errno == ERANGE && aug == LONG_MAX ) {
			// Priority number too large. Recover by setting the priority to 0.
			error(@1) << "priority number " << $1.data << " overflows" << endl;
			$$ = 0;
		}
		else if ( errno == ERANGE && aug == LONG_MIN ) {
			// Priority number too large in the neg. Recover by using 0.
			error(@1) << "priority number " << $1.data << " underflows" << endl;
			$$ = 0;
		}
		else {
			// No overflow or underflow.
			$$ = aug;
 		}
	};

PriorityAugNum:
	TK_UInt |
	'+' TK_UInt {
		$$ = $2;
	} |
	'-' TK_UInt {
		$$.data = "-";
		$$.length = 1;
		$$.append( $2 );
	};

/* Classes of transtions on which to embed actions or change priorities. */
AugTypeBase:
	'@' { $$ = at_finish; } |
	'%' { $$ = at_leave; } |
	'$' { $$ = at_all; } |
	'>' { $$ = at_start; };
		
/* Global error actions. */
AugTypeGblError:
	TK_StartGblError { $$ = at_start_gbl_error; } |
	'>' KW_Err { $$ = at_start_gbl_error; } |

	TK_NotStartGblError { $$ = at_not_start_gbl_error; } |
	'<' KW_Err { $$ = at_not_start_gbl_error; } |

	TK_AllGblError { $$ = at_all_gbl_error; } |
	'$' KW_Err { $$ = at_all_gbl_error; } |

	TK_FinalGblError { $$ = at_final_gbl_error; } |
	'%' KW_Err { $$ = at_final_gbl_error; } |

	TK_NotFinalGblError { $$ = at_not_final_gbl_error; } |
	'@' KW_Err { $$ = at_not_final_gbl_error; } |

	TK_MiddleGblError { $$ = at_middle_gbl_error; } |
	TK_Middle KW_Err { $$ = at_middle_gbl_error; };

/* Local error actions. */
AugTypeLocalError:
	TK_StartLocalError { $$ = at_start_local_error; } |
	'>' KW_Lerr { $$ = at_start_local_error; } |

	TK_NotStartLocalError { $$ = at_not_start_local_error; } |
	'<' KW_Lerr { $$ = at_not_start_local_error; } |

	TK_AllLocalError { $$ = at_all_local_error; } |
	'$' KW_Lerr { $$ = at_all_local_error; } |

	TK_FinalLocalError { $$ = at_final_local_error; } |
	'%' KW_Lerr { $$ = at_final_local_error; } |

	TK_NotFinalLocalError { $$ = at_not_final_local_error; } |
	'@' KW_Lerr { $$ = at_not_final_local_error; } |

	TK_MiddleLocalError { $$ = at_middle_local_error; } |
	TK_Middle KW_Lerr { $$ = at_middle_local_error; };

/* Eof state actions. */
AugTypeEOF:
	TK_StartEOF { $$ = at_start_eof; } |
	'>' KW_Eof { $$ = at_start_eof; } |

	TK_NotStartEOF { $$ = at_not_start_eof; } |
	'<' KW_Eof { $$ = at_not_start_eof; } |

	TK_AllEOF { $$ = at_all_eof; } |
	'$' KW_Eof { $$ = at_all_eof; } |

	TK_FinalEOF { $$ = at_final_eof; } |
	'%' KW_Eof { $$ = at_final_eof; } |

	TK_NotFinalEOF { $$ = at_not_final_eof; } |
	'@' KW_Eof { $$ = at_not_final_eof; } |

	TK_MiddleEOF { $$ = at_middle_eof; } |
	TK_Middle KW_Eof { $$ = at_middle_eof; };

/* To state actions. */
AugTypeToState:
	TK_StartToState { $$ = at_start_to_state; } |
	'>' KW_To { $$ = at_start_to_state; } |

	TK_NotStartToState { $$ = at_not_start_to_state; } |
	'<' KW_To { $$ = at_not_start_to_state; } |

	TK_AllToState { $$ = at_all_to_state; } |
	'$' KW_To { $$ = at_all_to_state; } |

	TK_FinalToState { $$ = at_final_to_state; } |
	'%' KW_To { $$ = at_final_to_state; } |

	TK_NotFinalToState { $$ = at_not_final_to_state; } |
	'@' KW_To { $$ = at_not_final_to_state; } |

	TK_MiddleToState { $$ = at_middle_to_state; } |
	TK_Middle KW_To { $$ = at_middle_to_state; };

/* From state actions. */
AugTypeFromState:
	TK_StartFromState { $$ = at_start_from_state; } |
	'>' KW_From { $$ = at_start_from_state; } |

	TK_NotStartFromState { $$ = at_not_start_from_state; } |
	'<' KW_From { $$ = at_not_start_from_state; } |

	TK_AllFromState { $$ = at_all_from_state; } |
	'$' KW_From { $$ = at_all_from_state; } |

	TK_FinalFromState { $$ = at_final_from_state; } |
	'%' KW_From { $$ = at_final_from_state; } |

	TK_NotFinalFromState { $$ = at_not_final_from_state; } |
	'@' KW_From { $$ = at_not_final_from_state; } |

	TK_MiddleFromState { $$ = at_middle_from_state; } |
	TK_Middle KW_From { $$ = at_middle_from_state; };


/* Different ways to embed actions. A TK_Word is reference to an action given by
 * the user as a statement in the fsm specification. An action can also be
 * specified immediately. */
ActionEmbed: 
	ActionEmbedWord | ActionEmbedBlock;

ActionEmbedWord:
	TK_Word {
		if ( id->active ) {
			/* Set the name in the actionDict. */
			Action *action = id->pd->actionDict.find( $1.data );
			if ( action != 0 ) {
				/* Pass up the action element */
				$$ = action;
			}
			else {
				/* Will recover by returning null as the action. */
				error(@1) << "action lookup of \"" << $1.data << "\" failed" << endl;
				$$ = 0;
			}
		}
	};

ActionEmbedBlock:
	'{' InlineBlock '}' {
		if ( id->active ) {
			/* Create the action, add it to the list and pass up. */
			Action *newAction = new Action( InputLoc(@1), 0, $2, id->nameRefList );
			id->pd->actionList.append( newAction );
			$$ = newAction;
		}
	};

/* The fourth level of precedence. These are the trailing unary operators that
 * allow for repetition. */
FactorWithRep:
	FactorWithRep '*' {
		$$ = new FactorWithRep( InputLoc(@2), $1, 0, 0,
				FactorWithRep::StarType );
	} |
	FactorWithRep TK_StarStar {
		$$ = new FactorWithRep( InputLoc(@2), $1, 0, 0,
				FactorWithRep::StarStarType );
	} |
	FactorWithRep '?' {
		$$ = new FactorWithRep( InputLoc(@2), $1, 0, 0,
				FactorWithRep::OptionalType );
	} |
	FactorWithRep '+' {
		$$ = new FactorWithRep( InputLoc(@2), $1, 0, 0,
				FactorWithRep::PlusType );
	} |
	FactorWithRep TK_RepOpOpen FactorRepNum '}' {
		$$ = new FactorWithRep( InputLoc(@2), $1, $3, 0,
				FactorWithRep::ExactType );
	} |
	FactorWithRep TK_RepOpOpen ',' FactorRepNum '}' {
		$$ = new FactorWithRep( InputLoc(@2), $1, 0, $4,
				FactorWithRep::MaxType );
	} |
	FactorWithRep TK_RepOpOpen FactorRepNum ',' '}' {
		$$ = new FactorWithRep( InputLoc(@2), $1, $3, 0,
				FactorWithRep::MinType );
	} |
	FactorWithRep TK_RepOpOpen FactorRepNum ',' FactorRepNum '}' {
		$$ = new FactorWithRep( InputLoc(@2), $1, $3, $5,
				FactorWithRep::RangeType );
	} |
	FactorWithNeg {
		$$ = new FactorWithRep( InputLoc(@1), $1 );
	};

FactorRepNum:
	TK_UInt {
		// Convert the priority number to a long. Check for overflow.
		errno = 0;
		int rep = strtol( $1.data, 0, 10 );
		if ( errno == ERANGE && rep == LONG_MAX ) {
			// Repetition too large. Recover by returing repetition 1. */
			error(@1) << "repetition number " << $1.data << " overflows" << endl;
			$$ = 1;
		}
		else {
			// Cannot be negative, so no overflow.
			$$ = rep;
 		}
	};

/* The fifth level up in precedence. Negation. */
FactorWithNeg:
	'!' FactorWithNeg {
		$$ = new FactorWithNeg( InputLoc(@1), $2, FactorWithNeg::NegateType );
	} |
	'^' FactorWithNeg {
		$$ = new FactorWithNeg( InputLoc(@1), $2, FactorWithNeg::CharNegateType );
	} |
	Factor {
		$$ = new FactorWithNeg( InputLoc(@1), $1 );
	};

/* The highest level in precedence. Atomic machines such as references to other
 * machines, literal machines, regular expressions or Expressions in side of
 * parenthesis. */
Factor:
	TK_Literal {
		// Create a new factor node going to a concat literal. */
		$$ = new Factor( new Literal( InputLoc(@1), $1, Literal::LitString ) );
	} |
	TK_CiLiteral {
		// Create a new factor node going to a concat literal. */
		$$ = new Factor( new Literal( InputLoc(@1), $1, Literal::LitString ) );
		$$->literal->caseInsensitive = true;
	} |
	AlphabetNum {
		// Create a new factor node going to a literal number. */
		$$ = new Factor( new Literal( InputLoc(@1), $1, Literal::Number ) );
	} |
	TK_Word {
		if ( id->active ) {
			// Find the named graph.
			GraphDictEl *gdNode = id->pd->graphDict.find( $1.data );
			if ( gdNode == 0 ) {
				// Recover by returning null as the factor node.
				error(@1) << "graph lookup of \"" << $1.data << "\" failed" << endl;
				$$ = 0;
			}
			else if ( gdNode->isInstance ) {
				// Recover by retuning null as the factor node.
				error(@1) << "references to graph instantiations not allowed "
						"in expressions" << endl;
				$$ = 0;
			}
			else {
				// Create a factor node that is a lookup of an expression.
				$$ = new Factor( InputLoc(@1), gdNode->value );
			}
		}
	} |
	RE_SqOpen RegularExprOrData RE_SqClose {
		// Create a new factor node going to an OR expression. */
		$$ = new Factor( new ReItem( InputLoc(@1), $2, ReItem::OrBlock ) );
	} |
	RE_SqOpenNeg RegularExprOrData RE_SqClose {
		// Create a new factor node going to a negated OR expression. */
		$$ = new Factor( new ReItem( InputLoc(@1), $2, ReItem::NegOrBlock ) );
	} |
	RE_Slash RegularExpr RE_Slash {
		if ( $3.length > 1 ) {
			for ( char *p = $3.data; *p != 0; p++ ) {
				if ( *p == 'i' )
					$2->caseInsensitive = true;
			}
		}

		// Create a new factor node going to a regular exp.
		$$ = new Factor( $2 );
	} |
	RangeLit TK_DotDot RangeLit {
		// Create a new factor node going to a range. */
		$$ = new Factor( new Range( $1, $3 ) );
	} |
	'(' Join ')' {
		/* Create a new factor going to a parenthesized join. */
		$$ = new Factor( $2 );
	};

/* Garble up to the closing brace of a parenthesized expression. */
Factor: '(' error ')' { $$ = 0; yyerrok; };

LmPartList:
	LmPartList LongestMatchPart {
		if ( $2 != 0 ) 
			$1->append( $2 );
		$$ = $1;
	} |
	LongestMatchPart {
		/* Create a new list with the part. */
		$$ = new LmPartList;
		if ( $1 != 0 )
			$$->append( $1 );
	};

LongestMatchPart: 
	ActionSpec { $$ = 0; } |
	Assignment { $$ = 0; } |
	Join OptLmPartAction ';' {
		$$ = 0;
		if ( id->active ) {
			Action *action = $2;
			if ( action != 0 )
				action->isLmAction = true;
			$$ = new LongestMatchPart( $1, action, id->pd->nextLongestMatchId++ );
		}
	};

OptLmPartAction:
	TK_DoubleArrow ActionEmbed { $$ = $2; } |
	ActionEmbedBlock { $$ = $1; } |
	/* Nothing */ { $$ = 0; };


/* Any form of a number that can be used as a basic machine. */
AlphabetNum:
	TK_UInt |
	'-' TK_UInt { 
		$$.data = "-";
		$$.length = 1;
		$$.append( $2 );
	} | 
	TK_Hex;

InlineBlock:
	InlineBlock InlineBlockItem {
		/* Append the item to the list, return the list. */
		$1->append( $2 );
		$$ = $1;
	} |
	/* Empty */ {
		/* Start with empty list. */
		$$ = new InlineList;
	};

/* Items in a struct block. */
InlineBlockItem:
	InlineBlockAny {
		/* Add a text segment. */
		$$ = new InlineItem( @1, $1.data, InlineItem::Text );
	} |
	InlineBlockSymbol {
		/* Add a text segment, need string on heap. */
		$$ = new InlineItem( @1, strdup($1.data), InlineItem::Text );
	} |
	InlineBlockInterpret {
		/* Pass the inline item up. */
		$$ = $1;
	};

/* Uninteresting tokens in a struct block. Data allocated by scanner. */
InlineBlockAny:
	IL_WhiteSpace | IL_Comment | IL_Literal | IL_Symbol |
	TK_UInt | TK_Hex | TK_Word;

/* Symbols in a struct block, no data allocated. */
InlineBlockSymbol:
	',' { $$.data = ","; $$.length = 1; } |
	';' { $$.data = ";"; $$.length = 1; } |
	'(' { $$.data = "("; $$.length = 1; } |
	')' { $$.data = ")"; $$.length = 1; } |
	'*' { $$.data = "*"; $$.length = 1; } |
	TK_NameSep { $$.data = "::"; $$.length = 2; };

/* Interpreted statements in a struct block. */
InlineBlockInterpret:
	InlineExprInterpret {
		/* Pass up interpreted items of inline expressions. */
		$$ = $1;
	} |
	KW_Hold SetNoWs ';' SetWs {
		$$ = new InlineItem( @1, InlineItem::Hold );
	} |
	KW_Exec SetNoWs InlineExpr ';' SetWs {
		$$ = new InlineItem( @1, InlineItem::Exec );
		$$->children = $3;
	} |
	KW_Goto SetNoWs StateRef ';' SetWs { 
		$$ = new InlineItem( @1, new NameRef(id->nameRef), InlineItem::Goto );
	} | 
	KW_Goto SetNoWs '*' SetWs InlineExpr ';' {
		$$ = new InlineItem( @1, InlineItem::GotoExpr );
		$$->children = $5;
	} |
	KW_Next SetNoWs StateRef ';' SetWs { 
		$$ = new InlineItem( @1, new NameRef(id->nameRef), InlineItem::Next );
	} |
	KW_Next SetNoWs '*' SetWs InlineExpr ';' {
		$$ = new InlineItem( @1, InlineItem::NextExpr );
		$$->children = $5;
	} |
	KW_Call SetNoWs StateRef ';' SetWs {
		$$ = new InlineItem( @1, new NameRef(id->nameRef), InlineItem::Call );
	} | 
	KW_Call SetNoWs '*' SetWs InlineExpr ';' {
		$$ = new InlineItem( @1, InlineItem::CallExpr );
		$$->children = $5;
	} |
	KW_Ret SetNoWs ';' SetWs {
		$$ = new InlineItem( @1, InlineItem::Ret );
	} |
	KW_Break SetNoWs ';' SetWs {
		$$ = new InlineItem( @1, InlineItem::Break );
	};

/* Turn off whitspace collecting when scanning inline blocks. */
SetNoWs: { inlineWhitespace = false; };

/* Turn on whitespace collecting when scanning inline blocks. */
SetWs: { inlineWhitespace = true; };

InlineExpr:
	InlineExpr InlineExprItem {
		$1->append( $2 );
		$$ = $1;
	} |
	/* Empty */ {
		/* Init the list used for this expr. */
		$$ = new InlineList;
	};

InlineExprItem:
	InlineExprAny {
		/* Return a text segment. */
		$$ = new InlineItem( @1, $1.data, InlineItem::Text );
	} |
	InlineExprSymbol {
		/* Return a text segment, must heap alloc the text. */
		$$ = new InlineItem( @1, strdup($1.data), InlineItem::Text );
	} |
	InlineExprInterpret {
		/* Pass the inline item up. */
		$$ = $1;
	};

InlineExprInterpret:
	KW_PChar {
		$$ = new InlineItem( @1, InlineItem::PChar );
	} |
	KW_Char {
		$$ = new InlineItem( @1, InlineItem::Char );
	} |
	KW_CurState {
		$$ = new InlineItem( @1, InlineItem::Curs );
	} |
	KW_TargState {
		$$ = new InlineItem( @1, InlineItem::Targs );
	} |
	KW_Entry SetNoWs '(' StateRef ')' SetWs {
		$$ = new InlineItem( @1, new NameRef(id->nameRef), InlineItem::Entry );
	};

InlineExprAny:
	IL_WhiteSpace | IL_Comment | IL_Literal | IL_Symbol |
	TK_UInt | TK_Hex | TK_Word;

/* Anything in a ExecValExpr that is not dynamically allocated. This includes
 * all special symbols caught in inline code except the semi. */
InlineExprSymbol:
	'(' { $$.data = "("; $$.length = 1; } | 
	')' { $$.data = ")"; $$.length = 1; } |
	'*' { $$.data = "*"; $$.length = 1; } |
	TK_NameSep { $$.data = "::"; $$.length = 1; };

/* Parser for regular expression fsms. Any number of expression items which
 * generally gives a machine one character long or one character long stared. */
RegularExpr:
	RegularExpr RegularExprItem {
		// An optimization to lessen the tree size. If a non-starred char is directly
		// under the left side on the right and the right side is another non-starred
		// char then paste them together and return the left side. Otherwise
		// just put the two under a new reg exp node.
		if ( $2->type == ReItem::Data && !$2->star &&
			$1->type == RegExpr::RecurseItem &&
			$1->item->type == ReItem::Data && !$1->item->star )
		{
			// Append the right side to the right side of the left and toss 
			// the right side.
			$1->item->data.append( $2->data );
			delete $2;
			$$ = $1;
		}
		else {
			$$ = new RegExpr( $1, $2 );
		}
	} |
	/* Nothing */ {
		// Can't optimize the tree.
		$$ = new RegExpr();
	};

/* RegularExprItems can be a character spec with an optional staring of the char. */
RegularExprItem:
	RegularExprChar RE_Star {
		$1->star = true;
		$$ = $1;
	} |
	RegularExprChar {
		$$ = $1;
	};

/* A character spec can be a set of characters inside of square parenthesis,
 * a dot specifying any character or some explicitly stated character. */
RegularExprChar:
	RE_SqOpen RegularExprOrData RE_SqClose {
		$$ = new ReItem( InputLoc(@1), $2, ReItem::OrBlock );
	} |
	RE_SqOpenNeg RegularExprOrData RE_SqClose {
		$$ = new ReItem( InputLoc(@1), $2, ReItem::NegOrBlock );
	} |
	RE_Dot {
		$$ = new ReItem( InputLoc(@1), ReItem::Dot );
	} |
	RE_Char {
		$$ = new ReItem( InputLoc(@1), $1.data[0] );
	};

/* The data inside of a [] expression in a regular expression. Accepts any
 * number of characters or ranges. */
RegularExprOrData:
	RegularExprOrData RegularExprOrChar {
		// An optimization to lessen the tree size. If an or char is directly
		// under the left side on the right and the right side is another or
		// char then paste them together and return the left side. Otherwise
		// just put the two under a new or data node.
		if ( $2->type == ReOrItem::Data &&
				$1->type == ReOrBlock::RecurseItem &&
				$1->item->type == ReOrItem::Data )
		{
			// Append the right side to right side of the left and toss
			// the right side.
			$1->item->data.append( $2->data );
			delete $2;
			$$ = $1;
		}
		else {
			// Can't optimize, put the left and right under a new node.
			$$ = new ReOrBlock( $1, $2 );
		}
	} | 
	/* Nothing */ {
		$$ = new ReOrBlock();
	};


/* A single character inside of an or expression. Can either be a character
 * or a set of characters. */
RegularExprOrChar:
	RE_Char {
		$$ = new ReOrItem( InputLoc(@1), $1.data[0] );
	} |
	RE_Char RE_Dash RE_Char {
		$$ = new ReOrItem( InputLoc(@2), $1.data[0], $3.data[0] );
	};

RangeLit:
	TK_Literal {
		// Range literas must have only one char.
		if ( strlen($1.data) != 1 ) {
			// Recover by using the literal anyways.
			error(@1) << "literal used in range must be of length 1" << endl;
		}
		$$ = new Literal( InputLoc(@1), $1, Literal::LitString );
	} |
	AlphabetNum {
		// Create a new literal number.
		$$ = new Literal( InputLoc(@1), $1, Literal::Number );
	};

%%

/* Try to do a definition, common to assignment and instantiation. Warns about 
 * instances other than main not being implemented yet. */
void tryMachineDef( const YYLTYPE &loc, char *name, JoinOrLm *joinOrLm, bool isInstance )
{
	GraphDictEl *newEl = id->pd->graphDict.insert( name );
	if ( newEl != 0 ) {
		/* New element in the dict, all good. */
		newEl->value = new VarDef( name, joinOrLm );
		newEl->isInstance = isInstance;
		newEl->loc = loc;

		/* It it is an instance, put on the instance list. */
		if ( isInstance )
			id->pd->instanceList.append( newEl );
	}
	else {
		// Recover by ignoring the duplicate.
		error(loc) << "fsm \"" << name << "\" previously defined" << endl;
	}
}

void doInclude( const InputLoc &loc, char *sectionName, char *inputFile )
{
	/* Bail if we hit the max include depth. */
	if ( includeDepth == INCLUDE_STACK_SIZE ) {
		error(loc) << "hit maximum include depth of " << INCLUDE_STACK_SIZE << endl;
	}
	else {
		char *includeTo = id->pd->fsmName;

		/* Implement defaults for the input file and section name. */
		if ( inputFile == 0 ) 
			inputFile = id->fileName;
		if ( sectionName == 0 )
			sectionName = id->pd->fsmName;

		/* Parse the included file. */
		InputData *oldId = id;
		id = new InputData( inputFile, sectionName, includeTo );
		includeDepth += 1;
		yyparse();
		includeDepth -= 1;
		delete id;
		id = oldId;
	}
}

void openSection()
{
	if ( ! sectionOpened ) {
		sectionOpened = true;
		*outStream << "<ragel_def name=\"" << id->pd->fsmName << "\">\n";
	}
}

void yyerror( char *err )
{
	/* Bison won't give us the location, but in the last call to the scanner we
	 * saved a pointer to the location variable. Use that. instead. */
	error(::yylloc->first_line, ::yylloc->first_column) << err << endl;
}
