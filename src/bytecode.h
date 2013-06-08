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

#ifndef _BYTECODE_H
#define _BYTECODE_H

#include <colm/pdarun.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

typedef unsigned long ulong;
typedef unsigned char uchar;

#define IN_LOAD_INT              0x01
#define IN_LOAD_STR              0x02
#define IN_LOAD_NIL              0x03
#define IN_LOAD_TRUE             0x04
#define IN_LOAD_FALSE            0x05
#define IN_LOAD_TREE             0x06
#define IN_LOAD_WORD             0x07

#define IN_ADD_INT               0x08
#define IN_SUB_INT               0x09
#define IN_MULT_INT              0x0a
#define IN_DIV_INT               0x0b

#define IN_TST_EQL               0x0c
#define IN_TST_NOT_EQL           0x0d
#define IN_TST_LESS              0x0e
#define IN_TST_GRTR              0x0f
#define IN_TST_LESS_EQL          0x10
#define IN_TST_GRTR_EQL          0x11
#define IN_TST_LOGICAL_AND       0x12
#define IN_TST_LOGICAL_OR        0x13

#define IN_NOT                   0x14

#define IN_JMP                   0x15
#define IN_JMP_FALSE             0x16
#define IN_JMP_TRUE              0x17

#define IN_STR_ATOI              0x18
#define IN_STR_LENGTH            0x19
#define IN_CONCAT_STR            0x1a
#define IN_TREE_TRIM             0x1b

#define IN_INIT_LOCALS           0x1c
#define IN_POP                   0x1d
#define IN_POP_N_WORDS           0x1e
#define IN_DUP_TOP               0x1f
#define IN_TOP_SWAP              0x20

#define IN_REJECT                0x21
#define IN_MATCH                 0x22
#define IN_CONSTRUCT             0x23
#define IN_TREE_NEW              0x24

#define IN_GET_LOCAL_R           0x25
#define IN_GET_LOCAL_WC          0x26
#define IN_SET_LOCAL_WC          0x27

#define IN_GET_LOCAL_REF_R       0x28
#define IN_GET_LOCAL_REF_WC      0x29
#define IN_SET_LOCAL_REF_WC      0x2a

#define IN_SAVE_RET              0x2b

#define IN_GET_FIELD_R           0x2c
#define IN_GET_FIELD_WC          0x2d
#define IN_GET_FIELD_WV          0x2e
#define IN_GET_FIELD_BKT         0x2f

#define IN_SET_FIELD_WV          0x30
#define IN_SET_FIELD_WC          0x31
#define IN_SET_FIELD_BKT         0x32
#define IN_SET_FIELD_LEAVE_WC    0x33

#define IN_GET_MATCH_LENGTH_R    0x34
#define IN_GET_MATCH_TEXT_R      0x35

#define IN_GET_TOKEN_DATA_R      0x36
#define IN_SET_TOKEN_DATA_WC     0x37
#define IN_SET_TOKEN_DATA_WV     0x38
#define IN_SET_TOKEN_DATA_BKT    0x39

#define IN_GET_TOKEN_POS_R       0x3a
#define IN_GET_TOKEN_LINE_R      0x3b

#define IN_INIT_RHS_EL           0x3c
#define IN_INIT_LHS_EL           0x3d
#define IN_INIT_CAPTURES         0x3e
#define IN_STORE_LHS_EL          0x3f
#define IN_RESTORE_LHS           0x40

#define IN_TRITER_FROM_REF       0x41
#define IN_TRITER_ADVANCE        0x42
#define IN_TRITER_NEXT_CHILD     0x43
#define IN_TRITER_GET_CUR_R      0x44
#define IN_TRITER_GET_CUR_WC     0x45
#define IN_TRITER_SET_CUR_WC     0x46
#define IN_TRITER_DESTROY        0x47
#define IN_TRITER_NEXT_REPEAT    0x48
#define IN_TRITER_PREV_REPEAT    0x49

#define IN_REV_TRITER_FROM_REF   0x4a
#define IN_REV_TRITER_DESTROY    0x4b
#define IN_REV_TRITER_PREV_CHILD 0x4c

#define IN_UITER_DESTROY         0x4d
#define IN_UITER_CREATE_WV       0x4e
#define IN_UITER_CREATE_WC       0x4f
#define IN_UITER_ADVANCE         0x50
#define IN_UITER_GET_CUR_R       0x51
#define IN_UITER_GET_CUR_WC      0x52
#define IN_UITER_SET_CUR_WC      0x53

#define IN_TREE_SEARCH           0x54

#define IN_LOAD_GLOBAL_R         0x55
#define IN_LOAD_GLOBAL_WV        0x56
#define IN_LOAD_GLOBAL_WC        0x57
#define IN_LOAD_GLOBAL_BKT       0x58

