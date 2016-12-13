/*
 * Copyright 2001, 2002 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_PCHECK_H
#define _COLM_PCHECK_H

class ParamCheck
{
public:
	ParamCheck( const char *paramSpec, int argc, const char **argv );

	bool check();

	const char *parameterArg; /* The argument to the parameter. */
	char parameter;     /* The parameter matched. */
	enum { match, invalid, noparam } state;

	const char *argOffset;    /* If we are reading params inside an
	                     * arg this points to the offset. */

	const char *curArg;       /* Pointer to the current arg. */
	int iCurArg;        /* Index to the current arg. */

private:
	const char *paramSpec;    /* Parameter spec supplied by the coder. */
	int argc;           /* Arguement data from the command line. */
	const char **argv;
};

#endif /* _COLM_PCHECK_H */

