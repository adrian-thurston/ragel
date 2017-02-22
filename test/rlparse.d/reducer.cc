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
using std::ifstream;
extern colm_sections rlparse_object;

void TopLevel::loadMachineName( string data )
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
		pd->id->error(loc) << "fsm \"" << name << "\" previously defined" << endl;
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
		pd->id->error(loc) << "priority number " << data << 
				" overflows" << endl;
		priorityNum = 0;
	}
	else if ( errno == ERANGE && aug == LONG_MIN ) {
		/* Priority number too large in the neg. Recover by using 0. */
		pd->id->error(loc) << "priority number " << data << 
				" underflows" << endl;
		priorityNum = 0;
	}
	else {
		/* No overflow or underflow. */
		priorityNum = aug;
	}

	return priorityNum;
}

void TopLevel::loadIncludeData( IncludeRec *el, IncludePass &includePass, const string &fileName )
{
	/* Count bytes. */
	size_t len = 0;
	for ( IncItem *ii = includePass.incItems.head; ii != 0; ii = ii->next )
		len += ii->length;

	/* Store bytes. */
	el->data = new char[len+1];
	len = 0;

	if ( id->inLibRagel ) {
		for ( IncItem *ii = includePass.incItems.head; ii != 0; ii = ii->next ) {
			memcpy( el->data + len, id->input + ii->start, ii->length );
			len += ii->length;
		}
	}
	else {
		for ( IncItem *ii = includePass.incItems.head; ii != 0; ii = ii->next ) {
			std::ifstream f( fileName.c_str() );

			f.seekg( ii->start, std::ios::beg );
			f.read( el->data + len, ii->length );
			size_t read = f.gcount();
			if ( read != ii->length ) {
				pd->id->error(ii->loc) << "unexpected length in read of included file: "
						"possible change to file" << endp;
			}
			len += read;
		}
	}

	el->data[len] = 0;
	el->len = len;
}

void TopLevel::include( const InputLoc &incLoc, bool fileSpecified, string fileName, string machine )
{
	/* Stash the current section name and pd. */
	string sectionName = pd->sectionName;
	ParseData *pd0 = pd;

	IncludeRec *el = id->includeDict.find( FnMachine( fileName, machine ) );
	if ( el == 0 ) {
		el = new IncludeRec( fileName, machine );

		const char **includeChecks = 0;
		long found = 0;

		/* First collect the locations of the text using an include pass. */
		IncludePass includePass( id, machine );
		if ( id->inLibRagel && !fileSpecified ) {
			el->foundFileName = curFileName;

			/* In LibRagel and no file was specified in the include statement.
			 * In this case we run the include pass on the input text supplied. */
			includePass.reduceStr( fileName.c_str(), id->hostLang, id->input );
		}
		else {
			const char *inclSectionName = machine.c_str();

			/* Implement defaults for the input file and section name. */
			if ( inclSectionName == 0 )
				inclSectionName = sectionName.c_str();

			if ( fileSpecified )
				includeChecks = pd->id->makeIncludePathChecks( curFileName, fileName.c_str() );
			else {
				char *test = new char[strlen(curFileName)+1];
				strcpy( test, curFileName );

				includeChecks = new const char*[2];

				includeChecks[0] = test;
				includeChecks[1] = 0;
			}

			ifstream *inFile = pd->id->tryOpenInclude( includeChecks, found );
			if ( inFile == 0 ) {
				id->error(incLoc) << "include: failed to locate file" << endl;
				const char **tried = includeChecks;
				while ( *tried != 0 )
					id->error(incLoc) << "include: attempted: \"" << *tried++ << '\"' << endl;
			}
			else {
				delete inFile;
				el->foundFileName = curFileName;

				/* Don't include anything that's already been included. */
				if ( !pd->duplicateInclude( includeChecks[found], inclSectionName ) ) {
					pd->includeHistory.push_back( IncludeHistoryItem( 
							includeChecks[found], inclSectionName ) );

					/* Either we are not in the lib, or a file was specifed, use the
					 * file-based include pass. */
					includePass.reduceFile( includeChecks[found], id->hostLang );
				}
			}
		}

		if ( includePass.incItems.length() == 0 ) {
			pd->id->error(incLoc) << "could not find machine " << machine <<
					" in " << fileName << endp;
		}
		else {
			/* Load the data into include el. Save in the dict. */
			loadIncludeData( el, includePass, includeChecks[found] );
			id->includeDict.insert( el );
			includePass.incItems.empty();
		}
	}

	const char *targetMachine0 = targetMachine;
	const char *searchMachine0 = searchMachine;

	includeDepth += 1;
	pd = 0;

	targetMachine = sectionName.c_str();
	searchMachine = machine.c_str();

	reduceStr( el->foundFileName.c_str(), el->data );

	pd = pd0;
	includeDepth -= 1;

	targetMachine = targetMachine0;
	searchMachine = searchMachine0;
}

void TopLevel::loadImport( std::string fileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "import-file";
	argv[2] = fileName.c_str();
	argv[3] = id->hostLang->rlhcArg;
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
				tok.set( s.c_str(), s.size(), loc );
				literal = new Literal( loc, false, tok.data, tok.length, Literal::LitString );
				break;
			}

			case import_val::Number: {
				string s = Import.Val().number().text();
				Token tok;
				tok.set( s.c_str(), s.size(), loc );
				literal = new Literal( loc, false, tok.data, tok.length, Literal::Number );
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

	id->streamFileNames.append( colm_extract_fns( program ) );
	colm_delete_program( program );
}

void TopLevel::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "toplevel-reduce-file";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	const char *prevCurFileName = curFileName;
	curFileName = inputFileName;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << Error.text() << std::endl;

	colm_delete_program( program );

	curFileName = prevCurFileName;
}

void TopLevel::reduceStr( const char *inputFileName, const char *input )
{
	const char *argv[6];
	argv[0] = "rlparse";
	argv[1] = "toplevel-reduce-str";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = input;
	argv[5] = 0;

	const char *prevCurFileName = curFileName;
	curFileName = inputFileName;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 5, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << "trs: " << Error.text() << std::endl;

	colm_delete_program( program );

	curFileName = prevCurFileName;
}


void SectionPass::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "section-reduce-file";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << Error.text() << std::endl;

	colm_delete_program( program );
}

void SectionPass::reduceStr( const char *inputFileName, const char *input )
{
	const char *argv[6];
	argv[0] = "rlparse";
	argv[1] = "section-reduce-str";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = input;
	argv[5] = 0;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 5, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << "srs: " << Error.text() << std::endl;

	colm_delete_program( program );
}


void IncludePass::reduceFile( const char *inputFileName, const HostLang *hostLang )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "include-reduce-file";
	argv[2] = inputFileName;
	argv[3] = hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << Error.text() << std::endl;

	colm_delete_program( program );
}

void IncludePass::reduceStr( const char *inputFileName, const HostLang *hostLang, const char *input )
{
	const char *argv[6];
	argv[0] = "rlparse";
	argv[1] = "include-reduce-str";
	argv[2] = inputFileName;
	argv[3] = hostLang->rlhcArg;
	argv[4] = input;
	argv[5] = 0;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 5, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	str Error = RagelError( program );
	if ( Error != 0 )
		id->error(Error.loc()) << Error.text() << std::endl;

	colm_delete_program( program );
}
