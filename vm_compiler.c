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
    if (!target.any) return false;

    unsigned fullsize = 0;
    switch (type) {
    case syn_void: return false;

    case syn_action:    fullsize = sizeof(struct syn_text);     break; // - %action (to be removed)
    case syn_apply:     fullsize = sizeof(struct syn_text);     break; // - @name
    case syn_begin:     fullsize = sizeof(struct syn_any);      break; // - set state.begin
    case syn_call:      fullsize = sizeof(struct syn_text);     break; // - name
    case syn_char:      fullsize = sizeof(struct syn_char);     break; // - 'chr
    case syn_check:     fullsize = sizeof(struct syn_operator); break; // - e &
    case syn_choice:    fullsize = sizeof(struct syn_list);     break; // - e1 e2 |
    case syn_dot:       fullsize = sizeof(struct syn_any);      break; // - .
    case syn_end:       fullsize = sizeof(struct syn_any);      break; // - set state.end
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

    SynAny result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->type = type;
    result->next = next;

    *(target.any) = result;

    return true;
}

static bool push(struct prs_file *file, SynNode value) {
    return true;
}

static bool pop(struct prs_file *file, SynTarget value) {
    return true;
}

extern bool checkRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData name;

    if (!input_Text(input, &name)) return false;

    SynDefine rule = file->rules;

    for ( ; rule ; rule = rule->next.define ) {
        if (rule->name.length != name.length) continue;
        if (!strncmp(rule->name.start, name.start, name.length)) continue;
        return push(file, rule);
    }

    if (!make_Any(syn_rule, file->rules, &rule)) return false;

    file->rules = rule;
    rule->name  = name;

    return push(file, rule);
}

extern bool defineRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynDefine rule = 0;
    SynAny   value = 0;

    if (pop(file, &value))      return false;
    if (pop(file, &rule))       return false;
    if (syn_rule != rule->type) return false;

    value->next.any = rule->last;
    rule->last = value;

    if (rule->first) return true;

    rule->first = value;

    return true;
}

extern bool makeEnd(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny value = 0;

    if (!make_Any(syn_end, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

extern bool makeBegin(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny value = 0;

    if (!make_Any(syn_begin, 0, &value)) return false;

    if (!push(file, value)) return false;

    return true;
}

static bool makeText(struct prs_file *file, enum syn_type type) {
    PrsData text;

    if (!input_Text((PrsInput) file, &text)) return false;

    SynText value = 0;

    if (!make_Any(type, 0, &value)) return false;

    value->text = text;

    if (!push(file, value)) return false;

    return true;
}

// nameless event
extern bool makeThunk(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;
    return makeText(file, syn_thunk);
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

    if (!make_Any(syn_dot, 0, &value)) return false;

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

static bool makeOperator(struct prs_file *file, enum syn_type type) {
    SynOperator operator = 0;
    SynAny      value    = 0;

    if (pop(file, &value)) return false;

    if (!make_Any(type, 0, &operator)) return false;

    operator->value.any = value;

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

extern bool makeSequence(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny before = 0;
    SynAny next   = 0;

    if (pop(file, &next))   return false;
    if (pop(file, &before)) return false;

    SynList sequence = 0;

    if (syn_sequence != before->type) {
        if (!make_Any(syn_sequence, 0, &sequence)) return false;

        sequence->first = next;
        sequence->last  = next;
    } else {
        sequence = (SynList) before;

        sequence->last->next.any = next;
        sequence->last = next;
    }

    if (!push(file, sequence)) return false;

    return true;
}

extern bool makeChoice(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    SynAny before = 0;
    SynAny next   = 0;

    if (pop(file, &next))   return false;
    if (pop(file, &before)) return false;

    SynList choice = 0;

    if (syn_choice != before->type) {
        if (!make_Any(syn_choice, 0, &choice)) return false;

        choice->first = next;
        choice->last  = next;
    } else {
        choice = (SynList) before;

        choice->last->next.any = next;
        choice->last = next;
    }

    if (!push(file, choice)) return false;

    return true;
}

extern bool makeHeader(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData text;

    if (!input_Text((PrsInput) file, &text)) return false;

    SynText value = 0;

    if (!make_Any(syn_header, file->headers, &value)) return false;

    value->text = text;

    file->headers = value;

    return true;
}

extern bool makeInclude(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData text;

    if (!input_Text((PrsInput) file, &text)) return false;

    SynText value = 0;

    if (!make_Any(syn_include, file->includes, &value)) return false;

    value->text = text;

    file->includes = value;

    return true;
}

extern bool makeFooter(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData text;

    if (!input_Text((PrsInput) file, &text)) return false;

    SynText value = 0;

    if (!make_Any(syn_footer, file->footer, &value)) return false;

    value->text = text;

    file->footer = value;

    return true;
}

