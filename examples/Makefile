
EXAMPLES = test rule accept dc dcv wc calc basic

COPPER     = ../copper-new
Copper.ext = cu

CFLAGS = -g -Wall
DIFF   = diff
TEE    = cat >

all : $(EXAMPLES) .FORCE
	@rm -f $(EXAMPLES:%=%.x)
	@ls -l $(EXAMPLES)

test : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

rule : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

accept : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

dc : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

dcv : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

wc : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=counter_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

calc : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=simple_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

basic : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=basic_test.c ${@:%=%.x}
	@MAKELEVEL=0 $(MAKE) ${@:%=%.check}

left : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=basic_test.c ${@:%=%.x}

thoms : .FORCE
	@MAKELEVEL=0 $(MAKE) TEMPLATE=thoms_test.c ${@:%=%.x}
	./thoms_test.x thoms.in

clean : .FORCE
	rm -f *~ *.o *.x *.inc *.out *.out left thoms $(EXAMPLES)

scrub spotless : clean

%.x : %.${Copper.ext}
	@$(COPPER) $(COPPER_OPT) -o ${@:%.x=%.inc} $< 2>/dev/null
	@sed -e 's,INCLUDE,"${@:%.x=%.inc}",' ${TEMPLATE} > ./${@:%.x=%.c}
	@$(CC) $(CFLAGS) -o $@ ${@:%.x=%.c}
	@rm -f ${@:%.x=%.c}

%.check : %.ref
	@cat ./${@:%.check=%.input}  | ./${@:%.check=%.x} | $(TEE) ${@:%.check=%.out}
	$(DIFF) --brief $< ${@:%.check=%.out}
	@rm -f ${@:%.check=%.out}
	@mv ${@:%.check=%.x} ${@:%.check=%}

.FORCE :

