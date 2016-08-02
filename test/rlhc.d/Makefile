
include ../subject.mk

all: rlhc

COLM = $(SUBJECT_BIN)
COLM_INC = $(SUBJECT_CPPFLAGS)
COLM_LIB = $(SUBJECT_LDFLAGS)

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
