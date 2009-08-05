
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "version.h"
#include "tree.h"

#include "compile.inc"

static int yyl(void)
{
  static int prev = 1;
  return ++prev;
}

static void charClassSet  (unsigned char bits[], int c)	{ bits[c >> 3] |=  (1 << (c & 7)); }
static void charClassClear(unsigned char bits[], int c)	{ bits[c >> 3] &= ~(1 << (c & 7)); }

typedef void (*setter)(unsigned char bits[], int c);

static char *makeCharClass(unsigned char *cclass)
{
  unsigned char	 bits[32];
  setter	 set;
  int		 c, prev= -1;
  static char	 string[256];
  char		*ptr;

  if ('^' == *cclass)
    {
      memset(bits, 255, 32);
      set= charClassClear;
      ++cclass;
    }
  else
    {
      memset(bits, 0, 32);
      set= charClassSet;
    }
  while ((c= *cclass++))
    {
      if ('-' == c && *cclass && prev >= 0)
	{
	  for (c= *cclass++;  prev <= c;  ++prev)
	    set(bits, prev);
	  prev= -1;
	}
      else if ('\\' == c && *cclass)
	{
	  switch (c= *cclass++)
	    {
	    case 'a':  c= '\a'; break;	/* bel */
	    case 'b':  c= '\b'; break;	/* bs */
	    case 'e':  c= '\e'; break;	/* esc */
	    case 'f':  c= '\f'; break;	/* ff */
	    case 'n':  c= '\n'; break;	/* nl */
	    case 'r':  c= '\r'; break;	/* cr */
	    case 't':  c= '\t'; break;	/* ht */
	    case 'v':  c= '\v'; break;	/* vt */
	    default:		break;
	    }
	  set(bits, prev= c);
	}
      else
	set(bits, prev= c);
    }

  ptr= string;
  for (c= 0;  c < 32;  ++c)
    ptr += sprintf(ptr, "\\%03o", bits[c]);

  return string;
}

static char *makeCCName(unsigned char *cclass)
{
    static char string[256];
    int	        chr;
    char*       ptr = string;

    while ((chr = *cclass++)) {
        switch (chr) {
        case '\'':
        case '\"':
        case '\\':
            *ptr++ = '\\';
        default:
            *ptr++ = chr;
        }
    }
    *ptr = 0;
    return string;
}

static void label(int n)
{
    fprintf(output, "\n  l%d:;\t", n);
}

static void jump(int n)
{
    if (1 < n) {
        fprintf(output, " goto l%d;", n);
    } else if (1 == n) {
        fprintf(output, " goto %s;", "passed");
    } else {
        fprintf(output, " goto %s;", "failed");
    }
}

static void save(int n)
{
  fprintf(output, "\n  YYState yystate%d;", n);
  fprintf(output, "\n  YY_SEND(save_, &yystate%d);", n);
}

static void restore(int n)
{
   fprintf(output, "\n  YY_SEND(restore_, &yystate%d);", n);
}

