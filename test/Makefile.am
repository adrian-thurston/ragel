SUBDIRS = . ragel.d

noinst_SCRIPTS = runtests subject.mk subject.sh

EXTRA_DIST = subject.mk.in subject.sh.in runtests.sh

subject.mk: subject.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.sh: subject.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

runtests: runtests.sh
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
