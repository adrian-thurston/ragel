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

struct KlangEl;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;
struct RunBuf;

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

struct InputStream
{
	InputStream( bool handlesLine ) :
		line(1),
		position(0),
		handlesLine(handlesLine) {}

	virtual ~InputStream() {}

	/* Basic functions. */
	virtual int getData( char *dest, int length ) = 0;
	virtual int isEOF() = 0;
	virtual int needFlush() = 0;
	virtual void pushBackBuf( RunBuf *runBuf ) = 0;

	/* Named language elements for patterns and replacements. */
	virtual int isLangEl() { return false; }
	virtual KlangEl *getLangEl( long &bindId, char *&data, long &length )
		{ assert( false ); return 0; }
	virtual void pushBackNamed()
		{ assert( false ); }
	
	RunBuf *runBuf;
	char *data, *de, *deof;
	char *token;
	bool eofSent;

	long line;
	long position;

	/* This is set true for input streams that do their own line counting.
	 * Causes FsmRun to ignore NLs. */
	bool handlesLine;
};

struct InputStreamString : public InputStream
{
	InputStreamString( const char *data, long dlen ) :
		InputStream(false), 
		data(data), dlen(dlen), offset(0), eof(false) {}

	int getData( char *dest, int length );
	int isEOF() { return eof; }
	int needFlush() { return eof; }
	void pushBackBuf( RunBuf *runBuf );

	const char *data;
	long dlen;
	int offset;
	bool eof;
};

struct InputStreamFile : public InputStream
{
	InputStreamFile( FILE *file ) :
		InputStream(false), 
		file(file), queue(0) {}

	int getData( char *dest, int length );
	int isEOF();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );

	FILE *file;
	RunBuf *queue;
};

struct InputStreamFD : public InputStream
{
	InputStreamFD( long fd ) :
		InputStream(false), 
		fd(fd), eof(false), queue(0) {}

	int isEOF();
	int needFlush();
	int getData( char *dest, int length );
	void pushBackBuf( RunBuf *runBuf );

	long fd;
	bool eof;
	RunBuf *queue;
};

struct InputStreamPattern : public InputStream
{
	InputStreamPattern( Pattern *pattern );

	int getData( char *dest, int length );
	int isEOF();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );

	int isLangEl();
	KlangEl *getLangEl( long &bindId, char *&data, long &length );

	void pushBackNamed();

	void backup();
	int shouldFlush();

	Pattern *pattern;
	PatternItem *patItem;
	int offset;
	bool flush;
};

struct InputStreamRepl : public InputStream
{
	InputStreamRepl( Replacement *replacement );

	int isLangEl();
	int getData( char *dest, int length );
	KlangEl *getLangEl( long &bindId, char *&data, long &length );
	int isEOF();
	int needFlush();
	void pushBackBuf( RunBuf *runBuf );

	void pushBackNamed();

	void backup();
	int shouldFlush();

	Replacement *replacement;
	ReplItem *replItem;
	int offset;
	bool flush;
};

#endif /* _INPUT_H */

