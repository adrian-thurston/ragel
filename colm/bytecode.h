/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _BYTECODE_H
#define _BYTECODE_H

#include "vector.h"
#include "resize.h"
#include "dlist.h"
#include "config.h"
#include "avlmap.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::ostream;

typedef unsigned long ulong;
typedef unsigned char uchar;

#define IN_LOAD_INT              0x01
#define IN_LOAD_STR              0x02
#define IN_LOAD_NIL              0x03
#define IN_LOAD_TRUE             0xa3
#define IN_LOAD_FALSE            0xa4

#define IN_ADD_INT               0x04
#define IN_SUB_INT               0x06
#define IN_MULT_INT              0x05

#define IN_TST_EQL               0xa0
#define IN_TST_NOT_EQL           0xa1
#define IN_TST_LESS              0x0e
#define IN_TST_GRTR              0x10
#define IN_TST_LESS_EQL          0x0f
#define IN_TST_GRTR_EQL          0x11
#define IN_TST_LOGICAL_AND       0x12
#define IN_TST_LOGICAL_OR        0x13

#define IN_NOT                   0x16

#define IN_JMP                   0x0c
#define IN_JMP_FALSE             0x0b
#define IN_JMP_TRUE              0x0d

#define IN_STR_ATOI              0x14
#define IN_STR_LENGTH            0x15
#define IN_CONCAT_STR            0x17

#define IN_INIT_LOCALS           0x18
#define IN_POP_LOCALS            0xb0
#define IN_POP                   0x19
#define IN_DUP_TOP               0x1a
#define IN_REJECT                0x1b
#define IN_MATCH                 0x1c
#define IN_CONSTRUCT             0x1d
#define IN_TREE_NEW              0x1f

#define IN_GET_LOCAL_R           0x20
#define IN_GET_LOCAL_WC          0x21
#define IN_SET_LOCAL_WC          0x22

#define IN_GET_LOCAL_REF_R       0x23
#define IN_GET_LOCAL_REF_WC      0x24
#define IN_SET_LOCAL_REF_WC      0x25

#define IN_SAVE_RET              0x26

#define IN_GET_FIELD_R           0x27
#define IN_GET_FIELD_WC          0x28
#define IN_GET_FIELD_WV          0x29
#define IN_GET_FIELD_BKT         0x2a

#define IN_SET_FIELD_WV          0x2b
#define IN_SET_FIELD_WC          0x2c
#define IN_SET_FIELD_BKT         0x2d
#define IN_SET_FIELD_LEAVE_WC    0x2e

#define IN_GET_MATCH_LENGTH_R    0x2f
#define IN_GET_MATCH_TEXT_R      0x30

#define IN_GET_TOKEN_DATA_R      0x31
#define IN_SET_TOKEN_DATA_WC     0x32
#define IN_SET_TOKEN_DATA_WV     0x33
#define IN_SET_TOKEN_DATA_BKT    0x34

#define IN_GET_TOKEN_POS_R       0x35

#define IN_INIT_RHS_EL           0x3b

#define IN_TRITER_FROM_REF       0x3c
#define IN_TRITER_ADVANCE        0x3d
#define IN_TRITER_NEXT_CHILD     0x98
#define IN_TRITER_PREV_CHILD     0x9b
#define IN_TRITER_GET_CUR_R      0x3e
#define IN_TRITER_GET_CUR_WC     0x3f
#define IN_TRITER_SET_CUR_WC     0x40
#define IN_TRITER_DESTROY        0x41

#define IN_UITER_DESTROY         0x52
#define IN_UITER_CREATE          0x53
#define IN_UITER_ADVANCE         0x54
#define IN_UITER_GET_CUR_R       0x55
#define IN_UITER_GET_CUR_WC      0x56
#define IN_UITER_SET_CUR_WC      0x57

#define IN_TREE_SEARCH           0x58

#define IN_LOAD_GLOBAL_R         0x59
#define IN_LOAD_GLOBAL_WV        0x5a
#define IN_LOAD_GLOBAL_WC        0x9d
#define IN_LOAD_GLOBAL_BKT       0x5b

#define IN_PTR_DEREF_R           0x5e
#define IN_PTR_DEREF_WV          0x5f
#define IN_PTR_DEREF_WC          0x60
#define IN_PTR_DEREF_BKT         0x61

