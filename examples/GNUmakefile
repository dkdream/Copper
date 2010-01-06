
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
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

rule : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

accept : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

dc : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

dcv : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

wc : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=counter_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

calc : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=simple_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

basic : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=basic_test.c ${@:%=%.x}
	@$(MAKE) --no-print-directory ${@:%=%.check}

left : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=basic_test.c ${@:%=%.x}

thoms : .FORCE
	@$(MAKE) --no-print-directory TEMPLATE=thoms_test.c ${@:%=%.x}
	./thoms_test.x thoms.in

clear : .FORCE
	-@rm -f *~ *.o *.x *.inc *.out *.out

clean : .FORCE
	-@rm -f *~ *.o *.x *.inc *.out *.out left thoms $(EXAMPLES) $(EXAMPLES:%=%.c)

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

