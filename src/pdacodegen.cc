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

#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "global.h"
#include "compiler.h"
#include "avlmap.h"
#include "avlbasic.h"
#include "avlset.h"
#include "mergesort.h"
#include "pdacodegen.h"

using std::cerr;
using std::endl;

#define FRESH_BLOCK 8128
#define act_sb "0x1"
#define act_rb "0x2"
#define lower "0x0000ffff"
#define upper "0xffff0000"

void escapeLiteralString( std::ostream &out, const char *path, int length )
{
	for ( const char *pc = path, *end = path+length; pc != end; pc++ ) {
		switch ( *pc ) {
			case '\\': out << "\\\\"; break;
			case '"':  out << "\\\""; break;
			case '\a': out << "\\a"; break;
			case '\b': out << "\\b"; break;
			case '\t': out << "\\t"; break;
			case '\n': out << "\\n"; break;
			case '\v': out << "\\v"; break;
			case '\f': out << "\\f"; break;
			case '\r': out << "\\r"; break;
			default:   out << *pc; break;
		}
	}
}

void escapeLiteralString( std::ostream &out, const char *path )
{
	escapeLiteralString( out, path, strlen(path) );
}

void PdaCodeGen::defineRuntime()
{
	out << 
		"extern RuntimeData colm_program_text;\n"
		"\n";
}

