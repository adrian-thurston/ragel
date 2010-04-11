/*
 *  Copyright 2007-2010 Adrian Thurston <thurston@complang.org>
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

#include <assert.h>
#include <stdio.h>
#include <iostream>

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

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

struct KlangEl;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;
struct RunBuf;
struct FsmRun;
typedef struct _Tree Tree;

struct RunBuf
{
	enum Type {
		DataType = 0,
		TokenType,
		IgnoreType
	};

	RunBuf()
	:
		type(DataType),
		length(0),
		tree(0),
		offset(0),
		next(0),
		prev(0)
	{}

	Type type;
	char data[FSM_BUFSIZE];
	long length;
	Tree *tree;
	long offset;
	RunBuf *next, *prev;
};

struct InputStream;

struct InputFuncs
{
	int (*getData)( InputStream *is, char *dest, int length );
};

struct InputStream
{
	InputStream( bool handlesLine )
	:
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(handlesLine),
		later(false),
		queue(0), 
		queueTail(0)
	{
		funcs = &baseFuncs;
	}

	InputFuncs *funcs;

	static InputFuncs baseFuncs;

	virtual ~InputStream() {}

	virtual int getData( char *dest, int length ) = 0;
	virtual int isEof() = 0;
	virtual int needFlush() = 0;
	virtual void pushBackBuf( RunBuf *runBuf ) = 0;
	virtual void append( const char *data, long len ) = 0;
	virtual void append( Tree *tree ) = 0;
	virtual bool tryAgainLater() = 0;
	virtual bool isTree() = 0;
	virtual Tree *getTree() = 0;
	virtual bool isIgnore() = 0;
	virtual bool isLangEl() = 0;
	virtual KlangEl *getLangEl( long &bindId, char *&data, long &length ) = 0;
	virtual void pushBackNamed() = 0;
	virtual Tree *undoPush( int length ) = 0;
	virtual Tree *undoAppend( int length ) = 0;

	/* Basic functions. */
	
	FsmRun *hasData;

	bool eofSent;
	bool flush;
	bool eof;

	long line;
	long column;
	long byte;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	bool handlesLine;
	bool later;

	RunBuf *queue;
	RunBuf *queueTail;

	RunBuf *head() { return queue; }
	RunBuf *tail() { return queueTail; }

	RunBuf *popHead()
	{
		RunBuf *ret = queue;
		queue = queue->next;
		if ( queue == 0 )
			queueTail = 0;
		else
			queue->prev = 0;
		return ret;
	}

	RunBuf *popTail()
	{
		RunBuf *ret = queueTail;
		queueTail = queue->prev;
		if ( queueTail == 0 )
			queue = 0;
		else
			queueTail->next = 0;
		return ret;
	}

	void append( RunBuf *runBuf )
	{
		if ( queue == 0 ) {
			runBuf->prev = runBuf->next = 0;
			queue = queueTail = runBuf;
		}
		else {
			queueTail->next = runBuf;
			runBuf->prev = queueTail;
			runBuf->next = 0;
			queueTail = runBuf;
		}
	}

	void prepend( RunBuf *runBuf )
	{
		if ( queue == 0 ) {
			runBuf->prev = runBuf->next = 0;
			queue = queueTail = runBuf;
		}
		else {
			queue->prev = runBuf;
			runBuf->prev = 0;
			runBuf->next = queue;
			queue = runBuf;
		}
	}
};

struct InputStreamDynamic : public InputStream
{
	InputStreamDynamic( bool handlesLine )
	:
		InputStream( handlesLine )
	{
		funcs = &dynamicFuncs;
	}

	static InputFuncs dynamicFuncs;

	int getData( char *dest, int length );
	int isEof();
	void append( const char *data, long len ) {}
	void append( Tree *tree ) {}
	bool tryAgainLater();
	bool isTree();
	Tree *getTree();
	bool isIgnore();
	bool isLangEl() { return false; }
	KlangEl *getLangEl( long &bindId, char *&data, long &length ) { assert(false); return 0; }
	void pushBackNamed() { assert(false); }
	Tree *undoPush( int length );
	int getDataRev( char *dest, int length );
	Tree *undoAppend( int length );

	virtual int getDataImpl( char *dest, int length ) = 0;
};

struct InputStreamString : public InputStreamDynamic
{
	InputStreamString( const char *data, long dlen ) :
		InputStreamDynamic(false), 
		data(data), dlen(dlen), offset(0) {}

	int needFlush() { return eof; }
	void pushBackBuf( RunBuf *runBuf );

	int getDataImpl( char *dest, int length );

	const char *data;
	long dlen;
	int offset;
};

struct InputStreamFile : public InputStreamDynamic
{
	InputStreamFile( FILE *file ) :
		InputStreamDynamic(false), 
		file(file)
	{}

	int needFlush();
	void pushBackBuf( RunBuf *runBuf );

	int getDataImpl( char *dest, int length );

	FILE *file;
};

struct InputStreamFd : public InputStreamDynamic
{
	InputStreamFd( long fd ) :
		InputStreamDynamic(false), 
		fd(fd)
	{}

	int needFlush();
	void pushBackBuf( RunBuf *runBuf );

	int getDataImpl( char *dest, int length );

	long fd;
};

struct InputStreamAccum : public InputStreamDynamic
{
	InputStreamAccum()
	:
		InputStreamDynamic(false), 
		offset(0)
	{}

	long offset;

	bool tryAgainLater();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );
	void append( const char *data, long len );
	void append( Tree *tree );

	int getDataImpl( char *dest, int length );
};

/*
 * The compile-time input streams.
 */

struct InputStreamStatic : public InputStream
{
	InputStreamStatic( bool handlesLine )
		: InputStream( handlesLine ) {}

	void append( const char *data, long len ) { assert(false); }
	void append( Tree *tree ) { assert(false); }

	bool tryAgainLater();
	bool isTree() { return false; }
	bool isIgnore() { return false; }
	Tree *getTree() { assert(false); }
	Tree *undoPush( int ) { assert( false ); }
	Tree *undoAppend( int ) { assert( false ); }
};

struct InputStreamPattern : public InputStreamStatic
{
	InputStreamPattern( Pattern *pattern );

	static InputFuncs patternFuncs;

	int getData( char *dest, int length );
	int isEof();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );
	bool isLangEl();
	KlangEl *getLangEl( long &bindId, char *&data, long &length );
	void pushBackNamed();
	int shouldFlush();
	void backup();

	Pattern *pattern;
	PatternItem *patItem;
	int offset;
};

struct InputStreamRepl : public InputStreamStatic
{
	InputStreamRepl( Replacement *replacement );

	static InputFuncs replFuncs;

	bool isLangEl();
	int getData( char *dest, int length );
	KlangEl *getLangEl( long &bindId, char *&data, long &length );
	int isEof();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );
	void pushBackNamed();
	void backup();
	int shouldFlush();

	Replacement *replacement;
	ReplItem *replItem;
	int offset;
};

#endif /* _INPUT_H */

