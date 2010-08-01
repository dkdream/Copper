
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include "version.h"
#include "tree.h"


static void charClassSet  (unsigned char bits[], int c)	{ bits[c >> 3] |=  (1 << (c & 7)); }
static void charClassClear(unsigned char bits[], int c)	{ bits[c >> 3] &= ~(1 << (c & 7)); }

typedef void (*setter)(unsigned char bits[], int c);

static unsigned char matchOne(unsigned char *cclass)
{
    if ('^' == *cclass) return 0;

    unsigned char value   = 0;
    unsigned char current = 0;
    int           prev    = -1;

    while ((current = *cclass++)) {
        if ('-' == current && *cclass && prev >= 0)
            {
                unsigned next = *cclass++;
                if (prev != next) return 0;
                prev = -1;
                continue;
            }

        if ('\\' != current)
            {
                if (value && (value != current)) return 0;
                value = prev = current;
                continue;
            }

        if (!*cclass)
            {
                if (value && (value != current)) return 0;
                value = prev = current;
                continue;
            }

        switch (current = *cclass++) {
        case 'a':  current = '\a'; break; /* bel */
        case 'b':  current = '\b'; break; /* bs */
        case 'e':  current = '\e'; break; /* esc */
        case 'f':  current = '\f'; break; /* ff */
        case 'n':  current = '\n'; break; /* nl */
        case 'r':  current = '\r'; break; /* cr */
        case 't':  current = '\t'; break; /* ht */
        case 'v':  current = '\v'; break; /* vt */
        default: break;
        }

        if (value && (value != current)) return 0;
        value = prev = current;
    }

    return value;
}

static char *makeChrText(unsigned char value)
{
    static char text[4];

    switch (value) {
    case '\a': return "\\a"; /* bel */
    case '\b': return "\\b"; /* bs */
    case '\e': return "\\e"; /* esc */
    case '\f': return "\\f"; /* ff */
    case '\n': return "\\n"; /* nl */
    case '\r': return "\\r"; /* cr */
    case '\t': return "\\t"; /* ht */
    case '\v': return "\\v"; /* vt */
    case '\'': return "\\'"; /* ' */
    case '\"': return "\\\"";
    case '\\': return "\\\\";
    default:
        break;
    }

    text[0] = (char) value;
    text[1] = 0;
    return text;
}

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

static unsigned textlen(char *text)
{
    unsigned len = 0;
    for ( ; *text; ++text) {
        if ('\\' == *text) {
            text += 1;
        }
        len += 1;
    }
    return len;
}

static bool Node_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current);

