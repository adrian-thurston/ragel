/*
 * @LANG: indep
 */

int return_to;

%%{
	machine goto;

	unused := 'unused';

	one := 'one' @{
		print_str "one\n";
		fnext *return_to;
	};

	two := 'two' @{
		print_str "two\n";
		fnext *return_to;
	};

	main := (
			'1' @{ return_to = ftargs; fnext one; }
		|	'2' @{ return_to = ftargs; fnext two; }
		|	'\n'
	)*;
}%%

##### INPUT #####
"1one2two1one\n"
##### OUTPUT #####
one
two
one
ACCEPT
