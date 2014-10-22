/*
 *  Copyright 2001, 2002 Adrian Thurston <thurston@complang.org>
 */

#ifndef _PCHECK_H
#define _PCHECK_H

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

#endif /* _PCHECK_H */
