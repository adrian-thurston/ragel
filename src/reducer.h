/*
 * Copyright 2001-2006 Adrian Thurston <thurston@colm.net>
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

#include <colm/pdarun.h>
#include <colm/bytecode.h>
#include <colm/defs.h>
#include <colm/input.h>
#include <colm/tree.h>
#include <colm/program.h>
#include <colm/colm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <vector>
#include <string>

#include "vector.h"
#include "inputdata.h"
#include "parsedata.h"

#ifndef _REDUCER_H
#define _REDUCER_H

char *unescape( const char *s, int slen );
char *unescape( const char *s );

struct SectionPass;
struct IncludePass;

struct TopLevel
{
	TopLevel( InputData *id, const HostLang *hostLang,
			MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
	:
		id(id),
		section(0),
		pd(0),
		machineSpec(0),
		machineName(0),
		includeDepth(0),
		hostLang(hostLang),
		minimizeLevel(minimizeLevel),
		minimizeOpt(minimizeOpt),
		
		/* Should be passed into the load, somehow. */
		targetMachine(0),
		searchMachine(0),
		paramList(0),
		success(true)
	{
		exportContext.append( false );
	}

	InputData *id;
	Section *section;
	SectionPass *sectionPass;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;
	const HostLang *hostLang;
	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;
	std::vector<std::string> writeArgs;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

	const char *curFileName;

	const char *targetMachine;
	const char *searchMachine;

	ActionParamList *paramList;
	bool success;

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );

	void loadMachineName( string data );
	void tryMachineDef( InputLoc &loc, std::string name, 
			MachineDef *machineDef, bool isInstance );
	long tryLongScan( const InputLoc &loc, const char *data );
	void include( const InputLoc &incLoc, string fileName, string machine );

	void reduceFile( const char *inputFileName );
	void reduceStr( const char *inputFileName, const char *input );
	void topReduce( const char *inputFileName );

	void loadIncludeData( IncludeRec *el, IncludePass &includePass, const string &fileName );
	void include( const InputLoc &incLoc, bool fileSpecified, string fileName, string machine );
};

struct IncludePass
{
	IncludePass( InputData *id, const string targetMachine )
	:
		id(id),
		targetMachine(targetMachine),
		section(0)
	{
	}

	InputData *id;
	const string targetMachine;
	string sectionMachine;
	Section *section;
	IncItemList incItems;
	SectionDict sectionDict2;

	void reduceFile( const char *inputFileName, const HostLang *hostLang );
	void reduceStr( const char *inputFileName, const HostLang *hostLang, const char *input );

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};

struct Import
{
	Import( InputData *id, ParseData *pd ) : id(id), pd(pd), curFileName(0) {}

	InputData *id;
	ParseData *pd;
	const char *curFileName;

	void import( const InputLoc &loc, std::string name, Literal *literal );
	void tryMachineDef( const InputLoc &loc, std::string name, MachineDef *machineDef, bool isInstance );
	void import( Literal *literal );
	void reduceImport( std::string fileName );

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};

#endif