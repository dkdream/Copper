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

OBJS = cu_engine.o compiler.o

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

uninstall : .FORCE
	rm -f $(BINDIR)/copper

push : .FORCE #-- put the new graph under version control
	cp copper.c copper_o.c.bootstrap

test examples : $(COPPER.test) .FORCE
        $(MAKE) --directory=examples

# -- -------------------------------------------------

cu_engine.o : cu_engine.c ; $(CC) $(CFLAGS) -I. -c -o $@ $<
cu_engine.o : copper.h

libCopper.a : copper.h
libCopper.a : cu_engine.o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ cu_engine.o
	$(RANLIB) $@

compiler.o  : compiler.c  ; $(CC) $(CFLAGS) -I. -c -o $@ $<
compiler.o  : copper.h compiler.h

# -- -------------------------------------------------

copper_o.c : copper_o.c.bootstrap ; cp $< $@
copper_o.o : copper_o.c           ; $(CC) $(CFLAGS) -I. -c -o $@ $<
copper_o.o : copper.h compiler.h

copper.ovm : main.o copper_o.o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o copper_o.o compiler.o -L. -lCopper

# -- -------------------------------------------------

copper.c : copper.cu ./copper.ovm ; ./copper.ovm --name copper_graph --output $@ --file copper.cu
copper.o : copper.c               ; $(CC) $(CFLAGS) -I. -c -o $@ $<
main.o   : main.c                 ; $(CC) $(CFLAGS) -I. -c -o $@ $<

copper.o main.o : copper.h compiler.h

copper.vm : main.o copper.o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o copper.o compiler.o -L. -lCopper

# -- -------------------------------------------------

TEMPS =
TEMPS += stage.$(STAGE).o
TEMPS += stage.$(STAGE)

current.stage : compare

stage.$(STAGE).c : copper.cu $(COPPER.test)
	@rm -f stage.$(STAGE).c
	$(COPPER.test) --name copper_graph --output $@ --file copper.cu

stage.$(STAGE).o : stage.$(STAGE).c
	$(CC) $(CFLAGS) -I. -c -o $@ $<

stage.$(STAGE) : main.o stage.$(STAGE).o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o stage.$(STAGE).o compiler.o -L. -lCopper

stage.$(STAGE).o : copper.h compiler.h

compare : $(COPPER.test) stage.$(STAGE) .FORCE
	@./compare_graphs.sh $(COPPER.test) stage.$(STAGE)
	@rm -f test.temp

# --

do.stage.zero : copper.vm     ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper.vm current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one  COPPER.test=stage.zero current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two  COPPER.test=stage.one  current.stage

# --

clean : .FORCE
	rm -f *~ *.o *.tmp
	rm -f stage.*
	rm -f copper_o.c copper.ovm libCopper.a
	rm -f copper.c copper.vm
	echo $(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f *~ *.o *.tmp
	rm -f stage.*
	rm -f copper.c copper.vm libCopper.a
	$(MAKE) --directory=examples --no-print-directory $@

scrub spotless : clean .FORCE
	$(MAKE) --directory=examples --no-print-directory $@

##
##
## bootstrap
##
##

copper.x : ; $(MAKE) bootstrap

bootstrap : copper.bootstrap.o $(OBJS)
	$(CC) $(CFLAGS) -DCOPPER_BOOTSTRAP -o copper.x $+

copper.bootstrap.o : copper.bootstrap.c
	$(CC) $(CFLAGS) -DCOPPER_BOOTSTRAP -c -o $@ $<

.FORCE :
