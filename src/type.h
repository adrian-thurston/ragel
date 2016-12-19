/*
 * Copyright 2007-2015 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COLM_TYPE_H
#define _COLM_TYPE_H

enum TYPE
{
	TYPE_NOTYPE      = 0x00,
	TYPE_NIL         = 0x01,
	TYPE_TREE        = 0x02,
	TYPE_REF         = 0x03,
	TYPE_ITER        = 0x04,
	TYPE_STRUCT      = 0x05,
	TYPE_GENERIC     = 0x06,
	TYPE_INT         = 0x07,
	TYPE_BOOL        = 0x08,
	TYPE_LIST_PTRS   = 0x09,
	TYPE_MAP_PTRS    = 0x0a
};

#endif /* _COLM_TYPE_H */

