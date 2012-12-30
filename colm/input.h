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
#define INPUT_LANG_EL  4
#define INPUT_TREE     5
#define INPUT_IGNORE   6

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
struct ColmTree;

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
	struct ColmTree *tree;
	long offset;
	struct _RunBuf *next, *prev;
} RunBuf;

RunBuf *newRunBuf();

typedef struct _SourceStream SourceStream;

struct SourceFuncs
{
	/* Data. */
	int (*getData)( SourceStream *is, int offset, char *dest, int length, int *copied );
	int (*consumeData)( SourceStream *is, int length );
	int (*undoConsumeData)( SourceStream *is, const char *data, int length );

	/* Language elments (compile-time). */
	struct LangEl *(*consumeLangEl)( SourceStream *is, long *bindId, char **data, long *length );
	void (*undoConsumeLangEl)( SourceStream *is );

	/* Private implmentation for some shared get data functions. */
	int (*getDataImpl)( SourceStream *is, char *dest, int length );
};

/* Implements a single source of input data such as a file, string, pattern.
 * These can be placed in an input stream to parse from multiple sources. */
struct _SourceStream
{
	struct SourceFuncs *funcs;

	char eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	int handlesLine;

	RunBuf *queue;
	RunBuf *queueTail;

	const char *data;
	long dlen;
	int offset;

	FILE *file;
	long fd;

	struct Pattern *pattern;
	struct PatternItem *patItem;
	struct Constructor *constructor;
	struct ConsItem *consItem;

	struct _FsmRun *attached;
};

SourceStream *newSourceStreamPat( struct Pattern *pattern );
SourceStream *newSourceStreamCons( struct Constructor *constructor );
SourceStream *newSourceStreamFile( FILE *file );
SourceStream *newSourceStreamFd( long fd );

void initInputFuncs();
void initStaticFuncs();
void initPatFuncs();
void initConsFuncs();

/* List of source streams. Enables streams to be pushed/popped. */
struct _InputStream
{
	char eofSent;
	char eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	int handlesLine;

	RunBuf *queue;
	RunBuf *queueTail;

	FILE *file;
	long fd;

	struct _FsmRun *attached;
};

typedef struct _InputStream InputStream;

/* The input stream interface. */

int getData( struct _FsmRun *fsmRun, InputStream *in, int offset, char *dest, int length, int *copied );
int consumeData( InputStream *in, int length );
int undoConsumeData( struct _FsmRun *fsmRun, InputStream *is, const char *data, int length );

struct ColmTree *consumeTree( InputStream *in );
void undoConsumeTree( InputStream *in, struct ColmTree *tree, int ignore );

struct LangEl *consumeLangEl( InputStream *in, long *bindId, char **data, long *length );
void undoConsumeLangEl( InputStream *in );

void setEof( InputStream *is );
void unsetEof( InputStream *is );

void prependData( InputStream *in, const char *data, long len );
int undoPrependData( InputStream *is, int length );

void prependTree( InputStream *is, struct ColmTree *tree, int ignore );
struct ColmTree *undoPrependTree( InputStream *is );

void appendData( InputStream *in, const char *data, long len );
void appendTree( InputStream *in, struct ColmTree *tree );
void appendStream( InputStream *in, struct ColmTree *tree );
struct ColmTree *undoAppendData( InputStream *in, int length );
struct ColmTree *undoAppendStream( InputStream *in );
struct ColmTree *undoAppendTree( InputStream *in );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
