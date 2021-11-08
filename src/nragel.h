#ifndef _NRAGEL_H
#define _NRAGEL_H

#include <libfsm/common.h>
#include <libfsm/ragel.h>

struct HostLang;

HostType *findAlphType( const HostLang *hostLang, const char *s1 );
HostType *findAlphType( const HostLang *hostLang, const char *s1, const char *s2 );
HostType *findAlphTypeInternal( const HostLang *hostLang, const char *s1 );

CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );
CodeGenData *makeCodeGenAsm( const HostLang *hostLang, const CodeGenArgs &args );

/* Selects and constructs the codegen based on the output options. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );
CodeGenData *asm_makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );

typedef CodeGenData *(*MakeCodeGenT)( const HostLang *hostLang, const CodeGenArgs &args );

struct HostLang
{
	HostType *hostTypes;
	int numHostTypes;
	int defaultAlphType;
	bool explicitUnsigned;
	bool loopLabels;

	RagelBackend backend;
	BackendFeature feature;

	MakeCodeGenT makeCodeGen;
	DefaultOutFnT defaultOutFn;
	GenLineDirectiveT genLineDirective;
};

#endif
