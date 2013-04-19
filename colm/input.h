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

/*
 * pdaRun <- fsmRun <- stream 
 *
 * Activities we need to support:
 *
 * 1. Stuff data into an input stream each time we <<
 * 2. Detach an input stream, and attach another when we include
 * 3. Send data back to an input stream when the parser backtracks
 * 4. Temporarily stop parsing due to a lack of input.
 *
 * At any given time, the fsmRun struct may have a prefix of the stream's
 * input. If getting data we first get what we can out of the fsmRun, then
 * consult the stream. If sending data back, we first shift pointers in the
 * fsmRun, then ship to the stream. If changing streams the old stream needs to
 * take back unprocessed data from the fsmRun.
 */

struct LangEl;
struct Pattern;
struct PatternItem;
struct Constructor;
struct ConsItem;
struct _FsmRun;
struct colm_tree;
struct colm_location;
struct colm_program;

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

typedef struct _StreamImpl StreamImpl;

struct StreamFuncs
{
	int (*getParseBlock)( StreamImpl *ss, int skip, char **pdp, int *copied );

	int (*getData)( StreamImpl *ss, char *dest, int length );

	int (*consumeData)( StreamImpl *ss, int length, struct colm_location *loc );
	int (*undoConsumeData)( StreamImpl *ss, const char *data, int length );

	struct colm_tree *(*consumeTree)( StreamImpl *ss );
	void (*undoConsumeTree)( StreamImpl *ss, struct colm_tree *tree, int ignore );

	/* Language elments (compile-time). */
	struct LangEl *(*consumeLangEl)( StreamImpl *ss, long *bindId, char **data, long *length );
	void (*undoConsumeLangEl)( StreamImpl *ss );

	/* Private implmentation for some shared get data functions. */
	int (*getDataSource)( StreamImpl *ss, char *dest, int length );

	void (*setEof)( StreamImpl *is );
	void (*unsetEof)( StreamImpl *is );

	/* Prepending to a stream. */
	void (*prependData)( StreamImpl *in, const char *data, long len );
	void (*prependTree)( StreamImpl *is, struct colm_tree *tree, int ignore );
	void (*prependStream)( StreamImpl *in, struct colm_tree *tree );
	int (*undoPrependData)( StreamImpl *is, int length );
	struct colm_tree *(*undoPrependTree)( StreamImpl *is );
	struct colm_tree *(*undoPrependStream)( StreamImpl *in );

	/* Appending to a stream. */
	void (*appendData)( StreamImpl *in, const char *data, long len );
	void (*appendTree)( StreamImpl *in, struct colm_tree *tree );
	void (*appendStream)( StreamImpl *in, struct colm_tree *tree );
	struct colm_tree *(*undoAppendData)( StreamImpl *in, int length );
	struct colm_tree *(*undoAppendTree)( StreamImpl *in );
	struct colm_tree *(*undoAppendStream)( StreamImpl *in );
};

/* List of source streams. Enables streams to be pushed/popped. */
struct _StreamImpl
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

StreamImpl *newSourceStreamPat( const char *name, struct Pattern *pattern );
StreamImpl *newSourceStreamCons( const char *name, struct Constructor *constructor );
StreamImpl *newSourceStreamFile( const char *name, FILE *file );
StreamImpl *newSourceStreamFd( const char *name, long fd );
StreamImpl *newSourceStreamGeneric( const char *name );

void updatePosition( StreamImpl *inputStream, const char *data, long length );
void undoPosition( StreamImpl *inputStream, const char *data, long length );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
