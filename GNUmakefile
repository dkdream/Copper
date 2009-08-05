PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

TIME := $(shell date +T=%s.%N)

COPPER     := ./copper.x
Copper.ext := cu

DIFF   = diff
CFLAGS = -g $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG -Wall

OBJS = tree.o compile.o copper_main.o

default : ; $(MAKE) --no-print-directory --keep-going clear all

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
	$(MAKE) stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function stage.one.c stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function header.one header.two
	$(DIFF) --ignore-blank-lines  --show-c-function footing.one footing.two
	$(DIFF) --ignore-blank-lines  --show-c-function copper.c stage.two.c
	$(DIFF) --ignore-blank-lines  --show-c-function header.inc header.two
	$(MAKE) test
	-@rm -f stage.one stage.one.c stage.one.o header.one footing.one
	-@rm -f stage.two stage.two.c stage.two.o header.two footing.two
	echo PASSED

push : .FORCE
	mv header_orig.inc header_orig.inc.BAK
	mv copper_orig.c copper_orig.c.BAK
	mv header.inc header_orig.inc
	cp copper.c copper_orig.c
	$(MAKE) bootstrap

test examples : copper-new .FORCE
	$(MAKE) --directory=examples


# --

copper.c : copper.cu $(COPPER)
	$(COPPER) -v -Hheader.inc -o $@ copper.cu 2>copper.log

copper.o : copper.c
	$(CC) $(CFLAGS) -DSTAGE_ZERO -c -o $@ $<

copper-new : copper.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ copper.o $(OBJS)

# --

stage.one.c : copper.cu copper-new
	./copper-new -v -Hheader.one -o $@ copper.cu 2>stage.one.log
	./copper-new -v -F -o footing.one 2>>stage.one.log

stage.one.o : stage.one.c
	$(CC) $(CFLAGS) -DSTAGE_ONE -c -o $@ $<

stage.one : stage.one.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ stage.one.o $(OBJS)

# --

stage.two.c : copper.cu stage.one
	./stage.one -Hheader.two -o $@ copper.cu
	./stage.one -F -o footing.two copper.cu

stage.two.o : stage.two.c
	$(CC) $(CFLAGS) -DSTAGE_TWO -c -o $@ $<

stage.two : stage.two.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ stage.one.o $(OBJS)

# --

clean : .FORCE
	rm -f *~ *.o copper copper-new compile.inc
	rm -f copper.c header.inc copper.log
	rm -f stage.one  stage.one.c stage.one.log stage.one.o header.one footing.one
	rm -f stage.two  stage.two.c stage.two.log stage.two.o header.two footing.one
	$(MAKE) --directory=examples --no-print-directory $@

clear : .FORCE
	rm -f copper.o copper-new header.one header.two
	rm -f stage.one  stage.one.c  stage.one.log  stage.one.o
	rm -f stage.two  stage.two.c  stage.two.log  stage.two.o
	$(MAKE) --directory=examples --no-print-directory $@

scrub spotless : clean .FORCE
	rm -f copper.x ascii2hex.x copper.[cd]
	$(MAKE) --directory=examples --no-print-directory $@

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

copper_orig.o : copper_orig.c
	$(CC) $(CFLAGS) -DSTAGE_BOOTSTRAP -c -o $@ $<

.FORCE :
