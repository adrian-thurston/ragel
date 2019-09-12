SUBDIRS = . aapl.d colm.d ragel.d rlhc.d rlparse.d trans.d

noinst_SCRIPTS = subject.mk subject.sh

subject.mk: subject.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.sh: subject.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
