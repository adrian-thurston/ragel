/*
 *  Copyright 2015 Adrian Thurston <thurston@complang.org>
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

#include "load.h"
#include "if.h"
#include "ragel.h"
#include "inputdata.h"
#include "parsedata.h"
#include "parsetree.h"

#include <colm/colm.h>
#include <colm/tree.h>
#include <errno.h>
#include <fstream>

using std::endl;
using std::ifstream;

extern colm_sections rlparse_object;

char *unescape( const char *s, int slen )
{
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

char *unescape( const char *s )
{
	return unescape( s, strlen(s) );
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

	if ( fileName == 0 )
		fileName = "-";
}

struct LoadRagel
{
	LoadRagel( InputData &id, const HostLang *hostLang,
			MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
	:
		id(id),
		section(0),
		pd(0),
		hostLang(hostLang),
		minimizeLevel(minimizeLevel),
		minimizeOpt(minimizeOpt)
	{}

	InputData &id;
	Section *section;
	ParseData *pd;
	const HostLang *hostLang;
	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

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
			pd->id->error(loc) << "fsm \"" << name << "\" previously defined" << endl;
		}
	}

	string loadMachineName( string data )
	{
		/* Make/get the priority key. The name may have already been referenced
		 * and therefore exist. */
		PriorDictEl *priorDictEl;
		if ( pd->priorDict.insert( data, pd->fsmCtx->nextPriorKey, &priorDictEl ) )
			pd->fsmCtx->nextPriorKey += 1;
		pd->curDefPriorKey = priorDictEl->value;

		/* Make/get the local error key. */
		LocalErrDictEl *localErrDictEl;
		if ( pd->localErrDict.insert( data, pd->nextLocalErrKey, &localErrDictEl ) )
			pd->nextLocalErrKey += 1;
		pd->curDefLocalErrKey = localErrDictEl->value;

		return data;
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
				tok.set( s.c_str(), s.size(), loc );
				literal = new Literal( tok.loc, false, tok.data, tok.length, Literal::LitString );
				break;
			}

			case import_val::Number: {
				string s = Import.Val().number().text();
				Token tok;
				tok.set( s.c_str(), s.size(), loc );
				literal = new Literal( tok.loc, false, tok.data, tok.length, Literal::Number );
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
		char *unescaped = prepareLitString( pd->id, loc,
					fileName.c_str(), fileName.size(),
					length, caseInsensitive );

		const char *argv[5];
		argv[0] = "rlparse";
		argv[1] = "import-file";
		argv[2] = unescaped;
		argv[3] = id.hostLang->rlhcArg;
		argv[4] = 0;

		colm_program *program = colm_new_program( &rlparse_object );
		colm_set_debug( program, 0 );
		colm_run_program( program, 4, argv );

		/* Extract the parse tree. */
		start Start = RagelTree( program );
		str Error = RagelError( program );
		_repeat_import ImportList = RagelImport( program );

		if ( Start == 0 ) {
			pd->id->error(Error.loc()) << fileName << ": parse error: " << Error.text() << std::endl;
			return;
		}

		loadImportList( ImportList );

		id.streamFileNames.append( colm_extract_fns( program ) );

		colm_delete_program( program );
	}
};
