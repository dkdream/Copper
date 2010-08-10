/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "compiler.h"

#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>

static char* program_name = 0;

extern bool copper_graph(PrsInput parser);

static void help() {
    fprintf(stderr, "copper [-verbose]+ --name c_func_name [--output outfile] [--file infile]\n");
    fprintf(stderr, "copper [-v]+ -n c_func_name [-o outfile] [-f infile]\n");
    fprintf(stderr, "copper [-h]\n");
    exit(1);
}

int main(int argc, char **argv)
{
    program_name = basename(argv[0]);

    const char* infile   = 0;
    const char* outfile  = 0;
    const char* funcname = 0;
    int     option_index = 0;

    cu_global_debug = 0;


    static struct option long_options[] = {
        {"verbose", 0, 0, 'v'},
        {"help",    0, 0, 'h'},
        {"file",    1, 0, 'f'},
        {"output",  1, 0, 'o'},
        {"name",    1, 0, 'n'},
        {0, 0, 0, 0}
    };

    int chr;

    while (-1 != (chr = getopt_long(argc, argv,
                                    "vhf:n:o:",
                                    long_options,
                                    &option_index)))
        {
            switch (chr) {
            case 'v':
                cu_global_debug += 1;
                break;

            case 'h':
                help();
                break;

            case 'f':
                infile = optarg;
                break;

            case 'o':
                outfile = optarg;
                break;

            case 'n':
                funcname = optarg;
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

    FILE* input;
    FILE* output;

    if (!funcname) {
        CU_ERROR("a function name MUST be defined");
    }

    if (!infile) {
        input  = stdin;
        infile = "<stdin>";
    } else {
        input = fopen(infile, "r");
        if (!input) {
            CU_ERROR("unable to open input file %s", infile);
        }
    }

    PrsInput parser = 0;

    CU_DEBUG(1, "creating file parser object\n");
    if (!make_PrsFile(input, infile, &parser)) {
        CU_ERROR("unable create parser object for %s", infile);
    }

    CU_DEBUG(1, "adding parser graph\n");

    copper_graph(parser);

    CU_DEBUG(1, "parsing infile %s\n", infile);
    if (!cu_Parse("grammar", parser)) {
        CU_ERROR("syntax error");
    }

    CU_DEBUG(1, "running events\n");
    if (!cu_RunQueue(parser)) {
        CU_ERROR("event error");
    }

    if (!outfile) {
        output = stdout;
    } else {
        output = fopen(outfile, "w");
        if (!output) {
            CU_ERROR("unable to open output file %s", outfile);
        }
    }

    CU_DEBUG(1, "writing tree\n");
    if (!file_WriteTree(parser, output, funcname)) {
        CU_ERROR("write tree error");
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

