#include <stdio.h>
#include <stdlib.h>

int lines= 0, words= 0, chars= 0;

#include "../header.var"

#include INCLUDE

#include "../footer.var"

int main()
{
    YYClass* theParser = yyMakeParser();

    while (yyParseFrom(theParser, yy_start, "start"));


    printf("%d lines\n", lines);
    printf("%d chars\n", chars);
    printf("%d words\n", words);

    return 0;
}
