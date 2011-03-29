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
typedef struct syn_operator *SynOperator;
typedef struct syn_tree     *SynTree;
/* */
typedef struct syn_first *SynFirst;

union syn_node {
    SynAny      any;
    SynDefine   define;
    SynChar     character;
    SynText     text;
    SynOperator operator;
    SynTree     tree;
} __attribute__ ((__transparent_union__));

typedef union syn_node SynNode;

union syn_target {
    SynAny      *any;
    SynDefine   *define;
    SynChar     *character;
    SynText     *text;
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
    syn_not,       // - e !
    syn_plus,      // - e +
    syn_predicate, // - %predicate
    syn_question,  // - e ?
    syn_rule,      // - identifier = ....
    syn_sequence,  // - e1 e2 ;
    syn_set,       // - [...]
    syn_star,      // - e *
    syn_string,    // - "..."
    //-------
    syn_void
} SynType;

typedef enum syn_kind {
    syn_unknown = 0,
    syn_any,
    syn_define,
    syn_text,
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
    case syn_not:       return syn_operator;  // - e !
    case syn_plus:      return syn_operator;  // - e +
    case syn_predicate: return syn_text;      // - %predicate
    case syn_question:  return syn_operator;  // - e ?
    case syn_rule:      return syn_define;    // - identifier = ....
    case syn_sequence:  return syn_tree;      // - e1 e2 ;
    case syn_set:       return syn_text;      // - [...]
    case syn_star:      return syn_operator;  // - e *
    case syn_string:    return syn_text;      // - "..." or '...'
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
    case syn_not:       return "syn_not";
    case syn_plus:      return "syn_plus";
    case syn_predicate: return "syn_predicate";
    case syn_question:  return "syn_question";
    case syn_rule:      return "syn_rule";
    case syn_sequence:  return "syn_sequence";
    case syn_set:       return "syn_set";
    case syn_star:      return "syn_star";
    case syn_string:    return "syn_string";
        /* */
    case syn_void: break;
    }
    return "syn-unknown";
}

struct syn_first {
    CuFirstType   type;
    unsigned char *bitfield;
    unsigned       count;
    CuData        name[];
};

// use for
// - .
// - set state.begin
// - set state.end
// and as a generic node
struct syn_any {
    SynType  type;
    unsigned id;
    SynFirst first;
};

// use for
// - identifier = ....
struct syn_define {
    SynType   type;
    unsigned id;
    SynFirst  first;
    SynDefine next;
    CuData   name;
    SynNode   value;
};

// use for
// - 'chr
struct syn_char {
    SynType  type;
    unsigned id;
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
    unsigned id;
    SynFirst first;
    CuData  value;
};

#if 0
// use for
// - %header {...}
// - %include "..." or  %include <...>
// - {...}
// - %footer ...
struct syn_chunk {
    SynType  type;
    unsigned id;
    SynFirst first;
    SynChunk next;
    CuData  value;
};
#endif

// use for
// - e !
// - e &
// - e +
// - e *
// - e ?
struct syn_operator {
    SynType  type;
    unsigned id;
    SynFirst first;
    SynNode  value;
};

// use for
// - e1 e2 /
// - e1 e2 ;
struct syn_tree {
    SynType  type;
    unsigned id;
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
    unsigned       code;
    const void*    key;
    void*          value;
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
    //SynChunk  chunks;
};

extern bool make_CuFile(FILE* file, const char* filename, Copper *input);
extern bool file_WriteTree(Copper input, FILE* output, const char* name);
extern bool file_SetPredicate(struct prs_file *file, CuName name, CuPredicate value);
extern bool file_SetEvent(struct prs_file *file, CuName name, CuEvent value);

extern bool writeTree(Copper input, CuCursor at);

extern bool checkRule(Copper input, CuCursor at);
extern bool defineRule(Copper input, CuCursor at);

extern bool make_Hash(Hashcode, Matchkey, unsigned, struct prs_hash **);
extern bool hash_Find(struct prs_hash *hash, const void* key, void** target);
extern bool hash_Replace(struct prs_hash *hash, const void* key, void* value, FreeValue release);

extern bool makeEnd(Copper input, CuCursor at);
extern bool makeBegin(Copper input, CuCursor at);
extern bool makeApply(Copper input, CuCursor at);
extern bool makePredicate(Copper input, CuCursor at);
extern bool makeDot(Copper input, CuCursor at);
extern bool makeSet(Copper input, CuCursor at);
extern bool makeString(Copper input, CuCursor at);
extern bool makeCall(Copper input, CuCursor at);
extern bool makePlus(Copper input, CuCursor at);
extern bool makeStar(Copper input, CuCursor at);
extern bool makeQuestion(Copper input, CuCursor at);
extern bool makeNot(Copper input, CuCursor at);
extern bool makeCheck(Copper input, CuCursor at);
extern bool makeSequence(Copper input, CuCursor at);
extern bool makeChoice(Copper input, CuCursor at);
extern bool defineRule(Copper input, CuCursor at);

#endif