#define IN_PTR_DEREF_R           0x59
#define IN_PTR_DEREF_WV          0x5a
#define IN_PTR_DEREF_WC          0x60
#define IN_PTR_DEREF_BKT         0x61

#define IN_REF_FROM_LOCAL        0x62
#define IN_REF_FROM_REF          0x63
#define IN_REF_FROM_QUAL_REF     0x64
#define IN_REF_FROM_BACK         0xe3
#define IN_TRITER_REF_FROM_CUR   0x65
#define IN_UITER_REF_FROM_CUR    0x66

#define IN_MAP_LENGTH            0x67
#define IN_MAP_FIND              0x68
#define IN_MAP_INSERT_WV         0x69
#define IN_MAP_INSERT_WC         0x6a
#define IN_MAP_INSERT_BKT        0x6b
#define IN_MAP_STORE_WV          0x6c
#define IN_MAP_STORE_WC          0x6d
#define IN_MAP_STORE_BKT         0x6e
#define IN_MAP_REMOVE_WV         0x6f
#define IN_MAP_REMOVE_WC         0x70
#define IN_MAP_REMOVE_BKT        0x71

#define IN_LIST_LENGTH           0x72
#define IN_LIST_APPEND_WV        0x73
#define IN_LIST_APPEND_WC        0x74
#define IN_LIST_APPEND_BKT       0x75
#define IN_LIST_REMOVE_END_WV    0x76
#define IN_LIST_REMOVE_END_WC    0x77
#define IN_LIST_REMOVE_END_BKT   0x78

#define IN_GET_LIST_MEM_R        0x79
#define IN_GET_LIST_MEM_WC       0x7a
#define IN_GET_LIST_MEM_WV       0x7b
#define IN_GET_LIST_MEM_BKT      0x7c
#define IN_SET_LIST_MEM_WV       0x7d
#define IN_SET_LIST_MEM_WC       0x7e
#define IN_SET_LIST_MEM_BKT      0x7f

#define IN_VECTOR_LENGTH         0x80
#define IN_VECTOR_APPEND_WV      0x81
#define IN_VECTOR_APPEND_WC      0x82
#define IN_VECTOR_APPEND_BKT     0x83
#define IN_VECTOR_INSERT_WV      0x84
#define IN_VECTOR_INSERT_WC      0x85
#define IN_VECTOR_INSERT_BKT     0x86

#define IN_PRINT                 0x87
#define IN_PRINT_XML_AC          0x88
#define IN_PRINT_XML             0x89
#define IN_PRINT_STREAM          0x8a

#define IN_HALT                  0x8b

#define IN_CALL_WC               0x8c
#define IN_CALL_WV               0x8d
#define IN_RET                   0x8e
#define IN_YIELD                 0x8f
#define IN_STOP                  0x90

#define IN_STR_UORD8             0x91
#define IN_STR_SORD8             0x92
#define IN_STR_UORD16            0x93
#define IN_STR_SORD16            0x94
#define IN_STR_UORD32            0x95
#define IN_STR_SORD32            0x96

#define IN_INT_TO_STR            0x97
#define IN_TREE_TO_STR           0x98
#define IN_TREE_TO_STR_TRIM      0x99

#define IN_CREATE_TOKEN          0x9a
#define IN_MAKE_TOKEN            0x9b
#define IN_MAKE_TREE             0x9c
#define IN_CONSTRUCT_TERM        0x9d

#define IN_INPUT_PULL_WV         0x9e
#define IN_INPUT_PULL_WC         0xe1
#define IN_INPUT_PULL_BKT        0x9f

#define IN_PARSE_SAVE_STEPS      0xa0
#define IN_PARSE_INIT_BKT        0xa1

#define IN_PARSE_FRAG_WC         0xa2
#define IN_PARSE_FRAG_EXIT_WC    0xa3

#define IN_PARSE_FRAG_WV         0xa4
#define IN_PARSE_FRAG_EXIT_WV    0xa5

#define IN_PARSE_FRAG_BKT        0xa6
#define IN_PARSE_FRAG_EXIT_BKT   0xa7

#define IN_INPUT_APPEND_WC       0xa8
#define IN_INPUT_APPEND_WV       0xa9
#define IN_INPUT_APPEND_BKT      0xaa

#define IN_PARSE_FINISH_WC       0xab
#define IN_PARSE_FINISH_EXIT_WC  0xac

#define IN_PARSE_FINISH_WV       0xad
#define IN_PARSE_FINISH_EXIT_WV  0xae

#define IN_PARSE_FINISH_BKT      0xaf
#define IN_PARSE_FINISH_EXIT_BKT 0xb0

