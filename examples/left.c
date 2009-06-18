#include <stdio.h>

#include "left_leg.inc"

int main()
{
  printf(yyparse() ? "success\n" : "failure\n");

  return 0;
}
