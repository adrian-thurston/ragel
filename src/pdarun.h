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

#ifndef __COLM_PDARUN_H
#define __COLM_PDARUN_H

#include <colm/input.h>
#include <colm/defs.h>
#include <colm/tree.h>
#include <colm/struct.h>

#ifdef __cplusplus
extern "C" {
#endif

struct colm_program;

#define MARK_SLOTS 32

struct fsm_tables
{
	long *actions;
	long *keyOffsets;
	char *transKeys;
	long *singleLengths;
	long *rangeLengths;
	long *indexOffsets;
	long *transTargsWI;
	long *transActionsWI;
	long *toStateActions;
	long *fromStateActions;
	long *eofActions;
	long *eofTargs;
	long *entryByRegion;

	long numStates;
	long numActions;
	long numTransKeys;
	long numSingleLengths;
	long numRangeLengths;
	long numIndexOffsets;
	long numTransTargsWI;
	long numTransActionsWI;
	long numRegions;

	long startState;
	long firstFinal;
	long errorState;

	struct GenAction **actionSwitch;
	long numActionSwitch;
};

void undoStreamPull( struct stream_impl *inputStream, const char *data, long length );

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

struct colm_execution;

struct rt_code_vect
{
	code_t *data;
	long tabLen;
	long allocLen;

	/* FIXME: leak when freed. */
};

void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el );
void listAddBefore( List *list, ListEl *next_el, ListEl *new_el );

void listPrepend( List *list, ListEl *new_el );
void listAppend( List *list, ListEl *new_el );

ListEl *listDetach( List *list, ListEl *el );
ListEl *listDetachFirst(List *list );
ListEl *listDetachLast(List *list );

long listLength(List *list);

struct function_info
{
	long frameId;
	long argSize;
	long frameSize;
};

/*
 * Program Data.
 */

struct pat_cons_info
{
	long offset;
	long numBindings;
};

struct pat_cons_node
{
	long id;
	long prodNum;
	long next;
	long child;
	long bindId;
	const char *data;
	long length;
	long leftIgnore;
	long rightIgnore;

	/* Just match nonterminal, don't go inside. */
	unsigned char stop;
};

/* FIXME: should have a descriptor for object types to give the length. */

struct lang_el_info
{
	const char *name;
	const char *xmlTag;
	unsigned char repeat;
	unsigned char list;
	unsigned char literal;
	unsigned char ignore;

	long frameId;

	long objectTypeId;
	long ofiOffset;
	long objectLength;

	long termDupId;
	long markId;
	long captureAttr;
	long numCaptureAttr;
};

struct struct_el_info
{
	long size;
	short *trees;
	long treesLen;
};

struct prod_info
{
	unsigned long lhsId;
	short prodNum;
	long length;
	const char *name;
	long frameId;
	unsigned char lhsUpref;
	unsigned char *copy;
	long copyLen;
};

/* Must match the LocalType enum. */
#define LI_Tree 1
#define LI_Iter 2
#define LI_RevIter 3
#define LI_UserIter 4

struct local_info
{
	char type;
	short offset;
};

struct frame_info
{
	const char *name;
	code_t *codeWV;
	long codeLenWV;
	code_t *codeWC;
	long codeLenWC;
	struct local_info *locals;
	long localsLen;
	long argSize;
	long frameSize;
	char retTree;
};

struct region_info
{
	long defaultToken;
	long eofFrameId;
	int ciLelId;
};

typedef struct _CaptureAttr
{
	long mark_enter;
	long mark_leave;
	long offset;
} CaptureAttr;

struct pda_tables
{
	/* Parser table data. */
	int *indicies;
	int *owners;
	int *keys;
	unsigned int *offsets;
	unsigned int *targs;
	unsigned int *actInds;
	unsigned int *actions;
	int *commitLen;
	int *tokenRegionInds;
	int *tokenRegions;
	int *tokenPreRegions;

	int numIndicies;
	int numKeys;
	int numStates;
	int numTargs;
	int numActInds;
	int numActions;
	int numCommitLen;
	int numRegionItems;
	int numPreRegionItems;
};

struct pool_block
{
	void *data;
	struct pool_block *next;
};

struct pool_item
{
	struct pool_item *next;
};

struct pool_alloc
{
	struct pool_block *head;
	long nextel;
	struct pool_item *pool;
	int sizeofT;
};

struct pda_run
{
	/*
	 * Scanning.
	 */
	struct fsm_tables *fsm_tables;

	RunBuf *consumeBuf;

	long region, preRegion;
	long fsm_cs, next_cs, act;
	char *start;
	char *tokstart;
	long tokend;
	long toklen;
	char *p, *pe;

	/* Bits. */
	char eof;
	char returnResult;
	char skipToklen;

	char *mark[MARK_SLOTS];
	long matchedToken;

	/*
	 * Parsing
	 */
	int numRetry;
	ParseTree *stackTop;
	Ref *tokenList;
	int pda_cs;
	int nextRegionInd;

	struct pda_tables *pda_tables;
	int parserId;

	/* Reused. */
	struct rt_code_vect rcodeCollect;
	struct rt_code_vect reverseCode;