#define IN_PCR_CALL              0xb1
#define IN_PCR_RET               0xb2
#define IN_PCR_END_DECK          0xb3
#define IN_CONTIGUOUS            0x5c

#define IN_OPEN_FILE             0xb4
#define IN_GET_STDIN             0xb5
#define IN_GET_STDOUT            0xb6
#define IN_GET_STDERR            0xb7
#define IN_LOAD_ARGV             0xb8
#define IN_TO_UPPER              0xb9
#define IN_TO_LOWER              0xba
#define IN_EXIT                  0xbb

#define IN_LOAD_PARSER_R         0xbd
#define IN_LOAD_PARSER_WV        0xbe
#define IN_LOAD_PARSER_WC        0xbf
#define IN_LOAD_PARSER_BKT       0xc0

#define IN_LOAD_INPUT_R          0xc1
#define IN_LOAD_INPUT_WV         0xc2
#define IN_LOAD_INPUT_WC         0xc3
#define IN_LOAD_INPUT_BKT        0xc4

#define IN_INPUT_PUSH_WV         0xc5
#define IN_INPUT_PUSH_BKT        0xc6
#define IN_INPUT_PUSH_IGNORE_WV  0xc7

#define IN_LOAD_CONTEXT_R        0xc8
#define IN_LOAD_CONTEXT_WV       0xc9
#define IN_LOAD_CONTEXT_WC       0xca
#define IN_LOAD_CONTEXT_BKT      0xcb

#define IN_GET_PARSER_CTX_R      0xcd
#define IN_GET_PARSER_CTX_WC     0xce
#define IN_GET_PARSER_CTX_WV     0xcf
#define IN_SET_PARSER_CTX_WC     0xd0
#define IN_SET_PARSER_CTX_WV     0xd1

#define IN_LOAD_CTX_R            0xd2
#define IN_LOAD_CTX_WC           0xd3
#define IN_LOAD_CTX_WV           0xd4
#define IN_LOAD_CTX_BKT          0xd5

#define IN_SPRINTF               0xd6

#define IN_GET_RHS_VAL_R         0xd7
#define IN_GET_RHS_VAL_WC        0xd8
#define IN_GET_RHS_VAL_WV        0xd9
#define IN_GET_RHS_VAL_BKT       0xda
#define IN_SET_RHS_VAL_WC        0xdb
#define IN_SET_RHS_VAL_WV        0xdc
#define IN_SET_RHS_VAL_BKT       0xdd

#define IN_CONSTRUCT_INPUT       0xde
#define IN_SET_INPUT             0xdf
#define IN_GET_INPUT             0xe0

#define IN_GET_PARSER_MEM_R      0x5b
#define IN_GET_PARSER_MEM_WC     0x00
#define IN_GET_PARSER_MEM_WV     0x00
#define IN_GET_PARSER_MEM_BKT    0x00
#define IN_SET_PARSER_MEM_WC     0x00
#define IN_SET_PARSER_MEM_WV     0x00
#define IN_SET_PARSER_MEM_BKT    0x00

#define IN_GET_ERROR             0xcc
#define IN_SET_ERROR             0xe2


/* Types */
#define TYPE_NIL          0x01
#define TYPE_TREE         0x02
#define TYPE_REF          0x03
#define TYPE_PTR          0x04
#define TYPE_ITER         0x05
#define TYPE_IGNORE_LIST  0x06

/* Types of Generics. */
#define GEN_LIST      0x10
#define GEN_MAP       0x11
#define GEN_VECTOR    0x12
#define GEN_PARSER    0x13

/* Known language element ids. */
#define LEL_ID_PTR          1
#define LEL_ID_BOOL         2
#define LEL_ID_INT          3
#define LEL_ID_STR          4
#define LEL_ID_STREAM       5
#define LEL_ID_IGNORE       6

/*
 * Flags
 */

/* A tree that has been generated by a termDup. */
#define PF_TERM_DUP            0x0001

/* Has been processed by the commit function. All children have also been
 * processed. */
#define PF_COMMITTED           0x0002

/* Created by a token generation action, not made from the input. */
#define PF_ARTIFICIAL          0x0004

/* Named node from a pattern or constructor. */
#define PF_NAMED               0x0008

/* There is reverse code associated with this tree node. */
#define PF_HAS_RCODE           0x0010

#define PF_RIGHT_IGNORE        0x0020

#define PF_LEFT_IL_ATTACHED    0x0400
#define PF_RIGHT_IL_ATTACHED   0x0800

#define AF_LEFT_IGNORE   0x0100
#define AF_RIGHT_IGNORE  0x0200

#define AF_SUPPRESS_LEFT  0x4000
#define AF_SUPPRESS_RIGHT 0x8000

/*
 * Call stack.
 */