#define IN_REF_FROM_LOCAL        0x62
#define IN_REF_FROM_REF          0x97
#define IN_TRITER_REF_FROM_CUR   0x63
#define IN_UITER_REF_FROM_CUR    0x64
                                
#define IN_MAP_LENGTH            0x65
#define IN_MAP_FIND              0x66
#define IN_MAP_INSERT_WV         0x67
#define IN_MAP_INSERT_WC         0x68
#define IN_MAP_INSERT_BKT        0x69
#define IN_MAP_STORE_WV          0x6a
#define IN_MAP_STORE_WC          0x6b
#define IN_MAP_STORE_BKT         0x6c
#define IN_MAP_REMOVE_WV         0x6d
#define IN_MAP_REMOVE_WC         0x6e
#define IN_MAP_REMOVE_BKT        0x6f

#define IN_LIST_LENGTH           0x70
#define IN_LIST_APPEND_WV        0x71
#define IN_LIST_APPEND_WC        0x72
#define IN_LIST_APPEND_BKT       0x73
#define IN_LIST_REMOVE_END_WV    0x74
#define IN_LIST_REMOVE_END_WC    0x75
#define IN_LIST_REMOVE_END_BKT   0x76

#define IN_GET_LIST_MEM_R        0x77
#define IN_GET_LIST_MEM_WC       0x78
#define IN_GET_LIST_MEM_WV       0x79
#define IN_GET_LIST_MEM_BKT      0x7a
#define IN_SET_LIST_MEM_WV       0x7b
#define IN_SET_LIST_MEM_WC       0x7c
#define IN_SET_LIST_MEM_BKT      0x7d

#define IN_VECTOR_LENGTH         0x7e
#define IN_VECTOR_APPEND_WV      0x7f
#define IN_VECTOR_APPEND_WC      0x80
#define IN_VECTOR_APPEND_BKT     0x81
#define IN_VECTOR_INSERT_WV      0x82
#define IN_VECTOR_INSERT_WC      0x83
#define IN_VECTOR_INSERT_BKT     0x84

#define IN_PRINT                 0x87
#define IN_PRINT_XML             0x88

#define IN_HALT                  0x8a

#define IN_CALL_WC               0x8b
#define IN_CALL_WV               0x9c
#define IN_RET                   0x8c
#define IN_YIELD                 0x8d
#define IN_STOP                  0x8e

#define IN_STR_UORD8             0x8f
#define IN_STR_SORD8             0x90
#define IN_STR_UORD16            0x91
#define IN_STR_SORD16            0x92
#define IN_STR_UORD32            0x93
#define IN_STR_SORD32            0x94

#define IN_INT_TO_STR            0x99

#define IN_CREATE_TOKEN          0x95
#define IN_MAKE_TOKEN            0x96
#define IN_MAKE_TREE             0xb2
#define IN_CONSTRUCT_TERM        0x9a
#define IN_PARSE                 0xb1
#define IN_PARSE_BKT             0xb3
#define IN_STREAM_PULL           0xb4
#define IN_STREAM_PULL_BKT       0xb5
#define IN_STREAM_PUSH           0xbc
#define IN_STREAM_PUSH_BKT       0xbd
#define IN_SEND                  0xb6
#define IN_IGNORE                0xb7

#define IN_OPEN_FILE             0xb8
#define IN_GET_STDIN             0xb9
#define IN_GET_STDOUT            0xba
#define IN_GET_STDERR            0xbb


/* Types */
#define TYPE_NIL      0x01
#define TYPE_TREE     0x02
#define TYPE_REF      0x03
#define TYPE_PTR      0x04
#define TYPE_ITER     0x05

/* Types of Generics. */
#define GEN_LIST      0x10
#define GEN_MAP       0x11
#define GEN_VECTOR    0x12

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

/* Virtual machine stack size, number of pointers. 
 * This will be mmapped. */
#define VM_STACK_SIZE (4*1024ll*1024ll) 

/* Known language element ids. */
#define LEL_ID_PTR    1
#define LEL_ID_BOOL   2
#define LEL_ID_INT    3
#define LEL_ID_STR    4
#define LEL_ID_STREAM 5

