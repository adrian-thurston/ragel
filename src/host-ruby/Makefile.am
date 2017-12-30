COLM        = @COLM@

noinst_PROGRAMS = ruby rlhc

ruby_SOURCES =
rlhc_SOURCES =

EXTRA_DIST = ruby.lm rlhc.lm rlhc-host.lm rlhc-ruby.lm

ruby$(EXEEXT): ruby.lm
	$(COLM) -I.. $<

rlhc$(EXEEXT): rlhc.lm rlhc-host.lm rlhc-ruby.lm
	$(COLM) $<

