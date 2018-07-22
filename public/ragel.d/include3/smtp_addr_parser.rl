/*
 * @LANG: c
 */

%%{
  machine smtp_addr_parser;
  include smtp_address "smtp_address.rl";

  main := 'main';
}%%

%% write data;

int main()
{

}
