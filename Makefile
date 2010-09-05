PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

TIME        := $(shell date +T=%s.%N)
STAGE       := zero
COPPER      := copper.vm
COPPER.test := copper.vm
Copper.ext  := cu

PATH := $(PATH):.

DIFF   = diff
CC     = gcc
AR     = ar
RANLIB = ranlib

CFLAGS = -ggdb $(OFLAGS) $(XFLAGS)
OFLAGS = -Wall
ARFLAGS = rcu

LIB_OBJS = cu_engine.o cu_firstset.o
CU_OBJS  = compiler.o
DEPENDS = 

default : copper.vm

all : copper.vm

full : do.stage.two 

install : $(COPPER) copper.h libCopper.a
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	[ -d $(INCDIR) ] || mkdir -p $(INCDIR)
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	cp -p $< $(BINDIR)/copper
	strip $(BINDIR)/copper
	cp -p copper.h    $(INCDIR)/copper.h
	cp -p libCopper.a $(LIBDIR)/libCopper.a

push : #-- put the new graph under version control
	cp copper.c copper_o.c.bootstrap

examples : $(COPPER.test) ; $(MAKE) --directory=examples

test : $(COPPER.test) ; $(MAKE) --directory=tests

err_test: copper.vm
	./copper.ovm --name test --file test_error.cu

# -- -------------------------------------------------
DEPENDS += .depends/cu_engine.d
DEPENDS += .depends/cu_firstset.d
DEPENDS += .depends/compiler.d

cu_engine.o   : cu_engine.c
cu_firstset.o : cu_firstset.c

libCopper.a : copper.h
libCopper.a : $(LIB_OBJS)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)
	$(RANLIB) $@

compiler.o  : compiler.c

# -- --------------------------------------- bootstrap

DEPENDS += .depends/copper_o.d

copper_o.c : copper_o.c.bootstrap ; cp $< $@
copper_o.o : copper_o.c

copper.ovm : main_o.o copper_o.o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main_o.o copper_o.o $(CU_OBJS) -L. -lCopper

main_o.o : main.c ; $(CC) $(CFLAGS) -DSKIP_META -I. -c -o $@ $<

# -- -------------------------------------------------
DEPENDS += .depends/main.d

copper.c : copper.cu ./copper.ovm ; ./copper.ovm --name copper_graph --output $@ --file copper.cu
copper.o : copper.c
main.o   : main.c

copper.vm : main.o copper.o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o copper.o $(CU_OBJS) -L. -lCopper

# -- -------------------------------------------------

TEMPS =
TEMPS += stage.$(STAGE).o
TEMPS += stage.$(STAGE)

current.stage : compare

stage.$(STAGE).c : copper.cu $(COPPER.test)
	@rm -f stage.$(STAGE).c
	$(COPPER.test) --name copper_graph --output $@ --file copper.cu

stage.$(STAGE).o : stage.$(STAGE).c

stage.$(STAGE) : main.o stage.$(STAGE).o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o stage.$(STAGE).o $(CU_OBJS) -L. -lCopper

compare : $(COPPER.test) stage.$(STAGE)
	@./compare_graphs.sh $(COPPER.test) stage.$(STAGE)
	@rm -f test.temp

# --

do.stage.zero : copper.vm     ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper.vm  current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one  COPPER.test=stage.zero current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two  COPPER.test=stage.one  current.stage

# --

clear :
	rm -f *~ *.o *.tmp
	rm -f stage.*
	$(MAKE) --directory=examples --no-print-directory $@

clean : clear
	rm -rf .depends
	rm -f copper.c copper.vm libCopper.a
	echo $(MAKE) --directory=examples --no-print-directory $@

scrub spotless : clean
	rm -rf copper.ovm copper_o.c
	$(MAKE) --directory=examples --no-print-directory $@


# --

.PHONY :: scrub
.PHONY :: spotless
.PHONY :: clean
.PHONY :: clear
.PHONY :: err_test
.PHONY :: test
.PHONY :: examples
.PHONY :: push
.PHONY :: install
.PHONY :: full
.PHONY :: all
.PHONY :: default
.PHONY :: compare
.PHONY :: current.stage

##
## rules
##
.depends : ; @mkdir .depends

.depends/%.d : %.c .depends ; @gcc -MM -MP -MG $(CFLAGS) -I. -MF $@ $<

%.o : %.c ; $(CC) $(CFLAGS) -I. -c -o $@ $<

-include $(DEPENDS)