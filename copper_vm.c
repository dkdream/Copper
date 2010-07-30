
#include "copper_vm.h"

extern bool copper_vm(PrsNode start, PrsInput input) {

    inline bool push()    { return input->push(input);    }
    inline bool replace() { return input->replace(input); }
    inline bool pop()     { return input->pop(input);     }
    inline bool reset()   { return input->reset(input);   }
    inline bool next()    { return input->next(input);    }

    inline bool current(PrsChar *target) {
        return input->current(input, target);
    }

    inline bool find(PrsName name, PrsNode* target) {
        return input->find(input, name, target);
    }

    inline bool prs_and() {
        push();
        if (!copper_vm(start->arg.pair->left, input))  {
            reset();
            pop();
            return false;
        }
        if (!copper_vm(start->arg.pair->right, input)) {
            reset();
            pop();
            return false;
        }
        pop();
        return true;
    }

    inline bool prs_choice() {
        push();
        if (copper_vm(start->arg.pair->left, input)) {
            pop();
            return true;
        }
        reset();
        if (copper_vm(start->arg.pair->right, input)) {
            pop();
            return true;
        }
        reset();
        pop();
        return false;
    }

    inline bool prs_zero_plus() {
        push();
        while (copper_vm(start->arg.node, input)) {
            replace();
        }
        reset();
        pop();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        push();
        while (copper_vm(start->arg.node, input)) {
            result = true;
            replace();
        }
        reset();
        pop();
        return result;
    }

    inline bool prs_optional() {
        push();
        if (copper_vm(start->arg.node, input)) {
            pop();
            return true;
        }
        reset();
        pop();
        return true;;
    }

    inline bool prs_assert_true() {
        push();
        if (copper_vm(start->arg.node, input)) {
            reset();
            pop();
            return true;
        }
        reset();
        pop();
        return false;
    }

    inline bool prs_assert_false() {
        push();
        if (copper_vm(start->arg.node, input)) {
            reset();
            pop();
            return false;
        }
        reset();
        pop();
        return true;
    }

    inline bool prs_char() {
        PrsChar chr = 0;
        if (!current(&chr)) {
            return false;
        }
        if (chr != start->arg.letter) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_between() {
        PrsChar chr = 0;
        if (!current(&chr)) {
            return false;
        }
        if (chr < start->arg.range->begin) {
            return false;
        }
        if (chr > start->arg.range->end) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_in() {
        PrsChar chr = 0;

        if (!current(&chr)) {
            return false;
        }

        unsigned             binx = chr;
        const unsigned char *bits = start->arg.set->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            next();
            return true;
        }

        return false;
    }

    inline bool prs_name() {
        PrsNode value;
        if (!find(start->arg.name, &value)) {
            return false;
        }
        return copper_vm(value, input);
    }

    inline bool prs_dot() {
        return false;
    }

    inline bool prs_string() {
        return false;
    }

    inline bool prs_predicate() { return false; }
    inline bool prs_event()     { return false; }
    inline bool prs_action()    { return false; }
    inline bool prs_void()      { return false; }

    switch (start->oper) {
    case prs_Sequence:    return prs_and();
    case prs_Choice:      return prs_choice();
    case prs_ZeroOrMore:  return prs_zero_plus();
    case prs_OneOrMore:   return prs_one_plus();
    case prs_ZeroOrOne:   return prs_optional();
    case prs_AssertTrue:  return prs_assert_true();
    case prs_AssertFalse: return prs_assert_false();
    case prs_MatchChar:   return prs_char();
    case prs_MatchRange:  return prs_between();
    case prs_MatchSet:    return prs_in();
    case prs_MatchName:   return prs_name();
    case prs_MatchDot:    return prs_dot();
    case prs_MatchString: return prs_string();
    case prs_Predicate:   return prs_predicate();
    case prs_Action:      return prs_action();
    case prs_Event:       return prs_event();
    case prs_Void:        return prs_void();
    }

    return false;
}

