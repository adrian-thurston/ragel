/*
 *  Copyright 2003 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Aapl.
 *
 *  Aapl is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 2.1 of the License, or (at your option)
 *  any later version.
 *
 *  Aapl is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Aapl; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _AAPL_TDLISTVAL_H
#define _AAPL_TDLISTVAL_H

#include "dlist.h"

#define BASE_EL(name) name
#define BASECAST(name) static_cast<TDListEl*>(name)
#define SUPERCAST(name) static_cast<TDListValEl<T>*>(name)
#define TDL_TEMPDEF class T
#define TDL_TEMPUSE T
#define TDList TDListVal
#define Element TDListValEl<T>
#define TDL_VALUE

#include "tdlcommon.h"

#undef BASE_EL
#undef BASECAST
#undef SUPERCAST
#undef TDL_TEMPDEF
#undef TDL_TEMPUSE
#undef TDList
#undef Element
#undef TDL_VALUE

#endif /* _AAPL_TDLISTVAL_H */

