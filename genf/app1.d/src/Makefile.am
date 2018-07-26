include genf.mk

GENF = @GENF@

bin_PROGRAMS = app1

app1_SOURCES = \
	main.h \
	main.cc \
	user.h \
	user.cc \
	listen.h \
	listen.cc \
	data.cc \
	$(BUILT_SOURCES)

BUILT_SOURCES += \
	main_gen.h \
	main_gen.cc \
	user_gen.h \
	user_gen.cc \
	listen_gen.h \
	listen_gen.cc \
	itq_gen.h \
	itq_gen.cc \
	packet_gen.h \
	packet_gen.cc

CLEANFILES = $(BUILT_SOURCES)

main_gen.cc: app1.gf
	$(GENF) $<

main_gen.cc: $(GENF)

main_gen.h: main_gen.cc
user_gen.h: main_gen.cc
user_gen.cc: main_gen.cc
itq_gen.h: main_gen.cc
itq_gen.cc: main_gen.cc
packet_gen.h: main_gen.cc
packet_gen.cc: main_gen.cc
listen_gen.h: main_gen.cc
listen_gen.cc: main_gen.cc
