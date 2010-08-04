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

MAIN_ORIG = copper_main.o
OBJS      = tree.o compile.o compile_vm.o

default : copper.old.vm copper.new.vm

all : copper     copper.old.vm copper.new.vm
new : copper-new copper.oldvm copper.new.vm

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
	./ascii2hex.x -x -lheader   header.var >$@
	./ascii2hex.x -x -lpreamble preamble.var >>$@
	./ascii2hex.x -x -lfooter   footer.var >>$@

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
	$(DIFF) --ignore-blank-lines  --show-c-function stage.zero.all.c stage.two.all.c
	$(MAKE) test
	@echo PASSED
	@echo Show differences
	-@$(DIFF) --ignore-blank-lines  --show-c-function  copper.c stage.two.c
	-@$(DIFF) --ignore-blank-lines  --show-c-function  copper.c copper.bootstrap.c
	@cmp --quiet copper.c copper.bootstrap.c || ( echo; echo Ready to push; echo )

push : .FORCE
	-@cp copper.bootstrap.c copper.bootstrap.c.Last
	-@cmp --quiet copper.c copper.bootstrap.c || echo cp copper.c copper.bootstrap.c
	-@cmp --quiet copper.c copper.bootstrap.c || cp copper.c copper.bootstrap.c
	$(MAKE) bootstrap

test examples : copper-new .FORCE
	$(MAKE) --directory=examples


# --

copper.c     : copper.cu $(COPPER)           ; $(COPPER) -C -s -o $@ copper.cu
copper.o     : copper.c                      ; $(CC) $(CFLAGS) -c -o $@ $<
copper-new   : $(MAIN_ORIG) copper.o $(OBJS) ; $(CC) $(CFLAGS) -o $@ $(MAIN_ORIG) copper.o $(OBJS)

copper.o $(OBJS) : header.var

# --

vm_copper.old.c : copper.cu     $(COPPER.test) ; $(COPPER.test) -A -o $@ copper.cu
vm_copper.new.c : copper_vm.cu  $(COPPER.test) ; $(COPPER.test) -A -o $@ copper_vm.cu

vm_copper.old.o : vm_copper.old.c ; $(CC) $(CFLAGS) -c -DTESTING_PARSER -o $@ $<
vm_copper.new.o : vm_copper.new.c ; $(CC) $(CFLAGS) -c -o $@ $<

main_vm.old.o  : main_vm.c ; $(CC) $(CFLAGS) -DTESTING_PARSER -c -o $@ $<
main_vm.new.o  : main_vm.c ; $(CC) $(CFLAGS) -c -o $@ $<

vm_compiler.o : vm_compiler.c ; $(CC) $(CFLAGS) -c -o $@ $<

copper.old.vm : main_vm.old.o vm_copper.old.o copper_vm.o
	 $(CC) $(CFLAGS) -o $@ main_vm.old.o vm_copper.old.o copper_vm.o

copper.new.vm : main_vm.new.o vm_copper.new.o copper_vm.o vm_compiler.o
	$(CC) $(CFLAGS) -o $@ main_vm.new.o vm_copper.new.o copper_vm.o vm_compiler.o

main_vm.new.o main_vm.old.o vm_copper.new.o vm_copper.old.o copper_vm.o vm_compiler.o : copper_vm.h syntax.h

# --
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
	rm -f *~ *.o copper copper-new copper.old.vm copper.new.vm
	rm -f copper.c vm_copper.old.c vm_copper.new.c header.inc copper.log  compile.inc
	rm -f stage.*
	$(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f *.o copper-new copper.old.vm copper.new.vm
	rm -f copper.c vm_copper.old.c vm_copper.new.c
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
	$(CC) $(CFLAGS) -DCOPPER_BOOTSTRAP -o copper.x $+

copper.bootstrap.o : copper.bootstrap.c
	$(CC) $(CFLAGS) -DCOPPER_BOOTSTRAP -c -o $@ $<

.FORCE :
