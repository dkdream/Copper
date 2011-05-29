/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **/
#include "compiler.h"
#include "copper_inline.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

struct prs_buffer {
    FILE     *file;
    unsigned  cursor;
    ssize_t   read;
    size_t    allocated;
    char     *line;
};
typedef struct prs_buffer PrsBuffer;


static char*  program_name = 0;
static Copper file_parser = 0;

static void help() {
    fprintf(stderr, "copper [-verbose]+ --name c_func_name [--output outfile] [--file infile]\n");
    fprintf(stderr, "copper [-v]+ -n c_func_name [-o outfile] [-f infile]\n");
    fprintf(stderr, "copper [-h]\n");
    exit(1);
}

static bool copper_GetLine(PrsBuffer *input, CuData *target)
{
    if (!target) return false;

    target->length = 0;
    target->start  = 0;

    if (!input) return false;

    if (input->cursor >= input->read) {
        int read = getline(&input->line, &input->allocated, input->file);
        if (read < 0) return false;
        input->cursor = 0;
        input->read   = read;
    }

    target->length = input->read - input->cursor;
    target->start  = input->line + input->cursor;
    input->cursor += target->length;

    return true;
}

static bool make_Copper() {
    file_parser = malloc(sizeof(struct copper));

    if (!file_parser) return false;

    memset(file_parser, 0, sizeof(struct copper));

    if (!file_ParserInit(file_parser)) return false;

#ifndef SKIP_META
    CU_DEBUG(1, "filling parser metadata\n");
    if (!cu_FillMetadata(file_parser)) return false;
#endif

    if (!cu_Start("grammar", file_parser)) return false;

    return true;
}


int main(int argc, char **argv)
{
    program_name = basename(argv[0]);

    const char* infile   = 0;
    const char* outfile  = 0;
    const char* funcname = 0;
    int     option_index = 0;
    CuData          data = { 0, 0 };
    PrsBuffer     buffer = { 0, 0, 0, 0, 0 };

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

    if (!funcname) {
        CU_ERROR("a function name MUST be defined\n");
    }

    memset(&buffer, 0, sizeof(struct prs_buffer));

    if (!infile) {
        buffer.file = stdin;
        infile = "<stdin>";
    } else {
        buffer.file = fopen(infile, "r");
        if (!buffer.file) {
            CU_ERROR("unable to open input file %s\n", infile);
        }
    }

    CU_DEBUG(1, "creating file parser object\n");
    if (!make_Copper()) {
        CU_ERROR("unable create parser object for %s\n", infile);
    }

    CU_DEBUG(1, "parsing infile %s\n", infile);

    for ( ; ; ) {
        if (!copper_GetLine(&buffer, &data)) {
            CU_ERROR("no more data\n");
            break;
        }

        switch(cu_Event(file_parser, data)) {
        case cu_NeedData:
            continue;

        case cu_FoundPath:
            CU_DEBUG(1, "running events\n");
            if (!cu_RunQueue(file_parser)) {
                CU_ERROR("event error\n");
            }
            break;

        case cu_NoPath:
            //cu_SyntaxError(stderr, file_parser, infile);
            break;

        case cu_Error:
            CU_ERROR("hard error\n");
            break;
        }

        break;
    }

    FILE* output;

    if (!outfile) {
        output = stdout;
    } else {
        output = fopen(outfile, "w");
        if (!output) {
            CU_ERROR("unable to open output file %s\n", outfile);
        }
    }

    CU_DEBUG(1, "writing tree\n");
    if (!file_WriteTree(file_parser, output, funcname)) {
        CU_ERROR("write tree error\n");
    }

    return 0;
}

/*****************
 ** end of file **
 *****************/

