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

static char* program_name = 0;

extern bool copper_graph(Copper parser);

static void help() {
    fprintf(stderr, "copper [-verbose]+ --name c_func_name [--output outfile] [--file infile]\n");
    fprintf(stderr, "copper [-v]+ -n c_func_name [-o outfile] [-f infile]\n");
    fprintf(stderr, "copper [-h]\n");
    exit(1);
}

static bool copper_GetLine(struct prs_buffer *input, CuData *target)
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

static unsigned long encode_name(CuName name) {
    const char *cursor = name;

    unsigned long result = 5381;

    for ( ; *cursor ; ++cursor ) {
        int val = *cursor;
        result = ((result << 5) + result) + val;
    }

    return result;
}

static unsigned compare_name(CuName lname, CuName rname) {
    const char *left  = lname;
    const char *right = rname;

    int result = strcmp(left, right);

    return (0 == result);
}

static bool noop_release(void* value) {
    return true;
}

static bool copper_FindNode(Copper input, CuName name, CuNode* target) {
    struct prs_file *file = (struct prs_file *)input;
    if (hash_Find(file->nodes, name, (void**)target)) {
        return true;
    }
    return false;
}

static bool copper_SetNode(Copper input, CuName name, CuNode value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->nodes, (void*)name, value, noop_release);
}

static bool copper_FindPredicate(Copper input, CuName name, CuPredicate* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->predicates, name, (void**)target);
}

static bool copper_SetPredicate(Copper input, CuName name, CuPredicate value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->predicates, (void*)name, value, noop_release);
}

static bool copper_FindEvent(Copper input, CuName name, CuEvent* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->events, name, (void**)target);
}

static bool copper_SetEvent(Copper input, CuName name, CuEvent value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->events, (void*)name, value, noop_release);
}

static bool make_Copper(const char* filename, Copper *target) {
    struct prs_file *result = malloc(sizeof(struct prs_file));

    if (!result) return false;

    memset(result, 0, sizeof(struct prs_file));

    result->base.node      = copper_FindNode;
    result->base.attach    = copper_SetNode;
    result->base.predicate = copper_FindPredicate;
    result->base.event     = copper_FindEvent;

    cu_InputInit(&result->base, 1024);

    /* */
    result->filename = strdup(filename);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->nodes);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->predicates);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->events);

    *target = (Copper) result;

    return true;
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

    struct prs_buffer buffer;

    memset(&buffer, 0, sizeof(struct prs_buffer));

    if (!funcname) {
        CU_ERROR("a function name MUST be defined\n");
    }

    if (!infile) {
         buffer.file = input  = stdin;
        infile = "<stdin>";
    } else {
        buffer.file = input = fopen(infile, "r");
        if (!input) {
            CU_ERROR("unable to open input file %s\n", infile);
        }
    }

    Copper parser = 0;

    CU_DEBUG(1, "creating file parser object\n");
    if (!make_Copper(infile, &parser)) {
        CU_ERROR("unable create parser object for %s\n", infile);
    }

    copper_SetEvent(parser, "writeTree", writeTree);
    copper_SetEvent(parser, "checkRule", checkRule);
    copper_SetEvent(parser, "defineRule", defineRule);
    copper_SetEvent(parser, "makeEnd", makeEnd);
    copper_SetEvent(parser, "makeBegin", makeBegin);
    copper_SetEvent(parser, "makeApply", makeApply);
    copper_SetEvent(parser, "makePredicate", makePredicate);
    copper_SetEvent(parser, "makeDot", makeDot);
    copper_SetEvent(parser, "makeSet", makeSet);
    copper_SetEvent(parser, "makeString", makeString);
    copper_SetEvent(parser, "makeCall", makeCall);
    copper_SetEvent(parser, "makePlus", makePlus);
    copper_SetEvent(parser, "makeStar", makeStar);
    copper_SetEvent(parser, "makeQuestion", makeQuestion);
    copper_SetEvent(parser, "makeNot", makeNot);
    copper_SetEvent(parser, "makeCheck", makeCheck);
    copper_SetEvent(parser, "makeSequence", makeSequence);
    copper_SetEvent(parser, "makeChoice", makeChoice);
    copper_SetEvent(parser, "defineRule", defineRule);

    CU_DEBUG(1, "adding parser graph\n");
    copper_graph(parser);

#ifndef SKIP_META
    CU_DEBUG(1, "filling parser metadata\n");
    cu_FillMetadata(parser);
#endif

    CU_DEBUG(1, "parsing infile %s\n", infile);

    CuData data;

    for ( ; ; ) {
        copper_GetLine(&buffer, &data);

        switch(cu_Event(parser, data)) {
        case cu_NeedData:
            continue;

        case cu_FoundPath:
            CU_DEBUG(1, "running events\n");
            if (!cu_RunQueue(parser)) {
                CU_ERROR("event error\n");
            }
            break;

        case cu_NoPath:
            cu_SyntaxError(stderr, parser, infile);
            break;

        case cu_Error:
            CU_ERROR("hard error\n");
            break;
        }

        break;
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

