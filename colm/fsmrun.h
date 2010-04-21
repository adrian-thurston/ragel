/*
 *  Copyright 2007-2009 Adrian Thurston <thurston@complang.org>
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

#ifndef _FSMRUN_H
#define _FSMRUN_H

#include "pdarun.h"
#include "input.h"

struct GenAction;
struct KlangEl;
struct ParseData;
typedef struct _Kid Kid;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;

#include <assert.h>
#include <stdio.h>
#include <iostream>

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

#include "fsmrun2.h"

void execAction( FsmRun *fsmRun, GenAction *genAction );
void fsmExecute( FsmRun *fsmRun, InputStream *inputStream );

void initInputStream( InputStream *in );
void sendQueuedTokens( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void sendHandleError( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
Head *extractMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream );
void queueBackTree( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
void parseLoop( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
long scan_token( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );

void sendNamedLangEl( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
Kid *makeToken( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
		Head *tokdata, bool namedLangEl, int bindId );

void newToken( PdaRun *pdaRun, FsmRun *fsmRun );

void initBindings( PdaRun *pdaRun );
void makeTokenPushBinding( PdaRun *pdaRun, int bindId, Tree *tree );

#endif
