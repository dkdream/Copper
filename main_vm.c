/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "copper_vm.h"

extern bool init__vm_copper_c(PrsInput input);

int main(int argc, char **argv)
{
    PrsInput input = 0;

    make_PrsFile("./copper.cu", &input);

    init__vm_copper_c(input);

    PrsNode start = 0;

    if (input->node(input, "grammar", &start)) {
        copper_vm(start, input);
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

