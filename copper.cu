# Copper Grammar for copper

%{
#include "copper.inc"
%}

# Hierarchical syntax

grammar=	- ( declaration | definition )+ trailer? end-of-file

declaration=	'%{' < ( !'%}' . )* > RPERCENT		{ makeHeader(yytext); }

trailer=	'%%' < .* >				{ makeTrailer(yytext); }

definition=	identifier 				{ checkRule(yytext); }
			EQUAL ( expression end        	{ defineRule(rule_with_end); }
                              | expression              { defineRule(simple_rule); }
                              | begin expression end    { defineRule(rule_with_both); }
                              | begin expression        { defineRule(rule_with_begin); }
                              ) SEMICOLON?

begin=          '%begin' - action                       { push(makePredicate(yytext)); }
end=            '%end'   - action                       { push(makePredicate(yytext)); }

expression=	sequence (BAR sequence			{ Node *f= pop();  push(Alternate_append(pop(), f)); }
			    )*

sequence=	prefix (prefix				{ Node *f= pop();  push(Sequence_append(pop(), f)); }
			  )*

prefix=		AND action				{ push(makePredicate(yytext)); }
|		AND suffix				{ push(makePeekFor(pop())); }
|		NOT suffix				{ push(makePeekNot(pop())); }
|		    suffix

suffix=		primary (QUESTION			{ push(makeQuery(pop())); }
			     | STAR			{ push(makeStar (pop())); }
			     | PLUS			{ push(makePlus (pop())); }
			   )?

primary=	identifier				{ push(makeVariable(yytext)); }
			COLON identifier !EQUAL		{ Node *name= makeName(findRule(yytext));  name->name.variable= pop();  push(name); }
|		identifier !EQUAL			{ push(makeName(findRule(yytext))); }
|		OPEN expression CLOSE
|		literal					{ push(makeString(yytext)); }
|		class					{ push(makeClass(yytext)); }
|		DOT					{ push(makeDot()); }
|		action					{ push(makeAction(yytext)); }
|		BEGIN					{ push(makeMark("YY_SEND(begin_, yystack)")); }
|		END					{ push(makeMark("YY_SEND(end_, yystack)")); }
|		MARK					{ push(makeAction("YY_SEND(mark_, yyrulename);")); }
|		COLLECT					{ push(makeAction("YY_SEND(collect_, yyrulename);")); }

# Lexical syntax

identifier=	< [-a-zA-Z_][-a-zA-Z_0-9]* > -

literal=	['] < ( !['] char )* > ['] -
|		["] < ( !["] char )* > ["] -

class=		'[' < ( !']' range )* > ']' -

range=		char '-' char | char

char=		'\\' [abefnrtv'"\[\]\\]
|		'\\' [0-3][0-7][0-7]
|		'\\' [0-7][0-7]?
|		!'\\' .

action=		'{' < braces* > '}' -

braces=		'{' (!'}' .)* '}'
|		!'}' .

EQUAL=		'=' -
COLON=		':' -
SEMICOLON=	';' -
BAR=		'|' -
AND=		'&' -
NOT=		'!' -
QUESTION=	'?' -
STAR=		'*' -
PLUS=		'+' -
OPEN=		'(' -
CLOSE=		')' -
DOT=		'.' -
BEGIN=		'<' -
END=		'>' -
MARK=           '@' -
COLLECT=        '$' -
RPERCENT=	'%}' -

-=		(space | comment)*
space=		' ' | '\t' | end-of-line
comment=	'#' (!end-of-line .)* end-of-line
end-of-line=	'\r\n' | '\n' | '\r'
end-of-file=	!.

%%

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
  trailer= strdup(text);
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
  fprintf(stderr, "  -v          be verbose\n");
  fprintf(stderr, "  -V          print version number and exit\n");
  fprintf(stderr, "if no <file> is given, input is read from stdin\n");
  fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
  exit(1);
}

int main(int argc, char **argv)
{
  Node *n;
  int   c;

  output     = stdout;
  input      = stdin;
  lineNumber = 1;
  fileName   = "<stdin>";

  while (-1 != (c= getopt(argc, argv, "Vho:v")))
    {
      switch (c)
	{
	case 'V':
	  version(basename(argv[0]));
	  exit(0);

	case 'h':
	  usage(basename(argv[0]));
	  break;

	case 'o':
	  if (!(output= fopen(optarg, "w")))
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

  if (argc)
    {
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
	  lineNumber= 1;
	  if (!yyparse())
	    yyerror("syntax error");
	  if (input != stdin)
	    fclose(input);
	}
    }
  else
    if (!yyparse())
      yyerror("syntax error");

  if (verboseFlag)
    for (n= rules;  n;  n= n->any.next)
      Rule_print(n);

  Rule_compile_c_header();

  for (; headers;  headers= headers->next)
    fprintf(output, "%s\n", headers->text);

  if (rules)
    Rule_compile_c(rules);

  if (trailer)
    fprintf(output, "%s\n", trailer);

  return 0;
}
