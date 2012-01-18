/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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


#ifndef _PDACODEGEN_H
#define _PDACODEGEN_H

struct ParseData;

struct PdaCodeGen
{
	PdaCodeGen( const char *fileName, const char *parserName, ParseData *pd, ostream &out )
	:
		fileName(fileName),
		parserName(parserName),
		pd(pd),
		out(out)
	{}

	/*
	 * Code Generation.
	 */
	void startCodeGen();
	void endCodeGen( int endLine );

	void writeTokenIds();
	void writeLangEls();

	void writeReference( Definition *prod, char *data );
	void writeUndoReference( Definition *prod, char *data );
	void writeFinalReference( Definition *prod, char *data );
	void writeFirstLocate( Definition *prod );
	void writeRhsLocate( Definition *prod );

	void defineRuntime();
	void writeRuntimeData( RuntimeData *runtimeData, PdaTables *pdaTables );
	void writeParserData( long id, PdaTables *tables );

	String PARSER() { return "parser_"; }

	String startState() { return PARSER() + "startState"; }
	String indicies() { return PARSER() + "indicies"; }
	String owners() { return PARSER() + "owners"; }
	String keys() { return PARSER() + "keys"; }
	String offsets() { return PARSER() + "offsets"; }
	String targs() { return PARSER() + "targs"; }
	String actInds() { return PARSER() + "actInds"; }
	String actions() { return PARSER() + "actions"; }
	String commitLen() { return PARSER() + "commitLen"; }
	String fssProdIdIndex() { return PARSER() + "fssProdIdIndex"; }
	String prodLengths() { return PARSER() + "prodLengths"; }
	String prodLhsIds() { return PARSER() + "prodLhsIds"; }
	String prodNames() { return PARSER() + "prodNames"; }
	String lelInfo() { return PARSER() + "lelInfo"; }
	String prodInfo() { return PARSER() + "prodInfo"; }
	String tokenRegionInds() { return PARSER() + "tokenRegionInds"; }
	String tokenRegionsDirect() { return PARSER() + "tokenRegionsDirect"; }
	String tokenRegions() { return PARSER() + "tokenRegions"; }
	String prodCodeBlocks() { return PARSER() + "prodCodeBlocks"; }
	String prodCodeBlockLens() { return PARSER() + "prodCodeBlockLens"; }
	String rootCode() { return PARSER() + "rootCode"; }
	String frameInfo() { return PARSER() + "frameInfo"; }
	String functionInfo() { return PARSER() + "functionInfo"; }
	String objFieldInfo() { return PARSER() + "objFieldInfo"; }
	String patReplInfo() { return PARSER() + "patReplInfo"; }
	String patReplNodes() { return PARSER() + "patReplNodes"; }
	String regionInfo() { return PARSER() + "regionInfo"; }
	String genericInfo() { return PARSER() + "genericInfo"; }
	String litdata() { return PARSER() + "litdata"; }
	String litlen() { return PARSER() + "litlen"; }
	String literals() { return PARSER() + "literals"; }
	String fsmTables() { return PARSER() + "fsmTables"; }

	/* 
	 * Graphviz Generation
	 */
	void writeTransList( PdaState *state );
	void writeDotFile( PdaGraph *graph );
	void writeDotFile( );
	

	const char *fileName;
	const char *parserName;
	ParseData *pd;
	ostream &out;
};

#endif
