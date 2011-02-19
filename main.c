/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "compiler.h"
#include "copper_inline.h"

#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <ctype.h>

static char* program_name = 0;

extern bool copper_graph(Copper parser);

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
        CU_ERROR("a function name MUST be defined\n");
    }

    if (!infile) {
        input  = stdin;
        infile = "<stdin>";
    } else {
        input = fopen(infile, "r");
        if (!input) {
            CU_ERROR("unable to open input file %s\n", infile);
        }
    }

    Copper parser = 0;

    CU_DEBUG(1, "creating file parser object\n");
    if (!make_CuFile(input, infile, &parser)) {
        CU_ERROR("unable create parser object for %s\n", infile);
    }

    file_SetEvent((struct prs_file *)parser, "writeTree", writeTree);
    file_SetEvent((struct prs_file *)parser, "checkRule", checkRule);
    file_SetEvent((struct prs_file *)parser, "defineRule", defineRule);
    file_SetEvent((struct prs_file *)parser, "makeEnd", makeEnd);
    file_SetEvent((struct prs_file *)parser, "makeBegin", makeBegin);
    file_SetEvent((struct prs_file *)parser, "makeThunk", makeThunk);
    file_SetEvent((struct prs_file *)parser, "makeApply", makeApply);
    file_SetEvent((struct prs_file *)parser, "makePredicate", makePredicate);
    file_SetEvent((struct prs_file *)parser, "makeDot", makeDot);
    file_SetEvent((struct prs_file *)parser, "makeSet", makeSet);
    file_SetEvent((struct prs_file *)parser, "makeString", makeString);
    file_SetEvent((struct prs_file *)parser, "makeCall", makeCall);
    file_SetEvent((struct prs_file *)parser, "makePlus", makePlus);
    file_SetEvent((struct prs_file *)parser, "makeStar", makeStar);
    file_SetEvent((struct prs_file *)parser, "makeQuestion", makeQuestion);
    file_SetEvent((struct prs_file *)parser, "makeNot", makeNot);
    file_SetEvent((struct prs_file *)parser, "makeCheck", makeCheck);
    file_SetEvent((struct prs_file *)parser, "makeSequence", makeSequence);
    file_SetEvent((struct prs_file *)parser, "makeChoice", makeChoice);
    file_SetEvent((struct prs_file *)parser, "defineRule", defineRule);
    file_SetEvent((struct prs_file *)parser, "makeHeader", makeHeader);
    file_SetEvent((struct prs_file *)parser, "makeInclude", makeInclude);
    file_SetEvent((struct prs_file *)parser, "makeFooter", makeFooter);

    CU_DEBUG(1, "adding parser graph\n");
    copper_graph(parser);

#ifndef SKIP_META
    CU_DEBUG(1, "filling parser metadata\n");
    cu_FillMetadata(parser);
#endif

    CU_DEBUG(1, "parsing infile %s\n", infile);

    if (!cu_Parse("grammar", parser)) {
        cu_SyntaxError(stderr, parser, infile);
    }

    CU_DEBUG(1, "running events\n");
    if (!cu_RunQueue(parser)) {
        CU_ERROR("event error\n");
    }

    if (!outfile) {
        output = stdout;
    } else {
        output = fopen(outfile, "w");
        if (!output) {
            CU_ERROR("unable to open output file %s\n", outfile);
        }
    }

    CU_DEBUG(1, "writing tree\n");
    if (!file_WriteTree(parser, output, funcname)) {
        CU_ERROR("write tree error\n");
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

