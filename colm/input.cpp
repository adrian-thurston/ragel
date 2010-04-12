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

#include "input.h"
#include "fsmrun.h"
#include <stdio.h>
#include <iostream>

using std::cerr;
using std::endl;

void initDynamicFuncs();
void initStringFuncs();
void initFileFuncs();
void initFdFuncs();
void initAccumFuncs();

void initStaticFuncs();
void initPatternFuncs();
void initReplFuncs();

InputFuncs baseFuncs;
InputFuncs accumFuncs;
InputFuncs dynamicFuncs;
InputFuncs stringFuncs;
InputFuncs fileFuncs;
InputFuncs fdFuncs;

void initInputFuncs()
{
	memset( &baseFuncs, 0, sizeof(InputFuncs) );

	initDynamicFuncs();
	initStringFuncs();
	initFileFuncs();
	initFdFuncs();
	initAccumFuncs();
	initStaticFuncs();
	initPatternFuncs();
	initReplFuncs();
}

/* 
 * Base run-time input streams.
 */

bool inputStreamDynamicIsTree( InputStream *_is )
{
	InputStreamDynamic *is = (InputStreamDynamic*)_is;
	if ( is->head() != 0 && is->head()->type == RunBuf::TokenType )
		return true;
	return false;
}

bool inputStreamDynamicIsIgnore( InputStream *_is )
{
	InputStreamDynamic *is = (InputStreamDynamic*)_is;
	if ( is->head() != 0 && is->head()->type == RunBuf::IgnoreType )
		return true;
	return false;
}

bool inputStreamDynamicIsLangEl( InputStream *_is )
{
	return false;
}

bool inputStreamDynamicIsEof( InputStream *_is )
{
	return _is->head() == 0 && _is->eof;
}

int inputStreamDynamicGetData( InputStream *_is, char *dest, int length )
{
	InputStreamDynamic *is = (InputStreamDynamic*)_is;

	/* If there is any data in the rubuf queue then read that first. */
	if ( is->head() != 0 ) {
		long avail = is->head()->length - is->head()->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->head()->data[is->head()->offset], avail );
			RunBuf *del = is->popHead();
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &is->head()->data[is->head()->offset], length );
			is->head()->offset += length;
			return length;
		}
	}
	else {
		/* No stored data, call the impl version. */
		return is->getDataImpl( dest, length );
	}
}

int InputStreamDynamic::getDataRev( char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( tail() != 0 ) {
		long avail = tail()->length - tail()->offset;
		if ( length >= avail ) {
			memcpy( dest, &head()->data[head()->offset], avail );
			RunBuf *del = popTail();
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &head()->data[head()->offset], length );
			head()->offset += length;
			return length;
		}
	}
	return 0;
}

bool InputStreamDynamic::tryAgainLater()
{
	if ( later )
		return true;

	return false;
}

Tree *InputStreamDynamic::getTree()
{
	if ( head() != 0 && head()->type == RunBuf::TokenType ) {
		RunBuf *runBuf = popHead();

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		delete runBuf;
		return tree;
	}

	return 0;
}

void InputStreamDynamic::pushText( const char *data, long length )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

//	takeBackBuffered( inputStream );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	assert( length < FSM_BUFSIZE );

	RunBuf *newBuf = new RunBuf;
	newBuf->length = length;
	memcpy( newBuf->data, data, length );

	pushBackBuf( newBuf );
}

void InputStreamDynamic::pushTree( Tree *tree, bool ignore )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

//	takeBackBuffered( inputStream );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = new RunBuf;
	newBuf->type = ignore ? RunBuf::IgnoreType : RunBuf::TokenType;
	newBuf->tree = tree;

	prepend( newBuf );
}

Tree *InputStreamDynamic::undoPush( int length )
{
	if ( head()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = this->funcs->getData( this, tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = popHead();
		Tree *tree = rb->tree;
		delete rb;
		return tree;
	}
}

Tree *InputStreamDynamic::undoAppend( int length )
{
	if ( tail()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = getDataRev( tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = popTail();
		Tree *tree = rb->tree;
		delete rb;
		return tree;
	}
}


void initDynamicFuncs()
{
	memcpy( &dynamicFuncs, &baseFuncs, sizeof(InputFuncs) );
	dynamicFuncs.isTree = &inputStreamDynamicIsTree;
	dynamicFuncs.isIgnore = &inputStreamDynamicIsIgnore;
	dynamicFuncs.isLangEl = &inputStreamDynamicIsLangEl;
	dynamicFuncs.isEof = &inputStreamDynamicIsEof;

	dynamicFuncs.getData = &inputStreamDynamicGetData;
}

/*
 * String
 */

int inputStreamStringNeedFlush( InputStream *is )
{
	return is->eof;
}

int InputStreamString::getDataImpl( char *dest, int length )
{ 
	int available = dlen - offset;

	if ( available < length )
		length = available;

	memcpy( dest, data+offset, length );
	offset += length;

	if ( offset == dlen )
		//eof = true;
		later = true;

	return length;
}

void InputStreamString::pushBackBuf( RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= offset );
	offset -= length;
}

void initStringFuncs()
{
	memcpy( &stringFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	stringFuncs.needFlush = &inputStreamStringNeedFlush;
}


/*
 * File
 */

int inputStreamFileNeedFlush( InputStream *_is )
{
	InputStreamFile *is = (InputStreamFile*)_is;
	return is->head() == 0 && feof( is->file );
}

int InputStreamFile::getDataImpl( char *dest, int length )
{
	size_t res = fread( dest, 1, length, file );
	if ( res < (size_t) length )
		later = true;
	return res;
}

void InputStreamFile::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

void initFileFuncs()
{
	memcpy( &fileFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	fileFuncs.needFlush = &inputStreamFileNeedFlush;
}

/*
 * FD
 */

int inputStreamFdNeedFlush( InputStream *is )
{
	return is->head() == 0 && is->eof;
}

void InputStreamFd::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

int InputStreamFd::getDataImpl( char *dest, int length )
{
	long got = read( fd, dest, length );
	if ( got == 0 )
		later = true;
	return got;
}

void initFdFuncs()
{
	memcpy( &fdFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	fdFuncs.needFlush = &inputStreamFdNeedFlush;
}


/*
 * Accum
 */

bool InputStreamAccum::tryAgainLater()
{
	if ( later || ( !flush && head() == 0 ))
		return true;

	return false;
}

int inputStreamAccumNeedFlush( InputStream *_is )
{
	InputStream *is = (InputStream*)_is;

	if ( is->flush ) {
		is->flush = false;
		return true;
	}

	if ( is->head() != 0 && is->head()->type != RunBuf::DataType )
		return true;

	if ( is->eof )
		return true;
		
	return false;
}

int InputStreamAccum::getDataImpl( char *dest, int length )
{
	/* No source of data, it is all done with RunBuf list appends. */
	return 0;
}

void InputStreamAccum::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

void InputStreamAccum::append( const char *data, long len )
{
	RunBuf *ad = new RunBuf;

	InputStream::append( ad );

	/* FIXME: need to deal with this. */
	assert( len < (int)sizeof(ad->data) );

	memcpy( ad->data, data, len );
	ad->length = len;
}

void InputStreamAccum::append( Tree *tree )
{
	RunBuf *ad = new RunBuf;

	InputStream::append( ad );

	ad->type = RunBuf::TokenType;
	ad->tree = tree;
	ad->length = 0;
}


void initAccumFuncs()
{
	memcpy( &accumFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	accumFuncs.needFlush = &inputStreamAccumNeedFlush;
}

