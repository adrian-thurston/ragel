/*
 * Copyright 2015 Adrian Thurston <thurston@colm.net>
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

#include "reducer.h"

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

void TopLevel::tryMachineDef( const InputLoc &loc, std::string name, 
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

void TopLevel::include( const InputLoc &incLoc, bool fileSpecified, string fileName, string machine )
{
	/* Stash the current section name and pd. */
	string sectionName = pd->sectionName;
	ParseData *pd0 = pd;

	const char **includeChecks = 0;
	long found = 0;

	const char *inclSectionName = machine.c_str();

	/* Implement defaults for the input file and section name. */
	if ( inclSectionName == 0 )
		inclSectionName = sectionName.c_str();

	/* Build the include checks. */
	if ( fileSpecified )
		includeChecks = pd->id->makeIncludePathChecks( curFileName, fileName.c_str() );
	else {
		char *test = new char[strlen(curFileName)+1];
		strcpy( test, curFileName );

		includeChecks = new const char*[2];

		includeChecks[0] = test;
		includeChecks[1] = 0;
	}

	/* Try to find the file. */
	ifstream *inFile = pd->id->tryOpenInclude( includeChecks, found );
	if ( inFile == 0 ) {
		id->error(incLoc) << "include: failed to locate file" << endl;
		const char **tried = includeChecks;
		while ( *tried != 0 )
			id->error(incLoc) << "include: attempted: \"" << *tried++ << '\"' << endl;

		return;
	}

	delete inFile;

//	/* Don't include anything that's already been included. */
//	if ( !pd->duplicateInclude( includeChecks[found], inclSectionName ) ) {
//		pd->includeHistory.push_back( IncludeHistoryItem( 
//				includeChecks[found], inclSectionName ) );
//
//		/* Either we are not in the lib, or a file was specifed, use the
//		 * file-based include pass. */
//		includePass.reduceFile( includeChecks[found], id->hostLang );
//	}

	const char *targetMachine0 = targetMachine;
	const char *searchMachine0 = searchMachine;

	includeDepth += 1;
	pd = 0;

	targetMachine = sectionName.c_str();
	searchMachine = machine.c_str();

	reduceFile( includeChecks[found], false );

//	if ( includePass.incItems.length() == 0 ) {
//		pd->id->error(incLoc) << "could not find machine " << machine <<
//				" in " << fileName << endp;
//	}
//	else {
//		/* Load the data into include el. Save in the dict. */
//		loadIncludeData( el, includePass, includeChecks[found] );
//		id->includeDict.insert( el );
//		includePass.incItems.empty();
//	}

	pd = pd0;
	includeDepth -= 1;

	targetMachine = targetMachine0;
	searchMachine = searchMachine0;
}

void TopLevel::importFile( std::string file )
{
	isImport = true;
	reduceFile( file.c_str(), true );
	isImport = false;
}

void TopLevel::import( const InputLoc &loc, std::string name, Literal *literal )
{
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

void TopLevel::reduceFile( const char *inputFileName, bool import )
{
	char idstr[64], imstr[64];
	sprintf( idstr, "%d", includeDepth );
	sprintf( imstr, "%d", import );

	const char *argv[7];
	argv[0] = "rlparse";
	argv[1] = inputFileName;
	argv[2] = imstr; // Import 
	argv[3] = idstr; // IncludeDepth
	argv[4] = targetMachine == 0 ? "" : targetMachine;
	argv[5] = searchMachine == 0 ? "" : searchMachine;
	argv[6] = 0;

	const char *prevCurFileName = curFileName;
	curFileName = inputFileName;

	colm_program *program = colm_new_program( &rlparse_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 6, argv );
	id->streamFileNames.append( colm_extract_fns( program ) );

	int length = 0;
	const char *err = colm_error( program, &length );
	if ( err != 0 ) {
		// std::cout << "error" << std::endl;
		id->error_plain() << string( err, length ) << std::endl;
	}

	colm_delete_program( program );

	curFileName = prevCurFileName;
}


