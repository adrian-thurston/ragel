# -*- Makefile -*-

SUFFIXES = .rl

.rl.c:
	$(RAGEL) $(RAGELFLAGS) -C $< -o $@
