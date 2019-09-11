noinst_SCRIPTS = gentests

gentests: gentests.sh Makefile
	@$(top_srcdir)/test/sedsubst $< $@ -w,+x $(SED_SUBST)
