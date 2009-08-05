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
#include "copper.inc"

FILE *input = 0;
int verboseFlag = 0;

extern int yy_grammar (YYClass* yySelf, YYStack* yystack);

static YYClass* theParser = ((YYClass*)0);

static int	 lineNumber = 0;
static char	*fileName   = 0;
static char	*trailer    = 0;
static Header	*headers    = 0;

static inline int my_yy_input(YYClass* yySelf, char* buffer, int max_size) {
    int chr = getc(input);
    if ('\n' == chr || '\r' == chr) ++lineNumber;
    if (EOF == chr) return 0;
    buffer[0] = chr;
    return 1;
}

static void my_debugger(YYClass* yySelf, const char* format, ...) {
    if (!verboseFlag) return;
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

void yyerror(char *message)
{
    fprintf(stderr, "%s:%d: %s", fileName, lineNumber, message);
    if (theParser->text[0]) fprintf(stderr, " near token '%s'", theParser->text);
    if (theParser->pos < theParser->limit || !feof(input))
        {
            theParser->buf[theParser->limit]= '\0';
            fprintf(stderr, " before text \"");
            while (theParser->pos < theParser->limit)
                {
                    if ('\n' == theParser->buf[theParser->pos] || '\r' == theParser->buf[theParser->pos]) break;
                    fputc(theParser->buf[theParser->pos++], stderr);
                }
            if (theParser->pos == theParser->limit)
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

static void version(char *name)
{
    printf("%s version %d.%d.%d\n", name, COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
}

static void usage(char *name)
{
    version(name);
    fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
    fprintf(stderr, "where <option> can be\n");
    fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "  -H <hfile>  write heading to <hfile>\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "if no <file> is given,  input is read from stdin\n");
    fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -F [<option>...] [<file>]\n", name);
    fprintf(stderr, "  -F          output the copper common function\n");
    fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -V [<option>...]\n", name);
    fprintf(stderr, "  -V          print version number\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s -h [<option>...]\n", name);
    fprintf(stderr, "  -h          print this help information\n");
    fprintf(stderr, "  -v          be verbose\n");
    exit(1);
}

static void parse_inputs(int argc, char **argv)
{
    theParser = malloc(sizeof(YYClass));
    yyInit(theParser);

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
    for (;  node;  node = node->any.next)
        Rule_print(node);
}

static void generate_parser(char* output_file, char* heading_file)
{
    if (!output_file) {
        output = stdout;
    } else {
        if (!(output = fopen(output_file, "w"))) {
            perror(output_file);
            exit(1);
        }
    }

    FILE* heading = 0;

    if (heading_file) {
        if (!strcmp(heading_file, "-")) {
            heading = stdout;
        } else {
            if (!strcmp(heading_file, output_file)) {
                heading = output;
            } else {
                if (!(heading = fopen(heading_file, "w"))) {
                    perror(heading_file);
                    exit(1);
                }
            }
        }
    }

    Rule_compile_c_heading(heading);
    Rule_compile_c_declare(heading, rules);

    for (; headers;  headers = headers->next)
        fprintf(output, "%s\n", headers->text);

    if (rules) {
        if (!heading) {
            Rule_compile_c_declare(output, rules);
        }
        Rule_compile_c(rules);
    }

    if (trailer) fprintf(output, "%s\n", trailer);

    if (output_file) {
        if (output != stdout) {
            fclose(output);
        }
    }

    if (heading_file) {
        if (heading != stdout) {
            if (output != heading) {
                fclose(heading);
            }
        }
    }
}

void generate_footer(int argc, char **argv,  char* output_file)
{
    FILE* footer = 0;

    if (!output_file) {
        footer = stdout;
    } else {
        if (!(footer = fopen(output_file, "w"))) {
            perror(output_file);
            exit(1);
        }
    }

    Rule_compile_c_footing(footer);

    if (output_file) {
        fclose(footer);
    }
}

int main(int argc, char **argv)
{
    input  = 0;
    output = stdout;

    int chr;
    char* header_name = 0;
    char* output_name = 0;

    enum Task {
        do_nothing,
        do_version,
        do_usage,
        do_rules,
        do_header,
        do_body,
        do_footer,
        do_default
    };

    enum Task my_task = do_default;

    while (-1 != (chr = getopt(argc, argv, "VhFRvo:H:")))
        {
            switch (chr) {
            case 'H':
                if (header_name) {
                    fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                    exit(1);
                }
                header_name = optarg;
                continue;

            case 'v':
                verboseFlag = 1;
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

            case 'h':
                my_task = do_usage;
                continue;

            case 'R':
                my_task = do_rules;
                continue;

            case 'F':
                my_task = do_footer;
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
        version(basename(argv[0]));
        exit(0);

    case do_usage:
        usage(basename(argv[0]));
        exit(0);

    case do_rules:
        parse_inputs(argc, argv);
        output_rules();
        exit(0);

    case do_footer:
        generate_footer(argc, argv, output_name);
        exit(0);

    case do_header:
    case do_body:
    default:
        parse_inputs(argc, argv);
        generate_parser(output_name, header_name);
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

