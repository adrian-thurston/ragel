#include "load.h"
#include "if.h"
#include "ragel.h"
#include "inputdata.h"
#include "parsedata.h"
#include "parsetree.h"

#include <colm/colm.h>
#include <colm/tree.h>
#include <errno.h>

using std::endl;

extern colm_sections colm_object;

InputLoc::InputLoc( colm_location *pcloc )
{
	if ( pcloc != 0 ) {
		fileName = pcloc->name;
		line = pcloc->line;
		col = pcloc->column;
	}
	else {
		fileName = 0;
		line = -1;
		col = -1;
	}
}

struct LoadRagel
{
	LoadRagel( InputData &id )
	:
		id(id),
		pd(0),
		machineSpec(0),
		machineName(0),
		includeDepth(0),
		machineNameError(false)
	{}

	InputData &id;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;
	int machineNameError;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

	void loadMachineName( ragel::word MachineName )
	{
		InputLoc sectionLoc;
		string fileName = "input.rl";
		string machine = MachineName.text();

		ParseDataDictEl *pdEl = id.parseDataDict.find( machine );
		if ( pdEl == 0 ) {
			pdEl = new ParseDataDictEl( machine );
			pdEl->value = new ParseData( fileName, machine, sectionLoc );
			id.parseDataDict.insert( pdEl );
			id.parseDataList.append( pdEl->value );
		}

		pd = pdEl->value;
	}

