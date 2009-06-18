#include <stdio.h>

#ifdef PEG
#include "test_peg.inc"
#endif

#ifdef LEG
#include "test_leg.inc"
#endif

int main()
{
  while (yyparse());
  return 0;
}
