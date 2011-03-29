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

LIB_SRCS = $(wildcard cu_*.c)
LIB_OBJS = $(LIB_SRCS:%.c=%.o)
CU_OBJS  = compiler.o
DEPENDS  = $(LIB_SRCS:%.c=.depends/%.d)

default : copper.vm

all : copper.vm copper_n.vm

full : do.stage.two 

install :: $(BINDIR)/copper
install :: $(INCDIR)/copper.h
install :: $(INCDIR)/static_table.h
install :: $(LIBDIR)/libCopper.a

push : #-- put the new graph under version control
	cp copper.c copper_o.c.bootstrap

checkpoint : ; git checkpoint

examples : $(COPPER.test) ; $(MAKE) --directory=examples

test : $(COPPER.test) ; $(MAKE) --directory=tests

err_test: copper.vm
	./copper.ovm --name test --file test_error.cu


$(BINDIR) : ; [ -d $@ ] || mkdir -p $@
$(INCDIR) : ; [ -d $@ ] || mkdir -p $@
$(LIBDIR) : ; [ -d $@ ] || mkdir -p $@

$(BINDIR)/copper : $(BINDIR) $(COPPER)
	cp -p $(COPPER) $@
	strip $@

$(INCDIR)/copper.h : $(INCDIR) copper.h
	cp -p copper.h $@

$(INCDIR)/static_table.h : $(INCDIR) static_table.h
	cp -p static_table.h $@

$(LIBDIR)/libCopper.a : $(LIBDIR) libCopper.a
	cp -p libCopper.a $@


# -- -------------------------------------------------
DEPENDS += .depends/compiler.d

libCopper.a : copper.h
libCopper.a : $(LIB_OBJS)
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)
	$(RANLIB) $@

compiler.o  : compiler.c

# -- --------------------------------------- bootstrap

DEPENDS += .depends/copper_o.d

copper_o.o : copper_o.c.bootstrap copper.h
copper.ovm : main_o.o copper_o.o cu_engine_o.o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main_o.o copper_o.o cu_engine_o.o $(CU_OBJS) -L. -lCopper

main_o.o   : main.c compiler.h copper.h
	$(CC) $(CFLAGS) -DSKIP_META -I. -c -o $@ $<

copper_o.o : copper_o.c.bootstrap
	@cp copper_o.c.bootstrap copper_o.c
	$(CC) $(CFLAGS) -I. -c -o $@ copper_o.c

cu_engine_o.o : main.c compiler.h copper.h cu_engine.c
	$(CC) $(CFLAGS) -DOLD_VM -I. -c -o $@ cu_engine.c

# -- -------------------------------------------------
DEPENDS += .depends/main.d

copper.c : copper.cu ./copper.ovm ; ./copper.ovm --name copper_graph --output $@ --file copper.cu
copper.o : copper.c
main.o   : main.c

copper.vm : main.o copper.o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o copper.o $(CU_OBJS) -L. -lCopper

# -- -------------------------------------------------
main_n.o   : main_n.c

copper_n.vm : main_n.o copper.o $(CU_OBJS) libCopper.a
	$(CC) $(CFLAGS) -o $@ main_n.o copper.o $(CU_OBJS) -L. -lCopper

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
.PHONY :: checkpoint
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