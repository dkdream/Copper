TIME    := $(shell date +T=%s.%N)
DATE    := $(shell date +%Y-%b-%d-%a-%I.%M%P)

TMP_DIR = /tmp
PREFIX	= /tools
BINDIR	= $(PREFIX)/bin
INCDIR  = $(PREFIX)/include
LIBDIR  = $(PREFIX)/lib

STAGE       := zero
COPPER      := copper.vm
COPPER.test := copper.vm
#COPPER.old  := /tools/Copper/bin/copper_o
COPPER.old  := ./copper.ovm
Copper.ext  := cu

PATH := $(PATH):.

DIFF   = diff
CC     = gcc
AR     = ar
RANLIB = ranlib

CFLAGS = -ggdb $(OFLAGS) $(XFLAGS)
OFLAGS = -Wall -W
ARFLAGS = qv

LIB_SRCS = cu_error.c cu_firstset.c cu_machine.c
LIB_OBJS = $(LIB_SRCS:%.c=%.o)
DEPENDS  = $(LIB_SRCS:%.c=.depends/%.d)

default : all

all : copper.vm

other : copper_n.vm

test : do.stage.two

install :: $(BINDIR)/copper
install :: $(INCDIR)/copper.h
install :: $(INCDIR)/static_table.h
install :: $(LIBDIR)/libCopper.a

push : #-- put the new graph under version control
	cp copper.c     copper_o.c.bootstrap
	cp main.c       main_o.c.bootstrap
	cp cu_machine.c cu_machine_o.c.bootstrap

checkpoint : ; git commit -a -m checkpoint

test.run : $(COPPER.test) ; $(MAKE) --directory=tests

err_test: copper.vm
	$(COPPER.old) --name test --file test_error.cu

$(BINDIR) : ; [ -d $@ ] || mkdir -p $@
$(INCDIR) : ; [ -d $@ ] || mkdir -p $@
$(LIBDIR) : ; [ -d $@ ] || mkdir -p $@

$(BINDIR)/copper : $(BINDIR) $(COPPER)
	cp -p $(COPPER) $@
	strip $@

$(INCDIR)/copper.h : $(INCDIR) copper.h
	cp -p copper.h $@

$(INCDIR)/static_table.h : $(INCDIR) static_table.h
	cp -p static_table.h $@

$(LIBDIR)/libCopper.a : $(LIBDIR) libCopper.a
	cp -p libCopper.a $@

copper_ver.h: FORCE
	@./Version.gen COPPER_VERSION copper_ver.h

# -- -------------------------------------------------
DEPENDS += .depends/compiler.d

libCopper.a : copper.h
libCopper.a : $(LIB_OBJS)
	-@$(RM) $@ /tmp/$@
	-@echo $(AR) $(ARFLAGS) $@ $(LIB_OBJS)
	@$(AR) $(ARFLAGS) /tmp/$@ $(LIB_OBJS)
	-@echo $(RANLIB) $@
	@$(RANLIB) /tmp/$@
	@cp /tmp/$@ $@

compiler.o : compiler.c

# -- --------------------------------------- bootstrap

DEPENDS += .depends/copper_o.d

copper.ovm : main_o.o copper_o.o cu_machine_o.o compiler_o.o cu_firstset_o.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main_o.o copper_o.o cu_machine_o.o compiler_o.o cu_firstset_o.o -L. -lCopper

main_o.o   : compiler.h copper.h main_o.c.bootstrap
	@cp main_o.c.bootstrap main_o.c
	$(CC) $(CFLAGS) -DSKIP_META -DSKIP_VERSION -DOLD_VM -I. -c -o $@ main_o.c

copper_o.o : copper.h copper_o.c.bootstrap
	@cp copper_o.c.bootstrap copper_o.c
	$(CC) $(CFLAGS) -DOLD_VM -I. -c -o $@ copper_o.c

