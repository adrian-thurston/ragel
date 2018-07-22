SUBDIRS = . pcre-colm.d pcre-ragel.d rlhc.d rlparse.d trans.d yellowbrick.d

noinst_SCRIPTS = colm.mk colm.sh ragel.mk ragel.sh

colm.mk: colm.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

colm.sh: colm.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

ragel.mk: ragel.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

ragel.sh: ragel.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
