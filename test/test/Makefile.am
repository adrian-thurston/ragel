noinst_SCRIPTS = runtests subject.mk subject.pl subject.sh

runtests: runtests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.mk: subject.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.pl: subject.pl.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.sh: subject.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
