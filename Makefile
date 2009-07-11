PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin

TIME := $(shell date +T=%s.%N)

COPPER     := ./copper.x
Copper.ext := cu

DIFF   = diff
CFLAGS = -g $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG

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
	./copper-new -v -o copper.test copper.cu 2>test_out.log
	$(DIFF) --ignore-blank-lines  --show-c-function --brief copper.test copper.c
	-@rm -f copper.test

push : .FORCE
	mv copper_orig.c copper_orig.c.BAK
	cp copper.c copper_orig.c

test examples : copper-new .FORCE
	$(SHELL) -ec '(cd examples;  $(MAKE))'

clean : .FORCE
	rm -f *~ *.o copper.[cd] copper copper-new copper.test compile.inc test_out.log
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

clear : .FORCE
	rm -f copper.test my_copper.o copper-new test_out.log

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
