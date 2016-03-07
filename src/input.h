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

#ifndef _INPUT_H
#define _INPUT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSM_BUFSIZE 8192
//#define FSM_BUFSIZE 8

#define INPUT_DATA     1
/* This is for data sources to return, not for the wrapper. */
#define INPUT_EOD      2
#define INPUT_EOF      3
#define INPUT_EOS      4
#define INPUT_LANG_EL  5
#define INPUT_TREE     6
#define INPUT_IGNORE   7

struct LangEl;
struct Pattern;
struct PatternItem;
struct Constructor;
struct ConsItem;
struct colm_tree;
struct colm_stream;
struct colm_location;
struct colm_program;
struct colm_struct;
struct colm_str;
struct colm_stream;

struct stream_impl;

enum run_buf_type {
	RunBufDataType = 0,
	RunBufTokenType,
	RunBufIgnoreType,
	RunBufSourceType
};

struct run_buf
{
	enum run_buf_type type;
	long length;
	struct colm_tree *tree;
	long offset;
	struct run_buf *next, *prev;

	/* Must be at the end. We will grow this struct to add data if the input
	 * demands it. */
	char data[FSM_BUFSIZE];
};

struct run_buf *new_run_buf( int sz );

struct stream_funcs
{
	int (*get_parse_block)( struct stream_impl *ss, int skip, char **pdp, int *copied );

	int (*get_data)( struct stream_impl *ss, char *dest, int length );

	int (*consume_data)( struct colm_program *prg, struct colm_tree **sp,
			struct stream_impl *ss, int length, struct colm_location *loc );
	int (*undo_consume_data)( struct stream_impl *ss, const char *data, int length );

	struct colm_tree *(*consume_tree)( struct stream_impl *ss );
	void (*undo_consume_tree)( struct stream_impl *ss,
			struct colm_tree *tree, int ignore );

	/* Language elments (compile-time). */
	struct LangEl *(*consume_lang_el)( struct stream_impl *ss,
			long *bind_id, char **data, long *length );
	void (*undo_consume_lang_el)( struct stream_impl *ss );

	/* Private implmentation for some shared get data functions. */
	int (*get_data_source)( struct stream_impl *ss, char *dest, int length );

	void (*set_eof)( struct stream_impl *is );
	void (*unset_eof)( struct stream_impl *is );

	/* Prepending to a stream. */
	void (*prepend_data)( struct stream_impl *in, const char *data, long len );
	void (*prepend_tree)( struct stream_impl *is, struct colm_tree *tree, int ignore );
	void (*prepend_stream)( struct stream_impl *in, struct colm_tree *tree );
	int (*undo_prepend_data)( struct stream_impl *is, int length );
	struct colm_tree *(*undo_prepend_tree)( struct stream_impl *is );
	struct colm_tree *(*undo_prepend_stream)( struct stream_impl *in );

	/* Appending to a stream. */
	void (*append_data)( struct stream_impl *in, const char *data, long len );
	void (*append_tree)( struct stream_impl *in, struct colm_tree *tree );
	void (*append_stream)( struct stream_impl *in, struct colm_tree *tree );
	struct colm_tree *(*undo_append_data)( struct stream_impl *in, int length );
	struct colm_tree *(*undo_append_tree)( struct stream_impl *in );
	struct colm_tree *(*undo_append_stream)( struct stream_impl *in );
};

/* List of source streams. Enables streams to be pushed/popped. */
struct stream_impl
{
	struct stream_funcs *funcs;

	char eof_sent;
	char eof;
	char eos_sent;

	struct run_buf *queue;
	struct run_buf *queue_tail;

	const char *data;
	long dlen;
	int offset;

	long line;
	long column;
	long byte;

	char *name;
	FILE *file;

	struct _StrCollect *collect;

	struct Pattern *pattern;
	struct PatternItem *pat_item;
	struct Constructor *constructor;
	struct ConsItem *cons_item;

	int consumed;

	/* Indentation. */
	int level;
	int indent;
};

struct stream_impl *colm_impl_new_pat( char *name, struct Pattern *pattern );
struct stream_impl *colm_impl_new_cons( char *name, struct Constructor *constructor );
struct stream_impl *colm_impl_new_generic( char *name );


void update_position( struct stream_impl *input_stream, const char *data, long length );
void undo_position( struct stream_impl *input_stream, const char *data, long length );

struct stream_impl *colm_stream_impl( struct colm_struct *s );

struct colm_str *collect_string( struct colm_program *prg, struct colm_stream *s );
struct colm_stream *colm_stream_open_collect( struct colm_program *prg );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