#define AF_GENERATED   0x1
#define AF_COMMITTED   0x2
#define AF_REV_FREED   0x4
#define AF_ARTIFICIAL  0x8
#define AF_NAMED       0x10
#define AF_GROUP_MEM   0x20
#define AF_IGNORE      0x40
#define AF_HAS_RCODE   0x80

/*
 * Call stack.
 */

/* Number of spots in the frame, after the args. */
#define FR_AA 3

/* Positions relative to the frame pointer. */
#define FR_RV 2    /* return value */
#define FR_RI 1    /* return instruction */
#define FR_RF 0    /* return frame pointer */

/*
 * Calling Convention:
 *   a1
 *   a2
 *   a3
 *   ...
 *   return value FR_RV
 *   return instr FR_RI
 *   return frame FR_RF
 */

/*
 * User iterator call stack. 
 * Adds an iframe pointer, removes the return value.
 */

/* Number of spots in the frame, after the args. */
#define IFR_AA  3

/* Positions relative to the frame pointer. */
#define IFR_RIN 2    /* return instruction */
#define IFR_RIF 1    /* return iframe pointer */
#define IFR_RFR 0    /* return frame pointer */

#define vm_push(i) (*(--sp) = (i))
#define vm_pop() (*sp++)
#define vm_top() (*sp)
#define vm_ptop() (sp)

struct Kid;
struct Tree;
struct Alg;
struct ListEl;
struct MapEl;
struct PdaTables;
struct RuntimeData;
struct FsmRun;
struct PdaRun;
struct Program;
struct List;
struct Map;
struct Stream;
struct Ref;
struct TreeIter;
struct Pointer;

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;

typedef Tree *SW;
typedef Tree **StackPtr;
typedef Tree **&StackRef;

Tree **alloc_obj_data( long length );

Kid *alloc_attrs( Program *prg, long length );
void free_attrs( Program *prg, Kid *attrs );
void set_attr( Tree *tree, long pos, Tree *val );
Tree *get_attr( Tree *tree, long pos );

/* Return the size of a type in words. */
template<class T> int sizeof_in_words()
{
	assert( (sizeof(T) % sizeof(Word)) == 0 );
	return sizeof(T) / sizeof(Word);
}

/* 
 * Code Vector
 */
struct CodeVect : public Vector<Code>
{
	void appendHalf( Half half )
	{
		/* not optimal. */
		append( half & 0xff );
		append( (half>>8) & 0xff );
	}
	
	void appendWord( Word word )
	{
		/* not optimal. */
		append( word & 0xff );
		append( (word>>8) & 0xff );
		append( (word>>16) & 0xff );
		append( (word>>24) & 0xff );
	}

	void setHalf( long pos, Half half )
	{
		/* not optimal. */
		data[pos] = half & 0xff;
		data[pos+1] = (half>>8) & 0xff;
	}
	
	void insertHalf( long pos, Half half )
	{
		/* not optimal. */
		insert( pos, half & 0xff );
		insert( pos+1, (half>>8) & 0xff );
	}

	void insertWord( long pos, Word word )
	{
		/* not optimal. */
		insert( pos, word & 0xff );
		insert( pos+1, (word>>8) & 0xff );
		insert( pos+2, (word>>16) & 0xff );
		insert( pos+3, (word>>24) & 0xff );
	}
	
	void insertTree( long pos, Tree *tree )
		{ insertWord( pos, (Word) tree ); }
};

/*
 * Strings
 */

/* Header located just before string data. */
struct Head
{
	const char *data;
	long length;
};

struct TreePair
{
	TreePair() : key(0), val(0) {}

	Tree *key;
	Tree *val;
};

struct Program;
struct Stream;

bool test_false( Program *prg, Tree *tree );

Head *string_alloc_new( Program *prg, const char *data, long length );
Head *string_alloc_const( Program *prg, const char *data, long length );
Head *string_copy( Program *prg, Head *head );
void string_free( Program *prg, Head *head );
long string_length( Head *str );
const char *string_data( Head *str );
void string_shorten( Head *tokdata, long newlen );
Head *concat_str( Head *s1, Head *s2 );
Word str_atoi( Head *str );
Word str_uord16( Head *head );
Word str_uord8( Head *head );
Word cmp_string( Head *s1, Head *s2 );

