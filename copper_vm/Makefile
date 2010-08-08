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

MAIN_ORIG = main_vm.c
OBJS      = copper.o copper_vm.o compiler.o

default : copper.vm

all : copper.vm

install : copper
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	cp -p $< $(BINDIR)/copper
	strip $(BINDIR)/copper

uninstall : .FORCE
	rm -f $(BINDIR)/copper

# --

copper.c    : copper.cu   ; $(COPPER.orig) -A -o $@ copper.cu
copper.o    : copper.c    ; $(CC) $(CFLAGS) -c -o $@ $<
main_vm.o   : main_vm.c   ; $(CC) $(CFLAGS) -c -o $@ $<
compiler.o  : compiler.c  ; $(CC) $(CFLAGS) -c -o $@ $<
copper_vm.o : copper_vm.c ; $(CC) $(CFLAGS) -c -o $@ $<

copper.vm : $(MAIN_ORIG) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_ORIG) $(OBJS)

$(MAIN_ORIG) $(OBJS) : copper_vm.h syntax.h






# -- -------------------------------------------------
TEMPS =
TEMPS += stage.$(STAGE).heading
TEMPS += stage.$(STAGE).preamble
TEMPS += stage.$(STAGE).footing
TEMPS += stage.$(STAGE).all.o
TEMPS += stage.$(STAGE)

current.stage : $(TEMPS)

stage.$(STAGE).c stage.$(STAGE).inc : copper.cu $(COPPER.test)
	@rm -f stage.$(STAGE).c stage.$(STAGE).inc 
	$(COPPER.test) -C -s -x -r stage.$(STAGE).inc -o stage.$(STAGE).c copper.cu

stage.$(STAGE).all.c : copper.cu $(COPPER.test)
	$(COPPER.test) -C -o $@ $<

stage.$(STAGE).heading : copper.cu $(COPPER.test)
	$(COPPER.test) -H -o $@

stage.$(STAGE).preamble : copper.cu $(COPPER.test)
	$(COPPER.test) -P -o $@

stage.$(STAGE).footing : copper.cu $(COPPER.test)
	$(COPPER.test) -F -o $@

stage.$(STAGE).o : stage.$(STAGE).c stage.$(STAGE).inc
	$(CC) $(CFLAGS) -c -o $@ $<

stage.$(STAGE) : $(MAIN_ORIG) stage.$(STAGE).o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+

# --

do.stage.zero : copper-new    ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper-new current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one  COPPER.test=stage.zero current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two  COPPER.test=stage.one  current.stage

# --

clean : .FORCE
	rm -f *~ *.o copper.vm copper.c
	rm -f stage.*
	echo $(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f *~ *.o copper.vm copper.c
	rm -f stage.*
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
