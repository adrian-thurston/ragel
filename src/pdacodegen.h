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

struct Compiler;

struct PdaCodeGen
{
	PdaCodeGen( ostream &out )
	:
		out(out)
	{}

	/*
	 * Code Generation.
	 */
	void startCodeGen();
	void endCodeGen( int endLine );

	void writeReference( Production *prod, char *data );
	void writeUndoReference( Production *prod, char *data );
	void writeFinalReference( Production *prod, char *data );
	void writeFirstLocate( Production *prod );
	void writeRhsLocate( Production *prod );

	void defineRuntime();
	void writeRuntimeData( colm_sections *runtimeData, struct pda_tables *pdaTables );
	void writeParserData( long id, struct pda_tables *tables );

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
	String selInfo() { return PARSER() + "selInfo"; }
	String prodInfo() { return PARSER() + "prodInfo"; }
	String tokenRegionInds() { return PARSER() + "tokenRegionInds"; }
	String tokenRegions() { return PARSER() + "tokenRegions"; }
	String tokenPreRegions() { return PARSER() + "tokenPreRegions"; }
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

	ostream &out;
};

extern "C"
{
	void internalFsmExecute( struct pda_run *pdaRun, struct stream_impl *inputStream );
	void internalSendNamedLangEl( program_t *prg, tree_t **sp,
			struct pda_run *pdaRun, struct stream_impl *is );
	void internalInitBindings( struct pda_run *pdaRun );
	void internalPopBinding( struct pda_run *pdaRun, parse_tree_t *parseTree );
}

#endif
