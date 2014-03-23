/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 *
 * const char *data = ts;
 * int len = te - ts;
 * cout << "<" << tok << "> ";
 * for ( int i = 0; i < len; i++ )
 *     cout << data[i];
 * cout << '\n';
 */

ptr ts;
ptr te;
int act;
int token;

%%{
	machine scanner;

	action comment {
		token = 242;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> { 
			token = 193;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> { 
			token = 192;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{
			token = 195;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {
			token = 194;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {
			token = 218;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {
			token = 219;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {
			token = 220;
			print_str "<";
			print_int token;
			print_str "> ";
			print_token;
			print_str "\n";
		};

	# Only buffer the second item, first buffered by symbol.
	'::' => {
		token = 197;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'==' => {
		token = 223;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'!=' => {
		token = 224;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'&&' => {
		token = 225;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'||' => {
		token = 226;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'*=' => {
		token = 227;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'/=' => {
		token = 228;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'%=' => {
		token = 229;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'+=' => {
		token = 230;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'-=' => {
		token = 231;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'&=' => {
		token = 232;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'^=' => {
		token = 233;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'|=' => {
		token = 234;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'++' => {
		token = 212;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'--' => {
		token = 213;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'->' => {
		token = 211;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'->*' => {
		token = 214;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	'.*' => {
		token = 215;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};

	# Three char compounds, first item already buffered.
	'...' => {
		token = 240;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};

	# Single char symbols.
	( punct - [_"'] ) => {
		token = <int>(first_token_char);
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => { 
		token = 241;
		print_str "<";
		print_int token;
		print_str "> ";
		print_token;
		print_str "\n";
	};
	*|;
}%%

#ifdef _____INPUT_____
"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22"
"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/"
"'\n'\n"
#endif

#ifdef _____OUTPUT_____
<192> "\"hi"
<241>  
<242> /!
!/
<241> 

<218> 44
<241>  
<194> .44
<241> 

<194> 44.
<241>  
<218> 44
<241> 

<218> 44
<241>  
<46> .
<241>  
<218> 44
<241> 

<194> 44.44
<241> 

<195> _hithere22
ACCEPT
<193> '\''
<192> "\n\d'\""
<241> 

<195> hi
<241> 

<218> 99
<241> 

<194> .99
<241> 

<194> 99e-4
<241> 

<214> ->*
<241> 

<226> ||
<241> 

<220> 0x98
<241> 

<218> 0
<195> x
<241> 

<242> //

<242> /! * !/
ACCEPT
FAIL
#endif
