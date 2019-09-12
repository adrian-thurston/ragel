noinst_SCRIPTS = gentests

gentests: gentests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