Head *int_to_str( Program *prg, Word i );

void rcode_downref( Tree **stack_root, Program *prg, Code *instr );
void rcode_downref_all( Tree **stack_root, Program *prg, CodeVect *cv );
void parsed_downref( Tree **root, Program *prg, Tree *tree );

bool match_pattern( Tree **bindings, Program *prg, long pat, Kid *kid, bool checkNext );
Head *make_literal( Program *prg, long litoffset );
Tree *construct_string( Program *prg, Head *s );
Tree *construct_integer( Program *prg, long i );
Tree *construct_string( Program *prg, Head *s );
Tree *construct_pointer( Program *prg, Tree *tree );
Tree *construct_term( Program *prg, Word id, Head *tokdata );
Tree *construct_replacement_tree( Tree **bindings, Program *prg, long pat );
Tree *create_generic( Program *prg, long genericId );

Tree *open_file( Program *prg, Tree *name );
Stream *open_stream_fd( Program *prg, long fd );
Stream *open_stream( Program *prg, FILE *file );

void tree_downref( Program *prg, Tree **sp, Tree *tree );
void tree_upref( Tree *tree );
Kid *tree_child( Program *prg, Tree *tree );
Kid *tree_extract_child( Program *prg, Tree *tree );
Kid *tree_ignore( Program *prg, Tree *tree );
bool tree_is_ignore( Program *prg, Kid *kid );
Kid *kid_list_concat( Kid *list1, Kid *list2 );
void ignore_data( Tree *tree, char *dest );
long ignore_length( Tree *tree );
Tree *split_tree( Program *prg, Tree *t );
Tree *copy_tree( Program *prg, Tree *tree, Kid *oldNextDown, Kid *&newNextDown );
Tree *copy_real_tree( Program *prg, Tree *tree, Kid *oldNextDown, Kid *&newNextDown );
Tree *make_tree( Tree **root, Program *prg, long nargs );
Tree *make_token( Tree **root, Program *prg, long nargs );

void print_tree( Tree **&sp, Program *prg, Tree *tree );
void print_tree( ostream &out, Tree **&sp, Program *prg, Tree *tree );
void print_str( Head *str );
void xml_print_tree( Tree **&sp, Program *prg, Tree *tree );
void xml_print_kid( Tree **&sp, Program *prg, Kid *kid, int depth );

long list_length( List *list );
void list_append( Program *prg, List *list, Tree *val );
Tree *list_remove_end( Program *prg, List *list );
Tree *get_list_mem( List *list, Word field );
Tree *get_list_mem_split( Program *prg, List *list, Word field );
Tree *set_list_mem( List *list, Half field, Tree *value );

Tree *map_find( Map *map, Tree *key );
long map_length( Map *map );
bool map_insert( Program *prg, Map *map, Tree *key, Tree *element );
void map_unremove( Program *prg, Map *map, Tree *key, Tree *element );
Tree *map_uninsert( Program *prg, Map *map, Tree *key );
Tree *map_store( Program *prg, Map *map, Tree *key, Tree *element );
Tree *map_unstore( Program *prg, Map *map, Tree *key, Tree *existing );
TreePair map_remove( Program *prg, Map *map, Tree *key );

Tree *get_ptr_val( Pointer *ptr );
Tree *get_ptr_val_split( Program *prg, Pointer *ptr );
Tree *get_field( Tree *tree, Word field );
Tree *get_field_split( Program *prg, Tree *tree, Word field );
Tree *get_rhs_el( Program *prg, Tree *lhs, long position );
void set_field( Program *prg, Tree *tree, long field, Tree *value );

Tree *tree_iter_advance( Program *prg, Tree **&sp, TreeIter *iter );
Tree *tree_iter_next_child( Program *prg, Tree **&sp, TreeIter *iter );
Tree *tree_iter_prev_child( Program *prg, Tree **&sp, TreeIter *iter );
Tree *tree_iter_deref_cur( TreeIter *iter );
void set_triter_cur( TreeIter *iter, Tree *tree );
void split_iter_cur( Tree **&sp, Program *prg, TreeIter *iter );
void ref_set_value( Ref *ref, Tree *v );
Tree *tree_search( Kid *kid, long id );
Tree *tree_search( Tree *tree, long id );
void split_ref( Tree **&sp, Program *prg, Ref *fromRef );

