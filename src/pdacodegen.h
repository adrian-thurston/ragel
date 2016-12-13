/*
 * Copyright 2007-2012 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_PDACODEGEN_H
#define _COLM_PDACODEGEN_H

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

#endif /* _COLM_PDACODEGEN_H */

