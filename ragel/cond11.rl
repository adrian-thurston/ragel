/* 
 * @LANG: indep
 * @NEEDS_EOF: yes
 * @PROHIBIT_GENFLAGS: -T1 -F0 -F1 -G0 -G1 -G2
 * @PROHIBIT_LANGUAGES: go cs java ruby ocaml rust crack julia
 */

int q;

%%{
	machine foo;

	action match { print_str "match\n"; }

	action ini_0 { q = 0; }
	action inc_0 { q = q + 1; }
	action min_0 { q >= 5 }
	action max_0 { q < 5 }

	action t { true }

	main := 
		( :condstar( ('a'), ini_0, inc_0, min_0, max_0 ): )
		( '' %when t | '' %when !t ) %when { 1==1 } %when { 2==2 } %when { 3==3 } %match;
	
}%%

##### INPUT #####
""
"a"
"aaaa"
"aaaaa"
"aaaaaa"
"aaaaaaa"
##### OUTPUT #####
FAIL
FAIL
FAIL
match
ACCEPT
FAIL
FAIL