cu_machine_o.o : copper.h copper_inline.h cu_machine_o.c.bootstrap
	@cp cu_machine_o.c.bootstrap cu_machine_o.c
	$(CC) $(CFLAGS) -DOLD_VM -I. -c -o $@ cu_machine_o.c

cu_firstset_o.o : copper.h copper_inline.h cu_firstset_o.c.bootstrap
	@cp cu_firstset_o.c.bootstrap cu_firstset_o.c
	$(CC) $(CFLAGS) -DOLD_VM -I. -c -o $@ cu_firstset_o.c

compiler_o.o : copper.h copper_inline.h compiler_o.c.bootstrap
	@cp compiler_o.c.bootstrap compiler_o.c
	$(CC) $(CFLAGS) -DOLD_VM -I. -c -o $@ compiler_o.c

# -- -------------------------------------------------
DEPENDS += .depends/main.d

copper.c : copper.cu $(COPPER.old) ; $(COPPER.old) --name copper_graph --output $@ --file copper.cu
copper.o : copper.c
main.o   : main.c

copper.vm : main.o copper.o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o copper.o compiler.o -L. -lCopper

# -- -------------------------------------------------

main_n.o   : main_n.c

copper_n.vm : main_n.o copper.o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main_n.o copper.o compiler.o -L. -lCopper

# -- -------------------------------------------------

TEMPS =
TEMPS += stage.$(STAGE).o
TEMPS += stage.$(STAGE)

current.stage : compare

stage.$(STAGE).c : copper.cu $(COPPER.test)
	@rm -f stage.$(STAGE).c
	$(COPPER.test) --name copper_graph --output $@ --file copper.cu

stage.$(STAGE).o : stage.$(STAGE).c

stage.$(STAGE) : main.o stage.$(STAGE).o compiler.o libCopper.a
	$(CC) $(CFLAGS) -o $@ main.o stage.$(STAGE).o compiler.o -L. -lCopper

compare : $(COPPER.test) stage.$(STAGE)
	@./compare_graphs.sh $(COPPER.test) stage.$(STAGE)
	@rm -f test.temp

# --

do.stage.zero : copper.vm     ; @$(MAKE) --no-print-directory STAGE=zero COPPER.test=copper.vm  current.stage
do.stage.one  : do.stage.zero ; @$(MAKE) --no-print-directory STAGE=one  COPPER.test=stage.zero current.stage
do.stage.two  : do.stage.one  ; @$(MAKE) --no-print-directory STAGE=two  COPPER.test=stage.one  current.stage

# --

clear :
	rm -f *~ *.o *.tmp
	rm -f stage.*

clean : clear
	rm -rf .depends
	rm -f copper.c copper.vm libCopper.a copper_ver.h

scrub spotless : clean
	rm -rf copper.ovm copper_o.c cu_machine_o.c main_o.c

echo ::
	@echo 'ROOT_DIR     = $(ROOT_DIR)'
	@echo 'H_SOURCES    = $(H_SOURCES)'
	@echo 'C_SOURCES    = $(C_SOURCES)'
	@echo 'WATER_TREES  = $(WATER_TREES)'
	@echo 'COPPER_TREES = $(COPPER_TREES)'
	@echo 'MAINS        = $(MAINS)'
	@echo 'DEPENDS      = $(DEPENDS)'

# --

.PHONY :: scrub
.PHONY :: spotless
.PHONY :: clean
.PHONY :: clear
.PHONY :: err_test
.PHONY :: test
.PHONY :: push
.PHONY :: checkpoint
.PHONY :: install
.PHONY :: test.run
.PHONY :: all
.PHONY :: default
.PHONY :: compare
.PHONY :: current.stage
.PHONY :: FORCE

##
## rules
##
.depends : ; @mkdir .depends

.depends/%.d : %.c .depends ; @gcc -MM -MP -MG $(CFLAGS) -I. -MF $@ $<

%.o : %.c ; $(CC) $(CFLAGS) -I. -c -o $@ $<

-include $(DEPENDS)
