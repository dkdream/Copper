/*-*- mode: c;-*-*/
/*-- end of copper.inc --*/
#ifndef COPPER_INC
#define COPPER_INC

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

extern FILE *input;
extern int verboseFlag;

void makeHeader(char *text);
void makeTrailer(char *text);
void yyerror(char *message);

static inline void checkRule(char *name) {
    if (push(beginRule(findRule(name)))->rule.expression) {
        fprintf(stderr, "rule '%s' redefined\n", name);
    }
}

static inline void checkMacro(char *name) {
    Node *node = findMacro(name);
    if (node->macro.value) {
        fprintf(stderr, "macro '%s' redefined\n", name);
    }
    push(node);
}

static inline void defineMacro(char* value) {
    Node *node = pop();
    assert(Macro == node->type);
    if (node->macro.value) {
        free(node->macro.value);
    }
    node->macro.value = strdup(value);
}

static inline char* fetchMacro(char *name) {
    Node *node = findMacro(name);
    if (node->macro.value) return node->macro.value;
    fprintf(stderr, "using undefined macro '%s' \n", name);
    return "";
}

enum RuleKind {
    simple_rule,
    rule_with_begin,
    rule_with_end,
    rule_with_both,
};

static inline void declareRule(char *name) {
    Node* node = findRule(name);

    if (node->rule.expression) {
        fprintf(stderr, "rule '%s' defined\n", name);
        return;
    }

    if (!(RuleDeclared & node->rule.flags)) {
        node->rule.flags |= RuleDeclared;
    } else {
        fprintf(stderr, "rule '%s' declared twice\n", name);
    }
}

static inline void exportRule(char *name) {
    Node* node = findRule(name);
    if (!(RuleExported & node->rule.flags)) {
        node->rule.flags |= RuleExported;
    } else {
        fprintf(stderr, "rule '%s' exported twice\n", name);
    }
}

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
        assert(Mark == begin->type);
        rule->rule.begin = begin->mark.text;
        rule->rule.end   = 0;
        Rule_setExpression(rule, expression);
        break;

    case rule_with_end:
        end        = pop();
        expression = pop();
        rule       = pop();
        assert(Rule == rule->type);
        assert(Mark == end->type);
        rule->rule.begin = 0;
        rule->rule.end   = end->mark.text;
        Rule_setExpression(rule, expression);
        break;

    case rule_with_both:
        end        = pop();
        expression = pop();
        begin      = pop();
        rule       = pop();
        assert(Rule == rule->type);
        assert(Mark == begin->type);
        assert(Mark == end->type);
        rule->rule.begin = begin->mark.text;
        rule->rule.end   = end->mark.text;
        Rule_setExpression(rule, expression);
        break;
    }
}

#include "header.var"
#include "preamble.var"

#endif
/*-- end of copper.inc --*/