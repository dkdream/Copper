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
                     SynNode   next,
                     SynTarget target)
{
    if (!target) return false;

    unsigned fullsize = 0;
    switch (type) {
    case syn_void: return false;

    case syn_action:    fullsize = sizeof(struct syn_text);     break; // - %action (to be removed)
    case syn_apply:     fullsize = sizeof(struct syn_text);     break; // - @name
    case syn_begin:     fullsize = sizeof(struct syn_char);     break; // - set state.begin
    case syn_call:      fullsize = sizeof(struct syn_text);     break; // - name
    case syn_char:      fullsize = sizeof(struct syn_char);     break; // - 'chr
    case syn_check:     fullsize = sizeof(struct syn_operator); break; // - e &
    case syn_choice:    fullsize = sizeof(struct syn_list);     break; // - e1 e2 |
    case syn_dot:       fullsize = sizeof(struct syn_char);     break; // - .
    case syn_end:       fullsize = sizeof(struct syn_char);     break; // - set state.end
    case syn_footer:    fullsize = sizeof(struct syn_text);     break; // - %footer ...
    case syn_header:    fullsize = sizeof(struct syn_text);     break; // - %header {...}
    case syn_include:   fullsize = sizeof(struct syn_text);     break; // - %include "..." or  %include <...>
    case syn_not:       fullsize = sizeof(struct syn_operator); break; // - e !
    case syn_plus:      fullsize = sizeof(struct syn_operator); break; // - e +
    case syn_predicate: fullsize = sizeof(struct syn_text);     break; // - &predicate
    case syn_question:  fullsize = sizeof(struct syn_operator); break; // - e ?
    case syn_rule:      fullsize = sizeof(struct syn_define);   break; // - identifier = ....
    case syn_sequence:  fullsize = sizeof(struct syn_list);     break; // - e1 e2 ;
    case syn_set:       fullsize = sizeof(struct syn_text);     break; // - [...]
    case syn_star:      fullsize = sizeof(struct syn_operator); break; // - e *
    case syn_string:    fullsize = sizeof(struct syn_text);     break; // - "..."
    case syn_thunk:     fullsize = sizeof(struct syn_text);     break; // - {...}

    case syn_omega: return false;
    }

    SynNode result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->type     = type;
    result->any.next = next;

    *target = result;

    return true;
}

static bool push(struct prs_file *file, SynNode value) {
    return true;
}

static bool pop(struct prs_file *file, SynTarget value) {
    return true;
}

extern bool makeEnd(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_end, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeBegin(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_begin, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

// nameless event
extern bool makeThunk(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_thunk, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

// namefull event
extern bool makeApply(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_apply, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makePredicate(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_predicate, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeDot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_dot, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeSet(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_set, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeString(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_string, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeCall(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_call, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makePlus(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_plus, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeStar(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_star, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeQuestion(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_question, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeNot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_not, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeCheck(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_check, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeSequence(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_sequence, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeChoice(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_choice, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool defineRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_rule, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool checkRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_rule, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeHeader(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_header, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeFooter(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynNode value = 0;

    if (!make_Any(syn_header, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}
