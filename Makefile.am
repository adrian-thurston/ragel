SUBDIRS = aapl ragel colm

noinst_SCRIPTS = runtests

runtests: runtests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
