/*
 * @LANG: c++
 */
#include <cstdio>
#include <cstdlib>
#include <string>

%%{
	machine part_token;

	main := |*
			' '+;
			',';
			'abcd' => { printf("token: %s\n", std::string(ts, te).c_str()); };
		*|;
}%%

%% write data;

int scan(const std::string &in)
{
	int cs = 0, act = 0;
	const char *p = in.c_str();
	const char *pe = in.c_str() + in.length();
	const char *ts = nullptr, *te = nullptr;
	const char *eof = pe;

	%% write init;
	%% write exec;

	printf("done\n");

	if (cs == part_token_error)
		printf("Error near %zd\n", p-in.data());
	else if(ts)
		printf("offsets: ts %zd te: %zd pe: %zd\n", ts-in.data(), te-in.data(), pe-in.data());
	else
		printf("ts is null\n");

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	printf("scan: %d\n", scan("abcd") );
	printf("scan: %d\n", scan("abcd,ab") );
}

##### OUTPUT #####
token: abcd
done
ts is null
scan: 0
token: abcd
done
offsets: ts 5 te: 5 pe: 7
scan: 0