	int stopParsing;
	long stopTarget;

	ParseTree *accumIgnore;

	Kid *btPoint;

	struct bindings *bindings;

	int revertOn;

	struct colm_struct *context;

	int stop;
	int parseError;

	long steps;
	long targetSteps;
	long shiftCount;
	long commitShiftCount;

	int onDeck;

	/*
	 * Data we added when refactoring the parsing engine into a coroutine.
	 */

	ParseTree *parseInput;
	struct frame_info *fi;
	int reduction;
	ParseTree *redLel;
	int curState;
	ParseTree *lel;
	int triggerUndo;

	int tokenId;
	Head *tokdata;
	int frameId;
	int next;
	ParseTree *undoLel;

	int checkNext;
	int checkStop;

	/* The lhs is sometimes saved before reduction actions in case it is
	 * replaced and we need to restore it on backtracking */
	Tree *parsed;

	int reject;

	/* Instruction pointer to use when we stop parsing and execute code. */
	code_t *code;

	int rcBlockCount;

	Tree *parseErrorText;
};

void colm_pda_init( struct colm_program *prg, struct pda_run *pdaRun,
		struct pda_tables *tables, int parserId, long stopTarget,
		int revertOn, struct colm_struct *context );

void colm_pda_clear( struct colm_program *prg, struct colm_tree **sp,
		struct pda_run *pdaRun );

void colm_rt_code_vect_replace( struct rt_code_vect *vect, long pos,
		const code_t *val, long len );
void colm_rt_code_vect_empty( struct rt_code_vect *vect );
void colm_rt_code_vect_remove( struct rt_code_vect *vect, long pos, long len );

void initRtCodeVect( struct rt_code_vect *codeVect );

inline static void append_code_val( struct rt_code_vect *vect, const code_t val );
inline static void append_code_vect( struct rt_code_vect *vect, const code_t *val, long len );
inline static void append_half( struct rt_code_vect *vect, half_t half );
inline static void append_word( struct rt_code_vect *vect, word_t word );

inline static void append_code_vect( struct rt_code_vect *vect, const code_t *val, long len )
{
	colm_rt_code_vect_replace( vect, vect->tabLen, val, len );
}

inline static void append_code_val( struct rt_code_vect *vect, const code_t val )
{
	colm_rt_code_vect_replace( vect, vect->tabLen, &val, 1 );
}

inline static void append_half( struct rt_code_vect *vect, half_t half )
{
	/* not optimal. */
	append_code_val( vect, half & 0xff );
	append_code_val( vect, (half>>8) & 0xff );
}

inline static void append_word( struct rt_code_vect *vect, word_t word )
{
	/* not optimal. */
	append_code_val( vect, word & 0xff );
	append_code_val( vect, (word>>8) & 0xff );
	append_code_val( vect, (word>>16) & 0xff );
	append_code_val( vect, (word>>24) & 0xff );
	#if SIZEOF_LONG == 8
	append_code_val( vect, (word>>32) & 0xff );
	append_code_val( vect, (word>>40) & 0xff );
	append_code_val( vect, (word>>48) & 0xff );
	append_code_val( vect, (word>>56) & 0xff );
	#endif
}

void colm_increment_steps( struct pda_run *pdaRun );
void colm_decrement_steps( struct pda_run *pdaRun );

void colm_clear_stream_impl( struct colm_program *prg, Tree **sp, struct stream_impl *inputStream );
void colm_clear_source_stream( struct colm_program *prg, Tree **sp, struct stream_impl *sourceStream );

#define PCR_START         1
#define PCR_DONE          2
#define PCR_REDUCTION     3
#define PCR_GENERATION    4
#define PCR_PRE_EOF       5
#define PCR_REVERSE       6

Head *colm_stream_pull( struct colm_program *prg, struct colm_tree **sp,
		struct pda_run *pdaRun, struct stream_impl *is, long length );
Head *colm_string_alloc_pointer( struct colm_program *prg, const char *data, long length );

void colm_stream_push_text( struct stream_impl *inputStream, const char *data, long length );
void colm_stream_push_tree( struct stream_impl *inputStream, Tree *tree, int ignore );
void colm_stream_push_stream( struct stream_impl *inputStream, Tree *tree );
void colm_undo_stream_push( struct colm_program *prg, Tree **sp,
		struct stream_impl *inputStream, long length );

Kid *make_token_with_data( struct colm_program *prg, struct pda_run *pdaRun,
		struct stream_impl *inputStream, int id, Head *tokdata );

long colm_parse_loop( struct colm_program *prg, Tree **sp, struct pda_run *pdaRun, 
		struct stream_impl *inputStream, long entry );

long colm_parse_frag( struct colm_program *prg, Tree **sp, struct pda_run *pdaRun,
		Stream *input, long stopId, long entry );
long colm_parse_finish( Tree **result, struct colm_program *prg, Tree **sp,
		struct pda_run *pdaRun, Stream *input , int revertOn, long entry );
long colm_parse_undo_frag( struct colm_program *prg, Tree **sp, struct pda_run *pdaRun,
		Stream *input, long steps, long entry );

#ifdef __cplusplus
}
#endif

#endif
