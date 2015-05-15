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

struct stream_impl;

enum RunBufType {
	RunBufDataType = 0,
	RunBufTokenType,
	RunBufIgnoreType,
	RunBufSourceType
};

typedef struct _RunBuf
{
	enum RunBufType type;
	char data[FSM_BUFSIZE];
	long length;
	struct colm_tree *tree;
	long offset;
	struct _RunBuf *next, *prev;
} RunBuf;

RunBuf *newRunBuf();

struct StreamFuncs
{
	int (*getParseBlock)( struct stream_impl *ss, int skip, char **pdp, int *copied );

	int (*getData)( struct stream_impl *ss, char *dest, int length );

	int (*consumeData)( struct colm_program *prg, struct colm_tree **sp,
			struct stream_impl *ss, int length, struct colm_location *loc );
	int (*undoConsumeData)( struct stream_impl *ss, const char *data, int length );

	struct colm_tree *(*consumeTree)( struct stream_impl *ss );
	void (*undoConsumeTree)( struct stream_impl *ss,
			struct colm_tree *tree, int ignore );

	/* Language elments (compile-time). */
	struct LangEl *(*consumeLangEl)( struct stream_impl *ss,
			long *bindId, char **data, long *length );
	void (*undoConsumeLangEl)( struct stream_impl *ss );

	/* Private implmentation for some shared get data functions. */
	int (*getDataSource)( struct stream_impl *ss, char *dest, int length );

	void (*setEof)( struct stream_impl *is );
	void (*unsetEof)( struct stream_impl *is );

	/* Prepending to a stream. */
	void (*prependData)( struct stream_impl *in, const char *data, long len );
	void (*prependTree)( struct stream_impl *is, struct colm_tree *tree, int ignore );
	void (*prependStream)( struct stream_impl *in, struct colm_tree *tree );
	int (*undoPrependData)( struct stream_impl *is, int length );
	struct colm_tree *(*undoPrependTree)( struct stream_impl *is );
	struct colm_tree *(*undoPrependStream)( struct stream_impl *in );

	/* Appending to a stream. */
	void (*appendData)( struct stream_impl *in, const char *data, long len );
	void (*appendTree)( struct stream_impl *in, struct colm_tree *tree );
	void (*appendStream)( struct stream_impl *in, struct colm_tree *tree );
	struct colm_tree *(*undoAppendData)( struct stream_impl *in, int length );
	struct colm_tree *(*undoAppendTree)( struct stream_impl *in );
	struct colm_tree *(*undoAppendStream)( struct stream_impl *in );
};

/* List of source streams. Enables streams to be pushed/popped. */
struct stream_impl
{
	struct StreamFuncs *funcs;

	char eofSent;
	char eof;
	char eosSent;

	RunBuf *queue;
	RunBuf *queueTail;

	const char *data;
	long dlen;
	int offset;

	long line;
	long column;
	long byte;

	const char *name;
	FILE *file;
	long fd;

	struct Pattern *pattern;
	struct PatternItem *patItem;
	struct Constructor *constructor;
	struct ConsItem *consItem;

	int consumed;
};

struct stream_impl *colm_impl_new_pat( const char *name, struct Pattern *pattern );
struct stream_impl *colm_impl_new_cons( const char *name, struct Constructor *constructor );
struct stream_impl *colm_impl_new_file( const char *name, FILE *file );
struct stream_impl *colm_impl_new_fd( const char *name, long fd );
struct stream_impl *colm_impl_new_generic( const char *name );

void updatePosition( struct stream_impl *inputStream, const char *data, long length );
void undoPosition( struct stream_impl *inputStream, const char *data, long length );

struct stream_impl *colm_stream_impl( struct colm_struct *s );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
