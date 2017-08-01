COLM        = @COLM@

noinst_PROGRAMS = ruby rlhc

ruby$(EXEEXT): ruby.lm
	$(COLM) -I.. $<

rlhc$(EXEEXT): rlhc.lm rlhc-host.lm rlhc-ruby.lm
	$(COLM) $<

