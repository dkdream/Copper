
COPPER.test  := ../copper.vm
COPPER.flags :=

Test_files := $(wildcard *.cu)

TFLAGS := -f 'Time %E KTime %S UTime %U PFaults %F Swapped %W CSwitch %w Signals %k Memory %M Exit %x :: %C'

all test : $(Test_files:%.cu=%.tmp)
	@ls -l $(Test_files:%.cu=%.tmp)
	@echo >> Times.list
	@cat Times.list

clean :       ; rm -f $(Test_files:%.cu=%.tmp)
scrub : clean ; rm -f Times.list

##
## Suffix Rules ------------------------------------------------------------------------
##

%.tmp : %.cu
	@echo $(COPPER.test) --name $(@:%.tmp=%_graph) --output $@ --file $(@:%.tmp=%.cu)
	@time $(TFLAGS) --append -o Times.list $(COPPER.test) --name $(@:%.tmp=%_graph) --output $@ --file $(@:%.tmp=%.cu)

##
## -------------------------------------------------------------------------------------
##

.PHONY        : all test echo rebuild
.PHONY        : clear clean scrub distclean
.PHONY        : gitadd checkin
.INTERMEDIATE :
.SECONDARY    :

