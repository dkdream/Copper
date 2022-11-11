
COPPER.test  := ../copper.vm
COPPER.flags :=

Test_files := $(wildcard *.cu)

TFLAGS := -f 'Time %E KTime %S UTime %U PFaults %F Swapped %W CSwitch %w Signals %k Memory %M Exit %x :: %C'

all test :
	@echo >> Times.list
	@date >> Times.list
	@$(COPPER.test) --version >> Times.list
	@make clear collect
	@ls -l $(Test_files:%.cu=%.tmp)
	@cat Times.list

clear :       ; rm -f $(Test_files:%.cu=%.tmp)
clean : clear
scrub : clean ; rm -f Times.list

collect : $(Test_files:%.cu=%.tmp)

##
## Suffix Rules ------------------------------------------------------------------------
##

%.tmp : %.cu
	@echo $(COPPER.test) --name $(@:%.tmp=%_graph) --output $@ --file $(@:%.tmp=%.cu)
	@time $(TFLAGS) --append -o Times.list $(COPPER.test) --name $(@:%.tmp=%_graph) --output $@ --file $(@:%.tmp=%.cu)

##
## -------------------------------------------------------------------------------------
##

echo ::
	@echo 'H_SOURCES    = $(H_SOURCES)'
	@echo 'C_SOURCES    = $(C_SOURCES)'
	@echo 'WATER_TREES  = $(WATER_TREES)'
	@echo 'COPPER_TREES = $(COPPER_TREES)'
	@echo 'MAINS        = $(MAINS)'
	@echo 'DEPENDS      = $(DEPENDS)'

.PHONY        : all test echo rebuild
.PHONY        : clear clean scrub distclean
.PHONY        : gitadd checkin
.INTERMEDIATE :
.SECONDARY    :
