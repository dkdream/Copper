/*-*- mode: c;-*-*/
#if !defined(_syntax_h_)
#define _syntax_h_
/***************************
 **
 ** Project: *current project*
 **
 ** Datatype List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "copper.h"

/* */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <error.h>

/* */
typedef struct syn_any      *SynAny;
typedef struct syn_define   *SynDefine;
typedef struct syn_char     *SynChar;
typedef struct syn_text     *SynText;
typedef struct syn_chunk    *SynChunk;
typedef struct syn_operator *SynOperator;
typedef struct syn_tree     *SynTree;

union syn_node {
    SynAny      any;
    SynDefine   define;
    SynChar     character;
    SynText     text;
    SynChunk    chunk;
    SynOperator operator;
    SynTree     tree;
} __attribute__ ((__transparent_union__));

typedef union syn_node SynNode;

union syn_target {
    SynAny      *any;
    SynDefine   *define;
    SynChar     *character;
    SynText     *text;
    SynChunk    *chunk;
    SynOperator *operator;
    SynTree     *tree;
    SynNode     *node;
} __attribute__ ((__transparent_union__));


typedef union syn_target SynTarget;

typedef enum syn_type {
    syn_action,    // - %action (to be removed)
    syn_apply,     // - @name
    syn_begin,     // - set state.begin
    syn_call,      // - name
    syn_char,      // - 'chr
    syn_check,     // - e &
    syn_choice,    // - e1 e2 |
    syn_dot,       // - .
    syn_end,       // - set state.end
    syn_footer,    // - %footer ...
    syn_header,    // - %header  {...}
    syn_include,   // - %include "..." or  %include <...>
    syn_not,       // - e !
    syn_plus,      // - e +
    syn_predicate, // - &predicate
    syn_question,  // - e ?
    syn_rule,      // - identifier = ....
    syn_sequence,  // - e1 e2 ;
    syn_set,       // - [...]
    syn_star,      // - e *
    syn_string,    // - "..."
    syn_thunk,     // - {...}
    //-------
    syn_void
} SynType;

typedef enum syn_kind {
    syn_unknown = 0,
    syn_any,
    syn_define,
    syn_text,
    syn_chunk,
    syn_operator,
    syn_tree
} SynKind;

static inline SynKind type2kind(SynType type) {
    switch (type) {
    case syn_action:    return syn_text;      // - %action (to be removed)
    case syn_apply:     return syn_text;      // - @name
    case syn_begin:     return syn_any;       // - set state.begin
    case syn_call:      return syn_text;      // - name
    case syn_char:      return syn_char;      // - 'chr
    case syn_check:     return syn_operator;  // - e &
    case syn_choice:    return syn_tree;      // - e1 e2 |
    case syn_dot:       return syn_any;       // - .
    case syn_end:       return syn_any;       // - set state.end
    case syn_footer:    return syn_chunk;     // - %footer ...
    case syn_header:    return syn_chunk;     // - %header {...}
    case syn_include:   return syn_chunk;     // - %include "..." or  %include <...>
    case syn_not:       return syn_operator;  // - e !
    case syn_plus:      return syn_operator;  // - e +
    case syn_predicate: return syn_text;      // - &predicate
    case syn_question:  return syn_operator;  // - e ?
    case syn_rule:      return syn_define;    // - identifier = ....
    case syn_sequence:  return syn_tree;      // - e1 e2 ;
    case syn_set:       return syn_text;      // - [...]
    case syn_star:      return syn_operator;  // - e *
    case syn_string:    return syn_text;      // - "..." or '...'
    case syn_thunk:     return syn_chunk;     // - {...}
        /* */
    case syn_void: break;
    }
    return syn_unknown;
}

static inline const char* type2name(SynType type) {
    switch (type) {
    case syn_action:    return "syn_action";
    case syn_apply:     return "syn_apply";
    case syn_begin:     return "syn_begin";
    case syn_call:      return "syn_call";
    case syn_char:      return "syn_char";
    case syn_check:     return "syn_check";
    case syn_choice:    return "syn_choice";
    case syn_dot:       return "syn_dot";
    case syn_end:       return "syn_end";
    case syn_footer:    return "syn_footer";
    case syn_header:    return "syn_header";
    case syn_include:   return "syn_include";
    case syn_not:       return "syn_not";
    case syn_plus:      return "syn_plus";
    case syn_predicate: return "syn_predicate";
    case syn_question:  return "syn_question";
    case syn_rule:      return "syn_rule";
    case syn_sequence:  return "syn_sequence";
    case syn_set:       return "syn_set";
    case syn_star:      return "syn_star";
    case syn_string:    return "syn_string";
    case syn_thunk:     return "syn_thunk";
        /* */
    case syn_void: break;
    }
    return "syn-unknown";
}

