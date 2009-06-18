#include <stdio.h>
#include <stdlib.h>


#ifdef PEG
#include "accept_peg.inc"
#endif

#ifdef LEG
#include "accept_leg.inc"
#endif

int main()
{
  while (yyparse());

  return 0;
}
