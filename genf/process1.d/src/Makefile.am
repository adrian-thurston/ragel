include genf.mk

GENF = @GENF@

bin_PROGRAMS = process1

process1_SOURCES = \
	main.h \
	main.cc \
	$(BUILT_SOURCES)

BUILT_SOURCES += \
	main_gen.h \
	main_gen.cc \
	itq_gen.h \
	itq_gen.cc \
	packet_gen.h \
	packet_gen.cc

CLEANFILES = $(BUILT_SOURCES)

main_gen.cc: process1.gf
	$(GENF) $<

main_gen.cc: $(GENF)

main_gen.h: main_gen.cc
itq_gen.h: main_gen.cc
itq_gen.cc: main_gen.cc
packet_gen.h: main_gen.cc
packet_gen.cc: main_gen.cc
