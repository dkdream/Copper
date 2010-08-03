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
#include <stdlib.h>
#include <stdio.h>
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
    case syn_rule:      fullsize = sizeof(struct syn_define);   break; // - identifier = ....
    case syn_char:      fullsize = sizeof(struct syn_char);     break; // - 'chr
    case syn_dot:       fullsize = sizeof(struct syn_char);     break; // - .
    case syn_begin:     fullsize = sizeof(struct syn_char);     break; // - set state.begin
    case syn_end:       fullsize = sizeof(struct syn_char);     break; // - set state.end
    case syn_header:    fullsize = sizeof(struct syn_text);     break; // - %header {...}
    case syn_include:   fullsize = sizeof(struct syn_text);     break; // - %include "..." or  %include <...>
    case syn_action:    fullsize = sizeof(struct syn_text);     break; // - %action (to be removed)
    case syn_event:     fullsize = sizeof(struct syn_text);     break; // - {...}
    case syn_apply:     fullsize = sizeof(struct syn_text);     break; // - @name
    case syn_set:       fullsize = sizeof(struct syn_text);     break; // - [...]
    case syn_string:    fullsize = sizeof(struct syn_text);     break; // - "..."
    case syn_predicate: fullsize = sizeof(struct syn_text);     break; // - &predicate
    case syn_footer:    fullsize = sizeof(struct syn_text);     break; // - %footer ...
    case syn_not:       fullsize = sizeof(struct syn_operator); break; // - e !
    case syn_check:     fullsize = sizeof(struct syn_operator); break; // - e &
    case syn_plus:      fullsize = sizeof(struct syn_operator); break; // - e +
    case syn_star:      fullsize = sizeof(struct syn_operator); break; // - e *
    case syn_question:  fullsize = sizeof(struct syn_operator); break; // - e ?
    case syn_choice:    fullsize = sizeof(struct syn_list);     break; // - e1 e2 |
    case syn_sequence:  fullsize = sizeof(struct syn_list);     break; // - e1 e2 ;
    case syn_omega: return false;
    }

    SynAny result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->type = type;
    result->next = next;

    target->any = result;
}

extern bool makeEnd(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeBegin(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeThunk(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeEvent(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makePredicate(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeDot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeClass(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeString(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeName(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makePlus(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeStar(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeQuery(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makePeekNot(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makePeekFor(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeSequence(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeAlternate(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool defineRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool checkRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}

extern bool makeHeader(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
}
