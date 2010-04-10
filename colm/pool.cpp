#include "bytecode.h"
#include "pdarun.h"
#include "pool.h"

Kid *kidAllocate( Program *prg )
{
	return (Kid*)prg->kidPool._allocate();
}

void kidFree( Program *prg, Kid *el )
{
	prg->kidPool._free( el );
}

void kidClear( Program *prg )
{
	prg->kidPool._clear();
}

long kidNumLost( Program *prg )
{
	return prg->kidPool._numLost();
}

Tree *treeAllocate( Program *prg )
{
	return (Tree*)prg->treePool._allocate();
}

void treeFree( Program *prg, Tree *el )
{
	prg->treePool._free( el );
}

void treeClear( Program *prg )
{
	prg->treePool._clear();
}

long treeNumLost( Program *prg )
{
	return prg->treePool._numLost();
}

ParseTree *parseTreeAllocate( Program *prg )
{
	return (ParseTree*)prg->parseTreePool._allocate();
}

void parseTreeFree( Program *prg, ParseTree *el )
{
	prg->parseTreePool._free( el );
}

void parseTreeClear( Program *prg )
{
	prg->parseTreePool._clear();
}

long parseTreeNumLost( Program *prg )
{
	return prg->parseTreePool._numLost();
}

ListEl *listElAllocate( Program *prg )
{
	return (ListEl*)prg->listElPool._allocate();
}

void listElFree( Program *prg, ListEl *el )
{
	prg->listElPool._free( el );
}

void listElClear( Program *prg )
{
	prg->listElPool._clear();
}

long listElNumLost( Program *prg )
{
	return prg->listElPool._numLost();
}

MapEl *mapElAllocate( Program *prg )
{
	return (MapEl*)prg->mapElPool._allocate();
}

void mapElFree( Program *prg, MapEl *el )
{
	prg->mapElPool._free( el );
}

void mapElClear( Program *prg )
{
	prg->mapElPool._clear();
}

long mapElNumLost( Program *prg )
{
	return prg->mapElPool._numLost();
}

Head *headAllocate( Program *prg )
{
	return (Head*)prg->headPool._allocate();
}

void headFree( Program *prg, Head *el )
{
	prg->headPool._free( el );
}

void headClear( Program *prg )
{
	prg->headPool._clear();
}

long headNumLost( Program *prg )
{
	return prg->headPool._numLost();
}

Location *locationAllocate( Program *prg )
{
	return (Location*)prg->locationPool._allocate();
}

void locationFree( Program *prg, Location *el )
{
	prg->locationPool._free( el );
}

void locationClear( Program *prg )
{
	prg->locationPool._clear();
}

long locationNumLost( Program *prg )
{
	return prg->locationPool._numLost();
}

