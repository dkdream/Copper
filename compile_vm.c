
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "version.h"
#include "tree.h"


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
      set = charClassClear;
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

static bool Node_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current);

static bool Alternate_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current)
{
    if (!node) {
        return false;
    }

    unsigned right = 0;

    if (!Alternate_compile_vm(ofile, name, index, node->alternate.next, &right)) {
        // start the tail;
        return Node_compile_vm(ofile, name, index, node->alternate.first, current);
    }

   unsigned left = 0;

    if (!Node_compile_vm(ofile, name, right + 1, node->alternate.first, &left)) {
        *current = right;
        return true;
    }

    fprintf(ofile,
            "static struct prs_pair ll_%s_%u_arg = { &ll_%s_%u, &ll_%s_%u };\n"
            "static struct prs_node ll_%s_%u = { prs_Choice, (union prs_arg) (&ll_%s_%u_arg) };\n",
            name, left + 1,  name, left, name, right,
            name, left + 1,  name, left + 1
            );

    *current = left + 1;
    return true;
}

static bool Sequence_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current)
{
    if (!node) return false;

    unsigned right = 0;

    if (!Sequence_compile_vm(ofile, name, index, node->sequence.next, &right)) {
        // start the tail;
        return Node_compile_vm(ofile, name, index, node->sequence.first, current);
    }

    unsigned left = 0;

    if (!Node_compile_vm(ofile, name, right + 1, node->sequence.first, &left)) {
        *current = right;
        return true;
    }

    fprintf(ofile,
            "static struct prs_pair ll_%s_%u_arg = { &ll_%s_%u, &ll_%s_%u };\n"
            "static struct prs_node ll_%s_%u     = { prs_Sequence, (union prs_arg) (&ll_%s_%u_arg) };\n",
            name, left + 1, name, left, name, right,
            name, left + 1,  name, left + 1
            );

    *current = left + 1;
    return true;
}

static bool Node_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current)
{
    switch (node->type) {
    case Rule: return false;

    case Dot:
        fprintf(ofile,
                "static struct prs_node ll_%s_%u = { prs_MatchDot };\n",
                name, index
                );
        *current = index;
        return true;

    case Name:
        fprintf(ofile,
                "static struct prs_node ll_%s_%u = { prs_MatchName, (union prs_arg) ((PrsName)\"%s\") };\n",
                name, index,
                node->name.rule->rule.name
                );
        *current = index;
        return true;

    case Character:
        fprintf(ofile,
                "static struct prs_node ll_%s_%u = { prs_MatchChar, (union prs_arg) ((PrsChar)\'%s\') };\n",
                name, index,
                node->string.value
                );
        *current = index;
        return true;

    case String:
        fprintf(ofile,
                "static struct prs_string ll_%s_%u_arg = { %d, \"%s\" };\n"
                "static struct prs_node   ll_%s_%u     = { prs_MatchString, (union prs_arg) (&ll_%s_%u_arg) };\n",
                name, index,  strlen(node->string.value), node->string.value,
                name, index,  name, index
                );
        *current = index;
        return true;

    case Class:
        {
            const char* label    = makeCCName(node->cclass.value);
            const char* bitfield = makeCharClass(node->cclass.value);
            fprintf(ofile,
                    "static struct prs_set  ll_%s_%u_arg = { \"%s\", \"%s\" };\n"
                    "static struct prs_node ll_%s_%u     = { prs_MatchSet, (union prs_arg) (&ll_%s_%u_arg) };\n",
                    name, index,  label,      bitfield,
                    name, index,  name, index
                    );
            *current = index;
        }
        return true;

    case Action:
        {
            const char* event = node->action.name;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_Event, (union prs_arg) (&event_%s) };\n",
                    name, index, event
                    );
        }
        return false;

    case Predicate:
        {
            fprintf(ofile,
                    "static bool predicate_%s_%u(PrsInput) {\n"
                    "  return %s;\n"
                    "}\n"
                    "static struct prs_node ll_%s_%u = { prs_Predicate, (union prs_arg) (&predicate_%s_%u) };\n",
                    name, index,
                    node->predicate.text,
                    name, index, name, index
                    );
        }
        return false;

    case Mark:
        {
            fprintf(ofile,
                    "static void action_%s_%u(PrsInput) {\n"
                    "   %s\n"
                    "}\n"
                    "static struct prs_node ll_%s_%u = { prs_Action, (union prs_arg) (&action_%s_%u) };\n",
                    name, index,
                    node->predicate.text,
                    name, index, name, index
                    );
        }
        return false;

    case Alternate:
        return Alternate_compile_vm(ofile, name, index, node, current);

    case Sequence:
        return Sequence_compile_vm(ofile, name, index, node, current);

    case PeekFor:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->peekFor.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_AssertTrue, (union prs_arg) (&ll_%s_%u) };\n",
                    name, child + 1,
                    name, child
                    );
            *current = child + 1;
        }
        return true;

    case PeekNot:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->peekNot.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_AssertFalse, (union prs_arg) (&ll_%s_%u) };\n",
                    name, child + 1,
                    name, child
                    );
            *current = child + 1;
        }
        return true;

    case Query:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->query.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_ZeroOrOne, (union prs_arg) (&ll_%s_%u) };\n",
                    name, child + 1,
                    name, child
                    );
            *current = child + 1;
        }
        return true;

    case Star:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->star.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_ZeroOrMore, (union prs_arg) (&ll_%s_%u) };\n",
                    name, child + 1,
                    name, child
                    );
            *current = child + 1;
        }
        return true;

    case Plus:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->plus.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node ll_%s_%u = { prs_OneOrMore, (union prs_arg) (&ll_%s_%u) };\n",
                    name, child + 1,
                    name, child
                    );
            *current = child + 1;
        }
        return true;

    default:
        fprintf(stderr, "\nNode_compile_vm: illegal node type %d\n", node->type);
        exit(1);
    }
}

void Rule_compile_vm(FILE* ofile)
{
    Node *current = 0;

    fprintf(ofile, "\n\n\n");
    fprintf(ofile, "#include \"copper_vm.h\"\n");
    fprintf(ofile, "\n\n\n");

    // rules
    for (current = rules; current;  current = current->rule.next) {
        fprintf(ofile,
                "extern PrsNode node_%s;\n",
                current->rule.name);
    }

    fprintf(ofile, "\n\n\n");

    // event
    for (current = actions;  current;  current = current->action.list) {
        fprintf(ofile, "static bool event_%s(PrsInput);\n", current->action.name);
    }

    fprintf(ofile, "\n\n\n");

    for (current = rules; current;  current = current->rule.next) {
        fprintf(ofile, "\n");
        unsigned last = 0;
        if (!Node_compile_vm(ofile, current->rule.name, 0, current->rule.expression, &last)) {
            fprintf(ofile,
                    "PrsNode node_%s = 0;\n",
                    current->rule.name);
        } else {
            fprintf(ofile,
                    "PrsNode node_%s = &ll_%s_%u;\n",
                    current->rule.name,
                    current->rule.name, last);
        }
    }

    fprintf(ofile, "\n\n\n");

    for (current = actions;  current;  current = current->action.list) {
        fprintf(ofile, "static bool action_%s(PrsInput) {\n", current->action.name);
        fprintf(ofile, "#if 0\n");
        fprintf(ofile, "  %s;\n", current->action.text);
        fprintf(ofile, "#endif\n");
        fprintf(ofile, "  return false\n");
        fprintf(ofile, "}\n");
    }

}
