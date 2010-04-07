#include "bytecode.h"
#include "pdarun.h"
#include "pool.h"

Kid *kidAllocate( Program *prg )
{
	return prg->kidPool._allocate();
}

void kidFree( Program *prg, Kid *el )
{
	prg->kidPool.free( el );
}

void kidClear( Program *prg )
{
	prg->kidPool.clear();
}

long kidNumlost( Program *prg )
{
	return prg->kidPool.numLost();
}

Tree *treeAllocate( Program *prg )
{
	return prg->treePool._allocate();
}

void treeFree( Program *prg, Tree *el )
{
	prg->treePool.free( el );
}

void treeClear( Program *prg )
{
	prg->treePool.clear();
}

long treeNumlost( Program *prg )
{
	return prg->treePool.numLost();
}

ParseTree *parseTreeAllocate( Program *prg )
{
	return prg->parseTreePool._allocate();
}

void parseTreeFree( Program *prg, ParseTree *el )
{
	prg->parseTreePool.free( el );
}

void parseTreeClear( Program *prg )
{
	prg->parseTreePool.clear();
}

long parseTreeNumlost( Program *prg )
{
	return prg->parseTreePool.numLost();
}

ListEl *listElAllocate( Program *prg )
{
	return prg->listElPool._allocate();
}

void listElFree( Program *prg, ListEl *el )
{
	prg->listElPool.free( el );
}

void listElClear( Program *prg )
{
	prg->listElPool.clear();
}

long listElNumlost( Program *prg )
{
	return prg->listElPool.numLost();
}

MapEl *mapElAllocate( Program *prg )
{
	return prg->mapElPool._allocate();
}

void mapElFree( Program *prg, MapEl *el )
{
	prg->mapElPool.free( el );
}

void mapElClear( Program *prg )
{
	prg->mapElPool.clear();
}

long mapElNumlost( Program *prg )
{
	return prg->mapElPool.numLost();
}

Head *headAllocate( Program *prg )
{
	return prg->headPool._allocate();
}

void headFree( Program *prg, Head *el )
{
	prg->headPool.free( el );
}

void headClear( Program *prg )
{
	prg->headPool.clear();
}

long headNumlost( Program *prg )
{
	return prg->headPool.numLost();
}

Location *locationAllocate( Program *prg )
{
	return prg->locationPool._allocate();
}

void locationFree( Program *prg, Location *el )
{
	prg->locationPool.free( el );
}

void locationClear( Program *prg )
{
	prg->locationPool.clear();
}

long locationNumlost( Program *prg )
{
	return prg->locationPool.numLost();
}

