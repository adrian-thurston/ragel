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
	RunBufIgnoreType
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

typedef struct _InputStream InputStream;

struct InputFuncs
{
	int (*isTree)( InputStream *is );
	int (*isIgnore)( InputStream *is );
	int (*isLangEl)( InputStream *is );
	int (*isEof)( InputStream *is );
	int (*needFlush)( InputStream *is );
	int (*tryAgainLater)( InputStream *is );
	int (*getData)( InputStream *is, char *dest, int length );
	int (*getDataImpl)( InputStream *is, char *dest, int length );
	struct ColmTree *(*getTree)( InputStream *is );
	struct LangEl *(*getLangEl)( InputStream *is, long *bindId, char **data, long *length );
	void (*pushTree)( InputStream *is, struct ColmTree *tree, int ignore );
	void (*pushText)( InputStream *is, const char *data, long len );
	struct ColmTree *(*undoPush)( InputStream *is, int length );
	void (*appendData)( InputStream *is, const char *data, long len );
	void (*appendTree)( InputStream *is, struct ColmTree *tree );
	struct ColmTree *(*undoAppend)( InputStream *is, int length );
	void (*pushBackNamed)( InputStream *is );
	void (*pushBackBuf)( InputStream *is, RunBuf *runBuf );
};

extern struct InputFuncs baseFuncs;
extern struct InputFuncs stringFuncs;
extern struct InputFuncs fileFuncs;
extern struct InputFuncs fdFuncs;
extern struct InputFuncs accumFuncs;
extern struct InputFuncs staticFuncs;
extern struct InputFuncs patternFuncs;
extern struct InputFuncs replFuncs;

struct _InputStream
{
	struct InputFuncs *funcs;

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

RunBuf *inputStreamHead( InputStream *is );
RunBuf *inputStreamTail( InputStream *is );
RunBuf *inputStreamPopHead( InputStream *is );
RunBuf *inputStreamPopTail( InputStream *is );
void inputStreamAppend( InputStream *is, RunBuf *runBuf );
void inputStreamPrepend( InputStream *is, RunBuf *runBuf );

InputStream *newInputStreamPattern( struct Pattern *pattern );
InputStream *newInputStreamRepl( struct Replacement *replacement );
InputStream *newInputStreamFile( FILE *file );
InputStream *newInputStreamFd( long fd );
InputStream *newInputStreamAccum();

void initInputFuncs();
void initStaticFuncs();
void initPatternFuncs();
void initReplFuncs();

/* List of input streams. Enables streams to be pushed/popped. */
typedef struct _Input
{
	InputStream *is;

	struct InputFuncs *funcs;

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
} Input;

int isTree( Input *in );
int isIgnore( Input *in );
int isLangEl( Input *in );
int isEof( Input *in );
int needFlush( Input *in );
int tryAgainLater( Input *in );
int getData( Input *in, char *dest, int length );
int getDataImpl( Input *in, char *dest, int length );
struct ColmTree *getTree( Input *in );
struct LangEl *getLangEl( Input *in, long *bindId, char **data, long *length );
void pushTree( Input *in, struct ColmTree *tree, int ignore );
void pushText( Input *in, const char *data, long len );
struct ColmTree *undoPush( Input *in, int length );
void appendData( Input *in, const char *data, long len );
void appendTree( Input *in, struct ColmTree *tree );
struct ColmTree *undoAppend( Input *in, int length );
void pushBackNamed( Input *in );
void pushBackBuf( Input *in, RunBuf *runBuf );

#ifdef __cplusplus
}
#endif

#endif /* _INPUT_H */
