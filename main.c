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
#include <string.h>

struct prs_buffer {
    FILE     *file;
    unsigned  cursor;
    ssize_t   read;
    size_t    allocated;
    char     *line;
};

static char* program_name = 0;
static Copper     file_parser = 0;
struct prs_buffer file_buffer;

static bool buffer_GetLine(Copper base)
{
    struct prs_buffer *input = &file_buffer;

    if (input->cursor >= input->read) {
        int read = getline(&input->line, &input->allocated, input->file);
        if (read < 0) return false;
        input->cursor = 0;
        input->read   = read;
    }

    unsigned  count = input->read - input->cursor;
    const char *src = input->line + input->cursor;

    if (!cu_AppendData(base, count, src)) return false;

    input->cursor += count;

    return true;
}

static bool copper_MoreData(Copper input) {
    return buffer_GetLine(input);
}

static bool make_Copper(FILE* file, const char* filename) {

    file_parser = malloc(sizeof(struct copper));

    if (!file_parser) return false;

    memset(file_parser,  0, sizeof(struct copper));
    memset(&file_buffer, 0, sizeof(struct prs_buffer));

    file_ParserInit(file_parser);
    file_buffer.file = file;

#ifndef SKIP_META
    CU_DEBUG(1, "filling parser metadata\n");
    cu_FillMetadata(file_parser);
#endif

    return true;
}

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

    if (!funcname) {
        CU_ERROR("a function name MUST be defined\n");
    }

    FILE* input;

    if (!infile) {
        input  = stdin;
        infile = "<stdin>";
    } else {
        input = fopen(infile, "r");
        if (!input) {
            CU_ERROR("unable to open input file %s\n", infile);
        }
    }

    CU_DEBUG(1, "creating file parser object\n");
    if (!make_Copper(input, infile)) {
        CU_ERROR("unable create parser object for %s\n", infile);
    }

    CU_DEBUG(1, "parsing infile %s\n", infile);

    if (!cu_Parse("grammar", file_parser, copper_MoreData)) {
        cu_SyntaxError(stderr, file_parser, infile, copper_MoreData);
    }

    CU_DEBUG(1, "running events\n");
    if (!cu_RunQueue(file_parser)) {
        CU_ERROR("event error\n");
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

