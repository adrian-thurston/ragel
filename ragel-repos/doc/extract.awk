#!/usr/bin/awk
#

BEGIN {
	in_generate = 0;
	in_verbatim = 0;
	return_val = 1;
}

/^% GENERATE: *[a-z0-9A-Z_\.\-]+ *$/ && $3 == exname {
	in_generate = 1;
	return_val = 0;
	next;
}

/^% END GENERATE$/ {
	in_generate = 0;
	next;
}

in_generate && /\\begin\{verbatim\}/ {
	in_generate = 0;
	in_verbatim = 1;
	next;
}

in_verbatim && /\\end\{verbatim\}/ {
	in_generate = 1;
	in_verbatim = 0;
	next;
}

in_generate && /^%/ {
	print substr( $0, 2 );
}

in_verbatim {
	print $0;
}

END { exit return_val; }