	InlineItem *loadExprAny( c_inline::expr_any ExprAny )
	{
		string t = ExprAny.text();
		InputLoc loc = ExprAny.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadExprSymbol( c_inline::expr_symbol ExprSymbol )
	{
		string t = ExprSymbol.text();
		InputLoc loc = ExprSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	NameRef *loadStateRefNames( c_inline::state_ref_names StateRefNames )
	{
		NameRef *nameRef = 0;
		switch ( StateRefNames.prodName() ) {
			case c_inline::state_ref_names::_Rec:
				nameRef = loadStateRefNames( StateRefNames.StateRefNames() );
				nameRef->append( StateRefNames.Word().text() );
				break;
			case c_inline::state_ref_names::_Base:
				nameRef = new NameRef;
				nameRef->append( StateRefNames.Word().text() );
				break;
		}
		return nameRef;
	}

	NameRef *loadStateRef( c_inline::state_ref StateRef )
	{
		NameRef *nameRef = loadStateRefNames( StateRef.StateRefNames() );
		if ( StateRef.OptNameSep().prodName() == c_inline::opt_name_sep::_ColonColon )
			nameRef->prepend( "" );
		return nameRef;
	}

	InlineItem *loadExprInterpret( c_inline::expr_interpret ExprInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = ExprInterpret.loc();
		switch ( ExprInterpret.prodName() ) {
			case c_inline::expr_interpret::_Fpc:
				inlineItem = new InlineItem( loc, InlineItem::PChar );
				break;
			case c_inline::expr_interpret::_Fc:
				inlineItem = new InlineItem( loc, InlineItem::Char );
				break;
			case c_inline::expr_interpret::_Fcurs:
				inlineItem = new InlineItem( loc, InlineItem::Curs );
				break;
			case c_inline::expr_interpret::_Ftargs:
				inlineItem = new InlineItem( loc, InlineItem::Targs );
				break;
			case c_inline::expr_interpret::_Fentry: {
				NameRef *nameRef = loadStateRef( ExprInterpret.StateRef() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Entry );
				break;
			}
		}
		return inlineItem;
	}

	InlineItem *loadExprItem( c_inline::expr_item ExprItem )
	{
		switch ( ExprItem.prodName() ) {
			case c_inline::expr_item::_ExprAny:
				return loadExprAny( ExprItem.ExprAny() );
			case c_inline::expr_item::_ExprSymbol:
				return loadExprSymbol( ExprItem.ExprSymbol() );
			case c_inline::expr_item::_ExprInterpret:
				return loadExprInterpret( ExprItem.ExprInterpret() );
		}
		return 0;
	}


	InlineItem *loadBlockSymbol( c_inline::block_symbol BlockSymbol )
	{
		string t = BlockSymbol.text();
		InputLoc loc = BlockSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadBlockInterpret( c_inline::block_interpret BlockInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = BlockInterpret.loc();
		switch ( BlockInterpret.prodName() ) {
			case c_inline::block_interpret::_ExprInterpret:
				inlineItem = loadExprInterpret( BlockInterpret.ExprInterpret() );
				break;
			case c_inline::block_interpret::_Fhold:
				inlineItem = new InlineItem( loc, InlineItem::Hold );
				break;
			case c_inline::block_interpret::_Fret:
				inlineItem = new InlineItem( loc, InlineItem::Ret );
				break;
			case c_inline::block_interpret::_Fbreak:
				inlineItem = new InlineItem( loc, InlineItem::Break );
				break;

			case c_inline::block_interpret::_FgotoExpr:
				inlineItem = new InlineItem( loc, InlineItem::GotoExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.InlineExpr() );
				break;
			case c_inline::block_interpret::_FnextExpr:
				inlineItem = new InlineItem( loc, InlineItem::NextExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.InlineExpr() );
				break;
			case c_inline::block_interpret::_FcallExpr:
				inlineItem = new InlineItem( loc, InlineItem::CallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.InlineExpr() );
				break;
			case c_inline::block_interpret::_Fexec:
				inlineItem = new InlineItem( loc, InlineItem::Exec );
				inlineItem->children = loadInlineExpr( BlockInterpret.InlineExpr() );
				break;
			case c_inline::block_interpret::_FgotoSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.StateRef() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Goto );
				break;
			}
			case c_inline::block_interpret::_FnextSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.StateRef() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Next );
				break;
			}
			case c_inline::block_interpret::_FcallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.StateRef() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Call );
				break;
			}
		}
		return inlineItem;
	}

	InlineList *loadInlineBlock( InlineList *inlineList, c_inline::inline_block InlineBlock )
	{
		c_inline::_repeat_block_item BlockItemList = InlineBlock.BlockItemList();
		while ( !BlockItemList.end() ) {
			loadBlockItem( inlineList, BlockItemList.value() );
			BlockItemList = BlockItemList.next();
		}
		return inlineList;
	}

	InlineList *loadInlineBlock( c_inline::inline_block InlineBlock )
	{
		InlineList *inlineList = new InlineList;
		return loadInlineBlock( inlineList, InlineBlock );
	}

	void loadBlockItem( InlineList *inlineList, c_inline::block_item BlockItem )
	{
		switch ( BlockItem.prodName() ) {
			case c_inline::block_item::_ExprAny: {
				InlineItem *inlineItem = loadExprAny( BlockItem.ExprAny() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::_BlockSymbol: {
				InlineItem *inlineItem = loadBlockSymbol( BlockItem.BlockSymbol() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::_BlockInterpret: {
				InlineItem *inlineItem = loadBlockInterpret( BlockItem.BlockInterpret() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::_RecBlock:
				InputLoc loc = BlockItem.loc();
				inlineList->append( new InlineItem( loc, "{", InlineItem::Text ) );
				loadInlineBlock( inlineList, BlockItem.InlineBlock() );
				inlineList->append( new InlineItem( loc, "}", InlineItem::Text ) );
				break;
		}
	}

	InlineList *loadInlineExpr( c_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = new InlineList;
		c_inline::_repeat_expr_item ExprItemList = InlineExpr.ExprItemList();
		while ( !ExprItemList.end() ) {
			InlineItem *inlineItem = loadExprItem( ExprItemList.value() );
			inlineList->append( inlineItem );
			ExprItemList = ExprItemList.next();
		}
		return inlineList;
	}

	void loadActionSpec( ragel::action_spec ActionSpec )
	{
		ragel::word Name = ActionSpec.Name();
		InputLoc loc = Name.loc();
		string name = Name.text();
		InlineList *inlineList = 0;

		if ( pd->actionDict.find( name ) ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "action \"" << name << "\" already defined" << endl;
		}
		else {
			inlineList = loadInlineBlock( ActionSpec.ActionBlock().InlineBlock() );

			/* Add the action to the list of actions. */
			Action *newAction = new Action( loc, name, 
					inlineList, pd->nextCondId++ );

			/* Insert to list and dict. */
			pd->actionList.append( newAction );
			pd->actionDict.insert( newAction );
		}
	}

	void loadPrePush( ragel::action_block PrePushBlock )
	{
		InlineList *inlineList = 0;
		InputLoc loc = PrePushBlock.loc();

		if ( pd->prePushExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "pre_push code already defined" << endl;
		}

		pd->prePushExpr = inlineList;
	}

	void loadPostPop( ragel::action_block PostPopBlock )
	{
		InlineList *inlineList = 0;
		InputLoc loc = PostPopBlock.loc();

		if ( pd->postPopExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "post_pop code already defined" << endl;
		}

		pd->postPopExpr = inlineList;
	}

	void tryMachineDef( InputLoc &loc, std::string name, 
			MachineDef *machineDef, bool isInstance )
	{
		GraphDictEl *newEl = pd->graphDict.insert( name );
		if ( newEl != 0 ) {
			/* New element in the dict, all good. */
			newEl->value = new VarDef( name, machineDef );
			newEl->isInstance = isInstance;
			newEl->loc = loc;
			newEl->value->isExport = exportContext[exportContext.length()-1];

			/* It it is an instance, put on the instance list. */
			if ( isInstance )
				pd->instanceList.append( newEl );
		}
		else {
			// Recover by ignoring the duplicate.
			error(loc) << "fsm \"" << name << "\" previously defined" << endl;
		}
	}

	Literal *loadRangeLit( ragel::range_lit RL )
	{
		Literal *literal = 0;
		switch ( RL.prodName() ) {
			case ragel::range_lit::_String: {
				string s = RL.String().text();
				Token tok;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::LitString );
				break;
			}

			case ragel::range_lit::_AN: {
				string s = RL.AN().text();
				Token tok;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::Number );
				break;
			}
		}
		return literal;
	}

	Factor *loadFactor( ragel::factor FactorTree )
	{
		InputLoc loc = FactorTree.loc();
		Factor *factor = 0;
		switch ( FactorTree.prodName() ) {
			case ragel::factor::_AlphabetNum: {
				string s = FactorTree.AN().text();
				Token tok;
				tok.set( s.c_str(), s.size() );

				factor = new Factor( new Literal( tok, Literal::Number ) );
				break;
			}

			case ragel::factor::_Word: {
				string s = FactorTree.W().text();
				/* Find the named graph. */
				GraphDictEl *gdNode = pd->graphDict.find( s );
				if ( gdNode == 0 ) {
					/* Recover by returning null as the factor node. */
					error(loc) << "graph lookup of \"" << s << "\" failed" << endl;
					factor = 0;
				}
				else if ( gdNode->isInstance ) {
					/* Recover by retuning null as the factor node. */
					error(loc) << "references to graph instantiations not allowed "
							"in expressions" << endl;
					factor = 0;
				}
				else {
					/* Create a factor node that is a lookup of an expression. */
					factor = new Factor( loc, gdNode->value );
				}
				break;
			}

			case ragel::factor::_String: {
				string s = FactorTree.S().text();
				Token tok;
				tok.set( s.c_str(), s.size() );

				factor = new Factor( new Literal( tok, Literal::LitString ) );
				break;
			}
			case ragel::factor::_Union:
				break;
			case ragel::factor::_Range: {
				Literal *lit1 = loadRangeLit( FactorTree.RL1() );
				Literal *lit2 = loadRangeLit( FactorTree.RL2() );
				factor = new Factor( new Range( lit1, lit2 ) );
				break;
			}
			case ragel::factor::_Regex:
				break;
			case ragel::factor::_Join:
				Join *join = loadJoin( FactorTree.Join() );
				join->loc = loc;
				factor = new Factor( join );
				break;
		}
		return factor;
	}

	FactorWithNeg *loadFactorNeg( ragel::factor_neg FactorNeg )
	{
		ragel::factor FactorTree = FactorNeg.Factor();
		return new FactorWithNeg( loadFactor( FactorTree ) );
	}

	FactorWithRep *loadFactorRep( ragel::factor_rep FactorRep )
	{
		FactorWithRep *factorWithRep = new FactorWithRep( loadFactorNeg( FactorRep.FactorNeg() ) );
		ragel::_repeat_factor_rep_op OpList = FactorRep.OpList();
		while ( !OpList.end() ) {
			ragel::factor_rep_op FactorRepOp = OpList.value();
			InputLoc loc = FactorRepOp.loc();
			switch ( FactorRepOp.prodName() ) {
				case ragel::factor_rep_op::_Star:
					factorWithRep = new FactorWithRep( loc, factorWithRep, 
							0, 0, FactorWithRep::StarType );
					break;
					
				case ragel::factor_rep_op::_StarStar:
					factorWithRep = new FactorWithRep( loc, factorWithRep, 
							0, 0, FactorWithRep::StarStarType );
					break;
				case ragel::factor_rep_op::_Optional:
					factorWithRep = new FactorWithRep( loc, factorWithRep, 
							0, 0, FactorWithRep::OptionalType );
					break;
				case ragel::factor_rep_op::_Plus:
					factorWithRep = new FactorWithRep( loc, factorWithRep, 
							0, 0, FactorWithRep::PlusType );
					break;
			}
			OpList = OpList.next();
		}

		return factorWithRep;
	}

	Action *loadActionRef( ragel::action_ref ActionRef )
	{
		InputLoc loc = ActionRef.loc();

		switch ( ActionRef.prodName() ) {
			case ragel::action_ref::_Word: {
				string s = ActionRef.Word().text();

				/* Set the name in the actionDict. */
				Action *action = pd->actionDict.find( s );
				if ( action == 0 ) {
					/* Will recover by returning null as the action. */
					error(loc) << "action lookup of \"" << s << "\" failed" << endl;
				}
				return action;
			}
		}

		return 0;
	}

	AugType loadAugBase( ragel::aug_base AugBase )
	{
		AugType augType = at_finish;
		switch ( AugBase.prodName() ) {
			case ragel::aug_base::_Finish:
				augType = at_finish;
				break;
			case ragel::aug_base::_Enter:
				augType = at_start;
				break;
			case ragel::aug_base::_Leave:
				augType = at_leave;
				break;
			case ragel::aug_base::_All:
				augType = at_all;
				break;
		}
		return augType;
	}

	int loadPriorAug( ragel::priority_aug PriorAug )
	{
		InputLoc loc = PriorAug.loc();
		string data = PriorAug.text();
		int priorityNum = 0;

		/* Convert the priority number to a long. Check for overflow. */
		errno = 0;

		//std::cerr << "PRIOR AUG: " << $1->token.data << std::endl;
		long aug = strtol( data.c_str(), 0, 10 );
		if ( errno == ERANGE && aug == LONG_MAX ) {
			/* Priority number too large. Recover by setting the priority to 0. */
			error(loc) << "priority number " << data << 
					" overflows" << endl;
			priorityNum = 0;
		}
		else if ( errno == ERANGE && aug == LONG_MIN ) {
			/* Priority number too large in the neg. Recover by using 0. */
			error(loc) << "priority number " << data << 
					" underflows" << endl;
			priorityNum = 0;
		}
		else {
			/* No overflow or underflow. */
			priorityNum = aug;
		}
		return priorityNum;
	}

	FactorWithAug *loadFactorAug( ragel::factor_aug FactorAug )
	{
		InputLoc loc = FactorAug.loc();
		FactorWithAug *factorWithAug = 0;
		switch ( FactorAug.prodName() ) {
			case ragel::factor_aug::_ActionRef: {
				factorWithAug = loadFactorAug( FactorAug.FactorAug() );
				AugType augType = loadAugBase( FactorAug.AugBase() );
				Action *action = loadActionRef( FactorAug.ActionRef() );
				factorWithAug->actions.append( ParserAction( loc, augType, 0, action ) );
				break;
			}
			case ragel::factor_aug::_PriorEmbed: {
				factorWithAug = loadFactorAug( FactorAug.FactorAug() );
				AugType augType = loadAugBase( FactorAug.AugBase() );
				int priorityNum = loadPriorAug( FactorAug.PriorityAug() );
				factorWithAug->priorityAugs.append( PriorityAug( augType, pd->curDefPriorKey, priorityNum ) );
				break;
			}
			case ragel::factor_aug::_Base:
				factorWithAug = new FactorWithAug( loadFactorRep( FactorAug.FactorRep() ) );
				break;
		}
		return factorWithAug;
	}

	FactorWithAug *loadFactorEp( ragel::factor_ep FactorEp )
	{
		return loadFactorAug( FactorEp.FactorAug() );
	}

	FactorWithAug *loadFactorLabel( ragel::factor_label FactorLabel )
	{
		return loadFactorEp( FactorLabel.FactorEp() );
	}

	Term *loadTerm( ragel::term TermTree )
	{
		Term *term = new Term( loadFactorLabel( TermTree.FactorLabel() ) );

		/* Walk the list of terms. */
		ragel::term_op_list_short TermOpList = TermTree.TermOpList();
		while ( TermOpList.prodName() == ragel::term_op_list_short::_Terms ) {
			/* Push. */
			ragel::term_op TermOp = TermOpList.TermOp();
			FactorWithAug *factorWithAug = loadFactorLabel( TermOp.FactorLabel() );

			Term::Type type;
			switch ( TermOp.prodName() ) {
				case ragel::term_op::_None:
				case ragel::term_op::_Dot:
					type = Term::ConcatType;
					break;
				case ragel::term_op::_ColonLt:
					type = Term::RightStartType;
					break;
				case ragel::term_op::_ColonLtLt:
					type = Term::RightFinishType;
					break;
				case ragel::term_op::_GtColon:
					type = Term::LeftType;
					break;
			}
			term = new Term( term, factorWithAug, type );
			TermOpList = TermOpList.TermOpList();
		}

		return term;
	}

	Expression *loadExpression( ragel::expression ExprTree )
	{
		Expression *expr = new Expression( loadTerm( ExprTree.Term() ) );

		/* Walk the list of terms. */
		ragel::_repeat_expression_op ExprOpList = ExprTree.ExprOpList();
		while ( !ExprOpList.end() ) {
			ragel::expression_op ExprOp = ExprOpList.value();
			Expression::Type type = Expression::OrType;
			switch ( ExprOp.prodName() ) {
				case ragel::expression_op::_Or:
					type = Expression::OrType;
					break;
				case ragel::expression_op::_And:
					type = Expression::IntersectType;
					break;
				case ragel::expression_op::_Sub:
					type = Expression::SubtractType;
					break;
				case ragel::expression_op::_Ssub:
					type = Expression::StrongSubtractType;
					break;
			}
			Term *term = loadTerm( ExprOp.Term() );
			expr = new Expression( expr, term, type );
			ExprOpList = ExprOpList.next();
		}

		return expr;
	}

	Join *loadJoin( ragel::join JoinTree )
	{
		Join *join = 0;
		switch ( JoinTree.prodName() ) {
			case ragel::join::_Rec: {
				join = loadJoin( JoinTree.Join() );
				Expression *expr = loadExpression( JoinTree.Expr() );
				join->exprList.append( expr );
				break;
			}
			case ragel::join::_Base: {
				Expression *expr = loadExpression( JoinTree.Expr() );
				join = new Join( expr );
				break;
			}
		}
		return join;
	}

	MachineDef *loadLm( ragel::lm Lm )
	{
		switch ( Lm.prodName() ) {
			case ragel::lm::_Join:
				return new MachineDef( loadJoin( Lm.Join() ) );
			case ragel::lm::_Lm:
				break;
		}
		return 0;
	}

	void loadInstantiation( ragel::instantiation Instantiation )
	{
		InputLoc loc = Instantiation.loc();

		MachineDef *machineDef = loadLm( Instantiation.Lm() );

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, Instantiation.Name().text(), machineDef, true );

		//if ( $1->isSet )
		//	exportContext.remove( exportContext.length()-1 );

		/* Pass a location to join_or_lm */
		if ( machineDef->join != 0 )
			machineDef->join->loc = loc;
	}

	void loadAssignment( ragel::assignment Assignment )
	{
		InputLoc loc = Assignment.loc();

		/* Main machine must be an instance. */
		bool isInstance = false;
		string name = Assignment.Name().text();
		if ( name == mainMachine ) {
			warning(loc) << "main machine will be implicitly instantiated" << endl;
			isInstance = true;
		}

		MachineDef *machineDef = new MachineDef( loadJoin( Assignment.Join() ) );

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, Assignment.Name().text(), machineDef, isInstance );

		//if ( $1->isSet )
		//	exportContext.remove( exportContext.length()-1 );

		/* Pass a location to join_or_lm */
		if ( machineDef->join != 0 )
			machineDef->join->loc = loc;
	}

	void loadWrite( ragel::word Cmd, ragel::_repeat_word WordList )
	{
		InputItem *inputItem = new InputItem;
		inputItem->type = InputItem::Write;
		inputItem->loc = Cmd.loc();
		inputItem->name = pd->sectionName;
		inputItem->pd = pd;

		inputItem->writeArgs.append( Cmd.text() );

		while ( !WordList.end() ) {
			inputItem->writeArgs.append( WordList.value().text() );
			std::cerr << WordList.value().text() << std::endl;
			WordList = WordList.next();
		}

		id.inputItems.append( inputItem );
	}

	void loadVariable( ragel::word Var, c_inline::inline_expr InlineExpr )
	{
		InputLoc loc = InlineExpr.loc();
		InlineList *inlineList = loadInlineExpr( InlineExpr );
		bool wasSet = pd->setVariable( Var.text().c_str(), inlineList );
		if ( !wasSet )
			error(loc) << "bad variable name: " << Var.text() << endl;
	}

	void loadStatement( ragel::statement Statement )
	{
		ragel::statement::prod_name prodName = Statement.prodName();
		if ( prodName != ragel::statement::_MachineName && pd == 0 && !machineNameError ) {
			InputLoc loc = Statement.loc();
			error(loc) << "this specification has no name, nor does any previous"
				" specification" << endl;
			machineNameError = true;
		}

		if ( machineNameError )
			return;

		switch( prodName ) {
			case ragel::statement::_PrePushSpec:
				loadPrePush( Statement.PrePushBlock() );
				break;
			case ragel::statement::_PostPopSpec:
				loadPostPop( Statement.PostPopBlock() );
				break;
			case ragel::statement::_MachineName:
				loadMachineName( Statement.MachineName() );
				break;
			case ragel::statement::_ActionSpec:
				loadActionSpec( Statement.ActionSpec() );
				break;
			case ragel::statement::_Instantiation:
				loadInstantiation( Statement.Instantiation() );
				break;
			case ragel::statement::_Assignment:
				loadAssignment( Statement.Assignment() );
				break;
			case ragel::statement::_Write:
				loadWrite( Statement.Cmd(), Statement.ArgList() );
				break;
			case ragel::statement::_Variable:
				loadVariable( Statement.Var(), Statement.Reparse().ActionExpr().InlineExpr() );
				break;
		}
	}

	void loadStmtList( ragel::_repeat_statement StmtList )
	{
		while ( !StmtList.end() ) {
			loadStatement( StmtList.value() );
			StmtList = StmtList.next();
		}
	}

	void loadSection( c_host::section Section )
	{
		switch ( Section.prodName() ) {
			case c_host::section::_MultiLine:
				loadStmtList( Section.StmtList() );
				break;

			case c_host::section::_Tok:
				if ( id.inputItems.tail->type != InputItem::HostData ) {
					/* Make the first input item. */
					InputItem *inputItem = new InputItem;
					inputItem->type = InputItem::HostData;
					inputItem->loc = Section.loc();
					id.inputItems.append( inputItem );
				}

				/* If no errors and we are at the bottom of the include stack (the
				 * source file listed on the command line) then write out the data. */
				if ( includeDepth == 0 && machineSpec == 0 && machineName == 0 ) {
					string text = Section.Tok().text();
					id.inputItems.tail->data << text;
				}

				break;
		}
	}

	void load( start Start )
	{
		InputLoc loc;
		exportContext.append( false );

		c_host::_repeat_section SectionList = Start.SectionList();
		while ( !SectionList.end() ) {
			loadSection( SectionList.value() );
			SectionList = SectionList.next();
		}
	}

	void load( const char *inputFileName )
	{
		const char *argv[2];
		argv[0] = inputFileName;
		argv[1] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_set_debug( program, 0 );
		colm_run_program( program, 1, argv );

		/* Extract the parse tree. */
		start Start = RagelTree( program );
		str Error = RagelError( program );

		if ( Start == 0 ) {
			gblErrorCount += 1;
			InputLoc loc( Error.loc() );
			error(loc) << inputFileName << ": parse error: " << Error.text() << std::endl;
			return;
		}

		load( Start );

		colm_delete_program( program );
	}
};

LoadRagel *newLoadRagel( InputData &id )
{
	return new LoadRagel( id );
}

void loadRagel( LoadRagel *lr, const char *inputFileName )
{
	lr->load( inputFileName );
}

void deleteLoadRagel( LoadRagel *lr )
{
	delete lr;
}
