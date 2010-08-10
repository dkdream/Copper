PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

COPPER.orig = ../copper-new

TIME        := $(shell date +T=%s.%N)
STAGE       := zero
COPPER      := copper.x
COPPER.test := copper.vm
Copper.ext  := cu

PATH := $(PATH):.

DIFF   = diff
CC     = gcc
CFLAGS = -ggdb $(OFLAGS) $(XFLAGS)
OFLAGS = -Wall

OBJS = syntax.o compiler.o

default : copper.vm

all : copper.vm

install : copper
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	cp -p $< $(BINDIR)/copper
	strip $(BINDIR)/copper

uninstall : .FORCE
	rm -f $(BINDIR)/copper

# -- -------------------------------------------------

compiler.o : compiler.c ; $(CC) $(CFLAGS) -c -o $@ $<
syntax.o   : syntax.c   ; $(CC) $(CFLAGS) -c -o $@ $<

compiler.o syntax.o : copper.h syntax.h

# -- -------------------------------------------------

copper_o.c : copper.cuo ; $(COPPER.orig) -A -o $@ copper.cuo
copper_o.o : copper_o.c ; $(CC) $(CFLAGS) -c -o $@ $<
main_o.o   : main.c     ; $(CC) $(CFLAGS) -DORIG_COPPER -c -o $@ $<

copper.ovm : main_o.o copper_o.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main_o.o copper_o.o $(OBJS)

main_o.o copper_o.o : copper.h syntax.h

# -- -------------------------------------------------

copper.c : copper.cu ./copper.ovm ; ./copper.ovm --name copper_graph --output $@ --file copper.cu
copper.o : copper.c               ; $(CC) $(CFLAGS) -I. -c -o $@ $<
main.o   : main.c                 ; $(CC) $(CFLAGS) -I. -c -o $@ $<

copper.vm : main.o copper.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main.o copper.o $(OBJS)

copper.o main.o : copper.h syntax.h

# -- -------------------------------------------------

TEMPS =
TEMPS += stage.$(STAGE).o
TEMPS += stage.$(STAGE)

current.stage : $(TEMPS)

stage.$(STAGE).c : copper.cu $(COPPER.test)
	@rm -f stage.$(STAGE).c
	$(COPPER.test) --verbose --name copper_graph --output $@ --file copper.cu

stage.$(STAGE).o : stage.$(STAGE).c
	$(CC) $(CFLAGS) -I. -c -o $@ $<

stage.$(STAGE) : main.o stage.$(STAGE).o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+

stage.$(STAGE).o : copper.h syntax.h

# --

do.stage.zero : copper.vm     ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper.vm current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one  COPPER.test=stage.zero current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two  COPPER.test=stage.one  current.stage

# --

clean : .FORCE
	rm -f *~ *.o
	rm -f stage.*
	rm -f copper_o.c copper.ovm
	rm -f copper.c copper.vm
	echo $(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f *~ *.o
	rm -f stage.*
	rm -f copper.c copper.vm
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
