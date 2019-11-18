SUBDIRS = . aapl.d colm.d ragel.d rlhc.d rlparse.d trans.d

noinst_SCRIPTS = subject.mk subject.sh

EXTRA_DIST = subject.mk.in subject.sh.in runtests timed

subject.mk: subject.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.sh: subject.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
