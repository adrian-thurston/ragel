/*
 * @LANG: c
 * @GENERATED: true
 */

#include <string.h>
#include <stdio.h>




%%{
	machine erract;

	action err_start {printf( "%s", "err_start\n" );
}
	action err_all {printf( "%s", "err_all\n" );
}
	action err_middle {printf( "%s", "err_middle\n" );
}
	action err_out {printf( "%s", "err_out\n" );
}

	action eof_start {printf( "%s", "eof_start\n" );
}
	action eof_all {printf( "%s", "eof_all\n" );
}
	action eof_middle {printf( "%s", "eof_middle\n" );
}
	action eof_out {printf( "%s", "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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
	if ( cs >= erract_first_final )
		printf( "ACCEPT\n" );
	else
		printf( "FAIL\n" );
}

char *inp[] = {
"",
"h",
"x",
"he",
"hx",
"hel",
"hex",
"hell",
"helx",
"hello",
"hellx",
"hello\n",
"hellox",
};

int inplen = 13;

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

