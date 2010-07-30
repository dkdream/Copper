///////////////////////
//
// Project: *current project*
// Module:  Copper
//
// CreatedBy:
// CreatedOn: Aug of 2009
//
// Routine List:
//    <routine-list-end>

#include "header.var"
#include "preamble.var"
#include "footer.var"

#define COPPER_MAIN
#include "copper.inc"

extern const char header[];
extern const char preamble[];
extern const char footer[];
extern int yy_grammar (YYClass* yySelf, YYStack* yystack);

FILE *input = 0;
int   debug = 0;

static YYClass* theParser = ((YYClass*)0);

static int     lineNumber = 0;
static char   *fileName   = 0;
static char   *trailer    = 0;
static Header *headers    = 0;
static int   no_header    = 0;
static int   no_preamble  = 0;
static int   no_footer    = 0;
static int   export_all   = 0;
static char* program_name = 0;
static char* rules_name   = 0;
static char* output_name  = 0;

static inline int my_yy_input(YYClass* yySelf, char* buffer, int max_size) {
    int chr = getc(input);
    if ('\n' == chr || '\r' == chr) ++lineNumber;
    if (EOF == chr) return 0;
    buffer[0] = chr;
    return 1;
}

static void my_debugger(YYClass* yySelf, DebugLevel level, const char* format, ...) {
    if (!debug) return;
    if (level > debug) return;

    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

void yyerror(char *message)
{
    int point = theParser->current.offset;

    lineNumber = theParser->current.lineno;

    fprintf(stderr, "%s:%d: %s", fileName, lineNumber, message);

    if (theParser->text[0]) fprintf(stderr, " near token '%s'", theParser->text);

    if (point < theParser->limit || !feof(input))
        {
            theParser->buf[theParser->limit]= '\0';
            fprintf(stderr, " before text \"");
            while (point < theParser->limit)
                {
                    if ('\n' == theParser->buf[point] || '\r' == theParser->buf[point]) break;
                    fputc(theParser->buf[point++], stderr);
                }
            if (point == theParser->limit)
                {
                    int c;
                    while (EOF != (c= fgetc(input)) && '\n' != c && '\r' != c)
                        fputc(c, stderr);
                }
            fputc('\"', stderr);
        }

    fprintf(stderr, "\n");
    exit(1);
}

void makeHeader(char *text)
{
    Header *header= (Header *)malloc(sizeof(Header));
    header->text= strdup(text);
    header->next= headers;
    headers= header;
}

void makeTrailer(char *text)
{
    trailer = strdup(text);
}

static void version()
{
    printf("%s version %d.%d.%d\n", program_name, COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
}

static void usage()
{
    version();
    fprintf(stderr, "usage: %s [-H|-P|-C|-F|-A|-h] [<option>...] [<ifile>...]\n", program_name);
    fprintf(stderr, "   read input files and generate a parser\n");
    fprintf(stderr, "   where <option> can be\n");
    fprintf(stderr, "     -t          strip the output of the heading\n");
    fprintf(stderr, "     -m          strip the output of the preamble\n");
    fprintf(stderr, "     -f          strip the output of the footing\n");
    fprintf(stderr, "     -s          strip the output of the heading, preamble and footing\n");
    fprintf(stderr, "     -x          mark all defined rules for export\n");
    fprintf(stderr, "     -r <hfile>  write rule declarations to <hfile>\n");
    fprintf(stderr, "     -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "     -v          be verbose\n");
    fprintf(stderr, "   if no <file>    is given, input is read from stdin\n");
    fprintf(stderr, "   if no <ofile>   is given, output is written to stdout\n");
    fprintf(stderr, "   if <hfile> == <ofile> then no rule declarations are written\n");
    fprintf(stderr, "   if <hfile> == -       then rule declarations are written to stdout\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -H [<option>...] [<ofile>]\n", program_name);
    fprintf(stderr, "   output the copper common type definitions (heading)\n");
    fprintf(stderr, "   where <option> can be\n");
    fprintf(stderr, "     -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "   if no <ofile> is given, output is written to stdout\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -P [<option>...] [<ofile>]\n", program_name);
    fprintf(stderr, "   output the copper common macros and inline functions (preamble)\n");
    fprintf(stderr, "   where <option> can be\n");
    fprintf(stderr, "     -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "   if no <ofile> is given, output is written to stdout\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -F [<option>...] [<ofile>]\n", program_name);
    fprintf(stderr, "   output the copper common function (footing)\n");
    fprintf(stderr, "   where <option> can be\n");
    fprintf(stderr, "     -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "   if no <ofile> is given, output is written to stdout\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -V\n", program_name);
    fprintf(stderr, "   print version number\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -h\n", program_name);
    fprintf(stderr, "   print this help information\n");
    exit(1);
}

static void parse_inputs(int argc, char **argv)
{
    theParser = yyMakeParser(0);

    theParser->input_ = my_yy_input;
    theParser->debug_ = my_debugger;

    input      = stdin;
    fileName   = "<stdin>";
    lineNumber = 1;

    if (!argc) {
        if (!yyParseFrom(theParser, yy_grammar, "grammar"))
            yyerror("syntax error");
    } else {
        for (;  argc;  --argc, ++argv)
            {
                if (!strcmp(*argv, "-")) {
                    input      = stdin;
                    fileName   = "<stdin>";
                    lineNumber = 1;
                } else {
                    if (!(input= fopen(*argv, "r"))) {
                        perror(*argv);
                        exit(1);
                    }
                    fileName   = *argv;
                    lineNumber = 1;
                }

                if (!yyParseFrom(theParser, yy_grammar, "grammar"))
                    yyerror("syntax error");

                if (input != stdin)
                    fclose(input);
            }
    }
}

static void output_rules()
{
    Node *node = rules;
    for (;  node;  node = node->any.next) {
        Rule_print(node);
    }
}

static void generate_parser()
{
    if (!output_name) {
        output = stdout;
    } else {
        if (!(output = fopen(output_name, "w"))) {
            perror(output_name);
            exit(1);
        }
    }

    FILE* heading = 0;

    if (rules_name) {
        if (!strcmp(rules_name, "-")) {
            heading = stdout;
        } else {
            if (strcmp(rules_name, output_name)) {
                if (!(heading = fopen(rules_name, "w"))) {
                    perror(rules_name);
                    exit(1);
                }
            }
        }
    }

    fprintf(output, "/* A recursive-descent parser generated by copper %d.%d.%d */\n",
            COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
    fprintf(output, "\n");

    if (!no_header) {
        fprintf(output, "%s", header);
        fprintf(output, "\n");
    }

    for (; headers;  headers = headers->next)
        fprintf(output, "%s\n", headers->text);

    if (!no_preamble) {
        fprintf(output, "%s", preamble);
        fprintf(output, "\n");
    }

    if (rules) {
        Rule_compile_c(rules, export_all);
    }

    if (!no_footer) {
        fprintf(output, "%s", footer);
        fprintf(output, "\n");
    }

    if (trailer) fprintf(output, "%s\n", trailer);

    if (output != heading) {
        Rule_compile_c_declare(heading, rules);
    }

    if (output_name) {
        if (output != stdout) {
            fclose(output);
        }
    }

    if (rules_name) {
        if (heading != stdout) {
            if (output != heading) {
                fclose(heading);
            }
        }
    }
}


static void generate_vm_parser()
{
    if (!output_name) {
        output = stdout;
    } else {
        if (!(output = fopen(output_name, "w"))) {
            perror(output_name);
            exit(1);
        }
    }

    Rule_compile_vm(output);
}


typedef void (*Writer)(FILE* ofile);

void generate(Writer section, int argc, char **argv)
{
    FILE* output_file = 0;

    if (!output_name) {
        if (argc) {
            output_name = *argv;
        }
    }

    if (!output_name) {
        output_file = stdout;
    } else {
        if (!(output_file = fopen(output_name, "w"))) {
            perror(output_name);
            exit(1);
        }
    }

    section(output_file);

    if (output_name) {
        fclose(output_file);
    }
}

int main(int argc, char **argv)
{
    program_name = basename(argv[0]);
    input        = 0;
    output       = stdout;

    int chr;

    enum Task {
        do_nothing,
        do_version,
        do_usage,
        do_rules,
        do_header,
        do_preamble,
        do_compile,
        do_vm_compile,
        do_footer,
        do_default
    };

    enum Task my_task = do_default;

    while (-1 != (chr = getopt(argc, argv, "AVHPCFhvtmfsxo:r:")))
        {
            switch (chr) {
            case 'r':
                if (rules_name) {
                    fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                    exit(1);
                }
                rules_name = optarg;
                continue;

            case 'v':
                debug += 1;
                continue;

            case 't':
                no_header = 1;
                continue;

            case 'm':
                no_preamble = 1;
                continue;

            case 'f':
                no_footer = 1;
                continue;

            case 's':
                no_header   = 1;
                no_preamble = 1;
                no_footer   = 1;
                continue;

            case 'x':
                export_all = 1;
                continue;

            case 'o':
                if (output_name) {
                    fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                    exit(1);
                }
                output_name = optarg;
                continue;

            default:
                break;
            }

            switch (chr) {
            case 'V':
                my_task = do_version;
                continue;

            case 'H':
                my_task = do_header;
                continue;

            case 'P':
                my_task = do_preamble;
                continue;

            case 'C':
                my_task = do_compile;
                continue;

            case 'A':
                my_task = do_vm_compile;
                continue;

            case 'F':
                my_task = do_footer;
                continue;

            case 'h':
                my_task = do_usage;
                continue;

            default:
                fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                exit(1);
            }
        }

    argc -= optind;
    argv += optind;

    switch (my_task) {
    case do_nothing:
        exit(0);

    case do_version:
        version();
        exit(0);

    case do_usage:
        usage();
        exit(0);

    case do_rules:
        parse_inputs(argc, argv);
        output_rules();
        exit(0);

    case do_header:
        generate(Rule_compile_c_heading, argc, argv);
        exit(0);

    case do_preamble:
        generate(Rule_compile_c_preamble, argc, argv);
        exit(0);

    case do_footer:
        generate(Rule_compile_c_footing, argc, argv);
        exit(0);

    case do_vm_compile:
        parse_inputs(argc, argv);
        generate_vm_parser();
        exit(0);

    case do_compile:
    default:
        parse_inputs(argc, argv);
        generate_parser();
        exit(0);
    }

    return 0;
}

/////////////////////
// end of file
/////////////////////
//
// $Source$
// $Author$
// $Date$
// $Revision$
// $State$
//
// $Log$
//
//

