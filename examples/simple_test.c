#include <stdio.h>
#include <stdlib.h>

int stack[1024];
int stackp = -1;

int vars[26];
int var = 0;


int push(int n) { return stack[++stackp]= n; }
int pop(void)   { return stack[stackp--]; }
int top(void)   { return stack[stackp]; }

#include INCLUDE

int main()
{
  while (yyparse());

  return 0;
}