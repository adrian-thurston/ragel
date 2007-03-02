/*
 * Convert a string to an integer.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

%%{
	machine atoi;
	write data noerror;
}%%

int atoi( char *str )
{
	char *p = str;
	int cs, val = 0;
	bool neg = false;

	%%{
		action see_neg {
			neg = true;
		}

		action add_digit { 
			val = val * 10 + (fc - '0');
		}

		main := 
			( '-'@see_neg | '+' )? ( digit @add_digit )+ 
			'\n' @{ fbreak; };

		# Initialize and execute.
		write init;
		write exec noend;
	}%%

	if ( neg )
		val = -1 * val;

	if ( cs < atoi_first_final )
		cerr << "atoi: there was an error" << endl;

	return val;
};


#define BUFSIZE 1024

int main()
{
	char buf[BUFSIZE];
	while ( fgets( buf, sizeof(buf), stdin ) != 0 ) {
		int value = atoi( buf );
		cout << value << endl;
	}
	return 0;
}
