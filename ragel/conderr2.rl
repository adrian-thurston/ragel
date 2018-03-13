/* 
 * @LANG: c
 */

#include <stdio.h>

%%{
	machine foo;

	sep = ( [ ] | ';' when { 0 } )+;

	cmt = 
	^[ ;] >!{ printf("A\n"); } .
	'x' >!{ printf("B\n"); };

	main := sep . cmt;
}%%

%% write data;

int main()
{
	char buffer[] = " ;";

	char *p = buffer;
	char *pe = buffer + sizeof(buffer);
	char *eof = pe;
	int  cs = foo_start;

	%% write exec;

	return 0;
} 

##### OUTPUT #####
A