// use for
// - .
// - set state.begin
// - set state.end
// and as a generic node
struct syn_any {
    SynType type;
};

// use for
// - identifier = ....
struct syn_define {
    SynType   type;
    SynDefine next;
    PrsData   name;
    SynNode   value;
};

// use for
// - 'chr
struct syn_char {
    SynType type;
    char    value;
};

// use for
// - %action (to be removed)
// - @name
// - [...]
// - "..."
// - '...'
// - &predicate
struct syn_text {
    SynType type;
    PrsData value;
};

// use for
// - %header {...}
// - %include "..." or  %include <...>
// - {...}
// - %footer ...
struct syn_chunk {
    SynType  type;
    SynChunk next;
    PrsData  value;
};

// use for
// - e !
// - e &
// - e +
// - e *
// - e ?
struct syn_operator {
    SynType type;
    SynNode value;
};

// use for
// - e1 e2 /
// - e1 e2 ;
struct syn_tree {
    SynType type;
    SynNode before;
    SynNode after;
};

/*------------------------------------------------------------*/

struct prs_buffer {
    FILE     *file;
    unsigned  cursor;
    ssize_t   read;
    size_t    allocated;
    char     *line;
};

struct prs_map {
    unsigned        code;
    const void*     key;
    void*           value;
    struct prs_map *next;
};

typedef unsigned long (*Hashcode)(const void*);
typedef bool     (*Matchkey)(const void*, const void*);
typedef bool     (*FreeValue)(void*);

struct prs_hash {
    Hashcode encode;
    Matchkey compare;
    unsigned size;
    struct prs_map *table[];
};

// use for the node stack
struct prs_cell {
    SynNode value;
    struct prs_cell *next;
};

struct prs_stack {
    struct prs_cell *top;
    struct prs_cell *free_list;
};

struct prs_file {
    struct prs_input   base;

    // parsing context
    const char        *filename;
    struct prs_buffer  buffer;
    struct prs_cursor  cursor;
    struct prs_cursor  reach;
    struct prs_hash   *nodes;
    struct prs_hash   *predicates;
    struct prs_hash   *actions;
    struct prs_hash   *events;

    // parsing state
    struct prs_stack stack;

    // parsing results
    SynDefine rules;
    SynChunk  chunks;
};

extern bool make_PrsFile(FILE* file, const char* filename, PrsInput *input);
extern bool file_WriteTree(PrsInput input, FILE* output, const char* name);

extern bool writeTree(PrsInput input, PrsCursor at);

extern bool checkRule(PrsInput input, PrsCursor at);
extern bool defineRule(PrsInput input, PrsCursor at);

extern bool makeEnd(PrsInput input, PrsCursor at);
extern bool makeBegin(PrsInput input, PrsCursor at);
extern bool makeThunk(PrsInput input, PrsCursor at);
extern bool makeApply(PrsInput input, PrsCursor at);
extern bool makePredicate(PrsInput input, PrsCursor at);
extern bool makeDot(PrsInput input, PrsCursor at);
extern bool makeSet(PrsInput input, PrsCursor at);
extern bool makeString(PrsInput input, PrsCursor at);
extern bool makeCall(PrsInput input, PrsCursor at);
extern bool makePlus(PrsInput input, PrsCursor at);
extern bool makeStar(PrsInput input, PrsCursor at);
extern bool makeQuestion(PrsInput input, PrsCursor at);
extern bool makeNot(PrsInput input, PrsCursor at);
extern bool makeCheck(PrsInput input, PrsCursor at);
extern bool makeSequence(PrsInput input, PrsCursor at);
extern bool makeChoice(PrsInput input, PrsCursor at);
extern bool defineRule(PrsInput input, PrsCursor at);
extern bool makeHeader(PrsInput input, PrsCursor at);
extern bool makeInclude(PrsInput input, PrsCursor at);
extern bool makeFooter(PrsInput input, PrsCursor at);


extern unsigned cu_global_debug;
extern void cu_debug(const char *filename, unsigned int linenum, const char *format, ...);
extern void cu_error(const char *filename, unsigned int linenum, const char *format, ...);


static inline void cu_noop() __attribute__((always_inline));
static inline void cu_noop() { return; }

#if 1
#define CU_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) cu_debug(__FILE__,  __LINE__, args); })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) arg; })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ERROR(args...) cu_error(__FILE__,  __LINE__, args)
#else
#define CU_ERROR(args...) cu_noop()
#endif

#if 1
#define CU_ERROR_AT(args...) cu_error(args)
#else
#define CU_ERROR_AT(args...) cu_noop()
#endif

#endif
