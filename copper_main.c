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
    fprintf(stderr, "  -h          print this help information\n");
    fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
    fprintf(stderr, "  -H <ofile>  write heading to <ofile>\n");
    fprintf(stderr, "  -F <ofile>  write footing to <ofile>\n");
    fprintf(stderr, "  -v          be verbose\n");
    fprintf(stderr, "  -V          print version number and exit\n");
    fprintf(stderr, "if no <file> is given, input is read from stdin\n");
    fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
    exit(1);
}

int main(int argc, char **argv)
{

    input      = stdin;
    output     = stdout;
    lineNumber = 1;
    fileName   = "<stdin>";

    int chr;
    FILE* heading = 0;
    FILE* footing = 0;

    while (-1 != (chr = getopt(argc, argv, "Vho:H:F:v")))
        {
            switch (chr)
                {
                case 'V':
                    version(basename(argv[0]));
                    exit(0);

                case 'h':
                    usage(basename(argv[0]));
                    break;

                case 'o':
                    if (!(output = fopen(optarg, "w")))
                        {
                            perror(optarg);
                            exit(1);
                        }
                    break;

                case 'H':
                    if (!(heading = fopen(optarg, "w")))
                        {
                            perror(optarg);
                            exit(1);
                        }
                    break;

                case 'F':
                    if (!(footing = fopen(optarg, "w")))
                        {
                            perror(optarg);
                            exit(1);
                        }
                    break;

                case 'v':
                    verboseFlag= 1;
                    break;

                default:
                    fprintf(stderr, "for usage try: %s -h\n", argv[0]);
                    exit(1);
                }
        }

    argc -= optind;
    argv += optind;

    theParser = malloc(sizeof(YYClass));
    yyInit(theParser);

    theParser->input_ = my_yy_input;
    theParser->debug_ = my_debugger;

    if (!argc) {
        if (!yyParseFrom(theParser, yy_grammar, "grammar"))
            yyerror("syntax error");
    } else {
        for (;  argc;  --argc, ++argv)
            {
                if (!strcmp(*argv, "-"))
                    {
                        input= stdin;
                        fileName= "<stdin>";
                    }
                else
                    {
                        if (!(input= fopen(*argv, "r")))
                            {
                                perror(*argv);
                                exit(1);
                            }
                        fileName= *argv;
                    }
                lineNumber = 1;

                if (!yyParseFrom(theParser, yy_grammar, "grammar"))
                    yyerror("syntax error");

                if (input != stdin)
                    fclose(input);
            }
    }

    if (verboseFlag) {
        Node *node = rules;
        for (;  node;  node = node->any.next)
            Rule_print(node);
    }

    for (; headers;  headers = headers->next)
        fprintf(output, "%s\n", headers->text);

    if (rules) {
        if (!heading) {
            Rule_compile_c_declare(output, rules);
        }
        Rule_compile_c(rules);
    }

    if (trailer)
        fprintf(output, "%s\n", trailer);


    Rule_compile_c_heading(heading);
    Rule_compile_c_declare(heading, rules);
    Rule_compile_c_footing(footing);

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

