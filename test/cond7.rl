/* 
 * @LANG: indep
 */

int i;
int c;

%%{
	machine foo;

	action testi {i > 0}
	action inc {
		i = i - 1;
		c = <int>(fc);
		print_str "item: ";
		print_int c;
		print_str "\n";
	}

	count = [0-9] @{ 
		i = <int>(fc - '0');
		print_str "count: ";
		print_int i;
		print_str "\n";
	};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
}%%

##### INPUT #####
"00\n"
"019\n"
"190\n"
"1719\n"
"1040000\n"
"104000a\n"
"104000\n"
##### OUTPUT #####
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
