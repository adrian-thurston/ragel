/* 
 * @LANG: c
 */

#include <stdio.h>

%%{
	machine foo;

	import "import2.h";

	main := A B C D E F;
	
}%%

%% write data;

int main()
{
	printf( "run\n" );
}

##### OUTPUT #####
run
