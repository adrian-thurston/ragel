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
		searchMachine(0)
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

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );

	void loadMachineName( string data );
	void tryMachineDef( InputLoc &loc, std::string name, 
			MachineDef *machineDef, bool isInstance );
	long tryLongScan( const InputLoc &loc, const char *data );
	void loadImport( std::string fileName );
	void include( string fileName, string machine );

	void reduceFile( const char *inputFileName );
	void reduceString( const char *data );
	void topReduce( const char *inputFileName );
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

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};

struct IncludePass
{
	IncludePass( InputData *id )
	:
		id(id),
		section(0),
		includeDepth(0)
	{
	}

	InputData *id;
	Section *section;
	int includeDepth;

	void reduceFile( const char *inputFileName );

	/* Generated and called by colm. */
	void commit_reduce_forward( program_t *prg, tree_t **root,
			struct pda_run *pda_run, parse_tree_t *pt );
};

#endif
