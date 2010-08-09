/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "syntax.h"

#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>

static char* program_name = 0;

extern bool init__copper_c(PrsInput input);

int main(int argc, char **argv)
{
    program_name = basename(argv[0]);

    const char* filename = 0;
    unsigned       debug = 0;
    int     option_index = 0;

    static struct option long_options[] = {
        {"verbose", 0, 0, 'v'},
        {"file",    1, 0, 'f'},
        {0, 0, 0, 0}
    };

    int chr;

    while (-1 != (chr = getopt_long(argc, argv,
                                    "vf:",
                                    long_options,
                                    &option_index)))
        {
            switch (chr) {
            case 'v':
                debug += 1;
                break;

            case 'f':
                filename = optarg;
                break;
            default:
                {
                    printf("invalid option %c\n", chr);
                    exit(1);
                }
            }
        }

    argc -= optind;
    argv += optind;

    if (!filename) {
        filename = "./testing.cu";
    }

    CU_DEBUG(1, "reading %s\n", filename);

    PrsInput input = 0;

    if (!make_PrsFile(filename, &input)) {
        CU_ERROR("unable to open file %s\n", filename);
        return 1;
    }

    init__copper_c(input);

    if (input_Parse("grammar", input)) {
        CU_DEBUG(1, "parsed\n");
        if (input_RunQueue(input)) {
            CU_DEBUG(1, "compiled\n");
        } else {
            CU_ERROR("compile error\n");
        }
    } else {
        CU_ERROR("syntax error\n");
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

