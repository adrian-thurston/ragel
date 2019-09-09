SUBDIRS = . aapl.d colm.d ragel.d rlhc.d rlparse.d trans.d

noinst_SCRIPTS = colm.mk colm.sh ragel.mk ragel.sh

colm.mk: colm.mk.in Makefile
	@$(top_srcdir)/test/sedsubst $< $@ -w,+x $(SED_SUBST)

colm.sh: colm.sh.in Makefile
	@$(top_srcdir)/test/sedsubst $< $@ -w,+x $(SED_SUBST)

ragel.mk: ragel.mk.in Makefile
	@$(top_srcdir)/test/sedsubst $< $@ -w,+x $(SED_SUBST)

ragel.sh: ragel.sh.in Makefile
	@$(top_srcdir)/test/sedsubst $< $@ -w,+x $(SED_SUBST)
