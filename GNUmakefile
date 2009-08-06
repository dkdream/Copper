PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

TIME        := $(shell date +T=%s.%N)
STAGE       := zero
COPPER      := copper.x
COPPER.test := copper-new
Copper.ext  := cu

PATH := $(PATH):.

DIFF   = diff
CC     = gcc
CFLAGS = -ggdb $(OFLAGS) $(XFLAGS)
OFLAGS = -Wall

OBJS = tree.o compile.o copper_main.o

default :
	-@$(MAKE) --no-print-directory clear all || true

all : copper
new : copper-new

echo : ; echo $(TIME) $(COPPER)

copper : check
	cp copper-new copper

install : copper
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	cp -p $< $(BINDIR)/copper
	strip $(BINDIR)/copper

uninstall : .FORCE
	rm -f $(BINDIR)/copper

compile.o : compile.inc

compile.inc : ascii2hex.x header.var footer.var preamble.var
	./ascii2hex.x -lheader   header.var >$@
	./ascii2hex.x -lfooter   footer.var >>$@
	./ascii2hex.x -lpreamble preamble.var >>$@

ascii2hex.x : ascii2hex.c
	$(CC) $(CFLAGS) -o $@ $<
	@./test_ascii2hex.sh "$(CC) $(CFLAGS)"

check : copper-new .FORCE
	-@rm -f stage.*
	$(MAKE) do.stage.two 
	$(DIFF) --ignore-blank-lines  --show-c-function stage.zero.c stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function stage.zero.inc stage.two.inc
	$(DIFF) --ignore-blank-lines  --show-c-function stage.zero.heading stage.two.heading
	$(DIFF) --ignore-blank-lines  --show-c-function stage.zero.footing stage.two.footing
	$(DIFF) --ignore-blank-lines  --show-c-function copper.c stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function header.inc stage.two.inc
	$(MAKE) test
	-@rm -f stage.*
	@echo PASSED

push : .FORCE
	-@cmp --quiet header.inc header.bootstrap.inc || cp header.inc header.bootstrap.inc
	-@cmp --quiet copper.c copper.bootstrap.c     || cp copper.c copper.bootstrap.c
	$(MAKE) bootstrap

test examples : copper-new .FORCE
	$(MAKE) --directory=examples


# --

copper.c   : copper.cu $(COPPER) ; $(COPPER) -v -R header.inc -o $@ copper.cu 2>copper.log
copper.o   : copper.c            ; $(CC) $(CFLAGS) -c -o $@ $<
copper-new : copper.o $(OBJS)    ; $(CC) $(CFLAGS) -o $@ copper.o $(OBJS)

# --
current.stage : stage.$(STAGE) stage.$(STAGE).heading stage.$(STAGE).footing

stage.$(STAGE).c stage.$(STAGE).inc : copper.cu $(COPPER.test)
	$(COPPER.test) -v -R stage.$(STAGE).inc -o stage.$(STAGE).c copper.cu 2>stage.$(STAGE).log

stage.$(STAGE).heading : copper.cu $(COPPER.test)
	$(COPPER.test) -H -o $@  2>>stage.$(STAGE).log

stage.$(STAGE).footing : copper.cu $(COPPER.test)
	$(COPPER.test) -F -o $@  2>>stage.$(STAGE).log

stage.$(STAGE).o : stage.$(STAGE).c stage.$(STAGE).inc
	$(CC) $(CFLAGS) -c -o $@ $<

stage.$(STAGE) : stage.$(STAGE).o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+

# --

do.stage.zero : copper-new    ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper-new current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one COPPER.test=stage.zero  current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two COPPER.test=stage.one  current.stage

# --

clean : .FORCE
	rm -f *~ *.o copper copper-new compile.inc
	rm -f copper.c header.inc copper.log
	rm -f stage.*
	$(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f copper.o copper-new header.one header.two
	rm -f stage.*
	$(MAKE) --directory=examples --no-print-directory $@

scrub spotless : clean .FORCE
	rm -f copper.x ascii2hex.x copper.[cd]
	$(MAKE) --directory=examples --no-print-directory $@

##
##
## bootstrap
##
##

copper.x : ; $(MAKE) bootstrap

bootstrap : copper.bootstrap.o $(OBJS)
	$(CC) $(CFLAGS) -o copper.x $+

copper.bootstrap.o : copper.bootstrap.c header.bootstrap.inc
	$(CC) $(CFLAGS) -DSTAGE=\"header.bootstrap.inc\" -c -o $@ $<

.FORCE :
