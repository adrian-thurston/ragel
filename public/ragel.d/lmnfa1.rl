/*
 * @LANG: c
 */

#include <stdio.h>
#include <string.h>

%%{
	machine nfalm;

	main := :nfa |*
		"hello" %when {0} => { printf("hello --fail\n"); };
		"hello"           => { printf("hello\n"); };
		[a-z]+ %when {0}  => { printf("other --fail\n"); };
		[a-z]+            => { printf("other\n"); };
		' '               => { printf("<space>\n"); };
	*|;
}%%

%% write data;

void exec( const char *data, int length )
{
	const char *p = data;
	const char *pe = data + length;
	const char *eof = pe;

	const char *ts, *te;
	int cs;

	int nfa_len, nfa_count;
	struct nfa_s { int state; const char *p; } nfa_bp[20];

	%% write init;
	%% write exec;
}

int main()
{
	const char *in = "hello hellos hello";
	exec( in, strlen(in) );
	return 0;
}

##### OUTPUT #####
hello
<space>
other
<space>
hello
