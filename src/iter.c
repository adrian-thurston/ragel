#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/program.h>

#include <assert.h>

void initTreeIter( TreeIter *treeIter, Tree **stackRoot, long rootSize,
		const Ref *rootRef, int searchId )
{
	treeIter->type = IT_Tree;
	treeIter->rootRef = *rootRef;
	treeIter->searchId = searchId;
	treeIter->stackRoot = stackRoot;
	treeIter->yieldSize = 0;
	treeIter->rootSize = rootSize;
	treeIter->ref.kid = 0;
	treeIter->ref.next = 0;
}

void initRevTreeIter( RevTreeIter *revTriter, Tree **stackRoot, long rootSize,
		const Ref *rootRef, int searchId, int children )
{
	revTriter->type = IT_RevTree;
	revTriter->rootRef = *rootRef;
	revTriter->searchId = searchId;
	revTriter->stackRoot = stackRoot;
	revTriter->yieldSize = children;
	revTriter->rootSize = rootSize;
	revTriter->kidAtYield = 0;
	revTriter->children = children;
	revTriter->ref.kid = 0;
	revTriter->ref.next = 0;
}

void initUserIter( UserIter *userIter, Tree **stackRoot, long rootSize,
		long argSize, long searchId )
{
	userIter->stackRoot = stackRoot;
	userIter->argSize = argSize;
	userIter->yieldSize = 0;
	userIter->rootSize = rootSize;
	userIter->resume = 0;
	userIter->frame = 0;
	userIter->searchId = searchId;

	userIter->ref.kid = 0;
	userIter->ref.next = 0;
}


UserIter *uiterCreate( Program *prg, Tree ***psp, FunctionInfo *fi, long searchId )
{
	Tree **sp = *psp;

	vm_pushn( sizeof(UserIter) / sizeof(Word) );
	void *mem = vm_ptop();
	UserIter *uiter = mem;

	Tree **stackRoot = vm_ptop();
	long rootSize = vm_ssize();

	initUserIter( uiter, stackRoot, rootSize, fi->argSize, searchId );

	*psp = sp;
	return uiter;
}

void uiterInit( Program *prg, Tree **sp, UserIter *uiter, 
		FunctionInfo *fi, int revertOn )
{
	/* Set up the first yeild so when we resume it starts at the beginning. */
	uiter->ref.kid = 0;
	uiter->yieldSize = vm_ssize() - uiter->rootSize;
	uiter->frame = &uiter->stackRoot[-IFR_AA];

	if ( revertOn )
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWV;
	else
		uiter->resume = prg->rtd->frameInfo[fi->frameId].codeWC;
}


void treeIterDestroy( Program *prg, Tree ***psp, TreeIter *iter )
{
	if ( (int)iter->type != 0 ) {
		Tree **sp = *psp;
		long curStackSize = vm_ssize() - iter->rootSize;
		assert( iter->yieldSize == curStackSize );
		vm_popn( iter->yieldSize );
		iter->type = 0;
		*psp = sp;
	}
}

void revTreeIterDestroy( struct colm_program *prg, Tree ***psp, RevTreeIter *riter )
{
	if ( (int)riter->type != 0 ) {
		Tree **sp = *psp;
		long curStackSize = vm_ssize() - riter->rootSize;
		assert( riter->yieldSize == curStackSize );
		vm_popn( riter->yieldSize );
		riter->type = 0;
		*psp = sp;
	}
}

void userIterDestroy( Program *prg, Tree ***psp, UserIter *uiter )
{
	if ( (int)uiter->type != 0 ) {
		Tree **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		long argSize = uiter->argSize;

		vm_popn( uiter->yieldSize );
		vm_popn( sizeof(UserIter) / sizeof(Word) );
		vm_popn( argSize );

		uiter->type = 0;

		*psp = sp;
	}
}

void userIterDestroy2( Program *prg, Tree ***psp, UserIter *uiter )
{
	if ( uiter != 0 && (int)uiter->type != 0 ) {
		Tree **sp = *psp;

		/* We should always be coming from a yield. The current stack size will be
		 * nonzero and the stack size in the iterator will be correct. */
		long curStackSize = vm_ssize() - uiter->rootSize;
		assert( uiter->yieldSize == curStackSize );

		long argSize = uiter->argSize;

		vm_popn( uiter->yieldSize );
		vm_popn( sizeof(UserIter) / sizeof(Word) );
		vm_popn( argSize );

		uiter->type = 0;

		*psp = sp;
	}
}

