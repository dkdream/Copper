#include <stdio.h>
#include <stdlib.h>


#ifdef PEG
#include "rule_peg.inc"
#endif

#ifdef LEG
#include "rule_leg.inc"
#endif

int main()
{
  while (yyparse());

  return 0;
}
