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
#include <ctype.h>

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

    CU_DEBUG(1, "filling parser metadata\n");
    cu_FillMetadata(parser);

    CU_DEBUG(1, "parsing infile %s\n", infile);

    if (!cu_Parse("grammar", parser)) {

        int pos     = parser->cursor.text_inx;
        int limit   = parser->data.limit;
        int reached = parser->reach.text_inx;

        if (reached >= limit && feof(input)) {
            CU_ERROR("%s:%d [%d] syntax error",
                     infile,
                     parser->reach.line_number + 1,
                     parser->reach.char_offset);
        } else {
            char *buffer = parser->data.buffer;
            int start    = reached;
            int chr;
            int inx;

            for ( ; pos <= start ; --start) {
                chr = buffer[start];
                if ('\n' == chr) break;
            }

            if ('\n' == chr) start += 1;

            CU_ERROR_PART("%s:%d:%d: syntax error in text\n",
                          infile,
                          parser->reach.line_number + 1,
                          parser->reach.char_offset - 1);

            for (inx = start ; inx < limit ; ++inx) {
                int chr =  buffer[inx];
                CU_ERROR_PART("%c", chr);
                if ('\n' == chr) break;
            }
            for (inx = start ; inx < reached ; ++inx) {
                int chr =  buffer[inx];
                if (!isblank(chr)) {
                    CU_ERROR_PART("-");
                    continue;
                }
                if (' ' == chr) {
                    CU_ERROR_PART("-");
                    continue;
                }
                CU_ERROR_PART("%c", chr);
            }
            CU_ERROR_PART("^\n");
            CU_ERROR("\n");
        }
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

