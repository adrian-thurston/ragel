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

#include "astring.h"

struct KlangEl;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;
struct RunBuf;

struct InputStream
{
	virtual ~InputStream() {}

	/* Basic functions. */
	virtual int getData( char *dest, int length ) = 0;
	virtual int isEOF() = 0;
	virtual int needFlush() = 0;

	virtual void pushBack( char *data, long len ) 
		{ assert(false); }
	virtual void pushBack( RunBuf *runBuf )
		{ assert(false); }

	/* Named language elements for patterns and replacements. */
	virtual int isLangEl() { return false; }
	virtual KlangEl *getLangEl( long &bindId, char *&data, long &length )
		{ assert( false ); return 0; }
	virtual void pushBackNamed()
		{ assert( false ); }
};

struct InputStreamString : public InputStream
{
	InputStreamString( const String &data )
		: data(data), offset(0), eof(false) {}

	int getData( char *dest, int length );
	int isEOF() { return eof; }
	int needFlush() { return eof; }
	void pushBack( char *data, long len );

	String data;
	int offset;
	bool eof;
};

struct InputStreamFile : public InputStream
{
	InputStreamFile( FILE *file )
		: file(file), queue(0) {}

	int getData( char *dest, int length );
	int isEOF();
	int needFlush();

	void pushBack( RunBuf *runBuf );

	FILE *file;
	RunBuf *queue;
};

struct InputStreamFD : public InputStream
{
	InputStreamFD( long fd )
		: fd(fd), eof(false), queue(0) {}

	int isEOF();
	int needFlush();
	int getData( char *dest, int length );

	void pushBack( RunBuf *runBuf );

	long fd;
	bool eof;
	RunBuf *queue;
};

struct InputStreamPattern : public InputStream
{
	InputStreamPattern( Pattern *pattern );

	int isLangEl();
	int getData( char *dest, int length );
	KlangEl *getLangEl( long &bindId, char *&data, long &length );
	int isEOF();
	int needFlush();
	void pushBack( char *data, long len );
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
	void pushBack( char *data, long len );
	void pushBackNamed();

	void backup();
	int shouldFlush();

	Replacement *replacement;
	ReplItem *replItem;
	int offset;
	bool flush;
};

#endif /* _INPUT_H */

