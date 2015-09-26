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

char *unescape( const char *s )
{
	int slen = strlen(s);
	char *out = new char[slen+1];
	char *d = out;

	for ( int i = 0; i < slen; ) {
		if ( s[i] == '\\' ) {
			switch ( s[i+1] ) {
				case '0': *d++ = '\0'; break;
				case 'a': *d++ = '\a'; break;
				case 'b': *d++ = '\b'; break;
				case 't': *d++ = '\t'; break;
				case 'n': *d++ = '\n'; break;
				case 'v': *d++ = '\v'; break;
				case 'f': *d++ = '\f'; break;
				case 'r': *d++ = '\r'; break;
				default: *d++ = s[i+1]; break;
			}
			i += 2;
		}
		else {
			*d++ = s[i];
			i += 1;
		}
	}
	*d = 0;
	return out;
}

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
	LoadRagel( InputData &id, const HostLang *hostLang,
			MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
	:
		id(id),
		section(0),
		pd(0),
		machineSpec(0),
		machineName(0),
		includeDepth(0),
		hostLang(hostLang),
		minimizeLevel(minimizeLevel),
		minimizeOpt(minimizeOpt)
	{}

	InputData &id;
	Section *section;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;
	const HostLang *hostLang;
	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

	void loadMachineStmt( ragel::word MachineName,
			const char *targetMachine, const char *searchMachine )
	{
		InputLoc sectionLoc;
		string fileName = "input.rl";
		string machine = MachineName.text();

		if ( includeDepth > 0 ) {
			/* Check if the the machine is the one we are searching for. If
			 * not, reset pd. Otherwise, rename it to target machine because we
			 * are drawing the statements into target. */
			if ( machine != searchMachine ) {
				pd = 0;
				return;
			}

			machine = targetMachine;
		}

		SectionDictEl *sdEl = id.sectionDict.find( machine.c_str() );
		if ( sdEl == 0 ) {
			Section *section = new Section( strdup(machine.c_str() ) );
			sdEl = new SectionDictEl( section->sectionName );
			sdEl->value = section;
			id.sectionDict.insert( sdEl );
		}

		ParseDataDictEl *pdEl = id.parseDataDict.find( machine );
		if ( pdEl == 0 ) {
			pdEl = new ParseDataDictEl( machine );
			pdEl->value = new ParseData( &id, fileName, machine,
					id.nextMachineId++, sectionLoc, hostLang,
					minimizeLevel, minimizeOpt );
			id.parseDataDict.insert( pdEl );
			id.parseDataList.append( pdEl->value );

		}

		section = sdEl->value;
		pd = pdEl->value;
	}

	/*
	 * Ruby Inline Loading.
	 */

	InlineItem *loadExprAny( ruby_inline::expr_any ExprAny )
	{
		string t = ExprAny.text();
		InputLoc loc = ExprAny.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadExprSymbol( ruby_inline::expr_symbol ExprSymbol )
	{
		string t = ExprSymbol.text();
		InputLoc loc = ExprSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	NameRef *loadStateRefNames( ruby_inline::state_ref_names StateRefNames )
	{
		NameRef *nameRef = 0;
		switch ( StateRefNames.prodName() ) {
			case ruby_inline::state_ref_names::Rec:
				nameRef = loadStateRefNames( StateRefNames._state_ref_names() );
				nameRef->append( StateRefNames.word().text() );
				break;
			case ruby_inline::state_ref_names::Base:
				nameRef = new NameRef;
				nameRef->append( StateRefNames.word().text() );
				break;
		}
		return nameRef;
	}

	NameRef *loadStateRef( ruby_inline::state_ref StateRef )
	{
		NameRef *nameRef = loadStateRefNames( StateRef.state_ref_names() );
		if ( StateRef.opt_name_sep().prodName() == ruby_inline::opt_name_sep::ColonColon )
			nameRef->prepend( "" );
		return nameRef;
	}

	InlineItem *loadExprInterpret( ruby_inline::expr_interpret ExprInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = ExprInterpret.loc();
		switch ( ExprInterpret.prodName() ) {
			case ruby_inline::expr_interpret::Fpc:
				inlineItem = new InlineItem( loc, InlineItem::PChar );
				break;
			case ruby_inline::expr_interpret::Fc:
				inlineItem = new InlineItem( loc, InlineItem::Char );
				break;
			case ruby_inline::expr_interpret::Fcurs:
				inlineItem = new InlineItem( loc, InlineItem::Curs );
				break;
			case ruby_inline::expr_interpret::Ftargs:
				inlineItem = new InlineItem( loc, InlineItem::Targs );
				break;
			case ruby_inline::expr_interpret::Fentry: {
				NameRef *nameRef = loadStateRef( ExprInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Entry );
				break;
			}
		}
		return inlineItem;
	}

	InlineItem *loadExprItem( ruby_inline::expr_item ExprItem )
	{
		switch ( ExprItem.prodName() ) {
			case ruby_inline::expr_item::ExprAny:
				return loadExprAny( ExprItem.expr_any() );
			case ruby_inline::expr_item::ExprSymbol:
				return loadExprSymbol( ExprItem.expr_symbol() );
			case ruby_inline::expr_item::ExprInterpret:
				return loadExprInterpret( ExprItem.expr_interpret() );
		}
		return 0;
	}

	InlineItem *loadBlockSymbol( ruby_inline::block_symbol BlockSymbol )
	{
		string t = BlockSymbol.text();
		InputLoc loc = BlockSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadBlockInterpret( ruby_inline::block_interpret BlockInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = BlockInterpret.loc();
		switch ( BlockInterpret.prodName() ) {
			case ruby_inline::block_interpret::ExprInterpret:
				inlineItem = loadExprInterpret( BlockInterpret.expr_interpret() );
				break;
			case ruby_inline::block_interpret::Fhold:
				inlineItem = new InlineItem( loc, InlineItem::Hold );
				break;
			case ruby_inline::block_interpret::Fret:
				inlineItem = new InlineItem( loc, InlineItem::Ret );
				break;
			case ruby_inline::block_interpret::Fnret:
				inlineItem = new InlineItem( loc, InlineItem::Nret );
				break;
			case ruby_inline::block_interpret::Fbreak:
				inlineItem = new InlineItem( loc, InlineItem::Break );
				break;
			case ruby_inline::block_interpret::Fnbreak:
				inlineItem = new InlineItem( loc, InlineItem::Nbreak );
				break;

			case ruby_inline::block_interpret::FgotoExpr:
				inlineItem = new InlineItem( loc, InlineItem::GotoExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ruby_inline::block_interpret::FnextExpr:
				inlineItem = new InlineItem( loc, InlineItem::NextExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ruby_inline::block_interpret::FcallExpr:
				inlineItem = new InlineItem( loc, InlineItem::CallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ruby_inline::block_interpret::FncallExpr:
				inlineItem = new InlineItem( loc, InlineItem::NcallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ruby_inline::block_interpret::Fexec:
				inlineItem = new InlineItem( loc, InlineItem::Exec );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ruby_inline::block_interpret::FgotoSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Goto );
				break;
			}
			case ruby_inline::block_interpret::FnextSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Next );
				break;
			}
			case ruby_inline::block_interpret::FcallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Call );
				break;
			}
			case ruby_inline::block_interpret::FncallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Ncall );
				break;
			}
		}
		return inlineItem;
	}

	InlineList *loadInlineBlock( InlineList *inlineList, ruby_inline::inline_block RubyInlineBlock )
	{
		ruby_inline::_repeat_block_item BlockItemList = RubyInlineBlock._repeat_block_item();
		while ( !BlockItemList.end() ) {
			loadBlockItem( inlineList, BlockItemList.value() );
			BlockItemList = BlockItemList.next();
		}
		return inlineList;
	}

	InlineList *loadInlineBlock( ruby_inline::inline_block RubyInlineBlock )
	{
		InlineList *inlineList = new InlineList;
		return loadInlineBlock( inlineList, RubyInlineBlock );
	}

	void loadBlockItem( InlineList *inlineList, ruby_inline::block_item BlockItem )
	{
		switch ( BlockItem.prodName() ) {
			case ruby_inline::block_item::ExprAny: {
				InlineItem *inlineItem = loadExprAny( BlockItem.expr_any() );
				inlineList->append( inlineItem );
				break;
			}
			case ruby_inline::block_item::BlockSymbol: {
				InlineItem *inlineItem = loadBlockSymbol( BlockItem.block_symbol() );
				inlineList->append( inlineItem );
				break;
			}
			case ruby_inline::block_item::BlockInterpret: {
				InlineItem *inlineItem = loadBlockInterpret( BlockItem.block_interpret() );
				inlineList->append( inlineItem );
				break;
			}
			case ruby_inline::block_item::RecBlock:
				InputLoc loc = BlockItem.loc();
				inlineList->append( new InlineItem( loc, "{", InlineItem::Text ) );
				loadInlineBlock( inlineList, BlockItem.inline_block() );
				inlineList->append( new InlineItem( loc, "}", InlineItem::Text ) );
				break;
		}
	}

	InlineList *loadInlineExpr( ruby_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = new InlineList;
		ruby_inline::_repeat_expr_item ExprItemList = InlineExpr._repeat_expr_item();
		while ( !ExprItemList.end() ) {
			InlineItem *inlineItem = loadExprItem( ExprItemList.value() );
			inlineList->append( inlineItem );
			ExprItemList = ExprItemList.next();
		}
		return inlineList;
	}

	/*
	 * OCaml Inline Loading.
	 */

	InlineItem *loadExprAny( ocaml_inline::expr_any ExprAny )
	{
		string t = ExprAny.text();
		InputLoc loc = ExprAny.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadExprSymbol( ocaml_inline::expr_symbol ExprSymbol )
	{
		string t = ExprSymbol.text();
		InputLoc loc = ExprSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	NameRef *loadStateRefNames( ocaml_inline::state_ref_names StateRefNames )
	{
		NameRef *nameRef = 0;
		switch ( StateRefNames.prodName() ) {
			case ocaml_inline::state_ref_names::Rec:
				nameRef = loadStateRefNames( StateRefNames._state_ref_names() );
				nameRef->append( StateRefNames.word().text() );
				break;
			case ocaml_inline::state_ref_names::Base:
				nameRef = new NameRef;
				nameRef->append( StateRefNames.word().text() );
				break;
		}
		return nameRef;
	}

	NameRef *loadStateRef( ocaml_inline::state_ref StateRef )
	{
		NameRef *nameRef = loadStateRefNames( StateRef.state_ref_names() );
		if ( StateRef.opt_name_sep().prodName() == ocaml_inline::opt_name_sep::ColonColon )
			nameRef->prepend( "" );
		return nameRef;
	}

	InlineItem *loadExprInterpret( ocaml_inline::expr_interpret ExprInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = ExprInterpret.loc();
		switch ( ExprInterpret.prodName() ) {
			case ocaml_inline::expr_interpret::Fpc:
				inlineItem = new InlineItem( loc, InlineItem::PChar );
				break;
			case ocaml_inline::expr_interpret::Fc:
				inlineItem = new InlineItem( loc, InlineItem::Char );
				break;
			case ocaml_inline::expr_interpret::Fcurs:
				inlineItem = new InlineItem( loc, InlineItem::Curs );
				break;
			case ocaml_inline::expr_interpret::Ftargs:
				inlineItem = new InlineItem( loc, InlineItem::Targs );
				break;
			case ocaml_inline::expr_interpret::Fentry: {
				NameRef *nameRef = loadStateRef( ExprInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Entry );
				break;
			}
		}
		return inlineItem;
	}

	InlineItem *loadExprItem( ocaml_inline::expr_item ExprItem )
	{
		switch ( ExprItem.prodName() ) {
			case ocaml_inline::expr_item::ExprAny:
				return loadExprAny( ExprItem.expr_any() );
			case ocaml_inline::expr_item::ExprSymbol:
				return loadExprSymbol( ExprItem.expr_symbol() );
			case ocaml_inline::expr_item::ExprInterpret:
				return loadExprInterpret( ExprItem.expr_interpret() );
		}
		return 0;
	}

	InlineItem *loadBlockSymbol( ocaml_inline::block_symbol BlockSymbol )
	{
		string t = BlockSymbol.text();
		InputLoc loc = BlockSymbol.loc();
		return new InlineItem( loc, t, InlineItem::Text );
	}

	InlineItem *loadBlockInterpret( ocaml_inline::block_interpret BlockInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = BlockInterpret.loc();
		switch ( BlockInterpret.prodName() ) {
			case ocaml_inline::block_interpret::ExprInterpret:
				inlineItem = loadExprInterpret( BlockInterpret.expr_interpret() );
				break;
			case ocaml_inline::block_interpret::Fhold:
				inlineItem = new InlineItem( loc, InlineItem::Hold );
				break;
			case ocaml_inline::block_interpret::Fret:
				inlineItem = new InlineItem( loc, InlineItem::Ret );
				break;
			case ocaml_inline::block_interpret::Fnret:
				inlineItem = new InlineItem( loc, InlineItem::Nret );
				break;
			case ocaml_inline::block_interpret::Fbreak:
				inlineItem = new InlineItem( loc, InlineItem::Break );
				break;
			case ocaml_inline::block_interpret::Fnbreak:
				inlineItem = new InlineItem( loc, InlineItem::Nbreak );
				break;

			case ocaml_inline::block_interpret::FgotoExpr:
				inlineItem = new InlineItem( loc, InlineItem::GotoExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ocaml_inline::block_interpret::FnextExpr:
				inlineItem = new InlineItem( loc, InlineItem::NextExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ocaml_inline::block_interpret::FcallExpr:
				inlineItem = new InlineItem( loc, InlineItem::CallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ocaml_inline::block_interpret::FncallExpr:
				inlineItem = new InlineItem( loc, InlineItem::NcallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ocaml_inline::block_interpret::Fexec:
				inlineItem = new InlineItem( loc, InlineItem::Exec );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case ocaml_inline::block_interpret::FgotoSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Goto );
				break;
			}
			case ocaml_inline::block_interpret::FnextSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Next );
				break;
			}
			case ocaml_inline::block_interpret::FcallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Call );
				break;
			}
			case ocaml_inline::block_interpret::FncallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Ncall );
				break;
			}
		}
		return inlineItem;
	}

	InlineList *loadInlineBlock( InlineList *inlineList, ocaml_inline::inline_block OCamlInlineBlock )
	{
		ocaml_inline::_repeat_block_item BlockItemList = OCamlInlineBlock._repeat_block_item();
		while ( !BlockItemList.end() ) {
			loadBlockItem( inlineList, BlockItemList.value() );
			BlockItemList = BlockItemList.next();
		}
		return inlineList;
	}

	InlineList *loadInlineBlock( ocaml_inline::inline_block OCamlInlineBlock )
	{
		InlineList *inlineList = new InlineList;
		return loadInlineBlock( inlineList, OCamlInlineBlock );
	}

	void loadBlockItem( InlineList *inlineList, ocaml_inline::block_item BlockItem )
	{
		switch ( BlockItem.prodName() ) {
			case ocaml_inline::block_item::ExprAny: {
				InlineItem *inlineItem = loadExprAny( BlockItem.expr_any() );
				inlineList->append( inlineItem );
				break;
			}
			case ocaml_inline::block_item::BlockSymbol: {
				InlineItem *inlineItem = loadBlockSymbol( BlockItem.block_symbol() );
				inlineList->append( inlineItem );
				break;
			}
			case ocaml_inline::block_item::BlockInterpret: {
				InlineItem *inlineItem = loadBlockInterpret( BlockItem.block_interpret() );
				inlineList->append( inlineItem );
				break;
			}
			case ocaml_inline::block_item::RecBlock:
				InputLoc loc = BlockItem.loc();
				inlineList->append( new InlineItem( loc, "{", InlineItem::Text ) );
				loadInlineBlock( inlineList, BlockItem.inline_block() );
				inlineList->append( new InlineItem( loc, "}", InlineItem::Text ) );
				break;
		}
	}

	InlineList *loadInlineExpr( ocaml_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = new InlineList;
		ocaml_inline::_repeat_expr_item ExprItemList = InlineExpr._repeat_expr_item();
		while ( !ExprItemList.end() ) {
			InlineItem *inlineItem = loadExprItem( ExprItemList.value() );
			inlineList->append( inlineItem );
			ExprItemList = ExprItemList.next();
		}
		return inlineList;
	}


	/*
	 * C Inline Loading
	 */

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
			case c_inline::state_ref_names::Rec:
				nameRef = loadStateRefNames( StateRefNames._state_ref_names() );
				nameRef->append( StateRefNames.word().text() );
				break;
			case c_inline::state_ref_names::Base:
				nameRef = new NameRef;
				nameRef->append( StateRefNames.word().text() );
				break;
		}
		return nameRef;
	}

	NameRef *loadStateRef( c_inline::state_ref StateRef )
	{
		NameRef *nameRef = loadStateRefNames( StateRef.state_ref_names() );
		if ( StateRef.opt_name_sep().prodName() == c_inline::opt_name_sep::ColonColon )
			nameRef->prepend( "" );
		return nameRef;
	}

	InlineItem *loadExprInterpret( c_inline::expr_interpret ExprInterpret )
	{
		InlineItem *inlineItem = 0;
		InputLoc loc = ExprInterpret.loc();
		switch ( ExprInterpret.prodName() ) {
			case c_inline::expr_interpret::Fpc:
				inlineItem = new InlineItem( loc, InlineItem::PChar );
				break;
			case c_inline::expr_interpret::Fc:
				inlineItem = new InlineItem( loc, InlineItem::Char );
				break;
			case c_inline::expr_interpret::Fcurs:
				inlineItem = new InlineItem( loc, InlineItem::Curs );
				break;
			case c_inline::expr_interpret::Ftargs:
				inlineItem = new InlineItem( loc, InlineItem::Targs );
				break;
			case c_inline::expr_interpret::Fentry: {
				NameRef *nameRef = loadStateRef( ExprInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Entry );
				break;
			}
		}
		return inlineItem;
	}

	InlineItem *loadExprItem( c_inline::expr_item ExprItem )
	{
		switch ( ExprItem.prodName() ) {
			case c_inline::expr_item::ExprAny:
				return loadExprAny( ExprItem.expr_any() );
			case c_inline::expr_item::ExprSymbol:
				return loadExprSymbol( ExprItem.expr_symbol() );
			case c_inline::expr_item::ExprInterpret:
				return loadExprInterpret( ExprItem.expr_interpret() );
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
			case c_inline::block_interpret::ExprInterpret:
				inlineItem = loadExprInterpret( BlockInterpret.expr_interpret() );
				break;
			case c_inline::block_interpret::Fhold:
				inlineItem = new InlineItem( loc, InlineItem::Hold );
				break;
			case c_inline::block_interpret::Fret:
				inlineItem = new InlineItem( loc, InlineItem::Ret );
				break;
			case c_inline::block_interpret::Fnret:
				inlineItem = new InlineItem( loc, InlineItem::Nret );
				break;
			case c_inline::block_interpret::Fbreak:
				inlineItem = new InlineItem( loc, InlineItem::Break );
				break;
			case c_inline::block_interpret::Fnbreak:
				inlineItem = new InlineItem( loc, InlineItem::Nbreak );
				break;

			case c_inline::block_interpret::FgotoExpr:
				inlineItem = new InlineItem( loc, InlineItem::GotoExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case c_inline::block_interpret::FnextExpr:
				inlineItem = new InlineItem( loc, InlineItem::NextExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case c_inline::block_interpret::FcallExpr:
				inlineItem = new InlineItem( loc, InlineItem::CallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case c_inline::block_interpret::FncallExpr:
				inlineItem = new InlineItem( loc, InlineItem::NcallExpr );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case c_inline::block_interpret::Fexec:
				inlineItem = new InlineItem( loc, InlineItem::Exec );
				inlineItem->children = loadInlineExpr( BlockInterpret.inline_expr() );
				break;
			case c_inline::block_interpret::FgotoSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Goto );
				break;
			}
			case c_inline::block_interpret::FnextSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Next );
				break;
			}
			case c_inline::block_interpret::FcallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Call );
				break;
			}
			case c_inline::block_interpret::FncallSr: {
				NameRef *nameRef = loadStateRef( BlockInterpret.state_ref() );
				inlineItem = new InlineItem( loc, nameRef, InlineItem::Ncall );
				break;
			}
		}
		return inlineItem;
	}

	InlineList *loadInlineBlock( InlineList *inlineList, c_inline::inline_block InlineBlock )
	{
		c_inline::_repeat_block_item BlockItemList = InlineBlock._repeat_block_item();
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
			case c_inline::block_item::ExprAny: {
				InlineItem *inlineItem = loadExprAny( BlockItem.expr_any() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::BlockSymbol: {
				InlineItem *inlineItem = loadBlockSymbol( BlockItem.block_symbol() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::BlockInterpret: {
				InlineItem *inlineItem = loadBlockInterpret( BlockItem.block_interpret() );
				inlineList->append( inlineItem );
				break;
			}
			case c_inline::block_item::RecBlock:
				InputLoc loc = BlockItem.loc();
				inlineList->append( new InlineItem( loc, "{", InlineItem::Text ) );
				loadInlineBlock( inlineList, BlockItem.inline_block() );
				inlineList->append( new InlineItem( loc, "}", InlineItem::Text ) );
				break;
		}
	}


	InlineList *loadInlineExpr( c_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = new InlineList;
		c_inline::_repeat_expr_item ExprItemList = InlineExpr._repeat_expr_item();
		while ( !ExprItemList.end() ) {
			InlineItem *inlineItem = loadExprItem( ExprItemList.value() );
			inlineList->append( inlineItem );
			ExprItemList = ExprItemList.next();
		}
		return inlineList;
	}

	/*
	 * End Inline Loading
	 */

	InlineBlock *loadInlineBlock( ragel::action_block ActionBlock )
	{
		InputLoc loc = ActionBlock.loc();

		InlineList *inlineList;
		if ( ActionBlock.RubyInlineBlock() )
			inlineList = loadInlineBlock( ActionBlock.RubyInlineBlock() );
		else if ( ActionBlock.OCamlInlineBlock() )
			inlineList = loadInlineBlock( ActionBlock.OCamlInlineBlock() );
		else
			inlineList = loadInlineBlock( ActionBlock.CInlineBlock() );
		return new InlineBlock( loc, inlineList );
	}

	Action *loadActionBlock( string name, ragel::action_block ActionBlock )
	{
		InputLoc loc = ActionBlock.loc();
		InlineList *inlineList;
		if ( ActionBlock.RubyInlineBlock() )
			inlineList = loadInlineBlock( ActionBlock.RubyInlineBlock() );
		else if ( ActionBlock.OCamlInlineBlock() )
			inlineList = loadInlineBlock( ActionBlock.OCamlInlineBlock() );
		else
			inlineList = loadInlineBlock( ActionBlock.CInlineBlock() );

		/* Add the action to the list of actions. */
		Action *newAction = new Action( loc, name, 
				inlineList, pd->nextCondId++ );

		/* Append to the action list. */
		pd->actionList.append( newAction );

		return newAction;
	}


	void loadActionSpec( ragel::action_spec ActionSpec )
	{
		ragel::word Name = ActionSpec.word();
		InputLoc loc = Name.loc();
		string name = Name.text();

		if ( pd->actionDict.find( name ) ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "action \"" << name << "\" already defined" << endl;
		}
		else {
			Action *newAction = loadActionBlock( name, ActionSpec.action_block() );
			pd->actionDict.insert( newAction );
		}
	}

	void loadPrePush( ragel::action_block PrePushBlock )
	{
		InputLoc loc = PrePushBlock.loc();

		if ( pd->prePushExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "prepush code already defined" << endl;
		}

		InlineList *inlineList;
		if ( PrePushBlock.RubyInlineBlock() )
			inlineList = loadInlineBlock( PrePushBlock.RubyInlineBlock() );
		else if ( PrePushBlock.OCamlInlineBlock() )
			inlineList = loadInlineBlock( PrePushBlock.OCamlInlineBlock() );
		else
			inlineList = loadInlineBlock( PrePushBlock.CInlineBlock() );
		pd->prePushExpr = new InlineBlock( loc, inlineList );
	}

	void loadPostPop( ragel::action_block PostPopBlock )
	{
		InputLoc loc = PostPopBlock.loc();

		if ( pd->postPopExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "postpop code already defined" << endl;
		}

		InlineList *inlineList;
		if ( PostPopBlock.RubyInlineBlock() )
			inlineList = loadInlineBlock( PostPopBlock.RubyInlineBlock() );
		else if ( PostPopBlock.OCamlInlineBlock() )
			inlineList = loadInlineBlock( PostPopBlock.OCamlInlineBlock() );
		else
			inlineList = loadInlineBlock( PostPopBlock.CInlineBlock() );
		pd->postPopExpr = new InlineBlock( loc, inlineList );
	}

	void loadNfaPrePush( ragel::action_block PrePushBlock )
	{
		InputLoc loc = PrePushBlock.loc();

		if ( pd->nfaPrePushExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "nfaprepush code already defined" << endl;
		}

		pd->nfaPrePushExpr = loadInlineBlock( PrePushBlock );
	}

	void loadNfaPostPop( ragel::action_block PostPopBlock )
	{
		InputLoc loc = PostPopBlock.loc();

		if ( pd->nfaPostPopExpr != 0 ) {
			/* Recover by just ignoring the duplicate. */
			error(loc) << "nfapostpop code already defined" << endl;
		}

		pd->nfaPostPopExpr = loadInlineBlock( PostPopBlock );
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
			if ( isInstance ) {
				pd->instanceList.append( newEl );

				InputItem *inputItem = new InputItem;
				inputItem->type = InputItem::EndSection;
				section->lastReference = inputItem;
				id.inputItems.append( inputItem );
			}
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
			case ragel::range_lit::String: {
				string s = RL.string().text();
				Token tok;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::LitString );
				break;
			}

			case ragel::range_lit::AN: {
				string s = RL.alphabet_num().text();
				Token tok;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::Number );
				break;
			}
		}
		return literal;
	}

	ReOrItem *loadRegOrChar( ragel::reg_or_char RegOrChar )
	{
		ReOrItem *orItem = 0;
		switch ( RegOrChar.prodName() ) {
			case ragel::reg_or_char::Char: {
				char *c = unescape( RegOrChar.re_or_char().text().c_str() );
				Token tok;
				tok.set( c, strlen(c) );
				orItem = new ReOrItem( RegOrChar.re_or_char().loc(), tok );
				delete[] c;
				break;
			}
			case ragel::reg_or_char::Range: {
				char *low = unescape( RegOrChar.Low().text().c_str() );
				char *high = unescape( RegOrChar.High().text().c_str() );
				orItem = new ReOrItem( RegOrChar.Low().loc(), low[0], high[0] );
				delete[] low;
				delete[] high;
				break;
			}
		}
		return orItem;
	}

	ReOrBlock *loadRegOrData( ragel::reg_or_data RegOrData )
	{
		ReOrBlock *block = 0;
		switch ( RegOrData.prodName() ) {
			case ragel::reg_or_data::Data: {
				ReOrBlock *left = loadRegOrData( RegOrData._reg_or_data() );
				ReOrItem *right = loadRegOrChar( RegOrData.reg_or_char() );
				block = new ReOrBlock( left, right );
				break;
			}
			case ragel::reg_or_data::Base: {
				block = new ReOrBlock();
				break;
			}
		}
		return block;
	}

	ReItem *loadRegexItem( ragel::reg_item RegItem )
	{
		InputLoc loc = RegItem.loc();
		ReItem *reItem = 0;
		switch ( RegItem.prodName() ) {
			case ragel::reg_item::PosOrBlock: {
				ReOrBlock *block = loadRegOrData( RegItem.reg_or_data() );
				reItem = new ReItem( loc, block, ReItem::OrBlock );
				break;
			}

			case ragel::reg_item::NegOrBlock: {
				ReOrBlock *block = loadRegOrData( RegItem.reg_or_data() );
				reItem = new ReItem( loc, block, ReItem::NegOrBlock );
				break;
			}

			case ragel::reg_item::Dot: {
				reItem = new ReItem( loc, ReItem::Dot );
				break;
			}

			case ragel::reg_item::Char: {
				char *c = unescape( RegItem.re_char().text().c_str() );
				Token tok;
				tok.set( c, strlen( c ) );
				reItem = new ReItem( loc, tok );
				delete[] c;
				break;
			}
		}
		return reItem;
	}

	ReItem *loadRegexItemRep( ragel::reg_item_rep RegItemRep )
	{
		ReItem *reItem = loadRegexItem( RegItemRep.reg_item() );
		if ( RegItemRep.prodName() == ragel::reg_item_rep::Star )
			reItem->star = true;
		return reItem;
	}

	RegExpr *loadRegex( ragel::regex Regex )
	{
		RegExpr *regExpr = new RegExpr();
		ragel::_repeat_reg_item_rep RegItemRepList = Regex._repeat_reg_item_rep();
		while ( !RegItemRepList.end() ) {
			ragel::reg_item_rep RegItemRep = RegItemRepList.value();
			ReItem *reItem = loadRegexItemRep( RegItemRep );
			regExpr = new RegExpr( regExpr, reItem );
			RegItemRepList = RegItemRepList.next();
		}

		return regExpr;
	}

	Factor *loadFactor( ragel::factor FactorTree )
	{
		InputLoc loc = FactorTree.loc();
		Factor *factor = 0;
		switch ( FactorTree.prodName() ) {
			case ragel::factor::AlphabetNum: {
				string s = FactorTree.alphabet_num().text();
				Token tok;
				tok.set( s.c_str(), s.size() );

				factor = new Factor( new Literal( tok, Literal::Number ) );
				break;
			}

			case ragel::factor::Word: {
				string s = FactorTree.word().text();
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

			case ragel::factor::String: {
				string s = FactorTree.string().text();
				Token tok;
				tok.set( s.c_str(), s.size() );

				factor = new Factor( new Literal( tok, Literal::LitString ) );
				break;
			}
			case ragel::factor::PosOrBlock: {
				ReOrBlock *block = loadRegOrData( FactorTree.reg_or_data() );
				factor = new Factor( new ReItem( loc, block, ReItem::OrBlock ) );
				break;
			}
			case ragel::factor::NegOrBlock: {
				ReOrBlock *block = loadRegOrData( FactorTree.reg_or_data() );
				factor = new Factor( new ReItem( loc, block, ReItem::NegOrBlock ) );
				break;
			}
			case ragel::factor::Regex: {
				RegExpr *regExp = loadRegex( FactorTree.regex() );
				factor = new Factor( regExp );
				break;
			}
			case ragel::factor::Range: {
				Literal *lit1 = loadRangeLit( FactorTree.RL1() );
				Literal *lit2 = loadRangeLit( FactorTree.RL2() );
				factor = new Factor( new Range( lit1, lit2, false ) );
				break;
			}
			case ragel::factor::RangeIndep: {
				Literal *lit1 = loadRangeLit( FactorTree.RL1() );
				Literal *lit2 = loadRangeLit( FactorTree.RL2() );
				factor = new Factor( new Range( lit1, lit2, true ) );
				break;
			}
			case ragel::factor::Join:
				Join *join = loadJoin( FactorTree.join() );
				join->loc = loc;
				factor = new Factor( join );
				break;
		}
		return factor;
	}

	FactorWithNeg *loadFactorNeg( ragel::factor_neg FactorNeg )
	{
		InputLoc loc = FactorNeg.loc();
		FactorWithNeg *factorWithNeg = 0;
		switch ( FactorNeg.prodName() ) {
			case ragel::factor_neg::Bang: {
				FactorWithNeg *rec = loadFactorNeg( FactorNeg._factor_neg() );
				factorWithNeg = new FactorWithNeg( loc,
						rec, FactorWithNeg::NegateType );
				break;
			}
			case ragel::factor_neg::Caret: {
				FactorWithNeg *rec = loadFactorNeg( FactorNeg._factor_neg() );
				factorWithNeg = new FactorWithNeg( loc,
						rec, FactorWithNeg::CharNegateType );
				break;
			}
			case ragel::factor_neg::Base: {
				Factor *factor = loadFactor( FactorNeg.factor() );
				factorWithNeg = new FactorWithNeg( factor );
				break;
			}
		}

		return factorWithNeg;
	}

	int loadFactorRepNum( ragel::factor_rep_num FactorRepNum )
	{
		// Convert the priority number to a long. Check for overflow.
		InputLoc loc = FactorRepNum.loc();
		errno = 0;
		long rep = strtol( FactorRepNum.text().c_str(), 0, 10 );
		if ( errno == ERANGE && rep == LONG_MAX ) {
			// Repetition too large. Recover by returing repetition 1. */
			error(loc) << "repetition number " << FactorRepNum.text() << " overflows" << endl;
			rep = 1;
		}
		return rep;
	}

	FactorWithRep *loadFactorRep( ragel::factor_rep FactorRep )
	{
		FactorWithRep *factorWithRep = 0;

		switch ( FactorRep.prodName() ) {
		case ragel::factor_rep::Op: {
			factorWithRep = new FactorWithRep( loadFactorNeg( FactorRep.factor_neg() ) );
			ragel::_repeat_factor_rep_op OpList = FactorRep._repeat_factor_rep_op();
			while ( !OpList.end() ) {
				ragel::factor_rep_op FactorRepOp = OpList.value();
				InputLoc loc = FactorRepOp.loc();
				switch ( FactorRepOp.prodName() ) {
					case ragel::factor_rep_op::Star:
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								0, 0, FactorWithRep::StarType );
						break;
						
					case ragel::factor_rep_op::StarStar:
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								0, 0, FactorWithRep::StarStarType );
						break;
					case ragel::factor_rep_op::Optional:
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								0, 0, FactorWithRep::OptionalType );
						break;
					case ragel::factor_rep_op::Plus:
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								0, 0, FactorWithRep::PlusType );
						break;
					case ragel::factor_rep_op::ExactRep: {
						int rep = loadFactorRepNum( FactorRepOp.factor_rep_num() );
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								rep, 0, FactorWithRep::ExactType );
						break;
					}
					case ragel::factor_rep_op::MaxRep: {
						int rep = loadFactorRepNum( FactorRepOp.factor_rep_num() );
						factorWithRep = new FactorWithRep( loc, factorWithRep, 
								0, rep, FactorWithRep::MaxType );
						break;
					}
					case ragel::factor_rep_op::MinRep: {
						int rep = loadFactorRepNum( FactorRepOp.factor_rep_num() );
						factorWithRep = new FactorWithRep( loc, factorWithRep,
								rep, 0, FactorWithRep::MinType );
						break;
					}
					case ragel::factor_rep_op::RangeRep: {
						int low = loadFactorRepNum( FactorRepOp.LowRep() );
						int high = loadFactorRepNum( FactorRepOp.HighRep() );
						factorWithRep = new FactorWithRep( loc, factorWithRep, 
								low, high, FactorWithRep::RangeType );
						break;
					}
				}
				OpList = OpList.next();
			}
			break;
		}

		case ragel::factor_rep::Nfa: {
			long repId = strtol( FactorRep.uint().text().c_str(), 0, 10 );
			FactorWithRep *toRepeat = loadFactorRep( FactorRep._factor_rep() );
			factorWithRep = new FactorWithRep( InputLoc(), repId, toRepeat,
					loadActionRef( FactorRep.A1() ),
					loadActionRef( FactorRep.A2() ),
					loadActionRef( FactorRep.A3() ),
					loadActionRef( FactorRep.A4() ),
					loadActionRef( FactorRep.A5() ),
					loadActionRef( FactorRep.A6() ),
					FactorWithRep::NfaRep );
			break;
		}

		case ragel::factor_rep::Cond: {
			long repId = strtol( FactorRep.uint().text().c_str(), 0, 10 );
			FactorWithRep *toRepeat = loadFactorRep( FactorRep._factor_rep() );
			factorWithRep = new FactorWithRep( InputLoc(), repId, toRepeat,
					loadActionRef( FactorRep.A1() ),
					loadActionRef( FactorRep.A2() ),
					loadActionRef( FactorRep.A3() ),
					loadActionRef( FactorRep.A4() ),
					0, 0,
					FactorWithRep::CondRep );
			break;
		}

		case ragel::factor_rep::NoMax: {
			long repId = strtol( FactorRep.uint().text().c_str(), 0, 10 );
			FactorWithRep *toRepeat = loadFactorRep( FactorRep._factor_rep() );
			factorWithRep = new FactorWithRep( InputLoc(), repId, toRepeat,
					loadActionRef( FactorRep.A1() ),
					loadActionRef( FactorRep.A2() ),
					loadActionRef( FactorRep.A3() ),
					0, 0, 0,
					FactorWithRep::NoMaxRep );
			break;
		}}

		return factorWithRep;
	}

	Action *loadActionRef( ragel::action_ref ActionRef )
	{
		InputLoc loc = ActionRef.loc();

		switch ( ActionRef.prodName() ) {
			case ragel::action_ref::NamedRef:
			case ragel::action_ref::ParenNamed: {
				string s = ActionRef.named_action_ref().text();

				/* Set the name in the actionDict. */
				Action *action = pd->actionDict.find( s );
				if ( action == 0 ) {
					/* Will recover by returning null as the action. */
					error(loc) << "action lookup of \"" << s << "\" failed" << endl;
				}
				return action;
			}
			case ragel::action_ref::Block: {
				InlineList *inlineList;
				if ( ActionRef.action_block().RubyInlineBlock() )
					inlineList = loadInlineBlock( ActionRef.action_block().RubyInlineBlock() );
				else if ( ActionRef.action_block().OCamlInlineBlock() )
					inlineList = loadInlineBlock( ActionRef.action_block().OCamlInlineBlock() );
				else
					inlineList = loadInlineBlock( ActionRef.action_block().CInlineBlock() );

				/* Create the action, add it to the list and pass up. */
				Action *action = new Action( loc, std::string(), inlineList, pd->nextCondId++ );
				pd->actionList.append( action );
				return action;
			}
		}

		return 0;
	}

	AugType loadAugBase( ragel::aug_base AugBase )
	{
		AugType augType = at_finish;
		switch ( AugBase.prodName() ) {
			case ragel::aug_base::Enter:
				augType = at_start;
				break;
			case ragel::aug_base::All:
				augType = at_all;
				break;
			case ragel::aug_base::Finish:
				augType = at_finish;
				break;
			case ragel::aug_base::Leave:
				augType = at_leave;
				break;
		}
		return augType;
	}

	AugType loadAugCond( ragel::aug_cond AugCond )
	{
		AugType augType = at_finish;
		switch ( AugCond.prodName() ) {
			case ragel::aug_cond::Start1:
			case ragel::aug_cond::Start2:
			case ragel::aug_cond::Start3:
				augType = at_start;
				break;
			case ragel::aug_cond::All1:
			case ragel::aug_cond::All2:
			case ragel::aug_cond::All3:
				augType = at_all;
				break;
			case ragel::aug_cond::Leave1:
			case ragel::aug_cond::Leave2:
			case ragel::aug_cond::Leave3:
				augType = at_leave;
				break;
		}
		return augType;
	}

	AugType loadAugToState( ragel::aug_to_state AugToState )
	{
		AugType augType = at_finish;
		switch ( AugToState.prodName() ) {
			case ragel::aug_to_state::Start1:
			case ragel::aug_to_state::Start2:
				augType = at_start_to_state;
				break;
			case ragel::aug_to_state::NotStart1:
			case ragel::aug_to_state::NotStart2:
				augType = at_not_start_to_state;
				break;
			case ragel::aug_to_state::All1:
			case ragel::aug_to_state::All2:
				augType = at_all_to_state;
				break;
			case ragel::aug_to_state::Final1:
			case ragel::aug_to_state::Final2:
				augType = at_final_to_state;
				break;
			case ragel::aug_to_state::NotFinal1:
			case ragel::aug_to_state::NotFinal2:
				augType = at_not_final_to_state;
				break;
			case ragel::aug_to_state::Middle1:
			case ragel::aug_to_state::Middle2:
				augType = at_middle_to_state;
				break;
		}
		return augType;
	}

	AugType loadAugFromState( ragel::aug_from_state AugFromState )
	{
		AugType augType = at_finish;
		switch ( AugFromState.prodName() ) {
			case ragel::aug_from_state::Start1:
			case ragel::aug_from_state::Start2:
				augType = at_start_from_state;
				break;
			case ragel::aug_from_state::NotStart1:
			case ragel::aug_from_state::NotStart2:
				augType = at_not_start_from_state;
				break;
			case ragel::aug_from_state::All1:
			case ragel::aug_from_state::All2:
				augType = at_all_from_state;
				break;
			case ragel::aug_from_state::Final1:
			case ragel::aug_from_state::Final2:
				augType = at_final_from_state;
				break;
			case ragel::aug_from_state::NotFinal1:
			case ragel::aug_from_state::NotFinal2:
				augType = at_not_final_from_state;
				break;
			case ragel::aug_from_state::Middle1:
			case ragel::aug_from_state::Middle2:
				augType = at_middle_from_state;
				break;
		}
		return augType;
	}

	AugType loadAugEof( ragel::aug_eof AugEof )
	{
		AugType augType = at_finish;
		switch ( AugEof.prodName() ) {
			case ragel::aug_eof::Start1:
			case ragel::aug_eof::Start2:
				augType = at_start_eof;
				break;
			case ragel::aug_eof::NotStart1:
			case ragel::aug_eof::NotStart2:
				augType = at_not_start_eof;
				break;
			case ragel::aug_eof::All1:
			case ragel::aug_eof::All2:
				augType = at_all_eof;
				break;
			case ragel::aug_eof::Final1:
			case ragel::aug_eof::Final2:
				augType = at_final_eof;
				break;
			case ragel::aug_eof::NotFinal1:
			case ragel::aug_eof::NotFinal2:
				augType = at_not_final_eof;
				break;
			case ragel::aug_eof::Middle1:
			case ragel::aug_eof::Middle2:
				augType = at_middle_eof;
				break;
		}
		return augType;
	}

	AugType loadAugGblError( ragel::aug_gbl_error AugGblError )
	{
		AugType augType = at_finish;
		switch ( AugGblError.prodName() ) {
			case ragel::aug_gbl_error::Start1:
			case ragel::aug_gbl_error::Start2:
				augType = at_start_gbl_error;
				break;
			case ragel::aug_gbl_error::NotStart1:
			case ragel::aug_gbl_error::NotStart2:
				augType = at_not_start_gbl_error;
				break;
			case ragel::aug_gbl_error::All1:
			case ragel::aug_gbl_error::All2:
				augType = at_all_gbl_error;
				break;
			case ragel::aug_gbl_error::Final1:
			case ragel::aug_gbl_error::Final2:
				augType = at_final_gbl_error;
				break;
			case ragel::aug_gbl_error::NotFinal1:
			case ragel::aug_gbl_error::NotFinal2:
				augType = at_not_final_gbl_error;
				break;
			case ragel::aug_gbl_error::Middle1:
			case ragel::aug_gbl_error::Middle2:
				augType = at_middle_gbl_error;
				break;
		}
		return augType;
	}

	AugType loadAugLocalError( ragel::aug_local_error AugLocalError )
	{
		AugType augType = at_finish;
		switch ( AugLocalError.prodName() ) {
			case ragel::aug_local_error::Start1:
			case ragel::aug_local_error::Start2:
				augType = at_start_local_error;
				break;
			case ragel::aug_local_error::NotStart1:
			case ragel::aug_local_error::NotStart2:
				augType = at_not_start_local_error;
				break;
			case ragel::aug_local_error::All1:
			case ragel::aug_local_error::All2:
				augType = at_all_local_error;
				break;
			case ragel::aug_local_error::Final1:
			case ragel::aug_local_error::Final2:
				augType = at_final_local_error;
				break;
			case ragel::aug_local_error::NotFinal1:
			case ragel::aug_local_error::NotFinal2:
				augType = at_not_final_local_error;
				break;
			case ragel::aug_local_error::Middle1:
			case ragel::aug_local_error::Middle2:
				augType = at_middle_local_error;
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
			case ragel::factor_aug::ActionRef: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugBase( FactorAug.aug_base() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, augType, 0, action ) );
				break;
			}
			case ragel::factor_aug::PriorEmbed: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugBase( FactorAug.aug_base() );
				int priorityNum = loadPriorAug( FactorAug.priority_aug() );
				factorWithAug->priorityAugs.append( PriorityAug( augType, pd->curDefPriorKey, priorityNum ) );
				break;
			}
			case ragel::factor_aug::NamedPriorEmbed: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugBase( FactorAug.aug_base() );

				/* Lookup/create the priority key. */
				PriorDictEl *priorDictEl;
				if ( pd->priorDict.insert( FactorAug.word().text(), pd->nextPriorKey, &priorDictEl ) )
					pd->nextPriorKey += 1;

				/* Use the inserted/found priority key. */
				int priorityName = priorDictEl->value;

				int priorityNum = loadPriorAug( FactorAug.priority_aug() );
				factorWithAug->priorityAugs.append( PriorityAug( augType, priorityName, priorityNum ) );
				break;
			}
			case ragel::factor_aug::CondEmbed: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugCond( FactorAug.aug_cond() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->conditions.append( ConditionTest( loc, 
						augType, action, true ) );
				break;
			}
			case ragel::factor_aug::NegCondEmbed: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugCond( FactorAug.aug_cond() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->conditions.append( ConditionTest( loc, 
						augType, action, false ) );
				break;
			}
			case ragel::factor_aug::ToStateAction: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugToState( FactorAug.aug_to_state() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, 
						augType, 0, action ) );
				break;
			}
			case ragel::factor_aug::FromStateAction: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugFromState( FactorAug.aug_from_state() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, 
						augType, 0, action ) );
				break;
			}
			case ragel::factor_aug::EofAction: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugEof( FactorAug.aug_eof() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, 
						augType, 0, action ) );
				break;
			}
			case ragel::factor_aug::GblErrorAction: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugGblError( FactorAug.aug_gbl_error() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, 
						augType, pd->curDefLocalErrKey, action ) );
				break;
			}
			case ragel::factor_aug::LocalErrorDef: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugLocalError( FactorAug.aug_local_error() );
				Action *action = loadActionRef( FactorAug.action_ref() );
				factorWithAug->actions.append( ParserAction( loc, 
						augType, pd->curDefLocalErrKey, action ) );
				break;
			}
			case ragel::factor_aug::LocalErrorName: {
				factorWithAug = loadFactorAug( FactorAug._factor_aug() );
				AugType augType = loadAugLocalError( FactorAug.aug_local_error() );
				Action *action = loadActionRef( FactorAug.action_ref() );

				string errName = FactorAug.word().text();
				LocalErrDictEl *localErrDictEl;
				if ( pd->localErrDict.insert( errName, pd->nextLocalErrKey, &localErrDictEl ) )
					pd->nextLocalErrKey += 1;

				factorWithAug->actions.append( ParserAction( loc, 
						augType, localErrDictEl->value, action ) );
				break;
			}
			case ragel::factor_aug::Base:
				factorWithAug = new FactorWithAug( loadFactorRep( FactorAug.factor_rep() ) );
				break;
		}
		return factorWithAug;
	}

	NameRef *loadEpsilonTarget( ragel::epsilon_target EpsilonTarget )
	{
		NameRef *nameRef = 0;
		switch ( EpsilonTarget.prodName() ) {
			case ragel::epsilon_target::Rec:
				nameRef = loadEpsilonTarget( EpsilonTarget._epsilon_target() );
				nameRef->append( EpsilonTarget.word().text() );
				break;
			case ragel::epsilon_target::Base:
				nameRef = new NameRef;
				nameRef->append( EpsilonTarget.word().text() );
				break;
		}
		return nameRef;
	}

	FactorWithAug *loadFactorEp( ragel::factor_ep FactorEp )
	{
		FactorWithAug *factorWithAug = 0;
		switch ( FactorEp.prodName() ) {
			case ragel::factor_ep::Epsilon: {
				InputLoc loc = FactorEp.loc();
				factorWithAug = loadFactorAug( FactorEp.factor_aug() );
				NameRef *nameRef = loadEpsilonTarget( FactorEp.epsilon_target() );

				/* Add the target to the list and return the factor object. */
				factorWithAug->epsilonLinks.append( EpsilonLink( loc, nameRef ) );
				break;
			}
				
			case ragel::factor_ep::Base:
				factorWithAug = loadFactorAug( FactorEp.factor_aug() );
				break;
		}
		return factorWithAug;
	}

	FactorWithAug *loadFactorLabel( ragel::factor_label FactorLabel )
	{
		FactorWithAug *factorWithAug = 0;
		switch ( FactorLabel.prodName() ) {
			case ragel::factor_label::Label: {
				InputLoc loc = FactorLabel.loc();
				string label = FactorLabel.word().text();
				factorWithAug = loadFactorLabel( FactorLabel._factor_label() );
				factorWithAug->labels.prepend( Label(loc, label) );
				break;
			}
			case ragel::factor_label::Ep:
				factorWithAug = loadFactorEp( FactorLabel.factor_ep() );
				break;
		}
		return factorWithAug;
	}

	Term *loadTerm( ragel::term TermTree )
	{
		Term *term = new Term( loadFactorLabel( TermTree.factor_label() ) );

		/* Walk the list of terms. */
		ragel::term_op_list_short TermOpList = TermTree.term_op_list_short();
		while ( TermOpList.prodName() == ragel::term_op_list_short::Terms ) {
			/* Push. */
			ragel::term_op TermOp = TermOpList.term_op();
			FactorWithAug *factorWithAug = loadFactorLabel( TermOp.factor_label() );

			Term::Type type = Term::ConcatType;
			switch ( TermOp.prodName() ) {
				case ragel::term_op::None:
				case ragel::term_op::Dot:
					type = Term::ConcatType;
					break;
				case ragel::term_op::ColonLt:
					type = Term::RightStartType;
					break;
				case ragel::term_op::ColonLtLt:
					type = Term::RightFinishType;
					break;
				case ragel::term_op::GtColon:
					type = Term::LeftType;
					break;
			}
			term = new Term( term, factorWithAug, type );
			TermOpList = TermOpList._term_op_list_short();
		}

		return term;
	}

	Expression *loadExpression( ragel::expression ExprTree )
	{
		Expression *expr = new Expression( loadTerm( ExprTree.term() ) );

		/* Walk the list of terms. */
		ragel::_repeat_expression_op ExprOpList = ExprTree._repeat_expression_op();
		while ( !ExprOpList.end() ) {
			ragel::expression_op ExprOp = ExprOpList.value();
			Expression::Type type = Expression::OrType;
			switch ( ExprOp.prodName() ) {
				case ragel::expression_op::Or:
					type = Expression::OrType;
					break;
				case ragel::expression_op::And:
					type = Expression::IntersectType;
					break;
				case ragel::expression_op::Sub:
					type = Expression::SubtractType;
					break;
				case ragel::expression_op::Ssub:
					type = Expression::StrongSubtractType;
					break;
			}
			Term *term = loadTerm( ExprOp.term() );
			expr = new Expression( expr, term, type );
			ExprOpList = ExprOpList.next();
		}

		return expr;
	}

	Join *loadJoin( ragel::join JoinTree )
	{
		Join *join = 0;
		switch ( JoinTree.prodName() ) {
			case ragel::join::Rec: {
				join = loadJoin( JoinTree._join() );
				Expression *expr = loadExpression( JoinTree.expression() );
				join->exprList.append( expr );
				break;
			}
			case ragel::join::Base: {
				Expression *expr = loadExpression( JoinTree.expression() );
				join = new Join( expr );
				break;
			}
		}
		return join;
	}

	Action *loadOptLmAction( ragel::opt_lm_act OptLmAct )
	{
		Action *action = 0;
		if ( OptLmAct.lm_act() != 0 ) {
			ragel::lm_act LmAct = OptLmAct.lm_act();
			switch ( LmAct.prodName() ) {
				case ragel::lm_act::ActionRef:
					action = loadActionRef( LmAct.action_ref() );
					break;
				case ragel::lm_act::ActionBlock:
					action = loadActionBlock( string(), LmAct.action_block() );
					break;
			}
		}
		return action;
	}

	LongestMatchPart *loadLmStmt( ragel::lm_stmt LmStmt )
	{
		LongestMatchPart *longestMatchPart = 0;
		switch ( LmStmt.prodName() ) {
			case ragel::lm_stmt::LmStmt: {
				InputLoc loc = LmStmt.loc();
				Join *join = loadJoin( LmStmt.join() );
				Action *action = loadOptLmAction( LmStmt.opt_lm_act() );

				if ( action != 0 )
					action->isLmAction = true;

				longestMatchPart = new LongestMatchPart( join, action, 
						loc, pd->nextLongestMatchId++ );

				/* Provide a location to join. Unfortunately We don't
				 * have the start of the join as in other occurances. Use the end. */
				join->loc = loc;
				break;
			}
			case ragel::lm_stmt::Assignment:
				loadAssignment( LmStmt.assignment() );
				break;
			case ragel::lm_stmt::ActionSpec:
				loadActionSpec( LmStmt.action_spec() );
				break;
		}
		return longestMatchPart;
	}

	LmPartList *loadLmStmtList( ragel::lm_stmt_list LmStmtList )
	{
		LmPartList *lmPartList = 0;
		switch ( LmStmtList.prodName() ) {
			case ragel::lm_stmt_list::Rec:
				lmPartList = loadLmStmtList( LmStmtList._lm_stmt_list() );
				break;
			case ragel::lm_stmt_list::Base:
				lmPartList = new LmPartList;
				break;
		}

		LongestMatchPart *lmPart = loadLmStmt( LmStmtList.lm_stmt() );
		if ( lmPart != 0 )
			lmPartList->append( lmPart );

		return lmPartList;
	}

	MachineDef *loadLm( ragel::lm Lm )
	{
		InputLoc loc = Lm.loc();
		MachineDef *machineDef = 0;
		switch ( Lm.prodName() ) {
			case ragel::lm::Join:
				machineDef = new MachineDef( loadJoin( Lm.join() ) );
				break;
			case ragel::lm::Lm:
				/* Create a new factor going to a longest match structure. Record
				 * in the parse data that we have a longest match. */
				LmPartList *lmPartList = loadLmStmtList( Lm.lm_stmt_list() );
				LongestMatch *lm = new LongestMatch( loc, lmPartList );
				pd->lmList.append( lm );
				for ( LmPartList::Iter lmp = *lmPartList; lmp.lte(); lmp++ )
					lmp->longestMatch = lm;
				machineDef = new MachineDef( lm );
				break;
		}
		return machineDef;
	}

	string loadMachineName( string data )
	{
		/* Make/get the priority key. The name may have already been referenced
		 * and therefore exist. */
		PriorDictEl *priorDictEl;
		if ( pd->priorDict.insert( data, pd->nextPriorKey, &priorDictEl ) )
			pd->nextPriorKey += 1;
		pd->curDefPriorKey = priorDictEl->value;

		/* Make/get the local error key. */
		LocalErrDictEl *localErrDictEl;
		if ( pd->localErrDict.insert( data, pd->nextLocalErrKey, &localErrDictEl ) )
			pd->nextLocalErrKey += 1;
		pd->curDefLocalErrKey = localErrDictEl->value;

		return data;
	}

	void loadAssignment( ragel::assignment Assignment )
	{
		InputLoc loc = Assignment.loc();
		bool exportMachine = Assignment.opt_export().prodName() == ragel::opt_export::Export;
		if ( exportMachine )
			exportContext.append( true );
		string name = loadMachineName( Assignment.word().text() );

		/* Main machine must be an instance. */
		bool isInstance = false;
		if ( name == MAIN_MACHINE ) {
			warning(loc) << "main machine will be implicitly instantiated" << endl;
			isInstance = true;
		}

		MachineDef *machineDef = new MachineDef( loadJoin( Assignment.join() ) );

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, name, machineDef, isInstance );

		if ( exportMachine )
			exportContext.remove( exportContext.length()-1 );

		/* Pass a location to join_or_lm */
		if ( machineDef->join != 0 )
			machineDef->join->loc = loc;
	}

	void loadInstantiation( ragel::instantiation Instantiation )
	{
		InputLoc loc = Instantiation.loc();
		bool exportMachine = Instantiation.opt_export().prodName() ==
				ragel::opt_export::Export;
		if ( exportMachine )
			exportContext.append( true );

		string name = loadMachineName( Instantiation.word().text() );

		MachineDef *machineDef = loadLm( Instantiation.lm() );

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, name, machineDef, true );

		if ( exportMachine )
			exportContext.remove( exportContext.length()-1 );

		/* Pass a location to join_or_lm */
		if ( machineDef->join != 0 )
			machineDef->join->loc = loc;
	}

	NfaUnion *loadNfaExpr( ragel::nfa_expr NfaExpr )
	{
		NfaUnion *nfaUnion;
		Term *term;

		if ( NfaExpr.prodName() == ragel::nfa_expr::Union ) {
			nfaUnion = loadNfaExpr( NfaExpr._nfa_expr() );
			term = loadTerm( NfaExpr.term() );
		}
		else {
			nfaUnion = new NfaUnion();
			term = loadTerm( NfaExpr.term() );
		}

		nfaUnion->terms.append( term );
		return nfaUnion;
	}

	void loadNfaUnion( ragel::nfa_union NfaUnionTree )
	{
		InputLoc loc = NfaUnionTree.loc();

		string name = loadMachineName( NfaUnionTree.word().text() );

		const char *depthText = NfaUnionTree.uint().text().c_str();
		errno = 0;
		long depth = strtol( depthText, 0, 10 );
		if ( depth == LONG_MAX && errno == ERANGE )
			error(loc) << "rounds " << depthText << " overflows" << endl;

		NfaUnion *nfaUnion = loadNfaExpr( NfaUnionTree.nfa_expr() );
		nfaUnion->roundsList = new NfaRoundVect;
		nfaUnion->roundsList->append( NfaRound( depth, 0 ) );
		MachineDef *machineDef = new MachineDef( nfaUnion );

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, name, machineDef, true );
	}

	void loadWrite( ragel::word Cmd, ragel::_repeat_word WordList )
	{
		InputItem *inputItem = new InputItem;
		inputItem->type = InputItem::Write;
		inputItem->loc = Cmd.loc();
		inputItem->name = pd->sectionName;
		inputItem->pd = pd;

		section->lastReference = inputItem;

		inputItem->writeArgs.append( Cmd.text() );

		while ( !WordList.end() ) {
			inputItem->writeArgs.append( WordList.value().text() );
			WordList = WordList.next();
		}

		id.inputItems.append( inputItem );
	}

	void loadVariable( ragel::variable_name Var, c_inline::inline_expr InlineExpr )
	{
		InputLoc loc = InlineExpr.loc();
		InlineList *inlineList = loadInlineExpr( InlineExpr );
		bool wasSet = pd->setVariable( Var.text().c_str(), inlineList );
		if ( !wasSet )
			error(loc) << "bad variable name: " << Var.text() << endl;
	}

	void loadGetKey( c_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = loadInlineExpr( InlineExpr );
		pd->getKeyExpr = inlineList;
	}

	void loadAlphType( ragel::alphtype_type AlphTypeType )
	{
		InputLoc loc = AlphTypeType.loc();
		switch ( AlphTypeType.prodName() ) {
			case ragel::alphtype_type::One: {
				string one = AlphTypeType.W1().text();
				if ( ! pd->setAlphType( loc, hostLang, one.c_str() ) ) {
					// Recover by ignoring the alphtype statement.
					error(loc) << "\"" << one << 
							"\" is not a valid alphabet type" << endl;
				}
				break;
			}

			case ragel::alphtype_type::Two: {
				string one = AlphTypeType.W1().text();
				string two = AlphTypeType.W2().text();
				if ( ! pd->setAlphType( loc, hostLang, one.c_str(), two.c_str() ) ) {
					// Recover by ignoring the alphtype statement.
					error(loc) << "\"" << one << 
							"\" is not a valid alphabet type" << endl;
				}
				break;
			}
		}
	}

	void loadAccess( c_inline::inline_expr InlineExpr )
	{
		InlineList *inlineList = loadInlineExpr( InlineExpr );
		pd->accessExpr = inlineList;
	}

	void loadInclude( ragel::include_spec IncludeSpec )
	{
		string machine = pd->sectionName;
		string fileName = id.inputFileName;

		if ( IncludeSpec.word() != 0 )
			machine = IncludeSpec.word().text();

		if ( IncludeSpec.string() != 0 ) {
			fileName = IncludeSpec.string().text();

			InputLoc loc = IncludeSpec.loc();
			long length;
			bool caseInsensitive;
			char *unescaped = prepareLitString( loc, fileName.c_str(), fileName.size(),
					length, caseInsensitive );
			fileName = unescaped;
		}

		string sectionName = pd->sectionName;

		ParseData *savedPd = pd;
		pd = 0;
		includeDepth += 1;
		loadFile( fileName.c_str(), sectionName.c_str(), machine.c_str() );
		includeDepth -= 1;
		pd = savedPd;
	}

	void loadImport( import Import )
	{
		InputLoc loc = Import.loc();
		string name = loadMachineName( Import.Name().text() );

		Literal *literal = 0;
		switch ( Import.Val().prodName() ) {
			case import_val::String: {
				string s = Import.Val().string().text();
				Token tok;
				tok.loc.fileName = loc.fileName;
				tok.loc.line = loc.line;
				tok.loc.col = loc.col;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::LitString );
				break;
			}

			case import_val::Number: {
				string s = Import.Val().number().text();
				Token tok;
				tok.loc.fileName = loc.fileName;
				tok.loc.line = loc.line;
				tok.loc.col = loc.col;
				tok.set( s.c_str(), s.size() );
				literal = new Literal( tok, Literal::Number );
				break;
			}
		}

		MachineDef *machineDef = new MachineDef(
			new Join(
				new Expression(
					new Term(
						new FactorWithAug(
							new FactorWithRep(
								new FactorWithNeg( new Factor( literal ) )
							)
						)
					)
				)
			)
		);

		/* Generic creation of machine for instantiation and assignment. */
		tryMachineDef( loc, name, machineDef, false );
		machineDef->join->loc = loc;
	}
	
	void loadImportList( _repeat_import ImportList )
	{
		while ( !ImportList.end() ) {
			loadImport( ImportList.value() );
			ImportList = ImportList.next();
		}
	}

	void loadImport( ragel::string ImportFn )
	{
		InputLoc loc = ImportFn.loc();
		std::string fileName = ImportFn.text();

		long length;
		bool caseInsensitive;
		char *unescaped = prepareLitString( loc,
					fileName.c_str(), fileName.size(),
					length, caseInsensitive );

		const char *argv[5];
		argv[0] = "rlparse";
		argv[1] = "import";
		argv[2] = unescaped;
		argv[3] = id.hostLang->rlhcArg;
		argv[4] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_set_debug( program, 0 );
		colm_run_program( program, 4, argv );

		/* Extract the parse tree. */
		start Start = RagelTree( program );
		str Error = RagelError( program );
		_repeat_import ImportList = RagelImport( program );

		if ( Start == 0 ) {
			gblErrorCount += 1;
			InputLoc loc( Error.loc() );
			error(loc) << fileName << ": parse error: " << Error.text() << std::endl;
			return;
		}

		loadImportList( ImportList );
		colm_delete_program( program );
	}

	/* Return true if we are to stop processing the statement list. */
	bool loadStatement( ragel::statement Statement )
	{
		ragel::statement::prod_name prodName = Statement.prodName();
		InputLoc loc = Statement.loc();

		if ( pd == 0 ) {
			/* No parse data can happen for two reasons. Either because none
			 * was given, or because we are in an included file and the machine
			 * name did not match the machine name we were looking for. */
			if ( includeDepth == 0 ) {
				error(loc) << "this specification has no name, nor does any previous"
					" specification" << endl;
			}

			return true;
		}

		switch( prodName ) {
			case ragel::statement::PrePush:
				loadPrePush( Statement.action_block() );
				break;
			case ragel::statement::PostPop:
				loadPostPop( Statement.action_block() );
				break;
			case ragel::statement::MachineName:
				error(loc) << "machine statements must be the first statement" << endl;
				break;
			case ragel::statement::ActionSpec:
				loadActionSpec( Statement.action_spec() );
				break;
			case ragel::statement::Instantiation:
				loadInstantiation( Statement.instantiation() );
				break;
			case ragel::statement::NfaUnion:
				loadNfaUnion( Statement.nfa_union() );
				break;
			case ragel::statement::Assignment:
				loadAssignment( Statement.assignment() );
				break;
			case ragel::statement::Write:
				loadWrite( Statement.Cmd(), Statement.ArgList() );
				break;
			case ragel::statement::Variable:
				loadVariable( Statement.variable_name(),
						Statement.inline_expr_reparse().action_expr().inline_expr() );
				break;
			case ragel::statement::GetKey:
				loadGetKey( Statement.inline_expr_reparse().action_expr().inline_expr() );
				break;
			case ragel::statement::AlphType:
				loadAlphType( Statement.alphtype_type() );
				break;
			case ragel::statement::Access:
				loadAccess( Statement.inline_expr_reparse().action_expr().inline_expr() );
				break;
			case ragel::statement::Include:
				loadInclude( Statement.include_spec() );
				break;
			case ragel::statement::Import:
				loadImport( Statement.string() );
				break;
			case ragel::statement::NfaPrePush:
				loadNfaPrePush( Statement.action_block() );
				break;
			case ragel::statement::NfaPostPop:
				loadNfaPostPop( Statement.action_block() );
				break;
		}
		return false;
	}

	void loadStmtList( ragel::_repeat_statement StmtList, const char *targetMachine,
			const char *searchMachine )
	{
		if ( StmtList.end() )
			return;

		/* Handle machine statements separately. They must appear at the head
		 * of a block. */
		ragel::statement::prod_name prodName = StmtList.value().prodName();
		if ( prodName == ragel::statement::MachineName ) {
			/* Process the name, move over it. */
			loadMachineStmt( StmtList.value().machine_name().word(), targetMachine, searchMachine );
			StmtList = StmtList.next();
		}

		while ( !StmtList.end() ) {
			bool stop = loadStatement( StmtList.value() );
			if ( stop )
				break;
			StmtList = StmtList.next();
		}
	}

	void loadSection( c_host::section Section, const char *targetMachine,
			const char *searchMachine )
	{
		switch ( Section.prodName() ) {
			case c_host::section::MultiLine:
				loadStmtList( Section.ragel_start()._repeat_statement(),
						targetMachine, searchMachine );
				break;

			case c_host::section::Tok:
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
					string text = Section.tok().text();
					id.inputItems.tail->data << text;
				}

				break;
		}
	}

	void loadSection( ruby_host::section Section, const char *targetMachine,
			const char *searchMachine )
	{
		switch ( Section.prodName() ) {
			case ruby_host::section::MultiLine:
				loadStmtList( Section.ragel_start()._repeat_statement(),
						targetMachine, searchMachine );
				break;

			case ruby_host::section::Tok:
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
					string text = Section.tok().text();
					id.inputItems.tail->data << text;
				}

				break;
		}
	}

	void loadSection( ocaml_host::section Section, const char *targetMachine,
			const char *searchMachine )
	{
		switch ( Section.prodName() ) {
			case ocaml_host::section::MultiLine:
				loadStmtList( Section.ragel_start()._repeat_statement(),
						targetMachine, searchMachine );
				break;

			case ocaml_host::section::Tok:
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
					string text = Section.tok().text();
					id.inputItems.tail->data << text;
				}

				break;
		}
	}


	void load( start Start, const char *targetMachine, const char *searchMachine )
	{
		InputLoc loc;
		exportContext.append( false );

		c_host::_repeat_section SectionList = Start.SectionList();
		if ( SectionList ) {
			while ( !SectionList.end() ) {
				loadSection( SectionList.value(), targetMachine, searchMachine );
				SectionList = SectionList.next();
			}
		}
		else if ( Start.RSectionList() ) {
			ruby_host::_repeat_section RSectionList = Start.RSectionList();
			if ( RSectionList ) {
				while ( !RSectionList.end() ) {
					loadSection( RSectionList.value(), targetMachine, searchMachine );
					RSectionList = RSectionList.next();
				}
			}
		}
		else {
			ocaml_host::_repeat_section OSectionList = Start.OSectionList();
			if ( OSectionList ) {
				while ( !OSectionList.end() ) {
					loadSection( OSectionList.value(), targetMachine, searchMachine );
					OSectionList = OSectionList.next();
				}
			}
		}
	}

	void loadFile( const char *inputFileName, const char *targetMachine,
			const char *searchMachine )
	{
		const char *argv[4];
		argv[0] = "load-ragel";
		argv[1] = inputFileName;
		argv[2] = id.hostLang->rlhcArg;
		argv[3] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_set_debug( program, 0 );
		colm_run_program( program, 3, argv );

		/* Extract the parse tree. */
		start Start = RagelTree( program );
		str Error = RagelError( program );

		if ( Start == 0 ) {
			gblErrorCount += 1;
			InputLoc loc( Error.loc() );
			error(loc) << inputFileName << ": parse error: " << Error.text() << std::endl;
			return;
		}

		load( Start, targetMachine, searchMachine );

		colm_delete_program( program );
	}
};

LoadRagel *newLoadRagel( InputData &id, const HostLang *hostLang,
		MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
{
	return new LoadRagel( id, hostLang, minimizeLevel, minimizeOpt );
}

void loadRagel( LoadRagel *lr, const char *inputFileName )
{
	lr->loadFile( inputFileName, 0, 0 );
}

void deleteLoadRagel( LoadRagel *lr )
{
	delete lr;
}
