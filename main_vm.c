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

    PrsNode start = 0;

    input->node(input, "grammer", &start);

    copper_vm(start, input);
}

/*****************
 ** end of file **
 *****************/