static bool Alternate_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current)
{
    if (!node) return false;

    unsigned right = 0;

    if (!Alternate_compile_vm(ofile, name, index, node->any.next, &right)) {
        // start the tail;
        return Node_compile_vm(ofile, name, index, node, current);
    }

   unsigned left = 0;

    if (!Node_compile_vm(ofile, name, right + 1, node, &left)) {
        *current = right;
        return true;
    }

    fprintf(ofile,
            "static struct prs_pair   ll_%s_%u_arg = { &ll_%s_%u, &ll_%s_%u };\n"
            "static struct prs_node   ll_%s_%u     = { 0,0, prs_Choice, (union prs_arg) (&ll_%s_%u_arg) };\n",
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

    if (!Sequence_compile_vm(ofile, name, index, node->any.next, &right)) {
        // start the tail;
        return Node_compile_vm(ofile, name, index, node, current);
    }

   unsigned left = 0;

    if (!Node_compile_vm(ofile, name, right + 1, node, &left)) {
        *current = right;
        return true;
    }

    fprintf(ofile,
            "static struct prs_pair   ll_%s_%u_arg = { &ll_%s_%u, &ll_%s_%u };\n"
            "static struct prs_node   ll_%s_%u     = { 0,0, prs_Sequence, (union prs_arg) (&ll_%s_%u_arg) };\n",
            name, left + 1,  name, left, name, right,
            name, left + 1,  name, left + 1
            );

    *current = left + 1;
    return true;
}

static bool Node_compile_vm(FILE* ofile, char* name, unsigned index, Node *node, unsigned* current)
{
    if (!node) return false;

    switch (node->type) {
    case Rule: return false;

    case Dot:
        fprintf(ofile,
                "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchDot };\n",
                name, index
                );
        *current = index;
        return true;

    case Name:
        fprintf(ofile,
                "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchName, (union prs_arg) ((PrsName)\"%s\") };\n",
                name, index,
                node->name.rule->rule.name
                );
        *current = index;
        return true;

    case Character:
        fprintf(ofile,
                "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchChar, (union prs_arg) ((PrsChar)\'%s\') };\n",
                name, index,
                node->string.value
                );
        *current = index;
        return true;

    case String:
        {
            unsigned tlen = textlen(node->string.value);
            if (1 < tlen) {
                fprintf(ofile,
                        "static struct prs_string ll_%s_%u_arg = { %u, \"%s\" };\n"
                        "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchString, (union prs_arg) (&ll_%s_%u_arg) };\n",
                        name, index,  textlen(node->string.value), node->string.value,
                        name, index,  name, index
                        );
            } else {
                fprintf(ofile,
                        "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchChar, (union prs_arg) ((PrsChar)\'%s\') };\n",
                        name, index,
                        node->string.value
                        );
            }
            *current = index;
        }
        return true;

    case Class:
        {
            const unsigned char value = matchOne(node->cclass.value);
            if (0 < value) {
                fprintf(ofile,
                        "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchChar, (union prs_arg) ((PrsChar)'%s') };\n",
                        name, index,
                        makeChrText(value)
                        );
            } else {
                const char* label    = makeCCName(node->cclass.value);
                const char* bitfield = makeCharClass(node->cclass.value);
                fprintf(ofile,
                        "static struct prs_set    ll_%s_%u_arg = { \"%s\", \"%s\" };\n"
                        "static struct prs_node   ll_%s_%u     = { 0,0, prs_MatchSet, (union prs_arg) (&ll_%s_%u_arg) };\n",
                        name, index,  label,      bitfield,
                        name, index,  name, index
                        );
            }
            *current = index;
        }
        return true;

    case Action:
        {
            const char* event = node->action.name;
            fprintf(ofile,
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_Event, (union prs_arg) (&event_%s) };\n",
                    name, index, event
                    );
            *current = index;
        }
        return true;

    case Predicate:
        {
            fprintf(ofile,
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_Predicate, (union prs_arg) ((PrsName)\"%s\") };\n",
                    name, index,  node->predicate.name
                    );
            *current = index;
        }
        return true;

    case Mark:
        {
            fprintf(ofile,
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_Action, (union prs_arg) ((PrsName)\"%s\") };\n",
                    name, index,   node->mark.name
                    );
            *current = index;
        }
        return true;

    case Alternate:
        return Alternate_compile_vm(ofile, name, index, node->alternate.first, current);

    case Sequence:
        return Sequence_compile_vm(ofile, name, index, node->sequence.first, current);

    case PeekFor:
        {
            unsigned child = 0;
            if (!Node_compile_vm(ofile, name, index, node->peekFor.element, &child)) return false;
            fprintf(ofile,
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_AssertTrue, (union prs_arg) (&ll_%s_%u) };\n",
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
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_AssertFalse, (union prs_arg) (&ll_%s_%u) };\n",
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
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_ZeroOrOne, (union prs_arg) (&ll_%s_%u) };\n",
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
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_ZeroOrMore, (union prs_arg) (&ll_%s_%u) };\n",
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
                    "static struct prs_node   ll_%s_%u     = { 0,0, prs_OneOrMore, (union prs_arg) (&ll_%s_%u) };\n",
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

void Rule_compile_vm(FILE* ofile, const char* label)
{
    Node *current = 0;

    fprintf(ofile, "/*-*- mode: c;-*-*/\n");
    fprintf(ofile, "/* A recursive-descent parser generated by copper %d.%d.%d */\n", COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
    fprintf(ofile, "\n");
    fprintf(ofile, "#include \"copper_vm.h\"\n");
    fprintf(ofile, "\n");

    // predicates
    for (current = predicates;  current;  current = current->predicate.list) {
        if (current->predicate.rule) {
            fprintf(ofile, "static bool %s(PrsInput);\n", current->predicate.name);
        }
    }

    // action
    for (current = marks;  current;  current = current->mark.list) {
        if (current->mark.rule) {
            fprintf(ofile, "static void %s(PrsInput);\n", current->mark.name);
        }
    }

    // event
    for (current = actions;  current;  current = current->action.list) {
        fprintf(ofile, "static bool event_%s(PrsInput, PrsState);\n", current->action.name);
    }

    for (current = rules; current;  current = current->rule.next) {
        fprintf(ofile, "\n");
        unsigned last = 0;
        if (!Node_compile_vm(ofile, current->rule.name, 0, current->rule.expression, &last)) {
            fprintf(ofile,
                    "static PrsNode node_%s = 0;\n",
                    current->rule.name);
        } else {
            fprintf(ofile,
                    "static PrsNode node_%s = &ll_%s_%u;\n",
                    current->rule.name,
                    current->rule.name, last);
        }
    }

    // predicates
    for (current = predicates;  current;  current = current->predicate.list) {
        if (current->predicate.rule) {
            fprintf(ofile, "static bool %s(PrsInput) {\n", current->predicate.name);
            fprintf(ofile, "#if 0\n");
            fprintf(ofile, "  %s;\n", current->predicate.text);
            fprintf(ofile, "#endif\n");
            fprintf(ofile, "  return false;\n");
            fprintf(ofile, "}\n");
        }
    }

    fprintf(ofile, "\n");

    // action
    for (current = marks;  current;  current = current->mark.list) {
        if (current->mark.rule) {
            fprintf(ofile, "static void %s(PrsInput);\n", current->mark.name);
            fprintf(ofile, "#if 0\n");
            fprintf(ofile, "  %s;\n", current->mark.text);
            fprintf(ofile, "#endif\n");
            fprintf(ofile, "}\n");
        }
    }

    for (current = actions;  current;  current = current->action.list) {
        fprintf(ofile, "static bool event_%s(PrsInput input, PrsState state) {\n", current->action.name);
        fprintf(ofile, "#if 0\n");
        fprintf(ofile, "  %s;\n", current->action.text);
        fprintf(ofile, "#endif\n");
        fprintf(ofile, "  return true;\n");
        fprintf(ofile, "}\n");
    }

    fprintf(ofile, "\n\n");

    fprintf(ofile, "extern bool init__");
    for (; *label; ++label) {
        if (isalnum(*label)) {
            fprintf(ofile, "%c", *label);
        } else {
            fprintf(ofile, "_");
        }
    }
    fprintf(ofile, "(PrsInput input) {\n");
    fprintf(ofile, "  inline bool attach(PrsName name,    PrsNode value)      { return input->attach(input, name, value); }\n");
    fprintf(ofile, "  inline bool predicate(PrsName name, PrsPredicate value) { return input->set_p(input, name, value); }\n");
    fprintf(ofile, "  inline bool action(PrsName name,    PrsAction value)    { return input->set_a(input, name, value); }\n");
    fprintf(ofile, "  inline bool event(PrsName name,     PrsEvent value)     { return input->set_e(input, name, value); }\n");
    fprintf(ofile, "\n");

    for (current = rules; current;  current = current->rule.next) {
        fprintf(ofile, "   attach(\"%s\", node_%s);\n",
                current->rule.name, current->rule.name);
    }

    fprintf(ofile, "\n");
    for (current = predicates;  current;  current = current->predicate.list) {
        if (current->predicate.rule) {
            fprintf(ofile, "   predicate(\"%s\", %s);\n",
                    current->predicate.name,
                    current->predicate.name);
        }
    }
    fprintf(ofile, "\n");
    for (current = marks;  current;  current = current->mark.list) {
        if (current->mark.rule) {
            fprintf(ofile, "   action(\"%s\", %s);\n",
                    current->mark.name,
                    current->mark.name);

        }
    }
    fprintf(ofile, "\n");
    for (current = actions;  current;  current = current->action.list) {
        fprintf(ofile, "   event(\"event_%s\", event_%s);\n",
                current->action.name,
                current->action.name);
    }
    fprintf(ofile, "\n");
    fprintf(ofile, "  return true;\n");
    fprintf(ofile, "}\n");
    fprintf(ofile, "\n\n\n");
}
