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

#include "reducer.h"
#include "if.h"

#include <colm/colm.h>
#include <colm/tree.h>

#include <errno.h>

using std::endl;


void TopLevel::loadMachineName( string data )
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
}

void TopLevel::tryMachineDef( InputLoc &loc, std::string name, 
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
	
long TopLevel::tryLongScan( const InputLoc &loc, const char *data )
{
	/* Convert the priority number to a long. Check for overflow. */
	long priorityNum;
	errno = 0;

	long aug = strtol( data, 0, 10 );
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

void TopLevel::include( string fileName, string machine )
{
	/* Stash the current section name and pd. */
	string sectionName = pd->sectionName;
	ParseData *pd0 = pd;

	IncludeRec *el = id->includeDict.find( FnMachine( fileName, machine ) );
	if ( el == 0 ) {
		el = new IncludeRec( fileName, machine );

		InputData idr;
		IncludePass includePass;
		includePass.reduceFile( fileName.c_str(), id->hostLang );

		/* Count bytes. */
		int len = 0;
		for ( IncItem *ii = includePass.incItems.head; ii != 0; ii = ii->next ) {
			if ( ii->section != 0 && ii->section->sectionName == machine )
				len += ii->end - ii->start + 3;
		}

		/* Load bytes. */
		el->data = new char[len+1];
		len = 0;
		for ( IncItem *ii = includePass.incItems.head; ii != 0; ii = ii->next ) {
			if ( ii->section != 0 && ii->section->sectionName == machine ) {
				std::ifstream f( fileName.c_str() );
				f.seekg( ii->start, std::ios::beg );
				f.read( el->data + len, ii->end - ii->start + 3 );
				len += f.gcount();
			}
		}
		el->data[len] = 0;
		el->len = len;

		id->includeDict.insert( el );

		includePass.incItems.empty();
	}

	const char *targetMachine0 = targetMachine;
	const char *searchMachine0 = searchMachine;

	includeDepth += 1;
	pd = 0;

	targetMachine = sectionName.c_str();
	searchMachine = machine.c_str();

	reduceString( el->data );

	pd = pd0;
	includeDepth -= 1;

	targetMachine = targetMachine0;
	searchMachine = searchMachine0;
}


void TopLevel::reduceString( const char *data )
{
	const char *argv[6];
	argv[0] = "rlparse";
	argv[1] = "string";
	argv[2] = "-";
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = data;
	argv[5] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 5, argv );
	colm_delete_program( program );

}

void TopLevel::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "reduce";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	colm_delete_program( program );
}

void TopLevel::loadImport( std::string fileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "import";
	argv[2] = fileName.c_str();
	argv[3] = id->hostLang->rlhcArg;
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

	while ( !ImportList.end() ) {
		import Import = ImportList.value();

		InputLoc loc = Import.loc();
		string name = Import.Name().text();
		loadMachineName( name );

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

		ImportList = ImportList.next();
	}

	colm_delete_program( program );
}

void SectionPass::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "section";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	colm_delete_program( program );
}

void IncludePass::reduceFile( const char *inputFileName, const HostLang *hostLang )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "include";
	argv[2] = inputFileName;
	argv[3] = hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	colm_delete_program( program );
}