void PdaCodeGen::writeRuntimeData( RuntimeData *runtimeData, PdaTables *pdaTables )
{
	/*
	 * Blocks of code in frames.
	 */
	for ( int i = 0; i < runtimeData->numFrames; i++ ) {
		/* FIXME: horrible code cloning going on here. */
		if ( runtimeData->frameInfo[i].codeLenWV > 0 ) {
			out << "static Code code_" << i << "_wv[] = {\n\t";

			Code *block = runtimeData->frameInfo[i].codeWV;
			for ( int j = 0; j < runtimeData->frameInfo[i].codeLenWV; j++ ) {
				out << (unsigned long) block[j];

				if ( j < runtimeData->frameInfo[i].codeLenWV-1 ) {
					out << ", ";
					if ( (j+1) % 8 == 0 )
						out << "\n\t";
				}
			}
			out << "\n};\n\n";
		}

		if ( runtimeData->frameInfo[i].codeLenWC > 0 ) {
			out << "static Code code_" << i << "_wc[] = {\n\t";

			Code *block = runtimeData->frameInfo[i].codeWC;
			for ( int j = 0; j < runtimeData->frameInfo[i].codeLenWC; j++ ) {
				out << (unsigned long) block[j];

				if ( j < runtimeData->frameInfo[i].codeLenWC-1 ) {
					out << ", ";
					if ( (j+1) % 8 == 0 )
						out << "\n\t";
				}
			}
			out << "\n};\n\n";
		}

		if ( runtimeData->frameInfo[i].localsLen > 0 ) {
			out << "static LocalInfo locals_" << i << "[] = {\n\t";

			LocalInfo *li = runtimeData->frameInfo[i].locals;
			for ( int j = 0; j < runtimeData->frameInfo[i].localsLen; j++ ) {
				out << "{ " << (int)li[j].type << ", " << li[j].offset << " }";

				if ( j < runtimeData->frameInfo[i].localsLen-1 ) {
					out << ", ";
					if ( (j+1) % 8 == 0 )
						out << "\n\t";
				}
			}
			out << "\n};\n\n";
		}
	}

	/*
	 * Blocks in production info.
	 */
	for ( int i = 0; i < runtimeData->numProds; i++ ) {
		if ( runtimeData->prodInfo[i].copyLen > 0 ) {
			out << "static unsigned char copy_" << i << "[] = {\n\t";

			unsigned char *block = runtimeData->prodInfo[i].copy;
			for ( int j = 0; j < runtimeData->prodInfo[i].copyLen; j++ ) {
				out << (long) block[j*2] << ", " << (long) block[j*2+1];

				if ( j < runtimeData->prodInfo[i].copyLen-1 ) {
					out << ", ";
					if ( (j+1) % 8 == 0 )
						out << "\n\t";
				}
			}
			out << "\n};\n\n";
		}
	}

	/* 
	 * Init code.
	 */
	out << "static Code " << rootCode() << "[] = {\n\t";
	Code *block = runtimeData->rootCode ;
	for ( int j = 0; j < runtimeData->rootCodeLen; j++ ) {
		out << (unsigned int) block[j];

		if ( j < runtimeData->rootCodeLen-1 ) {
			out << ", ";
			if ( (j+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	/*
	 * lelInfo
	 */
	out << "static LangElInfo " << lelInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numLangEls; i++ ) {
		LangElInfo *el = &runtimeData->lelInfo[i];
		out << "\t{";
		
		/* Name. */
		out << " \"";
		escapeLiteralString( out, el->name );
		out << "\", ";

		/* Name. */
		out << " \"";
		escapeLiteralString( out, el->xmlTag );
		out << "\", ";
		
		/* Repeat, literal, ignore flags. */
		out << (int)el->repeat << ", ";
		out << (int)el->list << ", ";
		out << (int)el->literal << ", ";
		out << (int)el->ignore << ", ";
		out << el->frameId << ", ";
		out << el->objectTypeId << ", ";
		out << el->ofiOffset << ", ";
		out << el->objectLength << ", ";
		out << el->termDupId << ", ";
		out << el->markId << ", ";
		out << el->captureAttr << ", ";
		out << el->numCaptureAttr;

		out << " }";

		if ( i < runtimeData->numLangEls-1 )
			out << ",\n";
	}
	out << "\n};\n\n";


	for ( int i = 0; i < runtimeData->numStructEls; i++ ) {
		StructElInfo *el = &runtimeData->selInfo[i];
		if ( el->treesLen > 0 ) {
			out << "static short struct_trees_" << i << "[] = {\n\t";

			short *ti = el->trees;
			for ( int j = 0; j < el->treesLen; j++ )
				out << ti[j] << ", ";
			out << "\n};\n\n";
		}
	}

	/*
	 * selInfo
	 */
	out << "static StructElInfo " << selInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numStructEls; i++ ) {
		StructElInfo *el = &runtimeData->selInfo[i];
		out << "\t{ ";
		out << el->size << ", ";

		/* trees. */
		if ( el->treesLen > 0 )
			out << "struct_trees_" << i << ", ";
		else
			out << "0, ";
		out << el->treesLen << ", ";

		out << " },\n";
	}
	out << "\n};\n\n";

	/*
	 * frameInfo
	 */
	out << "static FrameInfo " << frameInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numFrames; i++ ) {
		out << "\t{ ";

		/* The Name. */
		if ( runtimeData->frameInfo[i].name )
			out << "\"" << runtimeData->frameInfo[i].name << "\", ";
		else 
			out << "\"\", ";

		if ( runtimeData->frameInfo[i].codeLenWV > 0 )
			out << "code_" << i << "_wv, ";
		else
			out << "0, ";
		out << runtimeData->frameInfo[i].codeLenWV << ", ";

		if ( runtimeData->frameInfo[i].codeLenWC > 0 )
			out << "code_" << i << "_wc, ";
		else
			out << "0, ";
		out << runtimeData->frameInfo[i].codeLenWC << ", ";

		/* locals. */
		if ( runtimeData->frameInfo[i].localsLen > 0 )
			out << "locals_" << i << ", ";
		else
			out << "0, ";

		out << runtimeData->frameInfo[i].localsLen << ", ";

		out <<
			runtimeData->frameInfo[i].argSize << ", " <<
			runtimeData->frameInfo[i].frameSize;

		out << " }";

		if ( i < runtimeData->numFrames-1 )
			out << ",\n";
	}
	out << "\n};\n\n";


	/*
	 * prodInfo
	 */
	out << "static ProdInfo " << prodInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numProds; i++ ) {
		out << "\t{ ";

		out << runtimeData->prodInfo[i].lhsId << ", ";
		out << runtimeData->prodInfo[i].prodNum << ", ";
		out << runtimeData->prodInfo[i].length << ", ";

		out <<
			'"' << runtimeData->prodInfo[i].name << "\", " << 
			runtimeData->prodInfo[i].frameId << ", " << 
			(int)runtimeData->prodInfo[i].lhsUpref << ", ";

		if ( runtimeData->prodInfo[i].copyLen > 0 )
			out << "copy_" << i << ", ";
		else
			out << "0, ";

		out << runtimeData->prodInfo[i].copyLen << ", ";


		out << " }";

		if ( i < runtimeData->numProds-1 )
			out << ",\n";
	}
	out << "\n};\n\n";

	/*
	 * patReplInfo
	 */
	out << "static PatConsInfo " << patReplInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numPatterns; i++ ) {
		out << "	{ " << runtimeData->patReplInfo[i].offset << ", " <<
				runtimeData->patReplInfo[i].numBindings << " },\n";
	}
	out << "};\n\n";

	/*
	 * patReplNodes
	 */
	out << "static PatConsNode " << patReplNodes() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numPatternNodes; i++ ) {
		PatConsNode &node = runtimeData->patReplNodes[i];
		out << "	{ " << node.id << ", " << 
				node.prodNum << ", " << node.next << ", " << 
				node.child << ", " << node.bindId << ", ";
		if ( node.data == 0 )
			out << "0";
		else {
			out << '\"';
			escapeLiteralString( out, node.data, node.length );
			out << '\"';
		}
		out << ", " << node.length << ", ";

		out << node.leftIgnore << ", ";
		out << node.rightIgnore << ", ";

		out << (int)node.stop << " },\n";
	}
	out << "};\n\n";

	/*
	 * functionInfo
	 */
	out << "static FunctionInfo " << functionInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numFunctions; i++ ) {
		out << "\t{ " <<
				runtimeData->functionInfo[i].frameId << ", " <<
				runtimeData->functionInfo[i].argSize << ", " <<
				runtimeData->functionInfo[i].frameSize;
		out << " }";

		if ( i < runtimeData->numFunctions-1 )
			out << ",\n";
	}
	out << "\n};\n\n";

	/*
	 * regionInfo
	 */
	out << "static RegionInfo " << regionInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numRegions; i++ ) {
		out << "\t{ " << runtimeData->regionInfo[i].defaultToken <<
			", " << runtimeData->regionInfo[i].eofFrameId <<
			", " << runtimeData->regionInfo[i].ciLelId <<
			" }";

		if ( i < runtimeData->numRegions-1 )
			out << ",\n";
	}
	out << "\n};\n\n";

	/* 
	 * genericInfo
	 */
	out << "static GenericInfo " << genericInfo() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numGenerics; i++ ) {
		out << "\t{ " << 
				runtimeData->genericInfo[i].type << ", " <<
				runtimeData->genericInfo[i].typeArg << ", " <<
				runtimeData->genericInfo[i].keyOffset << ", " <<
				runtimeData->genericInfo[i].keyType << ", " <<
				runtimeData->genericInfo[i].parserId << ", " <<
				runtimeData->genericInfo[i].elOffset << ", " <<
				runtimeData->genericInfo[i].elStructId << " },\n";
	}
	out << "};\n\n";

	/* 
	 * literals
	 */
	out << "static const char *" << litdata() << "[] = {\n";
	for ( int i = 0; i < runtimeData->numLiterals; i++ ) {
		out << "\t\"";
		escapeLiteralString( out, runtimeData->litdata[i] );
		out << "\",\n";
	}
	out << "};\n\n";

	out << "static long " << litlen() << "[] = {\n\t";
	for ( int i = 0; i < runtimeData->numLiterals; i++ )
		out << runtimeData->litlen[i] << ", ";
	out << "};\n\n";

	out << "static Head *" << literals() << "[] = {\n\t";
	for ( int i = 0; i < runtimeData->numLiterals; i++ )
		out << "0, ";
	out << "};\n\n";

	out << "static int startStates[] = {\n\t";
	for ( long i = 0; i < runtimeData->numParsers; i++ ) {
		out << runtimeData->startStates[i] << ", ";
	}
	out << "};\n\n";

	out << "static int eofLelIds[] = {\n\t";
	for ( long i = 0; i < runtimeData->numParsers; i++ ) {
		out << runtimeData->eofLelIds[i] << ", ";
	}
	out << "};\n\n";

	out << "static int parserLelIds[] = {\n\t";
	for ( long i = 0; i < runtimeData->numParsers; i++ ) {
		out << runtimeData->parserLelIds[i] << ", ";
	}
	out << "};\n\n";

	out << "static CaptureAttr captureAttr[] = {\n";
	for ( long i = 0; i < runtimeData->numCapturedAttr; i++ ) {
		out << "\t{ " << 
			runtimeData->captureAttr[i].mark_enter << ", " <<
			runtimeData->captureAttr[i].mark_leave << ", " <<
			runtimeData->captureAttr[i].offset  << " },\n";
	}

	out << "};\n\n";

	out <<
		"RuntimeData colm_object = \n"
		"{\n"
		"	" << lelInfo() << ",\n"
		"	" << runtimeData->numLangEls << ",\n"
		"\n"
		"	" << selInfo() << ",\n"
		"	" << runtimeData->numStructEls << ",\n"
		"\n"
		"	" << prodInfo() << ",\n"
		"	" << runtimeData->numProds << ",\n"
		"\n"
		"	" << regionInfo() << ",\n"
		"	" << runtimeData->numRegions << ",\n"
		"\n"
		"	" << rootCode() << ",\n"
		"	" << runtimeData->rootCodeLen << ",\n"
		"	" << runtimeData->rootFrameId << ",\n"
		"\n"
		"	" << frameInfo() << ",\n"
		"	" << runtimeData->numFrames << ",\n"
		"\n"
		"	" << functionInfo() << ",\n"
		"	" << runtimeData->numFunctions << ",\n"
		"\n"
		"	" << patReplInfo() << ",\n"
		"	" << runtimeData->numPatterns << ",\n"
		"\n"
		"	" << patReplNodes() << ",\n"
		"	" << runtimeData->numPatternNodes << ",\n"
		"\n"
		"	" << genericInfo() << ",\n"
		"	" << runtimeData->numGenerics << ",\n"
		"	" << runtimeData->argvGenericId << ",\n"
		"\n"
		"	" << litdata() << ",\n"
		"	" << litlen() << ",\n"
		"	" << literals() << ",\n"
		"	" << runtimeData->numLiterals << ",\n"
		"\n"
		"	captureAttr,\n"
		"	" << runtimeData->numCapturedAttr << ",\n"
		"\n"
		"	&fsmTables_start,\n"
		"	&pid_0_pdaTables,\n"
		"	startStates, eofLelIds, parserLelIds, " << runtimeData->numParsers << ",\n"
		"\n"
		"	" << runtimeData->globalSize << ",\n"
		"\n"
		"	" << runtimeData->firstNonTermId << ",\n"
		"	" << runtimeData->integerId << ",\n"
		"	" << runtimeData->stringId << ",\n"
		"	" << runtimeData->anyId << ",\n"
		"	" << runtimeData->eofId << ",\n"
		"	" << runtimeData->noTokenId << ",\n"
		"	" << runtimeData->globalId << ",\n"
		"	" << runtimeData->argvElId << ",\n"
		"	&fsmExecute,\n"
		"	&sendNamedLangEl,\n"
		"	&initBindings,\n"
		"	&popBinding,\n"
		"};\n"
		"\n";
}

