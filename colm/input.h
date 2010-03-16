/*
 *  Copyright 2007, 2008 Adrian Thurston <thurston@complang.org>
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
struct Tree;

struct RunBuf
{
	enum Type {
		DataType,
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

struct InputStream
{
	InputStream( bool handlesLine ) :
		hasData(0),
		eofSent(false),
		flush(false),
		eof(false),
		line(1),
		column(1),
		byte(0),
		handlesLine(handlesLine),
		later(false),
		queue(0), queueTail(0)
	{}

	virtual ~InputStream() {}

	int getData( char *dest, int length );
	int isEof();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );
	void append( const char *data, long len );
	void append( Tree *tree );
	bool tryAgainLater();

	bool isTree();
	Tree *getTree();
	bool isIgnore();
	bool isLangEl();
	KlangEl *getLangEl( long &bindId, char *&data, long &length );
	void pushBackNamed();

	/* Basic functions. */
	virtual int getDataImpl( char *dest, int length ) = 0;
	virtual int isEofImpl() = 0;
	virtual int needFlushImpl() = 0;
	virtual void pushBackBufImpl( RunBuf *runBuf ) = 0;
	virtual void appendImpl( const char *data, long len ) = 0;
	virtual void appendImpl( Tree *tree ) = 0;
	virtual bool tryAgainLaterImpl();

	/* Named language elements for patterns and replacements. */
	virtual bool isTreeImpl();
	virtual Tree *getTreeImpl();
	virtual bool isIgnoreImpl();
	virtual bool isLangElImpl() { return false; }
	virtual KlangEl *getLangElImpl( long &bindId, char *&data, long &length ) { assert( false ); return 0; }
	virtual void pushBackNamedImpl() { assert( false ); }
	
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

	void reverseQueue()
	{
		RunBuf *last = 0, *cur = queue;
		queueTail = queue;

		while ( cur != 0 ) {
			/* Save for moving ahead. */
			RunBuf *next = cur->next;

			/* Reverse. */
			cur->next = last;
			cur->prev = next;
			
			/* Move ahead. */
			last = cur;
			cur = next;
		}

		queue = last;
	}
};

struct InputStreamString : public InputStream
{
	InputStreamString( const char *data, long dlen ) :
		InputStream(false), 
		data(data), dlen(dlen), offset(0) {}

	int getDataImpl( char *dest, int length );
	int isEofImpl() { return eof; }
	int needFlushImpl() { return eof; }
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len ) {}
	void appendImpl( Tree *tree ) {}

	const char *data;
	long dlen;
	int offset;
};

struct InputStreamFile : public InputStream
{
	InputStreamFile( FILE *file ) :
		InputStream(false), 
		file(file)
	{}

	int getDataImpl( char *dest, int length );
	int isEofImpl();
	int needFlushImpl();
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len ) {}
	void appendImpl( Tree *tree ) {}

	FILE *file;
};

struct InputStreamFd : public InputStream
{
	InputStreamFd( long fd ) :
		InputStream(false), 
		fd(fd)
	{}

	int isEofImpl();
	int needFlushImpl();
	int getDataImpl( char *dest, int length );
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len ) {}
	void appendImpl( Tree *tree ) {}

	long fd;
};

struct InputStreamAccum : public InputStream
{
	InputStreamAccum()
	:
		InputStream(false), 
		offset(0)
	{}

	int isEofImpl();
	int needFlushImpl();
	int getDataImpl( char *dest, int length );
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len );
	void appendImpl( Tree *tree );
	bool isTreeImpl();

	bool tryAgainLaterImpl();

	long offset;
};


struct InputStreamPattern : public InputStream
{
	InputStreamPattern( Pattern *pattern );

	int getDataImpl( char *dest, int length );
	int isEofImpl();
	int needFlushImpl();
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len ) {}
	virtual void appendImpl( Tree *tree ) {}

	bool isLangElImpl();
	KlangEl *getLangElImpl( long &bindId, char *&data, long &length );

	void pushBackNamedImpl();

	void backup();
	int shouldFlush();

	Pattern *pattern;
	PatternItem *patItem;
	int offset;
};

struct InputStreamRepl : public InputStream
{
	InputStreamRepl( Replacement *replacement );

	bool isLangElImpl();
	int getDataImpl( char *dest, int length );
	KlangEl *getLangElImpl( long &bindId, char *&data, long &length );
	int isEofImpl();
	int needFlushImpl();
	void pushBackBufImpl( RunBuf *runBuf );
	void appendImpl( const char *data, long len ) {}
	virtual void appendImpl( Tree *tree ) {}

	void pushBackNamedImpl();

	void backup();
	int shouldFlush();

	Replacement *replacement;
	ReplItem *replItem;
	int offset;
};

#endif /* _INPUT_H */

