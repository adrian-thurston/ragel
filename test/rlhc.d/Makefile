
include ../subject.mk

all: rlhc

COLM = $(SUBJ_COLM_BIN)
COLM_INC = $(SUBJ_COLM_CPPFLAGS)
COLM_LIB = $(SUBJ_COLM_LDFLAGS)

RLHC_DEP = \
	rlhc-host.lm \
	rlhc-c.lm \
	rlhc-d.lm \
	rlhc-go.lm \
	rlhc-csharp.lm \
	rlhc-java.lm \
	rlhc-ruby.lm \
	rlhc-rust.lm \
	rlhc-ocaml.lm \
	rlhc-crack.lm \
	rlhc-julia.lm \
	rlhc-js.lm

rlhc: rlhc.lm $(RLHC_DEP)
	$(COLM) -b rlhc_object -o $@ $<

clean:
	rm -f *.o
