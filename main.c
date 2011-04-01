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

struct prs_map {
    unsigned       code;
    const void*    key;
    void*          value;
    struct prs_map *next;
};

typedef bool (*FreeValue)(void*);

struct prs_hash {
    unsigned size;
    struct prs_map *table[];
};

static char* program_name = 0;
static Copper     file_parser = 0;
struct prs_buffer file_buffer;
static struct prs_hash *copper_nodes      = 0;
static struct prs_hash *copper_predicates = 0;
static struct prs_hash *copper_events     = 0;

extern bool copper_graph(Copper parser);

static bool make_Map(unsigned code,
                     const void* key,
                     void* value,
                     struct prs_map *next,
                     struct prs_map **target)
{
    struct prs_map *result = malloc(sizeof(struct prs_map));

    result->code  = code;
    result->key   = key;
    result->value = value;
    result->next  = next;

    *target = result;
    return true;
}

static bool make_Hash(unsigned size, struct prs_hash **target)
{
    unsigned fullsize = (sizeof(struct prs_hash)
                         + (size * sizeof(struct prs_map *)));

    struct prs_hash* result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size    = size;

    *target = result;
    return true;
}

static bool noop_release(void* value) {
    return true;
}

static unsigned long encode_name(const void* name) {
    const char *cursor = name;

    unsigned long result = 5381;

    for ( ; *cursor ; ++cursor ) {
        int val = *cursor;
        result = ((result << 5) + result) + val;
    }

    return result;
}

static unsigned compare_name(const void* lname, const void* rname) {
    const char *left  = lname;
    const char *right = rname;

    int result = strcmp(left, right);

    return (0 == result);
}

static bool hash_Find(struct prs_hash *hash,
                      const void* key,
                      void** target)
{
    if (!key) return false;

    unsigned code  = encode_name(key);
    unsigned index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!compare_name(key, map->key)) continue;
        *target = map->value;
        return true;
    }

    return false;
}

static bool hash_Replace(struct prs_hash *hash,
                         const void* key,
                         void* value,
                         FreeValue release)
{
    if (!key)     return false;
    if (!value)   return false;
    if (!release) return false;

    unsigned long code  = encode_name(key);
    unsigned      index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!compare_name(key, map->key)) continue;
        if (release(map->value)) {
            return false;
        }
        map->value = value;
        return true;
    }

    if (!make_Map(code, key, value, hash->table[index], &map)) {
        return false;
    }

    hash->table[index] = map;

    return true;
}

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

static bool copper_FindNode(Copper input, CuName name, CuNode* target) {
    return hash_Find(copper_nodes, name, (void**)target);
}

static bool copper_SetNode(Copper input, CuName name, CuNode value) {
    return hash_Replace(copper_nodes, (void*)name, value, noop_release);
}

static bool copper_FindPredicate(Copper input, CuName name, CuPredicate* target) {
    return hash_Find(copper_predicates, name, (void**)target);
}

static bool copper_SetPredicate(Copper input, CuName name, CuPredicate value) {
    return hash_Replace(copper_predicates, (void*)name, value, noop_release);
}

static bool copper_FindEvent(Copper input, CuName name, CuEvent* target) {
    return hash_Find(copper_events, name, (void**)target);
}

static bool copper_SetEvent(Copper input, CuName name, CuEvent value) {
    return hash_Replace(copper_events, (void*)name, value, noop_release);
}

static bool make_Copper(FILE* file, const char* filename) {

    file_parser = malloc(sizeof(struct copper));

    if (!file_parser) return false;

    memset(file_parser,  0, sizeof(struct copper));
    memset(&file_buffer, 0, sizeof(struct prs_buffer));

    file_parser->node      = copper_FindNode;
    file_parser->attach    = copper_SetNode;
    file_parser->predicate = copper_FindPredicate;
    file_parser->event     = copper_FindEvent;

    cu_InputInit(file_parser, 1024);

    /* */
    file_buffer.file = file;

    make_Hash(100, &copper_nodes);
    make_Hash(100, &copper_predicates);
    make_Hash(100, &copper_events);

    file_ParserInit(file_parser);

    copper_SetEvent(file_parser, "writeTree", writeTree);
    copper_SetEvent(file_parser, "checkRule", checkRule);
    copper_SetEvent(file_parser, "defineRule", defineRule);
    copper_SetEvent(file_parser, "makeEnd", makeEnd);
    copper_SetEvent(file_parser, "makeBegin", makeBegin);
    copper_SetEvent(file_parser, "makeApply", makeApply);
    copper_SetEvent(file_parser, "makePredicate", makePredicate);
    copper_SetEvent(file_parser, "makeDot", makeDot);
    copper_SetEvent(file_parser, "makeSet", makeSet);
    copper_SetEvent(file_parser, "makeString", makeString);
    copper_SetEvent(file_parser, "makeCall", makeCall);
    copper_SetEvent(file_parser, "makePlus", makePlus);
    copper_SetEvent(file_parser, "makeStar", makeStar);
    copper_SetEvent(file_parser, "makeQuestion", makeQuestion);
    copper_SetEvent(file_parser, "makeNot", makeNot);
    copper_SetEvent(file_parser, "makeCheck", makeCheck);
    copper_SetEvent(file_parser, "makeSequence", makeSequence);
    copper_SetEvent(file_parser, "makeChoice", makeChoice);
    copper_SetEvent(file_parser, "defineRule", defineRule);

    CU_DEBUG(1, "adding parser graph\n");
    copper_graph(file_parser);

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

