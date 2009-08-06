# include <stdio.h>

typedef struct line line;

struct line
{
    int	  number;
    int	  length;
    char *text;
};

line *lines= 0;
int   numLines= 0;
int   pc= -1, epc= -1;
int   batch= 0;

int getline(char *buf, int max);

# define min(x, y) ((x) < (y) ? (x) : (y))

union value {
    int		  number;
    char	 *string;
    int		(*binop)(int lhs, int rhs);
};

# define YYSTYPE union value

int variables[26];

void accept(int number, char *line);

void save(char *name);
void load(char *name);
void type(char *name);

int lessThan(int lhs, int rhs)	{ return lhs <  rhs; }
int lessEqual(int lhs, int rhs)	{ return lhs <= rhs; }
int notEqual(int lhs, int rhs)	{ return lhs != rhs; }
int equalTo(int lhs, int rhs)		{ return lhs == rhs; }
int greaterEqual(int lhs, int rhs)	{ return lhs >= rhs; }
int greaterThan(int lhs, int rhs)	{ return lhs >  rhs; }

int input(void);

int stack[1024], sp= 0;

char *help;

void error(char *fmt, ...);
int findLine(int n, int create);

#include "../header.var"

#include INCLUDE

#include "../footer.var"

#include <unistd.h>
#include <stdarg.h>

char *help=
    "print <num>|<string> [, <num>|<string> ...] [,]\n"
    "if <expr> <|<=|<>|=|>=|> <expr> then <stmt>\n"
    "input <var> [, <var> ...]     let <var> = <expr>\n"
    "goto <expr>                   gosub <expr>\n"
    "end                           return\n"
    "list                          clear\n"
    "run [\"filename\"]              rem <comment...>\n"
    "dir                           type \"filename\"\n"
    "save \"filename\"               load \"filename\"\n"
    "bye|quit|exit                 help\n"
    ;

void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (epc > 0)
        fprintf(stderr, "\nline %d: %s", lines[epc-1].number, lines[epc-1].text);
    else
        fprintf(stderr, "\n");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    epc= pc= -1;
}

static inline int my_yy_input(YYClass* yySelf, char* buf, int max_size) {
    int result = 0;
    if ((pc >= 0) && (pc < numLines)) {
        line *linep= lines+pc++;
        result= min(max_size, linep->length);
        memcpy(buf, linep->text, result);
    } else {
        result = getline(buf, max_size);
    }
    return result;
}

#ifdef USE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

int getline(char *buf, int max)
{
    pc= -1;
    if (batch) exit(0);
    if (isatty(fileno(stdin)))
        {
#    ifdef USE_READLINE
            char *line= readline(">");
            if (line)
                {
                    int len= strlen(line);
                    if (len >= max) len= max - 1;
                    strncpy(buf, line, len);
                    (buf)[len]= '\n';
                    add_history(line);
                    free(line);
                    return len + 1;
                }
            else
                {
                    printf("\n");
                    return 0;
                }
#    endif
            putchar('>');
            fflush(stdout);
        }
    return fgets(buf, max, stdin) ? strlen(buf) : 0;
}

int maxLines= 0;

int findLine(int n, int create)
{
    int lo= 0, hi= numLines - 1;
    while (lo <= hi)
        {
            int mid= (lo + hi) / 2, lno= lines[mid].number;
            if (lno > n)
                hi= mid - 1;
            else if (lno < n)
                lo= mid + 1;
            else
                return mid;
        }
    if (create)
        {
            if (numLines == maxLines)
                {
                    maxLines *= 2;
                    lines= realloc(lines, sizeof(line) * maxLines);
                }
            if (lo < numLines)
                memmove(lines + lo + 1, lines + lo, sizeof(line) * (numLines - lo));
            ++numLines;
            lines[lo].number= n;
            lines[lo].text= 0;
            return lo;
        }
    return -1;
}

void accept(int n, char *s)
{
    if (s[0] < 32)	/* delete */
        {
            int lno= findLine(n, 0);
            if (lno >= 0)
                {
                    if (lno < numLines - 1)
                        memmove(lines + lno, lines + lno + 1, sizeof(line) * (numLines - lno - 1));
                    --numLines;
                }
        }
    else			/* insert */
        {
            int lno= findLine(n, 1);
            if (lines[lno].text) free(lines[lno].text);
            lines[lno].length= strlen(s);
            lines[lno].text= strdup(s);
        }
}

char *extend(char *name)
{
    static char path[1024];
    int len= strlen(name);
    sprintf(path, "%s%s", name, (((len > 4) && !strcasecmp(".bas", name + len - 4)) ? "" : ".bas"));
    return path;
}

void save(char *name)
{
    FILE *f= fopen(name= extend(name), "w");
    if (!f)
        perror(name);
    else
        {
            int i;
            for (i= 0;  i < numLines;  ++i)
                fprintf(f, "%d %s", lines[i].number, lines[i].text);
            fclose(f);
        }
}

void load(char *name)
{
    FILE *f= fopen(name= extend(name), "r");
    if (!f)
        perror(name);
    else
        {
            int  lineNumber;
            char lineText[1024];
            while ((1 == fscanf(f, " %d ", &lineNumber)) && fgets(lineText, sizeof(lineText), f))
                accept(lineNumber, lineText);
            fclose(f);
        }
}

void type(char *name)
{
    FILE *f= fopen(name= extend(name), "r");
    if (!f)
        perror(name);
    else
        {
            int  c = 0;
            int  d = 0;
            while ((c= getc(f)) >= 0)
                putchar(d= c);
            fclose(f);
            if ('\n' != d && '\r' != d) putchar('\n');
        }
}

int input(void)
{
    char line[32];
    fgets(line, sizeof(line), stdin);
    return atoi(line);
}

int main(int argc, char **argv)
{
    lines= malloc(sizeof(line) * (maxLines= 32));
    numLines= 0;

    if (argc > 1)
        {
            batch= 1;
            while (argc-- > 1)
                load(*++argv);
            pc= 0;
        }

    YYClass* theParser = yyMakeParser();

    theParser->input_ = my_yy_input;

    while (!feof(stdin))
        yyParseFrom(theParser, yy_start, "start");

    return 0;
}
