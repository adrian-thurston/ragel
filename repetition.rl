/*
 * @LANG: c++
 */

/* Test repeptition operators. */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace std;

struct Rep
{
	int cs;

	int init( );
	int execute( const char *data, int len );
	int finish( );
};

%%{
	machine Rep;

	action begin { cout << "begin" << endl; }
	action in { cout << "in" << endl; }
	action end { cout << "end" << endl; }

	a = 'a' >begin @in %end;
	b = 'b' >begin @in %end;
	c = 'c' >begin @in %end;
	d = 'd' >begin @in %end;

	main := 
		( a {5} '\n' )* '-\n'
		( b {,5} '\n' )* '-\n'
		( c {5,} '\n' )* '-\n'
		( d {2,5} '\n' )*;
}%%

%% write data;

int Rep::init( )
{
	%% write init;
	return 1;
}

int Rep::execute( const char *_data, int _len )
{
	const char *p = _data;
	const char *pe = _data+_len;

	%% write exec;

	if ( cs == Rep_error )
		return -1;
	if ( cs >= Rep_first_final )
		return 1;
	return 0;
}

int Rep::finish( )
{
	if ( cs == Rep_error )
		return -1;
	if ( cs >= Rep_first_final )
		return 1;
	return 0;
}

void test( const char *buf )
{
	Rep rep;
	int len = strlen( buf );
	rep.init();
	rep.execute( buf, len );
	if ( rep.finish() > 0 )
		printf("ACCEPT\n");
	else
		printf("FAIL\n");
}

int main()
{
	test(
		"aaaaa\n"
		"-\n"
		"\n"
		"b\n"
		"bb\n"
		"bbb\n"
		"bbbb\n"
		"bbbbb\n"
		"-\n"
		"ccccc\n"
		"ccccccc\n"
		"cccccccccc\n"
		"-\n"
		"dd\n"
		"ddd\n"
		"dddd\n"
		"ddddd\n"
	);

	test(
		"a\n"
		"-\n"
		"b\n"
		"-\n"
		"c\n"
		"-\n"
		"d\n"
	);

	return 0;
}

#ifdef _____OUTPUT_____
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
begin
in
end
ACCEPT
begin
in
FAIL
#endif
