/*
 * @LANG: c
 */

#include <string.h>
#include <stdio.h>

#ifdef PERF_TEST

/* Calibrated to 1s on yoho. */
#define perf_iters ( 3448275ll * S )

int _perf_dummy = 0;
#define perf_printf(...) ( _perf_dummy += 1 )
#define perf_loop long _pi; for ( _pi = 0; _pi < perf_iters; _pi++ )

#else

#define perf_printf(...) printf( __VA_ARGS__ )
#define perf_loop

#endif

struct strs
{
	int cs;
};

%%{
	machine strs;

	main := 
		"__gmon_start__\n" |
		"cerr\n" |
		"__cp_push_exception\n" |
		"_DYNAMIC\n" |
		"__rtti_user\n" |
		"__rtti_si\n" |
		"_init\n" |
		"__throw\n" |
		"__deregister_frame_info\n" |
		"terminate__Fv\n" |
		"__builtin_vec_new\n" |
		"_fini\n" |
		"__builtin_vec_delete\n" |
		"_GLOBAL_OFFSET_TABLE_\n" |
		"__nw__FUiPv\n" |
		"__builtin_delete\n" |
		"__builtin_new\n" |
		"cout\n" |
		"__register_frame_info\n" |
		"__eh_alloc\n" |
		"strcpy\n" |
		"stdout\n" |
		"memmove\n" |
		"memcpy\n" |
		"malloc\n" |
		"isatty\n" |
		"strtoul\n" |
		"fprintf\n" |
		"stdin\n" |
		"ferror\n" |
		"strncpy\n" |
		"unlink\n" |
		"strcasecmp\n" |
		"realloc\n" |
		"_IO_getc\n" |
		"fread\n" |
		"memset\n" |
		"__assert_fail\n" |
		"strcmp\n" |
		"stderr\n" |
		"fwrite\n" |
		"exit\n" |
		"fopen\n" |
		"atoi\n" |
		"fileno\n" |
		"_IO_stdin_used\n" |
		"__libc_start_main\n" |
		"strlen\n" |
		"free\n" |
		"_edata\n" |
		"__bss_start\n" |
		"_end\n" |
		"QVhl\n" |
		"BPPh\n" |
		"PHRV\n" |
		"PHRj\n" |
		"PHRj\n" |
		"jphy\n" |
		"jqhy\n" |
		"PHRj\n" |
		"PHRj\n" |
		"LWVS\n" |
		"LWVS\n" |
		"bad_alloc\n" |
		"main\n" |
		"false\n" |
		"help\n" |
		"bad_alloc\n" |
		"bad_alloc\n" |
		"bad_alloc\n" |
		"ascii\n" |
		"extend\n" |
		"alnum\n" |
		"alpha\n" |
		"cntrl\n" |
		"digit\n" |
		"graph\n" |
		"lower\n" |
		"print\n" |
		"punct\n" |
		"space\n" |
		"upper\n" |
		"xdigit\n" |
		"false\n" |
		"bad_alloc\n" |
		"bad_alloc\n" |
		"bad_alloc\n" |
		"TransStruct\n" |
		"StateStruct\n" |
		"Struct\n" |
		"Init\n" |
		"bad_alloc\n" |
		"TransStruct\n" |
		"StateStruct\n" |
		"Struct\n" |
		"Init\n" |
		"Accept\n" |
		"Finish\n" |
		"bad_alloc\n" |
		"Struct\n" |
		"Init\n" |
		"Finish\n" |
		"Accept\n" |
		"bad_alloc\n" |
		"Struct\n" |
		"Init\n" |
		"bad_alloc\n" |
		"Struct\n" |
		"Init\n" |
		"Finish\n" |
		"Accept\n" |
		"bad_alloc\n" |
		"Struct\n" |
		"Init\n" |
		"Finish\n" |
		"Accept";
}%%

%% write data;

void strs_run( const char *_data, int _len )
{
	perf_loop
	{
		struct strs fsm;
		const char *p = _data;
		const char *pe = _data + _len;

		%% variable cs fsm.cs;
		%% write init;
		%% write exec;

		if ( fsm.cs >= strs_first_final ) {
			perf_printf("ACCEPT\n");
		}
		else {
			perf_printf("FAIL\n");
		}
	}
}

void test( const char *buf )
{
	int len = strlen( buf );
	strs_run( buf, len );
}


int main()
{
	test( "stdin\n" );
	test( "bad_alloc\n" );
	test( "_GLOBAL_OFFSET_TABLE_\n" );
	test( "not in\n" );
	test(
		"isatty\n"
		"junk on end.\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
ACCEPT
ACCEPT
ACCEPT
FAIL
FAIL
#endif