/* Number of spots in the frame, after the args. */
#define FR_AA 4

/* Positions relative to the frame pointer. */
#define FR_RV  3    /* return value */
#define FR_RI  2    /* return instruction */
#define FR_RFP 1    /* return frame pointer */
#define FR_RFD 0    /* return frame id. */

/*
 * Calling Convention:
 *   a1
 *   a2
 *   a3
 *   ...
 *   return value      FR_RV
 *   return instr      FR_RI
 *   return frame ptr  FR_RFP
 *   return frame id   FR_RFD
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

/* Exported to modules other than bytecode.c */
#define vm_push(i)      ( ( sp == prg->sb_beg ? (sp = vm_bs_add(prg, sp, 1)) : 0 ), (*(--sp) = (i)) )
#define vm_pushn(n)     ( ( (sp-(n)) < prg->sb_beg ? (sp = vm_bs_add(prg, sp, n)) : 0 ), (sp -= (n)) )

#define vm_pop()        ({ SW r = *sp; (sp+1) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, 1)) : (sp += 1); r; })
#define vm_pop_ignore() ({ (sp+1) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, 1)) : (sp += 1); })
#define vm_popn(n)      ({ (sp+(n)) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, n)) : (sp += (n)); })

#define vm_contiguous(n) ( ( (sp-(n)) < prg->sb_beg ? (sp = vm_bs_add(prg, sp, n)) : 0 ) )

#define vm_top() (*sp)
#define vm_ptop() (sp)

#define vm_ssize()       ( prg->sb_total + (prg->sb_end - sp) )

#define vm_local(o) (exec->framePtr[o])
#define vm_plocal(o) (&exec->framePtr[o])
#define vm_local_iframe(o) (exec->iframePtr[o])
#define vm_plocal_iframe(o) (&exec->iframePtr[o])

void vm_init( struct colm_program * );
Tree** vm_bs_add( struct colm_program *, Tree **, int );
Tree** vm_bs_pop( struct colm_program *, Tree **, int );
void vm_clear( struct colm_program * );

typedef Tree *SW;
typedef Tree **StackPtr;

/* Can't use sizeof() because we have used types that are bigger than the
 * serial representation. */
#define SIZEOF_CODE 1
#define SIZEOF_HALF 2
#define SIZEOF_WORD sizeof(Word)

typedef struct _Execution
{
	Tree **framePtr;
	Tree **iframePtr;
	long frameId;

	long rcodeUnitLen;

	Parser *parser;
	long steps;
	long pcr;
} Execution;

long stringLength( Head *str );
const char *stringData( Head *str );
Head *stringAllocFull( struct colm_program *prg, const char *data, long length );
Head *initStrSpace( long length );
Head *stringCopy( struct colm_program *prg, Head *head );
void stringFree( struct colm_program *prg, Head *head );
void stringShorten( Head *tokdata, long newlen );
Head *concatStr( Head *s1, Head *s2 );
Word strAtoi( Head *str );
Word strUord16( Head *head );
Word strUord8( Head *head );
Word cmpString( Head *s1, Head *s2 );
Head *stringToUpper( Head *s );
Head *stringToLower( Head *s );
Head *stringSprintf( struct colm_program *prg, Str *format, Int *integer );

Head *makeLiteral( struct colm_program *prg, long litoffset );
Head *intToStr( struct colm_program *prg, Word i );

Tree *constructString( struct colm_program *prg, Head *s );

void mainExecution( struct colm_program *prg, Execution *exec, Code *code );
void reductionExecution( Execution *exec, Tree **sp );
void generationExecution( Execution *exec, Tree **sp );
void reverseExecution( Execution *exec, Tree **sp, RtCodeVect *allRev );

Kid *allocAttrs( struct colm_program *prg, long length );
void freeAttrs( struct colm_program *prg, Kid *attrs );
void setAttr( Tree *tree, long pos, Tree *val );
Kid *getAttrKid( Tree *tree, long pos );

Tree *splitTree( struct colm_program *prg, Tree *t );
void rcodeDownrefAll( struct colm_program *prg, Tree **sp, RtCodeVect *cv );
void commitFull( struct colm_program *prg, Tree **sp, PdaRun *pdaRun, long commitReduce );
Tree *getParsedRoot( PdaRun *pdaRun, int stop );
void splitRef( struct colm_program *prg, Tree ***sp, Ref *fromRef );

void allocGlobal( struct colm_program *prg );
Tree **executeCode( struct colm_program *prg, Execution *exec, Tree **sp, Code *instr );
void rcodeDownref( struct colm_program *prg, Tree **sp, Code *instr );
Code *popReverseCode( RtCodeVect *allRev );

#ifdef __cplusplus
}
#endif

#endif
