/*
 *  Copyright 2007-2011 Adrian Thurston <thurston@complang.org>
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
struct Replacement;
struct ReplItem;
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
	int (*getData)( SourceStream *is, int offset, char *dest, int length, int *copied );
	int (*consumeData)( SourceStream *is, int length );
	int (*undoConsumeData)( SourceStream *is, const char *data, int length );

	int (*isTree)( SourceStream *is );
	int (*isIgnore)( SourceStream *is );
	int (*isLangEl)( SourceStream *is );
	int (*isEof)( SourceStream *is, int offset );
	int (*getDataImpl)( SourceStream *is, char *dest, int length );
	struct ColmTree *(*getTree)( SourceStream *is );
	struct LangEl *(*getLangEl)( SourceStream *is, long *bindId, char **data, long *length );
	void (*pushTree)( SourceStream *is, struct ColmTree *tree, int ignore );
	void (*pushText)( SourceStream *is, const char *data, long len );
	struct ColmTree *(*undoPush)( SourceStream *is, int length );
	struct ColmTree *(*undoAppend)( SourceStream *is, int length );
	void (*pushBackNamed)( SourceStream *is );
	void (*pushBackBuf)( SourceStream *is, RunBuf *runBuf );
};

extern struct SourceFuncs baseFuncs;
extern struct SourceFuncs stringFuncs;
extern struct SourceFuncs fileFuncs;
extern struct SourceFuncs fdFuncs;
extern struct SourceFuncs accumFuncs;
extern struct SourceFuncs staticFuncs;
extern struct SourceFuncs patternFuncs;
extern struct SourceFuncs replFuncs;

struct _SourceStream
{
	struct SourceFuncs *funcs;

	struct _FsmRun *hasData;

	char eofSent;
	char flush;
	char eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	int handlesLine;
	int later;

	RunBuf *queue;
	RunBuf *queueTail;

	const char *data;
	long dlen;
	int offset;

	FILE *file;
	long fd;

	struct Pattern *pattern;
	struct PatternItem *patItem;
	struct Replacement *replacement;
	struct ReplItem *replItem;
};

SourceStream *newInputStreamPattern( struct Pattern *pattern );
SourceStream *newInputStreamRepl( struct Replacement *replacement );
SourceStream *newInputStreamFile( FILE *file );
SourceStream *newInputStreamFd( long fd );

void initInputFuncs();
void initStaticFuncs();
void initPatternFuncs();
void initReplFuncs();

/* List of input streams. Enables streams to be pushed/popped. */
struct _InputStream
{
	char eofSent;
	char flush;
	char eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	int handlesLine;
	int later;

	RunBuf *queue;
	RunBuf *queueTail;

	const char *data;
	long dlen;
	int offset;

	FILE *file;
	long fd;

	struct Pattern *pattern;
	struct PatternItem *patItem;
	struct Replacement *replacement;
	struct ReplItem *replItem;
};

typedef struct _InputStream InputStream;

int getData( InputStream *in, int offset, char *dest, int length, int *copied );
int consumeData( InputStream *in, int length );
int undoConsumeData( InputStream *is, const char *data, int length );

int isTree( InputStream *in );
int isIgnore( InputStream *in );
int isLangEl( InputStream *in );
int isEof( InputStream *in, int offset );
int tryAgainLater( InputStream *in, int offset );
void setEof( InputStream *is );
void unsetEof( InputStream *is );
void unsetLater( InputStream *is );
int getDataImpl( InputStream *in, char *dest, int length );
struct ColmTree *getTree( InputStream *in );
struct LangEl *getLangEl( InputStream *in, long *bindId, char **data, long *length );
void pushTree( InputStream *in, struct ColmTree *tree, int ignore );
void pushText( InputStream *in, const char *data, long len );
struct ColmTree *undoPush( InputStream *in, int length );
void appendData( InputStream *in, const char *data, long len );
void appendTree( InputStream *in, struct ColmTree *tree );
void appendStream( InputStream *in, struct ColmTree *tree );
struct ColmTree *undoAppend( InputStream *in, int length );
struct ColmTree *undoAppendStream( InputStream *in );
void pushBackNamed( InputStream *in );
void pushBackBuf( InputStream *in, RunBuf *runBuf );

RunBuf *inputStreamPopHead2( InputStream *is );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
