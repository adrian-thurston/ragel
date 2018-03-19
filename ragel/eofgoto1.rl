/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 * @PROHIBIT_LANGUAGES: ruby ocaml
 * @PROHIBIT_FEATFLAGS: --var-backend
 *
 * Testing fgoto in an EOF action.
 */


bool neg;
int value;

value = 0;
neg = false;

%%{
	machine atoi;

	action begin {
		neg = false;
		value = 0;
	}

	action see_neg {
		neg = true;
	}

	action add_digit { 
		value = value * 10 + <int>(fc - 48);
	}

	action finish {
		if ( neg != 0 ) {
			value = -1 * value;
		}
	}
	action print {
		print_int value;
		print_str "\n";
	}


	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	action done
	{
		print_str "done: ";
		print_off;
		print_str "\n";
	}

	action extra
	{
		print_str "goto extra\n";
		fgoto extra;
	}

	extra := "" %done;

	main := atoi '\n' @print %extra;
}%%

##### INPUT #####
	"1\n"
	"12\n"
	"222222\n"
	"+2123\n"
	"213 3213\n"
	"-12321\n"
	"--123\n"
	"-99\n"
	" -3000\n"
##### OUTPUT #####
1
goto extra
done: 2
ACCEPT
12
goto extra
done: 3
ACCEPT
222222
goto extra
done: 7
ACCEPT
2123
goto extra
done: 6
ACCEPT
FAIL
-12321
goto extra
done: 7
ACCEPT
FAIL
-99
goto extra
done: 4
ACCEPT
FAIL
