/*-*- mode: c;-*-*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE     4096
#define COLCOUNT 20

static FILE *input  = 0;
static FILE *output = 0;

static int  do_debug = 0;
static int  do_begin = 0;
static const char* varname = "varname";

static void usage(char *name) {
    fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
    fprintf(stderr, "where <option> can be\n");
    fprintf(stderr, "  -h          print this help information\n");
    fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "if no <file> is given, input is read from stdin\n");
    fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
    exit(1);
}

static void checkLine() {
    static int counter = COLCOUNT;
    if (COLCOUNT <= ++counter) {
        fprintf(output, "%s", "\n\t");
        counter = 0;
    }
}

static void text_to_hex(int chr) {
    checkLine();
    fprintf(output, " 0x%.2X,", chr);
}

static void doParse() {
    int max       = SIZE - 1;
    int asciiSize = 0;
    int chr       = 0;

    if (!do_begin) {
        fprintf(output, "static const char %s[] = {", varname);
        do_begin = 1;
    }

    while (EOF != (chr = fgetc(input))) {
        text_to_hex(chr);
    }
}

static void doOpen(char* name)
{
    if (do_debug) {
        fprintf(stderr, "\nopening %s\n", name);
    }

    if (!(input= fopen(name, "r"))) {
        perror(name);
        exit(1);
    }
}

static void doClose() {
    fclose(input);
}

int main(int argc, char **argv) {
    int chr;
    int notSOUT = 0;

    output = stdout;
    input  = stdin;

    while (-1 != (chr = getopt(argc, argv, "ho:vl:"))) {
        switch (chr) {
        case 'h':
            usage(argv[0]);
            break;

        case 'v':
            ++do_debug;
            break;

        case 'o':
            if (!(output= fopen(optarg, "w"))) {
                perror(optarg);
                exit(1);
            }
            notSOUT = 1;
            break;

        case 'l':
            varname = optarg;
            break;

        default:
            fprintf(stderr, "for usage try: %s -h\n", argv[0]);
            exit(1);
        }
    }

    if (optind >= argc) {
        doParse();
    }

    int index = optind;

    for (;  index < argc; ++index) {
        if (!strcmp(argv[index], "-")) {
            input = stdin;
            doParse();
            continue;
        }

        doOpen(argv[index]);
        doParse();
        doClose();
    }

    if (do_begin) {
        checkLine();
        fprintf(output, " %s };\n", "0x00");
    }

    if (notSOUT) {
        fclose(output);
    }

    return 0;
}
