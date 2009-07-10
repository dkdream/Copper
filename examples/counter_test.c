#include <stdio.h>
#include <stdlib.h>

int lines= 0, words= 0, chars= 0;

#include INCLUDE

int main()
{
  while (yyparse());

  printf("%d lines\n", lines);
  printf("%d chars\n", chars);
  printf("%d words\n", words);

  return 0;
}
