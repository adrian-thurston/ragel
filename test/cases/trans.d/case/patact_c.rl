/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>

char comm ;
int top ;
int stack [32];
char * ts ;
char * te ;
int act ;
int value ;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {printf( "%s", "word\n" );
};
		[0-9]+ => {printf( "%s", "num\n" );
};
		[\n ] => {printf( "%s", "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {printf( "%s", "word (w/lbh)\n" );
fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {printf( "%s", "word (c/lbh)\n" );
};
		[\n ] => {printf( "%s", "space\n" );
};
		'22' => {printf( "%s", "num (w/switch)\n" );
};
		[0-9]+ => {printf( "%s", "num (w/switch)\n" );
fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {printf( "%s", "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {printf( "%s", "in semi\n" );
fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {printf( "%s", "word (w/lbh)\n" );
fhold;fgoto other;};
		[a-z]+ ' foil' => {printf( "%s", "word (c/lbh)\n" );
};
		[\n ] => {printf( "%s", "space\n" );
};
		'22' => {printf( "%s", "num (w/switch)\n" );
};
		[0-9]+ => {printf( "%s", "num (w/switch)\n" );
fhold;fgoto other;};
		[0-9]+ ' foil' => {printf( "%s", "num (c/switch)\n" );
};
		';' => {printf( "%s", "going to semi\n" );
fhold;fgoto semi;};
		'!' => {printf( "%s", "immdiate\n" );
fgoto exec_test;};
	*|;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}

void exec( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	char *eof = pe;
	%% write exec;
}

void finish( )
{
	if ( cs >= patact_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
};

int inplen = 7;

int main( )
{
	int i;
	for ( i = 0; i < inplen; i++ ) {
		init();
		exec( inp[i], strlen(inp[i]) );
		finish();
	}
	return 0;
}

