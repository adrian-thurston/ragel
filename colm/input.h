/*
 * Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COLM_INPUT_H
#define _COLM_INPUT_H

#include <stdio.h>
#include "colm.h"

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
struct colm_tree;
struct colm_stream;
struct colm_location;
struct colm_program;
struct colm_struct;
struct colm_str;
struct colm_stream;

struct input_impl;
struct stream_impl;

typedef colm_alph_t alph_t;

#define DEF_INPUT_FUNCS( input_funcs, _input_impl ) \
struct input_funcs \
{ \
	int (*get_parse_block)( struct colm_program *prg, struct _input_impl *si, int *pskip, alph_t **pdp, int *copied ); \
	int (*get_data)( struct colm_program *prg, struct _input_impl *si, alph_t *dest, int length ); \
	int (*consume_data)( struct colm_program *prg, struct _input_impl *si, int length, struct colm_location *loc ); \
	int (*undo_consume_data)( struct colm_program *prg, struct _input_impl *si, const alph_t *data, int length ); \
	struct colm_tree *(*consume_tree)( struct colm_program *prg, struct _input_impl *si ); \
	void (*undo_consume_tree)( struct colm_program *prg, struct _input_impl *si, struct colm_tree *tree, int ignore ); \
	struct LangEl *(*consume_lang_el)( struct colm_program *prg, struct _input_impl *si, long *bind_id, alph_t **data, long *length ); \
	void (*undo_consume_lang_el)( struct colm_program *prg, struct _input_impl *si ); \
	void (*prepend_data)( struct colm_program *prg, struct _input_impl *si, struct colm_location *loc, const alph_t *data, long len ); \
	int (*undo_prepend_data)( struct colm_program *prg, struct _input_impl *si, int length ); \
	void (*prepend_tree)( struct colm_program *prg, struct _input_impl *si, struct colm_tree *tree, int ignore ); \
	struct colm_tree *(*undo_prepend_tree)( struct colm_program *prg, struct _input_impl *si ); \
	void (*prepend_stream)( struct colm_program *prg, struct _input_impl *si, struct colm_stream *stream ); \
	struct colm_tree *(*undo_prepend_stream)( struct colm_program *prg, struct _input_impl *si ); \
	void (*append_data)( struct colm_program *prg, struct _input_impl *si, const alph_t *data, long length ); \
	struct colm_tree *(*undo_append_data)( struct colm_program *prg, struct _input_impl *si, int length ); \
	void (*append_tree)( struct colm_program *prg, struct _input_impl *si, struct colm_tree *tree ); \
	struct colm_tree *(*undo_append_tree)( struct colm_program *prg, struct _input_impl *si ); \
	void (*append_stream)( struct colm_program *prg, struct _input_impl *si, struct colm_stream *stream ); \
	struct colm_tree *(*undo_append_stream)( struct colm_program *prg, struct _input_impl *si ); \
	void (*set_eof_mark)( struct colm_program *prg, struct _input_impl *si, char eof_mark ); \
	void (*transfer_loc)( struct colm_program *prg, struct colm_location *loc, struct _input_impl *si ); \
	void (*destructor)( struct colm_program *prg, struct colm_tree **sp, struct _input_impl *si ); \
	int (*get_option)( struct colm_program *prg, struct _input_impl *si, int option ); \
	void (*set_option)( struct colm_program *prg, struct _input_impl *si, int option, int value ); \
}

#define DEF_STREAM_FUNCS( stream_funcs, _stream_impl ) \
struct stream_funcs \
{ \
	int (*get_parse_block)( struct colm_program *prg, struct _stream_impl *si, int *pskip, alph_t **pdp, int *copied ); \
	int (*get_data)( struct colm_program *prg, struct _stream_impl *si, alph_t *dest, int length ); \
	int (*get_data_source)( struct colm_program *prg, struct _stream_impl *si, alph_t *dest, int length ); \
	int (*consume_data)( struct colm_program *prg, struct _stream_impl *si, int length, struct colm_location *loc ); \
	int (*undo_consume_data)( struct colm_program *prg, struct _stream_impl *si, const alph_t *data, int length ); \
	void (*transfer_loc)( struct colm_program *prg, struct colm_location *loc, struct _stream_impl *si ); \
	struct colm_str_collect *(*get_collect)( struct colm_program *prg, struct _stream_impl *si ); \
	void (*flush_stream)( struct colm_program *prg, struct _stream_impl *si ); \
	void (*close_stream)( struct colm_program *prg, struct _stream_impl *si ); \
	void (*print_tree)( struct colm_program *prg, struct colm_tree **sp, \
			struct _stream_impl *impl, struct colm_tree *tree, int trim ); \
	struct stream_impl *(*split_consumed)( struct colm_program *prg, struct _stream_impl *si ); \
	int (*append_data)( struct colm_program *prg, struct _stream_impl *si, const alph_t *data, int len ); \
	int (*undo_append_data)( struct colm_program *prg, struct _stream_impl *si, int length ); \
	void (*destructor)( struct colm_program *prg, struct colm_tree **sp, struct _stream_impl *si ); \
	int (*get_option)( struct colm_program *prg, struct _stream_impl *si, int option ); \
	void (*set_option)( struct colm_program *prg, struct _stream_impl *si, int option, int value ); \
}

DEF_INPUT_FUNCS( input_funcs, input_impl );
DEF_STREAM_FUNCS( stream_funcs, stream_impl );

/* List of source streams. Enables streams to be pushed/popped. */
struct input_impl
{
	struct input_funcs *funcs;
};

