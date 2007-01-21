/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@cs.queensu.ca>
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

#ifndef _RLCODEGEN_H
#define _RLCODEGEN_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include "avltree.h"
#include "vector.h"
#include "config.h"

#define PROGNAME "rlcodegen"

/* Target language. */
enum OutputFormat
{
	OutCode,
	OutGraphvizDot
};

/* Target output style. */
enum CodeStyleEnum
{
	GenTables,
	GenFTables,
	GenFlat,
	GenFFlat,
	GenGoto,
	GenFGoto,
	GenIpGoto,
	GenSplit
};

/* Filter on the output stream that keeps track of the number of lines
 * output. */
class output_filter : public std::filebuf
{
public:
	output_filter() : line(1) { }

	virtual int sync();
	virtual std::streamsize xsputn(const char* s, std::streamsize n);

	int line;
};
	
extern OutputFormat outputFormat;
extern CodeStyleEnum codeStyle;

/* IO filenames and stream. */
extern char *outputFileName;
extern std::ostream *outStream;
extern output_filter *outFilter;

extern bool printPrintables;
extern bool graphvizDone;

int xml_parse( std::istream &input, char *fileName );

extern int gblErrorCount;
extern char machineMain[];

extern int numSplitPartitions;

/* 
 * Error reporting. 
 */

/* Location in an input file. */
struct InputLoc
{
	int line;
	int col;
};

struct AttrMarker
{
	char *id;
	int idLen;
	char *value;
	int valueLen;
};

struct Attribute
{
	char *id;
	char *value;
};

typedef Vector<AttrMarker> AttrMkList;
typedef Vector<Attribute> AttrList;
struct XMLTagHashPair;

struct XMLTag
{
	enum TagType { Open, Close };

	XMLTag( XMLTagHashPair *tagId, TagType type ) : 
		tagId(tagId), type(type), 
		content(0), attrList(0) {}
	
	Attribute *findAttr( char *id )
	{
		if ( attrList != 0 ) {
			for ( AttrList::Iter attr = *attrList; attr.lte(); attr++ ) {
				if ( strcmp( id, attr->id ) == 0 )
					return attr;
			}
		}
		return 0;
	}

	XMLTagHashPair *tagId;
	TagType type;

	/* Content is associtated with closing tags. */
	char *content;

	/* Attribute lists are associated with opening tags. */
	AttrList *attrList;
};


std::ostream &error();
//std::ostream &error( const YYLTYPE &loc ); 
std::ostream &error( const InputLoc &loc ); 
std::ostream &error( int first_line, int first_column );
std::ostream &warning( ); 
std::ostream &warning( const InputLoc &loc ); 
std::ostream &warning( int first_line, int first_column );
std::ostream &xml_error( const InputLoc &loc );
//std::ostream &xml_error( const YYLTYPE &loc ); 



void openOutput( char *inputFile );
char *fileNameFromStem( char *stemFile, char *suffix );

/* Size of the include stack. */
#define INCLUDE_STACK_SIZE 32

#endif /* _RLCODEGEN_H */
