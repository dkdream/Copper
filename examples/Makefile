
EXAMPLES = test rule accept dc dcv wc calc basic

COPPER     = ../copper-new
Copper.ext = cu

CFLAGS = -g
DIFF   = diff
TEE    = cat >

all : $(EXAMPLES) .FORCE
	@rm -f $(EXAMPLES:%=%.x)
	@ls -l $(EXAMPLES)

test : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

rule : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

accept : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

dc : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

dcv : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

wc : .FORCE
	@MAKELEVEL=0 make TEMPLATE=counter_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

calc : .FORCE
	@MAKELEVEL=0 make TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

basic : .FORCE
	@MAKELEVEL=0 make TEMPLATE=basic_test.c ${@:%=%.x}
	@MAKELEVEL=0 make ${@:%=%.check}

left : .FORCE
	@MAKELEVEL=0 make TEMPLATE=basic_test.c ${@:%=%.x}

thoms : .FORCE
	@MAKELEVEL=0 make TEMPLATE=thoms_test.c ${@:%=%.x}
	./thoms_test.x thoms.in

clean : .FORCE
	rm -f *~ *.o *.x *.inc *.out *.out left thoms $(EXAMPLES)

scrub spotless : clean

%.x : %.${Copper.ext}
	$(COPPER) $(COPPER_OPT) -o ${@:%.x=%.inc} $<
	sed -e 's,INCLUDE,"${@:%.x=%.inc}",' ${TEMPLATE} > ./${@:%.x=%.c}
	$(CC) $(CFLAGS) -o $@ ${@:%.x=%.c}
	rm -f ${@:%.x=%.c}

%.check : %.ref
	cat ./${@:%.check=%.input}  | ./${@:%.check=%.x} | $(TEE) ${@:%.check=%.out}
	$(DIFF) $< ${@:%.check=%.out}
	rm -f ${@:%.check=%.out}
	mv ${@:%.check=%.x} ${@:%.check=%}
	@echo

.FORCE :