/* List of source streams. Enables streams to be pushed/popped. */
struct stream_impl
{
	struct stream_funcs *funcs;
};

enum seq_buf_type {
	SB_TOKEN = 1,
	SB_IGNORE,
	SB_SOURCE,
	SB_ACCUM
};

struct seq_buf
{
	enum seq_buf_type type;
	char own_si;
	struct colm_tree *tree;
	struct stream_impl *si;
	struct seq_buf *next, *prev;
};

/* List of source streams. Enables streams to be pushed/popped. */
struct input_impl_seq
{
	struct input_funcs *funcs;
	char type;

	char eof_mark;
	char eof_sent;

	struct {
		struct seq_buf *head;
		struct seq_buf *tail;
	} queue;

	struct seq_buf *stash;

	int consumed;
	int auto_trim;
};

struct run_buf
{
	long length;
	long offset;
	struct run_buf *next, *prev;

	/* Must be at the end. We will grow this struct to add data if the input
	 * demands it. */
	alph_t data[FSM_BUFSIZE];
};

struct run_buf *new_run_buf( int sz );

struct stream_impl_data
{
	struct stream_funcs *funcs;
	char type;

	struct {
		struct run_buf *head;
		struct run_buf *tail;
	} queue;

	const alph_t *data;
	long dlen;
	int offset;

	long line;
	long column;
	long byte;

	char *name;
	FILE *file;

	struct colm_str_collect *collect;

	int consumed;

	struct indent_impl indent;

	int *line_len;
	int lines_alloc;
	int lines_cur;

	int auto_trim;
};

void stream_impl_push_line( struct stream_impl_data *ss, int ll );
int stream_impl_pop_line( struct stream_impl_data *ss );

struct input_impl *colm_impl_new_generic( char *name );

void update_position( struct stream_impl *input_stream, const char *data, long length );
void undo_position( struct stream_impl *input_stream, const char *data, long length );

struct stream_impl *colm_stream_impl( struct colm_struct *s );

struct colm_str *collect_string( struct colm_program *prg, struct colm_stream *s );
struct colm_stream *colm_stream_open_collect( struct colm_program *prg );

char *colm_filename_add( struct colm_program *prg, const char *fn );
struct stream_impl *colm_impl_new_accum( char *name );
struct stream_impl *colm_impl_consumed( char *name, int len );
struct stream_impl *colm_impl_new_text( char *name, struct colm_location *loc, const alph_t *data, int len );

#ifdef __cplusplus
}
#endif

#endif /* _COLM_INPUT_H */

