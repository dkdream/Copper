/*-*- mode: c;-*-*/
#include "tree.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <stdarg.h>

typedef struct Header Header;

struct Header {
    char   *text;
    Header *next;
};

FILE *input = 0;

int verboseFlag = 0;

static int	 lineNumber = 0;
static char	*fileName   = 0;
static char	*trailer    = 0;
static Header	*headers    = 0;

void makeHeader(char *text);
void makeTrailer(char *text);

void yyerror(char *message);

static inline void checkRule(char *name) {
    if (push(beginRule(findRule(name)))->rule.expression) {
        fprintf(stderr, "rule '%s' redefined\n", name);
    }
}

enum RuleKind {
    simple_rule,
    rule_with_begin,
    rule_with_end,
    rule_with_both,
};

static inline void defineRule(enum RuleKind kind) {
    Node* expression;
    Node* begin;
    Node* end;
    Node* rule;

    switch (kind) {
    case simple_rule:
        expression = pop();
        rule       = pop();
        assert(Rule == rule->type);
        rule->rule.begin = 0;
        rule->rule.end = 0;
        Rule_setExpression(rule, expression);
        break;

    case rule_with_begin:
        expression = pop();
        begin      = pop();
        rule       = pop();
        assert(Rule == rule->type);
        assert(Predicate == begin->type);
        rule->rule.begin = begin->predicate.text;
        rule->rule.end   = 0;
        Rule_setExpression(rule, expression);
        break;

    case rule_with_end:
        end        = pop();
        expression = pop();
        rule       = pop();
        assert(Rule == rule->type);
        assert(Predicate == end->type);
        rule->rule.begin = 0;
        rule->rule.end   = end->predicate.text;
        Rule_setExpression(rule, expression);
        break;

    case rule_with_both:
        end        = pop();
        expression = pop();
        begin      = pop();
        rule       = pop();
        assert(Rule == rule->type);
        assert(Predicate == begin->type);
        assert(Predicate == end->type);
        rule->rule.begin = begin->predicate.text;
        rule->rule.end   = end->predicate.text;
        Rule_setExpression(rule, expression);
        break;
    }
}

/*-- end of copper.inc --*/
