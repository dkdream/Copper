#include <stdio.h>
#include <unistd.h>

FILE *input  = 0;
FILE *output = 0;

static int   lineNumber = 0;
static char *fileName   = 0;

void yyerror(char *message);
int  doMatch(char *message, char chr);
void doDone();

#define YY_INPUT(buf, result, max)          \
{                                           \
  int c = getc(input);                      \
  if ('\n' == c || '\r' == c) ++lineNumber; \
  result = (EOF == c) ? 0 : (*(buf)= c, 1); \
}

#define YY_DONE doDone()
#include "thoms_test.peg.c"

void yyerror(char *message)
{
  fprintf(stderr, "%s:%d: %s", fileName, lineNumber, message);
  if (yytext[0]) fprintf(stderr, " near token '%s'", yytext);
  if (yypos < yylimit || !feof(input))
    {
      yybuf[yylimit]= '\0';
      fprintf(stderr, " before text \"");
      while (yypos < yylimit)
        {
          if ('\n' == yybuf[yypos] || '\r' == yybuf[yypos]) break;
          fputc(yybuf[yypos++], stderr);
        }
      if (yypos == yylimit)
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

int doMatch(char *message, char chr) {
  fprintf(stderr, "(%s %c)? ", message, chr);
  return (yybuf[yypos] == chr);
}

static void usage(char *name)
{
  fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
  fprintf(stderr, "where <option> can be\n");
  fprintf(stderr, "  -h          print this help information\n");
  fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
  fprintf(stderr, "  -v          be verbose\n");
  fprintf(stderr, "if no <file> is given, input is read from stdin\n");
  fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
  exit(1);
}

static void doOpen(char* name)
{
  if (!(input= fopen(name, "r"))) {
    perror(name);
    exit(1);
  }

  fileName = name;
}

int isDone;

void doDone()
{
  isDone = 1;
}

static void doParse()
{
  int value  = 0;
  lineNumber = 1;
  isDone     = 0;

  printf("=============\n");

  while (value = yyparse()) {
    printf("result = %d\n", value);
    printf("=============\n");
    if (isDone) return;
  }

  yyerror("syntax error");
  printf("=============\n");
}

static void doClose()
{
  fclose(input);
}

int main(int argc, char **argv)
{
  int   c;
  output     = stdout;
  input      = stdin;
  lineNumber = 1;
  fileName   = "<stdin>";

  while (-1 != (c= getopt(argc, argv, "ho:v")))
    {
      switch (c)
        {
        case 'h':
          usage(argv[0]);
          break;

        case 'o':
          if (!(output= fopen(optarg, "w")))
            {
              perror(optarg);
              exit(1);
            }
          break;

        default:
          fprintf(stderr, "for usage try: %s -h\n", argv[0]);
          exit(1);
        }
    }

  argc -= optind;
  argv += optind;

  if (!argc) {
    doParse();
    return 0;
  }

  for (;  argc;  --argc, ++argv) {
    if (!strcmp(*argv, "-")) {
      input    = stdin;
      fileName = "<stdin>";
      doParse();
      continue;
    }

    doOpen(*argv);
    doParse();
    doClose();
  }

  return 0;
}
