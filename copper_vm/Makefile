PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

TIME        := $(shell date +T=%s.%N)
STAGE       := zero
COPPER      := copper.vm
COPPER.test := copper.vm
Copper.ext  := cu

PATH := $(PATH):.

DIFF   = diff
CC     = gcc
CFLAGS = -ggdb $(OFLAGS) $(XFLAGS)
OFLAGS = -Wall

OBJS = cu_engine.o compiler.o

default : copper.vm

all : copper.vm

full : do.stage.two 

install : $(COPPER)
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	cp -p $< $(BINDIR)/copper
	strip $(BINDIR)/copper

uninstall : .FORCE
	rm -f $(BINDIR)/copper

push : .FORCE #-- put the new graph under version control
	cp copper.c

test examples : $(COPPER.test) .FORCE
        $(MAKE) --directory=examples

# -- -------------------------------------------------

compiler.o  : compiler.c ; $(CC) $(CFLAGS) -c -o $@ $<
cu_engine.o : cu_engine.c ; $(CC) $(CFLAGS) -c -o $@ $<

cu_engine.o : copper.h
compiler.o  : copper.h compiler.h

# -- -------------------------------------------------

copper_o.c : copper_o.c.bootstrap ; cp $< $@
copper_o.o : copper_o.c           ; $(CC) $(CFLAGS) -c -o $@ $<

copper.ovm : main.o copper_o.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main.o copper_o.o $(OBJS)

copper_o.o : copper.h compiler.h

# -- -------------------------------------------------

copper.c : copper.cu ./copper.ovm ; ./copper.ovm --name copper_graph --output $@ --file copper.cu
copper.o : copper.c               ; $(CC) $(CFLAGS) -I. -c -o $@ $<
main.o   : main.c                 ; $(CC) $(CFLAGS) -I. -c -o $@ $<

copper.vm : main.o copper.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main.o copper.o $(OBJS)

copper.o main.o : copper.h compiler.h

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

stage.$(STAGE) : main.o stage.$(STAGE).o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+

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
	rm -f copper_o.c copper.ovm
	rm -f copper.c copper.vm
	echo $(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f *~ *.o *.tmp
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