/*
 * Maps
 */
struct GenericInfo
{
	long type;
	long typeArg;
	long keyOffset;
	long keyType;
	long langElId;
};

long cmp_tree( const Tree *tree1, const Tree *tree2 );

/*
 * Runtime environment
 */

struct PoolItem
{
	PoolItem *next;
};

template <class T> struct PoolBlock
{
	T data[FRESH_BLOCK];
	PoolBlock<T> *next;
};

template <class T> struct PoolAlloc
{
	PoolAlloc() : 
		head(0), nextel(FRESH_BLOCK), pool(0)
	{}

	T *allocate();
	void free( T *el );
	void clear();
	long numlost();

	PoolBlock<T> *head;
	long nextel;
	PoolItem *pool;
};

template <class T> T *PoolAlloc<T>::allocate()
{
	//#ifdef COLM_LOG_BYTECODE
	//cerr << "allocating in: " << __PRETTY_FUNCTION__ << endl;
	//#endif

	T *newEl = 0;
	if ( pool == 0 ) {
		if ( nextel == FRESH_BLOCK ) {
			#ifdef COLM_LOG_BYTECODE
			cerr << "allocating " << FRESH_BLOCK << " Elements of type T" << endl;
			#endif

			PoolBlock<T> *newBlock = new PoolBlock<T>;
			newBlock->next = head;
			head = newBlock;
			nextel = 0;
		}
		newEl = &head->data[nextel++];
	}
	else {
		newEl = (T*)pool;
		pool = pool->next;
	}
	memset( newEl, 0, sizeof(T) );
	return newEl;
}

template <class T> void PoolAlloc<T>::free( T *el )
{
	//#ifdef COLM_LOG_BYTECODE
	//cerr << "freeing in: " << __PRETTY_FUNCTION__ << endl;
	//#endif

	memset( el, 0, sizeof(T) );
	PoolItem *pi = (PoolItem*) el;
	pi->next = pool;
	pool = pi;
}

template <class T> void PoolAlloc<T>::clear()
{
	PoolBlock<T> *block = head;
	while ( block != 0 ) {
		PoolBlock<T> *next = block->next;
		delete block;
		block = next;
	}

	head = 0;
	nextel = 0;
	pool = 0;
}

template <class T> long PoolAlloc<T>::numlost()
{
	/* Count the number of items allocated. */
	long lost = 0;
	PoolBlock<T> *block = head;
	if ( block != 0 ) {
		lost = nextel;
		block = block->next;
		while ( block != 0 ) {
			lost += FRESH_BLOCK;
			block = block->next;
		}
	}

	/* Subtract. Items that are on the free list. */
	PoolItem *pi = pool;
	while ( pi != 0 ) {
		lost -= 1;
		pi = pi->next;
	}

	return lost;
}

struct Int;

struct Program
{
	Program( bool ctxDepParsing, RuntimeData *rtd );

	bool ctxDepParsing;
	RuntimeData *rtd;
	Tree *global;

	PoolAlloc<Kid> kidPool;
	PoolAlloc<Tree> treePool;
	PoolAlloc<Alg> algPool;
	PoolAlloc<ListEl> listElPool;
	PoolAlloc<MapEl> mapElPool;

	Tree *trueVal;
	Tree *falseVal;

	void run();

	void clear( Tree **vm_stack, Tree **sp );
	void clearGlobal( Tree **sp );
	void allocGlobal();

	Kid *heap;

	Stream *stdinVal;
	Stream *stdoutVal;
	Stream *stderrVal;
};

struct Execution
{
	Execution( Program *prg, CodeVect &reverseCode,
			PdaRun *parser, Code *code, Tree *lhs, Head *matchText );

	Program *prg;
	PdaTables *pdaTables;
	PdaRun *parser;
	Code *code;
	Tree **frame;
	Tree **iframe;
	Tree *lhs;

	Head *matchText;
	bool reject;

	/* Reverse code. */
	CodeVect &reverseCode;
	long rcodeUnitLen;

	void execute( Tree **root );
	void rexecute( Tree **root, CodeVect *allRev );
	void execute( Tree **&sp, Code *instr );
	void rdownref( Code *instr );
};

#endif
