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

RunBuf *InputStream::popHead()
{
	RunBuf *ret = queue;
	queue = queue->next;
	if ( queue == 0 )
		queueTail = 0;
	else
		queue->prev = 0;
	return ret;
}

RunBuf *InputStream::popTail()
{
	RunBuf *ret = queueTail;
	queueTail = queue->prev;
	if ( queueTail == 0 )
		queue = 0;
	else
		queueTail->next = 0;
	return ret;
}

void InputStream::append( RunBuf *runBuf )
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

void InputStream::prepend( RunBuf *runBuf )
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

bool inputStreamDynamicIsTree( InputStream *is )
{
	if ( is->head() != 0 && is->head()->type == RunBuf::TokenType )
		return true;
	return false;
}

bool inputStreamDynamicIsIgnore( InputStream *is )
{
	if ( is->head() != 0 && is->head()->type == RunBuf::IgnoreType )
		return true;
	return false;
}

bool inputStreamDynamicIsLangEl( InputStream *is )
{
	return false;
}

bool inputStreamDynamicIsEof( InputStream *is )
{
	return is->head() == 0 && is->eof;
}

int inputStreamDynamicGetData( InputStream *is, char *dest, int length )
{
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
		return is->funcs->getDataImpl( is, dest, length );
	}
}

int inputStreamDynamicGetDataRev( InputStream *is, char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( is->tail() != 0 ) {
		long avail = is->tail()->length - is->tail()->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->head()->data[is->head()->offset], avail );
			RunBuf *del = is->popTail();
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &is->head()->data[is->head()->offset], length );
			is->head()->offset += length;
			return length;
		}
	}
	return 0;
}

bool inputStreamDynamicTryAgainLater( InputStream *is )
{
	if ( is->later )
		return true;

	return false;
}

Tree *inputStreamDynamicGetTree( InputStream *is )
{
	if ( is->head() != 0 && is->head()->type == RunBuf::TokenType ) {
		RunBuf *runBuf = is->popHead();

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		delete runBuf;
		return tree;
	}

	return 0;
}

void inputStreamDynamicPushText( InputStream *is, const char *data, long length )
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

	is->funcs->pushBackBuf( is, newBuf );
}

void inputStreamDynamicPushTree( InputStream *is, Tree *tree, bool ignore )
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

	is->prepend( newBuf );
}

Tree *inputStreamDynamicUndoPush( InputStream *is, int length )
{
	if ( is->head()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = is->funcs->getData( is, tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = is->popHead();
		Tree *tree = rb->tree;
		delete rb;
		return tree;
	}
}

Tree *inputStreamDynamicUndoAppend( InputStream *is, int length )
{
	if ( is->tail()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = inputStreamDynamicGetDataRev( is, tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = is->popTail();
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
	dynamicFuncs.tryAgainLater = &inputStreamDynamicTryAgainLater;
	dynamicFuncs.getData = &inputStreamDynamicGetData;
	dynamicFuncs.getTree = &inputStreamDynamicGetTree;
	dynamicFuncs.pushTree = &inputStreamDynamicPushTree;
	dynamicFuncs.undoPush = &inputStreamDynamicUndoPush;
	dynamicFuncs.undoAppend = &inputStreamDynamicUndoAppend;
	dynamicFuncs.pushText = &inputStreamDynamicPushText;
}

/*
 * String
 */

int inputStreamStringNeedFlush( InputStream *is )
{
	return is->eof;
}

int inputStreamStringGetDataImpl( InputStream *is, char *dest, int length )
{ 
	int available = is->dlen - is->offset;

	if ( available < length )
		length = available;

	memcpy( dest, is->data+is->offset, length );
	is->offset += length;

	if ( is->offset == is->dlen ) {
		//eof = true;
		is->later = true;
	}

	return length;
}

void inputStreamStringPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= is->offset );
	is->offset -= length;
}

void initStringFuncs()
{
	memcpy( &stringFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	stringFuncs.needFlush = &inputStreamStringNeedFlush;
	stringFuncs.getDataImpl = &inputStreamStringGetDataImpl;
	stringFuncs.pushBackBuf = &inputStreamStringPushBackBuf;
}


/*
 * File
 */

int inputStreamFileNeedFlush( InputStream *is )
{
	return is->head() == 0 && feof( is->file );
}

int inputStreamFileGetDataImpl( InputStream *is, char *dest, int length )
{
	size_t res = fread( dest, 1, length, is->file );
	if ( res < (size_t) length )
		is->later = true;
	return res;
}

void inputStreamFilePushBackBuf( InputStream *is, RunBuf *runBuf )
{
	is->prepend( runBuf );
}

void initFileFuncs()
{
	memcpy( &fileFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	fileFuncs.needFlush = &inputStreamFileNeedFlush;
	fileFuncs.getDataImpl = &inputStreamFileGetDataImpl;
	fileFuncs.pushBackBuf = &inputStreamFilePushBackBuf;
}

/*
 * FD
 */

int inputStreamFdNeedFlush( InputStream *is )
{
	return is->head() == 0 && is->eof;
}

void inputStreamFdPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	is->prepend( runBuf );
}

int inputStreamFdGetDataImpl( InputStream *is, char *dest, int length )
{
	long got = read( is->fd, dest, length );
	if ( got == 0 )
		is->later = true;
	return got;
}

void initFdFuncs()
{
	memcpy( &fdFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	fdFuncs.needFlush = &inputStreamFdNeedFlush;
	fdFuncs.getDataImpl = &inputStreamFdGetDataImpl;
	fdFuncs.pushBackBuf = &inputStreamFdPushBackBuf;
}


/*
 * Accum
 */

bool inputStreamAccumTryAgainLater( InputStream *is )
{
	if ( is->later || ( !is->flush && is->head() == 0 ))
		return true;

	return false;
}

int inputStreamAccumNeedFlush( InputStream *is )
{
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

int inputStreamAccumGetDataImpl( InputStream *is, char *dest, int length )
{
	/* No source of data, it is all done with RunBuf list appends. */
	return 0;
}

void inputStreamAccumPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	is->prepend( runBuf );
}

void inputStreamAccumAppendData( InputStream *_is, const char *data, long len )
{
	InputStream *is = (InputStream*)_is;

	RunBuf *ad = new RunBuf;
	is->InputStream::append( ad );

	/* FIXME: need to deal with this. */
	assert( len < (int)sizeof(ad->data) );

	memcpy( ad->data, data, len );
	ad->length = len;
}

void inputStreamAccumAppendTree( InputStream *_is, Tree *tree )
{
	InputStream *is = (InputStream*)_is;

	RunBuf *ad = new RunBuf;

	is->InputStream::append( ad );

	ad->type = RunBuf::TokenType;
	ad->tree = tree;
	ad->length = 0;
}

void initAccumFuncs()
{
	memcpy( &accumFuncs, &dynamicFuncs, sizeof(InputFuncs) );
	accumFuncs.needFlush = &inputStreamAccumNeedFlush;
	accumFuncs.tryAgainLater = &inputStreamAccumTryAgainLater;
	accumFuncs.getDataImpl = &inputStreamAccumGetDataImpl;
	accumFuncs.appendData = &inputStreamAccumAppendData;
	accumFuncs.appendTree = &inputStreamAccumAppendTree;
	accumFuncs.pushBackBuf = &inputStreamAccumPushBackBuf;
}

