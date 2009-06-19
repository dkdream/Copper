
PREFIX	= /tools/Leg
BINDIR	= $(PREFIX)/bin

LEG    = ./leg.x
DIFF   = diff
CFLAGS = -g $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG
#OFLAGS = -pg

TIME := $(shell date +T=%s.%N)

OBJS = tree.o compile.o

all : leg

echo : ; echo $(TIME)

leg-new : my_leg.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ my_leg.o $(OBJS)

leg : check
	cp leg-new leg

install : $(BINDIR)/leg

uninstall : .FORCE
	rm -f $(BINDIR)/leg

compile.o : compile.inc

compile.inc : ascii2hex.x header.var footer.var preamble.var
	./ascii2hex.x -lheader   header.var >$@
	./ascii2hex.x -lfooter   footer.var >>$@
	./ascii2hex.x -lpreamble preamble.var >>$@

ascii2hex.x : ascii2hex.c
	$(CC) $(CFLAGS) -o $@ $<
	@./test_ascii2hex.sh "$(CC) $(CFLAGS)"

my_leg.c : my.leg $(LEG)
	$(LEG) -o $@ $<

check : leg-new .FORCE
	./leg-new -v -o leg.test my.leg 2>test_out.log
	$(DIFF) leg.test my_leg.c
	-@rm -f leg.test

push : .FORCE
	mv leg_orig.c leg_orig.c.BAK
	cp my_leg.c leg_orig.c

test examples : leg-new .FORCE
	$(SHELL) -ec '(cd examples;  $(MAKE))'

clean : .FORCE
	rm -f *~ *.o *_leg.[cd] leg leg-new leg.test compile.inc test_out.err 
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

clear : .FORCE
	rm -f leg.test my_leg.o leg-new test_out.log

scrub spotless : clean .FORCE
	rm -f leg.x ascii2hex.x
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

leg.x : leg_orig.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ leg_orig.o $(OBJS)

.FORCE :
