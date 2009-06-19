
EXAMPLES = test rule accept dc dcv wc calc basic

CFLAGS = -g

DIFF = diff
LEG  = ../leg-new
TEE = cat >

all : $(EXAMPLES) .FORCE
	@ls -l $(EXAMPLES)

test : .FORCE
	$(LEG) $(LEG_OPT) -o test_leg.inc test.leg
	$(CC) $(CFLAGS) -DLEG -o ./$@ test.c
	echo 'ab.ac.ad.ae.afg.afh.afg.afh.afi.afj.' | ./$@ | $(TEE) $@.out
	$(DIFF) test.ref $@.out
	rm -f $@.out
	@echo

thoms : .FORCE
	rm -f thoms_test_leg.c thoms_test
	$(LEG) $(LEG_OPT) -o thoms_test_leg.c thoms_test.leg 
	$(CC) $(CFLAGS) -o thoms_test thoms_test_leg.c
	./thoms_test thoms.in

rule : .FORCE
	$(LEG) $(LEG_OPT) -o rule_leg.inc rule.leg
	$(CC) $(CFLAGS) -DLEG -o ./$@ rule.c
	echo 'abcbcdabcbcdabcbcdabcbcd' | ./$@ | $(TEE) $@.out
	$(DIFF) rule.ref $@.out
	rm -f $@.out
	@echo

accept : .FORCE
	$(LEG) $(LEG_OPT) -o accept_leg.inc accept.leg
	$(CC) $(CFLAGS) -DLEG -o ./$@ accept.c
	echo 'abcbcdabcbcdabcbcdabcbcd' | ./$@ | $(TEE) $@.out
	$(DIFF) accept.ref $@.out
	rm -f $@.out
	@echo

dc : .FORCE
	$(LEG) $(LEG_OPT) -o dc_leg.inc dc.leg
	$(CC) $(CFLAGS) -DLEG -o ./$@ dc.c
	echo '  2  *3 *(3+ 4) ' | ./$@ | $(TEE) $@.out
	$(DIFF) dc.ref $@.out
	rm -f $@.out
	@echo

dcv : .FORCE
	$(LEG) $(LEG_OPT) -o dcv_leg.inc dcv.leg
	$(CC) $(CFLAGS) -DLEG -o ./$@ dcv.c
	echo 'a = 6;  b = 7;  a * b' | ./$@ | $(TEE) $@.out
	$(DIFF) dcv.ref $@.out
	rm -f $@.out
	@echo

wc : .FORCE
	$(LEG) $(LEG_OPT) -o wc_leg.c wc.leg
	$(CC) $(CFLAGS) -o wc wc_leg.c
	cat wc.leg | ./$@ | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

calc : .FORCE
	$(LEG) $(LEG_OPT) -o calc_leg.c calc.leg
	$(CC) $(CFLAGS) -o calc calc_leg.c
#	echo 'a = 6;  b = 7;  a * b' | ./calc | $(TEE) $@.out
	cat calc.input | ./calc | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

basic : .FORCE
	$(LEG) $(LEG_OPT) -o basic_leg.c basic.leg
	$(CC) $(CFLAGS) -o basic basic_leg.c
	( echo 'load "test"'; echo "run" ) | ./basic | $(TEE) $@.out
	$(DIFF) $@.ref $@.out
	rm -f $@.out
	@echo

left : .FORCE
	$(LEG) $(LEG_OPT) -o $@_leg.inc $@.leg
	$(CC) $(CFLAGS) -o ./$@ $@.c
	@echo

javascript : .FORCE
	$(LEG) $(LEG_OPT) -o javascript_leg.c javascript.leg 2>&1
	$(CC) $(CFLAGS) -o javascript javascript_leg.c
	cat test_javascript.js | ./javascript | $(TEE) $@.out

clean : .FORCE
	rm -f *~ *.o *_peg.inc *_leg.inc *.out *_leg.c *.out left javascript thoms $(EXAMPLES)

scrub spotless : clean

.FORCE :

