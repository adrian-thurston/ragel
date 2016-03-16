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
	TopLevel( InputData *id, SectionPass *sectionPass, const HostLang *hostLang,
			MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
	:
		id(id),
		sectionPass(sectionPass),
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
	SectionPass *sectionPass;
	ParseData *pd;
	char *machineSpec;
	char *machineName;
	int includeDepth;
	const HostLang *hostLang;
	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;
	Vector<std::string> writeArgs;

	/* Should this go in the parse data? Probably. */
	Vector<bool> exportContext;

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
	void loadImport( std::string fileName );
	void include( const InputLoc &incLoc, string fileName, string machine );

	void reduceFile( const char *inputFileName );
	void reduceStr( const char *inputFileName, const char *input );
	void topReduce( const char *inputFileName );

	void loadIncludeData( IncludeRec *el, IncludePass &includePass, const string &fileName );
	void include( const InputLoc &incLoc, bool fileSpecified, string fileName, string machine );
};

struct SectionPass
{
	SectionPass( InputData *id )
	:
		id(id),
		section(0)
	{
	}

	InputData *id;
	Section *section;

	void reduceFile( const char *inputFileName );
	void reduceStr( const char *inputFileName, const char *input );

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
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

#endif
