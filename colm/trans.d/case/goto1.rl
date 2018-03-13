/*
 * @LANG: indep
 * @PROHIBIT_LANGUAGES: ruby ocaml
 */

int target;

%%{
	machine goto1;

	unused := 'unused';

	one := 'one' @{
		print_str "one\n";
		target = fentry(main);
		fgoto *target;
	};

	two := 'two' @{
		print_str "two\n";
		target = fentry(main);
		fgoto *target;
	};

	main := 
		'1' @{ target = fentry(one); fgoto *target; }
	|	'2' @{ target = fentry(two); fgoto *target; }
	|	'\n';
}%%

##### INPUT #####
"1one2two1one\n"
##### OUTPUT #####
one
two
one
ACCEPT