void PdaCodeGen::writeParserData( long id, PdaTables *tables )
{
	String prefix = "pid_" + String(0, "%ld", id) + "_";

	out << "static int " << prefix << indicies() << "[] = {\n\t";
	for ( int i = 0; i < tables->numIndicies; i++ ) {
		out << tables->indicies[i];

		if ( i < tables->numIndicies-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << owners() << "[] = {\n\t";
	for ( int i = 0; i < tables->numIndicies; i++ ) {
		out << tables->owners[i];

		if ( i < tables->numIndicies-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << keys() << "[] = {\n\t";
	for ( int i = 0; i < tables->numKeys; i++ ) {
		out << tables->keys[i];

		if ( i < tables->numKeys-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static unsigned int " << prefix << offsets() << "[] = {\n\t";
	for ( int i = 0; i < tables->numStates; i++ ) {
		out << tables->offsets[i];

		if ( i < tables->numStates-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static unsigned int " << prefix << targs() << "[] = {\n\t";
	for ( int i = 0; i < tables->numTargs; i++ ) {
		out << tables->targs[i];

		if ( i < tables->numTargs-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static unsigned int " << prefix << actInds() << "[] = {\n\t";
	for ( int i = 0; i < tables->numActInds; i++ ) {
		out << tables->actInds[i];

		if ( i < tables->numActInds-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static unsigned int " << prefix << actions() << "[] = {\n\t";
	for ( int i = 0; i < tables->numActions; i++ ) {
		out << tables->actions[i];

		if ( i < tables->numActions-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << commitLen() << "[] = {\n\t";
	for ( int i = 0; i < tables->numCommitLen; i++ ) {
		out << tables->commitLen[i];

		if ( i < tables->numCommitLen-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << tokenRegionInds() << "[] = {\n\t";
	for ( int i = 0; i < tables->numStates; i++ ) {
		out << tables->tokenRegionInds[i];

		if ( i < tables->numStates-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << tokenRegions() << "[] = {\n\t";
	for ( int i = 0; i < tables->numRegionItems; i++ ) {
		out << tables->tokenRegions[i];

		if ( i < tables->numRegionItems-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << "static int " << prefix << tokenPreRegions() << "[] = {\n\t";
	for ( int i = 0; i < tables->numPreRegionItems; i++ ) {
		out << tables->tokenPreRegions[i];

		if ( i < tables->numPreRegionItems-1 ) {
			out << ", ";
			if ( (i+1) % 8 == 0 )
				out << "\n\t";
		}
	}
	out << "\n};\n\n";

	out << 
		"static PdaTables " << prefix << "pdaTables =\n"
		"{\n"
		"	" << prefix << indicies() << ",\n"
		"	" << prefix << owners() << ",\n"
		"	" << prefix << keys() << ",\n"
		"	" << prefix << offsets() << ",\n"
		"	" << prefix << targs() << ",\n"
		"	" << prefix << actInds() << ",\n"
		"	" << prefix << actions() << ",\n"
		"	" << prefix << commitLen() << ",\n"

		"	" << prefix << tokenRegionInds() << ",\n"
		"	" << prefix << tokenRegions() << ",\n"
		"	" << prefix << tokenPreRegions() << ",\n"
		"\n"
		"	" << tables->numIndicies << ",\n"
		"	" << tables->numKeys << ",\n"
		"	" << tables->numStates << ",\n"
		"	" << tables->numTargs << ",\n"
		"	" << tables->numActInds << ",\n"
		"	" << tables->numActions << ",\n"
		"	" << tables->numCommitLen << ",\n"
		"	" << tables->numRegionItems << ",\n"
		"	" << tables->numPreRegionItems << "\n"
		"};\n"
		"\n";
}

