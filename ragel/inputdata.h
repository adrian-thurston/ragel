/*
 *  Copyright 2008 Adrian Thurston <thurston@complang.org>
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

#ifndef _INPUT_DATA
#define _INPUT_DATA

#include "gendata.h"

struct InputData
{
	InputData( const char *sourceFileName, bool outputActive, bool wantComplete ) : 
		sourceFileName(sourceFileName),
		outStream(0),
		outputActive(outputActive),
		wantComplete(wantComplete),
		cgd(0) { }

	void openOutput();

	/* The name of the root section, this does not change during an include. */
	const char *sourceFileName;
	ostream *outStream;
	bool outputActive;
	bool wantComplete;

	CodeGenData *cgd;
	CodeGenMap codeGenMap;

	Vector <char*> writeOptions;
};

void generateReduced( const char *sourceFileName, bool outputActive, bool wantComplete );

#endif