static void Node_compile_c_ko(Node *node, int ko)
{
  assert(node);
  switch (node->type)
    {
    case Rule:
      fprintf(stderr, "\ninternal error #1 (%s)\n", node->rule.name);
      exit(1);
      break;

    case Dot:
      fprintf(output, "  if (!yymatchDot(yySelf))");
      jump(ko);
      break;

    case Name:
      fprintf(output, "  if (!YY_SEND(apply_, yystack, &yy_%s, \"%s\"))",
              node->name.rule->rule.name,
              node->name.rule->rule.name);
      jump(ko);
      if (node->name.variable)
	fprintf(output, "  yyDo(yySelf, \"yySet\", yySet, %d, yystate0);", node->name.variable->variable.offset);
      break;

    case Character:
    case String:
      {
	int len = strlen(node->string.value);
	if (1 == len || (2 == len && '\\' == node->string.value[0]))
	  fprintf(output, "  if (!yymatchChar(yySelf, '%s'))", node->string.value);
	else
	  fprintf(output, "  if (!yymatchString(yySelf, \"%s\"))", node->string.value);
        jump(ko);
      }
      break;

    case Class:
      fprintf(output, "  if (!yymatchClass(yySelf, \"%s\", (unsigned char *)\"%s\"))",
              makeCCName(node->cclass.value),
              makeCharClass(node->cclass.value));
      jump(ko);
      break;

    case Action:
        fprintf(output, "  yyDo(yySelf, \" yy%s\", yy%s, 0, yystate0);", node->action.name, node->action.name);
      break;

    case Predicate:
      fprintf(output, "  if (!(%s))", node->predicate.text);
      jump(ko);
      break;

    case Mark:
      fprintf(output, "  { %s; }", node->mark.text);
      break;

    case Alternate:
      {
	int ok = yyl();
	save(ok);
	for (node = node->alternate.first;  node;  node = node->alternate.next) {
	  if (!node->alternate.next)
              Node_compile_c_ko(node, ko);
	  else {
              int next = yyl();
              Node_compile_c_ko(node, next);
              jump(ok);
              label(next);
              restore(ok);
          }
        }
	label(ok);
      }
      break;

    case Sequence:
      for (node = node->sequence.first;  node;  node = node->sequence.next)
	Node_compile_c_ko(node, ko);
      break;

    case PeekFor:
      {
	int ok = yyl();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ko);
	restore(ok);
      }
      break;

    case PeekNot:
      {
	int ok = yyl();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ok);
	jump(ko);
	label(ok);
	restore(ok);
      }
      break;

    case Query:
      {
	int qko = yyl(), qok = yyl();
	save(qko);
	Node_compile_c_ko(node->query.element, qko);
	jump(qok);
	label(qko);
	restore(qko);
	label(qok);
      }
      break;

    case Star:
      {
	int again = yyl(), out = yyl();
	label(again);
	save(out);
	Node_compile_c_ko(node->star.element, out);
	jump(again);
	label(out);
	restore(out);
      }
      break;

    case Plus:
      {
	int again = yyl(), out = yyl();
	Node_compile_c_ko(node->plus.element, ko);
	label(again);
	save(out);
	Node_compile_c_ko(node->plus.element, out);
	jump(again);
	label(out);
	restore(out);
      }
      break;

    default:
      fprintf(stderr, "\nNode_compile_c_ko: illegal node type %d\n", node->type);
      exit(1);
    }
}

static int countVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      ++count;
      node= node->variable.next;
    }
  return count;
}

static void defineVariables(Node *node, const char* name)
{
    fprintf(output, "  static const char* yyrulename = \"%s\";\n", name);
    fprintf(output, "  int   yyleng = yyThunkText(yySelf, thunk);\n");
    fprintf(output, "  char* yytext = yySelf->text;\n");
    fprintf(output, "  int   yypos  = yySelf->pos;\n\n");

    if (node) {
        int count = 0;
        fprintf(output, "  const int frame = yySelf->frame;\n");
        while (node)
            {
                fprintf(output, "#define %s yySelf->vals[frame + %d]\n", node->variable.name, --count);
                node->variable.offset= count;
                node= node->variable.next;
            }
    }

    fprintf(output, "#define yy yySelf->result\n");
    fprintf(output, "#define yythunkpos yySelf->thunkpos\n");
}

static void undefineVariables(Node *node)
{
    fprintf(output, "#undef yy\n");
    fprintf(output, "#undef yythunkpos\n");

    if (!node) fprintf(output, "\n  // for references ONLY\n");
    else
        {
            while (node)
                {
                    fprintf(output, "#undef %s\n", node->variable.name);
                    node= node->variable.next;
                }
            fprintf(output, "\n  // for references ONLY\n");
            fprintf(output, "  (void)frame;\n");
        }

    fprintf(output, "  (void)yyrulename;\n");
    fprintf(output, "  (void)yyleng;\n");
    fprintf(output, "  (void)yytext;\n");
    fprintf(output, "  (void)yypos;\n");
}


static void Rule_compile_c2(Node *node)
{
  assert(node);
  assert(Rule == node->type);

  if (!node->rule.expression) {
      if (!(RuleDeclared & node->rule.flags))
          fprintf(stderr, "rule '%s' used but not defined\n", node->rule.name);
  } else {
      if (!(RuleUsed & node->rule.flags)) {
          if (!(RuleExported & node->rule.flags))
              fprintf(stderr, "rule '%s' defined but not used\n", node->rule.name);
      }

      int safe = ((Query == node->rule.expression->type) || (Star == node->rule.expression->type));

      fprintf(output, "\nint yy_%s(YYClass* yySelf, YYStack* yystack)\n{", node->rule.name);
      fprintf(output, "\n  static const char* yyrulename = \"%s\";\n", node->rule.name);
      fprintf(output, "\n  YYState yystate0 = yystack->begin;\n");

      if (node->rule.variables)
          fprintf(output, "  yyDo(yySelf, \"yyPush\", yyPush, %d, yystate0);\n", countVariables(node->rule.variables));

      if (node->rule.begin)
        {
          fprintf(output, "\n%s;\n", node->rule.begin);
          fprintf(output, "  goto start_rule;\n");
        }

      fprintf(output, "\n  start_rule:;\n");

      Node_compile_c_ko(node->rule.expression, (safe ? 1 : 0));
      fprintf(output, "\n  goto passed;");
      fprintf(output, "\n\n  passed:");
      if (node->rule.end)
          fprintf(output, "\n{ %s }\n", node->rule.end);

      if (node->rule.variables)
          fprintf(output, "  yyDo(yySelf, \"yyPop\", yyPop, %d, yystate0);", countVariables(node->rule.variables));

      fprintf(output, "\n  return 1;");

      if (!safe) {
          fprintf(output, "\n  goto failed;");
          fprintf(output, "\n\n  failed:");
          if (node->rule.end)
              fprintf(output, "\n{ %s }\n", node->rule.end);

          fprintf(output, "\n  return 0;");
      }

      fprintf(output, "\n  // for references ONLY");
      fprintf(output, "\n  (void)yyrulename;");
      fprintf(output, "\n  (void)yystate0;");
      fprintf(output, "\n  if (0) goto start_rule;");
      fprintf(output, "\n}\n");
  }

  if (node->rule.next)
      Rule_compile_c2(node->rule.next);
}

