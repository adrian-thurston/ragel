SUBDIRS = app1
noinst_SCRIPTS = runtests

runtests: runtests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
