/*-*- mode: c;-*-*/
#if !defined(_syntax_h_)
#define _syntax_h_

typedef union syn_node   *SynNode;
typedef union syn_target *SynTarget;

typedef struct syn_any      *SynAny;
typedef struct syn_define   *SynDefine;
typedef struct syn_char     *SynChar;
typedef struct syn_text     *SynText;
typedef struct syn_operator *SynOperator;
typedef struct syn_list     *SynList;

typedef enum syn_type {
    syn_void = 0,
    syn_rule,      // - identifier = ....
    syn_char,      // - 'chr
    syn_dot,       // - .
    syn_begin,     // - set state.begin
    syn_end,       // - set state.end
    syn_header,    // - %header  {...}
    syn_include,   // - %include "..." or  %include <...>
    syn_action,    // - %action (to be removed)
    syn_event,     // - {...}
    syn_apply,     // - @name
    syn_set,       // - [...]
    syn_string,    // - "..."
    syn_predicate, // - &predicate
    syn_footer,    // - %footer ...
    syn_not,       // - e !
    syn_check,     // - e &
    syn_plus,      // - e +
    syn_star,      // - e *
    syn_question,  // - e ?
    syn_choice,    // - e1 e2 |
    syn_sequence,  // - e1 e2 ;
    syn_omega,
} SynType;

struct syn_any {
    SynType type;
    SynNode next;
};

// use for
// - identifier = ....
struct syn_define {
    SynType type;
    SynNode next;
    const char* name;
    SynNode first; //first in list
    SynNode last;  //last in list
};

// use for
// - 'chr
// - .
// - set state.begin
// - set state.end
struct syn_char {
    SynType type;
    SynNode next;
    char    value;
};

// use for
// - %header {...}
// - %include "..." or  %include <...>
// - %action (to be removed)
// - {...}
// - @name
// - [...]
// - "..."
// - &predicate
// - %footer ...
struct syn_text {
    SynType     type;
    SynNode     next;
    unsigned    length;
    const char* value;
};

// use for
// - e !
// - e &
// - e +
// - e *
// - e ?
struct syn_operator {
    SynType type;
    SynNode next;
    SynNode value;
};

// use for
// - e1 e2 /
// - e1 e2 ;
struct syn_list {
    SynType type;
    SynNode next;
    SynNode first; //first in list
    SynNode last;  //last in list
};

union syn_node {
    SynType type;
    struct syn_any      any;
    struct syn_define   define;
    struct syn_char     character;
    struct syn_text     text;
    struct syn_operator operator;
    struct syn_list     list;
};

union syn_target {
    SynAny      any;
    SynDefine   define;
    SynChar     character;
    SynText     text;
    SynOperator operator;
    SynList     list;
};

extern bool make_PrsFile(const char* filename, PrsInput *input);

extern bool makeEnd(PrsInput input, PrsCursor at);
extern bool makeBegin(PrsInput input, PrsCursor at);
extern bool makeThunk(PrsInput input, PrsCursor at);
extern bool makeEvent(PrsInput input, PrsCursor at);
extern bool makePredicate(PrsInput input, PrsCursor at);
extern bool makeDot(PrsInput input, PrsCursor at);
extern bool makeClass(PrsInput input, PrsCursor at);
extern bool makeString(PrsInput input, PrsCursor at);
extern bool makeName(PrsInput input, PrsCursor at);
extern bool makePlus(PrsInput input, PrsCursor at);
extern bool makeStar(PrsInput input, PrsCursor at);
extern bool makeQuery(PrsInput input, PrsCursor at);
extern bool makePeekNot(PrsInput input, PrsCursor at);
extern bool makePeekFor(PrsInput input, PrsCursor at);
extern bool makeSequence(PrsInput input, PrsCursor at);
extern bool makeAlternate(PrsInput input, PrsCursor at);
extern bool defineRule(PrsInput input, PrsCursor at);
extern bool checkRule(PrsInput input, PrsCursor at);
extern bool makeHeader(PrsInput input, PrsCursor at);

#endif
