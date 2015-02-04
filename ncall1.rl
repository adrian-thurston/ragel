/*
 * @LANG: indep
 */

int top;
int stack[32];

int target;

%%{
	machine goto;

	unused := 'unused';

	one := 'one' @{
		print_str "one\n";
		fnret;
	};

	two := 'two' @{
		print_str "two\n";
		fnret;
	};

	main := (
			'1' @{ target = fentry(one); fncall *target; }
		|	'2' @{ target = fentry(two); fncall *target; }
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
