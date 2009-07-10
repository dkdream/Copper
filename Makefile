
COPPER     = ./copper.x
Copper.ext = cu

PREFIX	= /tools/Copper
BINDIR	= $(PREFIX)/bin


DIFF   = diff
CFLAGS = -g $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG
#OFLAGS = -pg

TIME := $(shell date +T=%s.%N)

OBJS = tree.o compile.o

all : copper

echo : ; echo $(TIME)

copper-new : copper.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ copper.o $(OBJS)

copper : check
	cp copper-new copper

install : $(BINDIR)/copper

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
	$(DIFF) copper.test copper.c
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

$(BINDIR)/% : %
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	cp -p $< $@
	strip $@

##
##
## bootstrap
##
##

copper.x : copper_orig.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ copper_orig.o $(OBJS)

.FORCE :