int consumesInput(Node *node)
{
  if (!node) return 0;

  switch (node->type)
    {
    case Rule:
      {
	int result= 0;
	if (RuleReached & node->rule.flags)
          {
            node->rule.flags |= LeftRecursion;
            fprintf(stderr, "possible infinite left recursion in rule '%s'\n", node->rule.name);
          }
	else
	  {
	    node->rule.flags |= RuleReached;
	    result = consumesInput(node->rule.expression);
	    node->rule.flags &= ~RuleReached;
	  }
	return result;
      }
      break;

    case Dot:		return 1;
    case Name:		return consumesInput(node->name.rule);
    case Character:
    case String:	return strlen(node->string.value) > 0;
    case Class:		return 1;
    case Action:	return 0;
    case Predicate:	return 0;

    case Alternate:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (!consumesInput(n))
	    return 0;
      }
      return 1;

    case Sequence:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (consumesInput(n))
	    return 1;
      }
      return 0;

    case PeekFor:	return 0;
    case PeekNot:	return 0;
    case Query:		return 0;
    case Star:		return 0;
    case Plus:		return consumesInput(node->plus.element);

    default:
      fprintf(stderr, "\nconsumesInput: illegal node type %d\n", node->type);
      exit(1);
    }
  return 0;
}


void Rule_compile_c_heading(FILE* ofile)
{
    if (!ofile) return;

    fprintf(ofile, "/* A recursive-descent parser generated by copper %d.%d.%d */\n", COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
    fprintf(ofile, "\n");
    fprintf(ofile, "%s", header);
    fprintf(ofile, "\n");
}

void Rule_compile_c_declare(FILE* ofile, Node *node) {
    if (!ofile) return;

    fprintf(ofile, "\n");
    fprintf(ofile, "/* %s */\n", "rules");

    Node *current;
    for (current = node; current;  current = current->rule.next)
        fprintf(ofile, "%s int yy_%s (YYClass* yySelf, YYStack* yystack);\n",
                current->rule.scope,
                current->rule.name);

    fprintf(ofile, "\n");
}

void Rule_compile_c(Node *node)
{
  Node *current;

  /*
  for (current = rules; current; current = current->rule.next)
    consumesInput(n);
  */

  fprintf(output, "%s", preamble);

  fprintf(output, "\n/* %s */\n", "actions");
  for (current = actions;  current;  current = current->action.list)
      fprintf(output, "static void yy%s (YYClass* yySelf, YYThunk thunk);\n", current->action.name);

  fprintf(output, "\n");

  for (current = actions;  current;  current = current->action.list)
    {
      fprintf(output, "static void yy%s(YYClass* yySelf, YYThunk thunk)\n{\n", current->action.name);
      defineVariables(current->action.rule->rule.variables, current->action.rule->rule.name);
      fprintf(output, "  YY_SEND(debug_, \"do yy%s (%%s) \'%%s\'\\n\", yyrulename, yytext);\n\n", current->action.name);
      fprintf(output, "  %s;\n", current->action.text);
      undefineVariables(current->action.rule->rule.variables);

      fprintf(output, "}\n");
    }

  Rule_compile_c2(node);
}

void Rule_compile_c_footing(FILE* ofile)
{
    if (!ofile) return;

    fprintf(ofile, "/* A recursive-descent parser generated by copper %d.%d.%d */\n", COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
    fprintf(ofile, "\n");
    fprintf(ofile, "%s", footer);
    fprintf(ofile, "\n");
}
