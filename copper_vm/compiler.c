/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "copper_vm.h"
#include "syntax.h"

/* */
#include <string.h>

/* */

static bool make_Any(SynType   type,
                     SynTarget target)
{
    if (!target.any) return false;

    unsigned fullsize = 0;

    switch (type2kind(type)) {
    case syn_unknown:  return false;
    case syn_any:      fullsize = sizeof(struct syn_any);      break;
    case syn_define:   fullsize = sizeof(struct syn_define);   break;
    case syn_text:     fullsize = sizeof(struct syn_text);     break;
    case syn_chunk:    fullsize = sizeof(struct syn_chunk);    break;
    case syn_operator: fullsize = sizeof(struct syn_operator); break;
    case syn_tree:     fullsize = sizeof(struct syn_tree);     break;
    }

    SynAny result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->type = type;

    *(target.any) = result;

    return true;
}

static bool push(struct prs_file *file, SynNode value) {
    if (!file)      return false;
    if (!value.any) return false;

    unsigned       fullsize = sizeof(struct prs_cell);
    struct prs_stack *stack = &file->stack;
    struct prs_cell  *top  = stack->free_list;

    if (top) {
        stack->free_list = top->next;
    } else {
        top = malloc(fullsize);
    }

    memset(top, 0, fullsize);

    top->value = value;
    top->next  = stack->top;

    stack->top = top;

    return true;
}

static bool pop(struct prs_file *file, SynTarget target) {
    if (!file)       return false;
    if (!target.any) return false;

    struct prs_stack *stack = &file->stack;
    struct prs_cell  *top   = stack->top;

    if (!top) return false;

    stack->top = top->next;
    top->next  = stack->free_list;
    stack->free_list = top;

    *(target.node) = top->value;

    return true;
}

extern bool defineRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData name;

    if (!input_Text(input, &name)) return false;

    SynDefine rule = file->rules;

    for ( ; rule ; rule = rule->next ) {
        if (rule->name.length != name.length) continue;
        if (!strncmp(rule->name.start, name.start, name.length)) continue;
        break;
    }

    SynNode value;

    if (pop(file, &value)) return false;

    if (!rule) {
        if (!make_Any(syn_rule, &rule)) return false;
        rule->next  = file->rules;
        file->rules = rule;
        rule->name  = name;
        rule->value = value;
    }

    SynTree tree;

    if (!make_Any(syn_choice, &tree)) return false;

    tree->before = rule->value;
    tree->after  = value;

    rule->value.tree = tree;

    return true;
}

extern bool makeEnd(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny value = 0;

    if (!make_Any(syn_end, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeBegin(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny value = 0;

    if (!make_Any(syn_begin, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

static bool makeText(struct prs_file *file, SynType type) {
    PrsData value;

    if (syn_text != type2kind(type)) return false;

    if (!input_Text((PrsInput) file, &value)) return false;

    SynText text = 0;

    if (!make_Any(type, &text)) return false;

    text->value = value;

    return push(file, text);
}

static bool makeChunk(struct prs_file *file, SynType type, SynChunk *target) {
    PrsData value;

    if (syn_chunk != type2kind(type)) return false;

    if (!input_Text((PrsInput) file, &value)) return false;

    SynChunk chunk = 0;

    if (!make_Any(type, &chunk)) return false;

    chunk->next  = file->chunks;
    chunk->value = value;

    file->chunks = chunk;

    if (target) *target = chunk;

    return true;
}

// nameless event
extern bool makeThunk(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynChunk thunk = 0;

    if (!makeChunk(file, syn_thunk, &thunk)) return false;

    return push(file, thunk);
}

// namefull event
extern bool makeApply(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_apply);
}

extern bool makePredicate(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_predicate);
}

extern bool makeDot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny value = 0;

    if (!make_Any(syn_dot, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeSet(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_set);
}

extern bool makeString(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_string);
}

extern bool makeCall(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_call);
}

static bool makeOperator(struct prs_file *file, SynType type) {
    SynOperator operator = 0;
    SynNode     value;

    if (syn_operator != type2kind(type)) return false;

    if (pop(file, &value)) return false;

    if (!make_Any(type, &operator)) return false;

    operator->value = value;

    if (!push(file, operator)) return false;

    return true;
}

extern bool makePlus(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeOperator(file, syn_plus);
}

extern bool makeStar(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeOperator(file, syn_star);
}

extern bool makeQuestion(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeOperator(file, syn_question);
}

extern bool makeNot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeOperator(file, syn_not);
}

extern bool makeCheck(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeOperator(file, syn_check);
}

static bool makeTree(struct prs_file *file, SynType type) {
    SynNode before;
    SynNode after;

    if (syn_tree != type2kind(type)) return false;

    if (pop(file, &after))  return false;
    if (pop(file, &before)) return false;

    SynTree tree = 0;

    if (!make_Any(type, &tree)) return false;

    tree->before = before;
    tree->after  = after;

    return push(file, tree);
}

extern bool makeSequence(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeTree(file, syn_sequence);
}

extern bool makeChoice(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeTree(file, syn_choice);
}

extern bool makeHeader(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeChunk(file, syn_header, 0);
}

extern bool makeInclude(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeChunk(file, syn_include, 0);
}

extern bool makeFooter(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeChunk(file, syn_footer, 0);
}

