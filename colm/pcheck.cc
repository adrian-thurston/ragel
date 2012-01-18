/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "pcheck.h"
#include <assert.h>

/* Construct a new parameter checker with for paramSpec. */
ParamCheck::ParamCheck( const char *paramSpec, int argc,  const char **argv )
:
	state(noparam),
	argOffset(0),
	curArg(0),
	iCurArg(1),
	paramSpec(paramSpec), 
	argc(argc), 
	argv(argv)
{
}

/* Check a single option. Returns the index of the next parameter.  Sets p to
 * the arg character if valid, 0 otherwise.  Sets parg to the parameter arg if
 * there is one, NULL otherwise. */
bool ParamCheck::check()
{
	bool requiresParam;

	if ( iCurArg >= argc ) {            /* Off the end of the arg list. */
		state = noparam;
		return false;
	}

	if ( argOffset != 0 && *argOffset == 0 ) {
		/* We are at the end of an arg string. */
		iCurArg += 1;
		if ( iCurArg >= argc ) {
			state = noparam;
			return false;
		}
		argOffset = 0;
	}

	if ( argOffset == 0 ) {
		/* Set the current arg. */
		curArg = argv[iCurArg];

		/* We are at the beginning of an arg string. */
		if ( argv[iCurArg] == 0 ||        /* Argv[iCurArg] is null. */
			 argv[iCurArg][0] != '-' ||   /* Not a param. */
			 argv[iCurArg][1] == 0 ) {    /* Only a dash. */
			parameter = 0;
			parameterArg = 0;

			iCurArg += 1;
			state = noparam;
			return true;
		}
		argOffset = argv[iCurArg] + 1;
	}

	/* Get the arg char. */
	char argChar = *argOffset;
	
	/* Loop over all the parms and look for a match. */
	const char *pSpec = paramSpec;
	while ( *pSpec != 0 ) {
		char pSpecChar = *pSpec;

		/* If there is a ':' following the char then
		 * it requires a parm.  If a parm is required
		 * then move ahead two in the parmspec. Otherwise
		 * move ahead one in the parm spec. */
		if ( pSpec[1] == ':' ) {
			requiresParam = true;
			pSpec += 2;
		}
		else {
			requiresParam = false;
			pSpec += 1;
		}

		/* Do we have a match. */
		if ( argChar == pSpecChar ) {
			if ( requiresParam ) {
				if ( argOffset[1] == 0 ) {
					/* The param must follow. */
					if ( iCurArg + 1 == argc ) {
						/* We are the last arg so there
						 * cannot be a parameter to it. */
						parameter = argChar;
						parameterArg = 0;
						iCurArg += 1;
						argOffset = 0;
						state = invalid;
						return true;
					}
					else {
						/* the parameter to the arg is the next arg. */
						parameter = pSpecChar;
						parameterArg = argv[iCurArg + 1];
						iCurArg += 2;
						argOffset = 0;
						state = match;
						return true;
					}
				}
				else {
					/* The param for the arg is built in. */
					parameter = pSpecChar;
					parameterArg = argOffset + 1;
					iCurArg += 1;
					argOffset = 0;
					state = match;
					return true;
				}
			}
			else {
				/* Good, we matched the parm and no
				 * arg is required. */
				parameter = pSpecChar;
				parameterArg = 0;
				argOffset += 1;
				state = match;
				return true;
			}
		}
	}

	/* We did not find a match. Bad Argument. */
	parameter = argChar;
	parameterArg = 0;
	argOffset += 1;
	state = invalid;
	return true;
}


