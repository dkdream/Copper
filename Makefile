PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

TIME := $(shell date +T=%s.%N)

COPPER     := ./copper.x
Copper.ext := cu

DIFF   = diff
CFLAGS = -g $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG -Wall

OBJS = tree.o compile.o

all : copper
new : copper-new

echo : ; echo $(TIME) $(COPPER)

copper-new : copper.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ copper.o $(OBJS)

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

copper.c : copper.cu $(COPPER)
	$(COPPER) -o $@ $<

check : copper-new .FORCE
	$(MAKE) stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function stage.one.c stage.two.c
	$(MAKE) test
	-@rm -f stage.one  stage.one.c stage.one.o stage.two stage.two.c stage.two.o
	echo PASSED

push : .FORCE
	mv copper_orig.c copper_orig.c.BAK
	cp copper.c copper_orig.c
	$(MAKE) bootstrap

test examples : copper-new .FORCE
	$(SHELL) -ec '(cd examples;  $(MAKE))'

stage.one.o : stage.one.c
stage.one.c : copper.cu copper-new
	./copper-new -v -o $@ copper.cu 2>stage.one.log

stage.one : stage.one.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ stage.one.o $(OBJS)

stage.two.o : stage.two.c
stage.two.c : copper.cu stage.one
	./stage.one -v -o $@ copper.cu 2>stage.two.log

stage.two : stage.two.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ stage.one.o $(OBJS)

clean : .FORCE
	rm -f *~ *.o copper.[cd] copper copper-new compile.inc
	rm -f stage.one  stage.one.c  stage.one.log  stage.one.o  stage.two  stage.two.c  stage.two.log  stage.two.o
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

clear : .FORCE
	rm -f my_copper.o copper-new
	rm -f stage.one  stage.one.c  stage.one.log  stage.one.o  stage.two  stage.two.c  stage.two.log  stage.two.o

scrub spotless : clean .FORCE
	rm -f copper.x ascii2hex.x
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

##
##
## patterns
##
##
##

##
##
## bootstrap
##
##

copper.x :
	$(MAKE) bootstrap

bootstrap : copper_orig.o $(OBJS)
	$(CC) $(CFLAGS) -o copper.x copper_orig.o $(OBJS)

.FORCE :
