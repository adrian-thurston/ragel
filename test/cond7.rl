/* 
 * @LANG: indep
 */
int i;
int c;
%%

%%{
	machine foo;

	action testi {i > 0}
	action inc {
		i = i - 1;
		c = <int>(fc);
		prints "item: ";
		printi c;
		prints "\n";
	}

	count = [0-9] @{ 
		i = <int>(fc - '0');
		prints "count: ";
		printi i;
		prints "\n";
	};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
}%%

/* _____INPUT_____ 
"00\n"
"019\n"
"190\n"
"1719\n"
"1040000\n"
"104000a\n"
"104000\n"
_____INPUT_____ */
/* _____OUTPUT_____
count: 0
count: 0
ACCEPT
count: 0
count: 1
item: 57
ACCEPT
count: 1
item: 57
count: 0
ACCEPT
count: 1
item: 55
count: 1
item: 57
ACCEPT
count: 1
item: 48
count: 4
item: 48
item: 48
item: 48
item: 48
ACCEPT
count: 1
item: 48
count: 4
item: 48
item: 48
item: 48
FAIL
count: 1
item: 48
count: 4
item: 48
item: 48
item: 48
FAIL
_____OUTPUT_____ */
