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
/* */
typedef struct syn_first *SynFirst;

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
    syn_predicate, // - %predicate
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
    case syn_predicate: return syn_text;      // - %predicate
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

struct syn_first {
    PrsFirstType   type;
    unsigned char *bitfield;
    unsigned       count;
    PrsData        name[];
};

// use for
// - .
// - set state.begin
// - set state.end
// and as a generic node
struct syn_any {
    SynType  type;
    SynFirst first;
};

// use for
// - identifier = ....
struct syn_define {
    SynType   type;
    SynFirst  first;
    SynDefine next;
    PrsData   name;
    SynNode   value;
};

// use for
// - 'chr
struct syn_char {
    SynType  type;
    SynFirst first;
    unsigned char value;
};

// use for
// - @name
// - [...]
// - "..."
// - '...'
// - %predicate
struct syn_text {
    SynType  type;
    SynFirst first;
    PrsData  value;
};

// use for
// - %header {...}
// - %include "..." or  %include <...>
// - {...}
// - %footer ...
struct syn_chunk {
    SynType  type;
    SynFirst first;
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
    SynType  type;
    SynFirst first;
    SynNode  value;
};

// use for
// - e1 e2 /
// - e1 e2 ;
struct syn_tree {
    SynType  type;
    SynFirst first;
    SynNode  before;
    SynNode  after;
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
    struct copper   base;

    // parsing context
    const char        *filename;
    struct prs_buffer  buffer;
    struct prs_hash   *nodes;
    struct prs_hash   *predicates;
    struct prs_hash   *events;

    // parsing state
    struct prs_stack stack;

    // parsing results
    SynDefine rules;
    SynChunk  chunks;
};

extern bool make_PrsFile(FILE* file, const char* filename, Copper *input);
extern bool file_WriteTree(Copper input, FILE* output, const char* name);
extern bool file_SetPredicate(struct prs_file *file, PrsName name, PrsPredicate value);
extern bool file_SetEvent(struct prs_file *file, PrsName name, PrsEvent value);

extern bool writeTree(Copper input, PrsCursor at);

extern bool checkRule(Copper input, PrsCursor at);
extern bool defineRule(Copper input, PrsCursor at);

extern bool makeEnd(Copper input, PrsCursor at);
extern bool makeBegin(Copper input, PrsCursor at);
extern bool makeThunk(Copper input, PrsCursor at);
extern bool makeApply(Copper input, PrsCursor at);
extern bool makePredicate(Copper input, PrsCursor at);
extern bool makeDot(Copper input, PrsCursor at);
extern bool makeSet(Copper input, PrsCursor at);
extern bool makeString(Copper input, PrsCursor at);
extern bool makeCall(Copper input, PrsCursor at);
extern bool makePlus(Copper input, PrsCursor at);
extern bool makeStar(Copper input, PrsCursor at);
extern bool makeQuestion(Copper input, PrsCursor at);
extern bool makeNot(Copper input, PrsCursor at);
extern bool makeCheck(Copper input, PrsCursor at);
extern bool makeSequence(Copper input, PrsCursor at);
extern bool makeChoice(Copper input, PrsCursor at);
extern bool defineRule(Copper input, PrsCursor at);
extern bool makeHeader(Copper input, PrsCursor at);
extern bool makeInclude(Copper input, PrsCursor at);
extern bool makeFooter(Copper input, PrsCursor at);

#endif
