/**********************************************************************
This file is part of Copper.

Copper is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Copper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Copper.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

/***************************
 **
 ** Project: Copper
 **
 ** Routine List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "compiler.h"
#include "copper.h"
#include "copper_inline.h"

/* */
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>


/* */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <error.h>

/* */
typedef struct syn_any      *SynAny;
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
    syn_argument,  // - :[name]
    syn_begin,     // - set state.begin
    syn_call,      // - name
    syn_char,      // - 'chr
    syn_check,     // - e &
    syn_choice,    // - e1 e2 |
    syn_dot,       // - .
    syn_end,       // - set state.end
    syn_loop,      // - { e1 ; e2 }
    syn_not,       // - e !
    syn_plus,      // - e +
    syn_predicate, // - %predicate
    syn_question,  // - e ?
    syn_rule,      // - identifier = ....
    syn_sequence,  // - e1 e2
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
// - e1 e2
// - e1 ; e2
struct syn_tree {
    SynType  type;
    unsigned id;
    SynFirst first;
    SynNode  before;
    SynNode  after;
};

/* */
// use for the name to object hashtables
struct prs_map {
    unsigned long  code;
    const void*    key;
    void*          value;
    struct prs_map *next;
};

typedef bool (*FreeValue)(void*);

struct prs_hash {
    unsigned size;
    struct prs_map *table[];
};

/* */
// use for the node stack
struct prs_cell {
    SynNode value;
    struct prs_cell *next;
};

/*
struct prs_stack {
    struct prs_cell *top;
    struct prs_cell *free_list;
};
*/

extern bool copper_graph(CuCallback parser);

//static char             buffer[4096];
//static struct prs_stack file_stack = { 0, 0 };

static inline SynKind type2kind(SynType type) {
    switch (type) {
    case syn_apply:     return syn_text;      // - @name
    case syn_argument:  return syn_text;      // - :[name]
    case syn_begin:     return syn_any;       // - set state.begin
    case syn_call:      return syn_text;      // - name
    case syn_char:      return syn_text;      // - 'chr
    case syn_check:     return syn_operator;  // - e &
    case syn_choice:    return syn_tree;      // - e1 e2 |
    case syn_dot:       return syn_any;       // - .
    case syn_end:       return syn_any;       // - set state.end
    case syn_loop:      return syn_tree;      // - e1 e2
    case syn_not:       return syn_operator;  // - e !
    case syn_plus:      return syn_operator;  // - e +
    case syn_predicate: return syn_text;      // - %predicate
    case syn_question:  return syn_operator;  // - e ?
    case syn_rule:      return syn_define;    // - identifier = ....
    case syn_sequence:  return syn_tree;      // - e1 e2
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
    case syn_argument:  return "syn_argument";
    case syn_begin:     return "syn_begin";
    case syn_call:      return "syn_call";
    case syn_char:      return "syn_char";
    case syn_check:     return "syn_check";
    case syn_choice:    return "syn_choice";
    case syn_dot:       return "syn_dot";
    case syn_end:       return "syn_end";
    case syn_loop:      return "syn_loop";
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

static const char* convert(CuData name) {
    static char buffer[4096];
    strncpy(buffer, name.start, name.length);
    buffer[name.length] = 0;
    return buffer;
}

static bool make_Map(unsigned long code,
                     const void* key,
                     void* value,
                     struct prs_map *next,
                     struct prs_map **target)
{
    struct prs_map *result = malloc(sizeof(struct prs_map));

    result->code  = code;
    result->key   = key;
    result->value = value;
    result->next  = next;

    *target = result;
    return true;
}

static bool make_Hash(unsigned size, struct prs_hash **target)
{
    unsigned fullsize = (sizeof(struct prs_hash)
                         + (size * sizeof(struct prs_map *)));

    struct prs_hash* result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size    = size;

    *target = result;
    return true;
}

static unsigned long encode_name(const void* name) {
    const char *cursor = name;

    unsigned long result = 5381;

    for ( ; *cursor ; ++cursor ) {
        int val = *cursor;
        result = ((result << 5) + result) + val;
    }

    return result;
}

static unsigned compare_name(const void* lname, const void* rname) {
    const char *left  = lname;
    const char *right = rname;

    int result = strcmp(left, right);

    return (0 == result);
}

static bool hash_Find(struct prs_hash *hash,
                      const void* key,
                      void** target)
{
    if (!key) return false;

    unsigned long code  = encode_name(key);
    unsigned      index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!compare_name(key, map->key)) continue;
        *target = map->value;
        return true;
    }

    return false;
}

static bool hash_Replace(struct prs_hash *hash,
                         const void* key,
                         void* value)
{
    if (!key)     return false;
    if (!value)   return false;

    unsigned long code  = encode_name(key);
    unsigned      index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!compare_name(key, map->key)) continue;
        map->value = value;
        return true;
    }

    if (!make_Map(code, key, value, hash->table[index], &map)) {
        return false;
    }

    hash->table[index] = map;

    return true;
}

static bool copper_FindNode(CuCallback input, CuName name, CuNode* target) {
    return hash_Find(input->node_table, name, (void**)target);
    (void) input;
}

static bool copper_SetNode(CuCallback input, CuName name, CuNode value) {
    return hash_Replace(input->node_table, name, value);
}

static bool copper_FindPredicate(CuCallback input, CuName name, CuPredicate* target) {
    return hash_Find(input->predicate_table, name, (void**)target);
}

static bool copper_SetPredicate(CuCallback input, CuName name, CuPredicate value) {
    return hash_Replace(input->predicate_table, name, value);
}

static bool copper_FindEvent(CuCallback input, CuName name, CuEvent* target) {
    return hash_Find(input->event_table, name, (void**)target);
}

static bool copper_SetEvent(CuCallback input, CuName name, CuEvent value) {
    return hash_Replace(input->event_table, name, value);
}

static bool make_Any(SynType type, SynTarget target)
{
    if (!target.any) return false;

    static unsigned next_id = 1;
    unsigned fullsize = 0;

    switch (type2kind(type)) {
    case syn_unknown:  return false;
    case syn_any:      fullsize = sizeof(struct syn_any);      break;
    case syn_define:   fullsize = sizeof(struct syn_define);   break;
    case syn_text:     fullsize = sizeof(struct syn_text);     break;
    case syn_operator: fullsize = sizeof(struct syn_operator); break;
    case syn_tree:     fullsize = sizeof(struct syn_tree);     break;
    }

    SynAny result = malloc(fullsize);

    memset(result, 0, fullsize);

    result->type = type;
    result->id   = next_id++;

    *(target.any) = result;

    return true;
}

static unsigned depth(Copper file) {
    if (!file) return 0;

    CuContext local = theContext(file);

    struct prs_stack *stack = &(local->file_stack);
    struct prs_cell  *top   = stack->top;

    unsigned result = 0;

    for ( ; top ; top = top->next) {
        result += 1;
    }

    return result;
}

static bool push(Copper file, SynNode value) {
    if (!file)      return false;
    if (!value.any) return false;

    CuContext local = theContext(file);

    unsigned       fullsize = sizeof(struct prs_cell);
    struct prs_stack *stack = &(local->file_stack);
    struct prs_cell   *top  = stack->free_list;

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

static bool pop(Copper file, SynTarget target) {
    if (!file)       return false;
    if (!target.any) return false;

    CuContext local = theContext(file);

    struct prs_stack *stack = &(local->file_stack);
    struct prs_cell  *top   = stack->top;

    if (!top) return false;

    stack->top = top->next;
    top->next  = stack->free_list;
    stack->free_list = top;

    *(target.node) = top->value;

    return true;
}

static bool empty(Copper file) {
    if (!file) return true;

    CuContext local = theContext(file);

    struct prs_stack *stack = &(local->file_stack);

    if (!stack->top) return true;

    return false;
}

static bool makeText(Copper file, SynType type) {
    CuContext local = theContext(file);
    SynText   text  = 0;
    CuData    value;

    if (syn_text != type2kind(type)) return false;

    if (!cu_MarkedText(local, &value)) return false;

    if (!make_Any(type, &text)) return false;

    CU_DEBUG(1, "make %s \"%s\"\n", type2name(type), convert(value));

    text->value = value;

    return push(file, text);
}

static bool makeOperator(Copper file, SynType type) {
    SynOperator operator = 0;
    SynNode     value;

    if (syn_operator != type2kind(type)) return false;

    if (!pop(file, &value)) return false;

    if (!make_Any(type, &operator)) return false;

    operator->value = value;

    CU_DEBUG(1, "make %s\n", type2name(type));

    if (!push(file, operator)) return false;

    return true;
}

static bool makeTree(Copper file, SynType type) {
    SynTree tree = 0;
    SynNode before;
    SynNode after;

    if (syn_tree != type2kind(type)) return false;

    if (!pop(file, &after))  return false;
    if (!pop(file, &before)) return false;

    if (!make_Any(type, &tree)) return false;

    CU_DEBUG(1, "make %s (%s %s)\n",
             type2name(type),
             type2name(before.any->type),
             type2name(after.any->type));

    tree->before = before;
    tree->after  = after;

    return push(file, tree);
}

static bool checkRule(Copper file, CuCursor at, CuName xname) {
    CuContext local = theContext(file);
    CuData    name;

    if (!cu_MarkedText(local, &name)) return false;

    SynDefine rule = local->file_rules;

    for ( ; rule ; rule = rule->next ) {
        if (rule->name.length != name.length) continue;
        if (strncmp(rule->name.start, name.start, name.length)) continue;
        return push(file, rule);
    }

    if (!make_Any(syn_rule, &rule)) return false;

    rule->next = local->file_rules;
    local->file_rules = rule;
    rule->name = name;

    return push(file, rule);
    (void) at;
    (void) xname;
}

static bool defineRule(Copper file, CuCursor at, CuName name) {
    SynDefine rule;
    SynNode   value;

    if (!pop(file, &value)) return false;
    if (!pop(file, &rule))  return false;

    if (syn_rule != rule->type) {
        error_at_line(1, 0, __FILE__, __LINE__,
                      "invalid node type for rule: %s",
                      type2name(rule->type));
        return false;
    }

    CU_DEBUG(1, "make rule %s\n", convert(rule->name));

    if (!rule->value.any) {
        rule->value = value;
        return true;
    }

    SynTree tree;

    if (!make_Any(syn_choice, &tree)) return false;

    tree->before = rule->value;
    tree->after  = value;

    rule->value.tree = tree;

    return true;
    (void) at;
    (void) name;
}

static bool makeEnd(Copper file, CuCursor at, CuName name) {
    SynAny value = 0;

    if (!make_Any(syn_end, &value)) return false;

    if (!push(file, value)) return false;

    return true;
    (void) at;
    (void) name;
}

static bool makeBegin(Copper file, CuCursor at, CuName name) {
    SynAny value = 0;

    if (!make_Any(syn_begin, &value)) return false;

    if (!push(file, value)) return false;

    return true;
    (void) at;
    (void) name;
}

// namefull event
static bool makeApply(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_apply);
    (void) at;
    (void) name;
}

static bool makeArgument(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_argument);
    (void) at;
    (void) name;
}

static bool makePredicate(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_predicate);
    (void) at;
    (void) name;
}

static bool makeBinding(Copper file, CuCursor at, CuName name) {
    SynNode after;
    SynNode before;

    if (!pop(file, &after))  return false;
    if (!pop(file, &before)) return false;

    CU_DEBUG(1, "binding (%s %s)\n",
             type2name(before.any->type),
             type2name(after.any->type));

    if (!push(file, after)) return false;
    if (!push(file, before)) return false;

    return makeTree(file, syn_sequence);
    (void) at;
    (void) name;
}

static bool makeDot(Copper file, CuCursor at, CuName name) {
    SynAny value = 0;

    if (!make_Any(syn_dot, &value)) return false;

    if (!push(file, value)) return false;

    return true;
    (void) at;
    (void) name;
}

static bool makeSet(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_set);
    (void) at;
    (void) name;
}

static bool makeString(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_string);
    (void) at;
    (void) name;
}

static bool makeCall(Copper file, CuCursor at, CuName name) {
    return makeText(file, syn_call);
    (void) at;
    (void) name;
}

static bool makePlus(Copper file, CuCursor at, CuName name) {
    return makeOperator(file, syn_plus);
    (void) at;
    (void) name;
}

static bool makeStar(Copper file, CuCursor at, CuName name) {
    return makeOperator(file, syn_star);
    (void) at;
    (void) name;
}

static bool makeQuestion(Copper file, CuCursor at, CuName name) {
    return makeOperator(file, syn_question);
    (void) at;
    (void) name;
}

static bool makeNot(Copper file, CuCursor at, CuName name) {
    return makeOperator(file, syn_not);
    (void) at;
    (void) name;
}

static bool makeCheck(Copper file, CuCursor at, CuName name) {
    return makeOperator(file, syn_check);
    (void) at;
    (void) name;
}

static bool makeSequence(Copper file, CuCursor at, CuName name) {
    return makeTree(file, syn_sequence);
    (void) at;
    (void) name;
}

static bool makeLoop(Copper file, CuCursor at, CuName name) {
    return makeTree(file, syn_loop);
    (void) at;
    (void) name;
}

static bool makeChoice(Copper file, CuCursor at, CuName name) {
    return makeTree(file, syn_choice);
    (void) at;
    (void) name;
}

static void data_Write(CuData data, FILE* output);
static void char_Write(unsigned char value, FILE* output);
static void charclass_Write(unsigned char *bits, FILE* output);
static void namelist_Write(unsigned count, CuData name[], FILE* output);

static unsigned char *makeBitfield(CuData data)
{
    static unsigned char bits[32];
    bool clear = false;

    const char *cclass = data.start;
    unsigned inx = 0;
    unsigned max = data.length;

    inline void set(int value) {
        if (clear) {
            bits[value >> 3] &= ~(1 << (value & 7));
        } else {
            bits[value >> 3] |=  (1 << (value & 7));
        }
    }

    int curr;
    int prev= -1;

    if ('^' != cclass[inx]) {
        memset(bits, 0, 32);
    } else {
        memset(bits, 255, 32);
        clear = true;
        ++cclass;
    }

    for ( ; inx < max ; ) {
        curr = cclass[inx++];

        if ('-' == curr && (inx < max) && prev >= 0) {
            for (curr = cclass[inx++];  prev <= curr;  ++prev) set(prev);
            prev = -1;
            continue;
        }

        if ('\\' == curr && (inx < max)) {
            switch (curr = cclass[inx++]) {
            case 'a':  curr = '\a'; break;      /* bel */
            case 'b':  curr = '\b'; break;      /* bs */
            case 'e':  curr = '\e'; break;      /* esc */
            case 'f':  curr = '\f'; break;      /* ff */
            case 'n':  curr = '\n'; break;      /* nl */
            case 'r':  curr = '\r'; break;      /* cr */
            case 't':  curr = '\t'; break;      /* ht */
            case 'v':  curr = '\v'; break;      /* vt */
            default: break;
            }
            set(prev = curr);
        }

        set(prev = curr);
    }

    return bits;
}

static int data_FirstChar(CuData data)
{
    const char *at = data.start;
    unsigned inx = 0;
    unsigned max = data.length;

    int curr = at[inx++];

    if (('\\' == curr) && (inx < max)) {
        switch (curr = at[inx++]) {
        case 'a':  curr = '\a'; break;      /* bel */
        case 'b':  curr = '\b'; break;      /* bs */
        case 'e':  curr = '\e'; break;      /* esc */
        case 'f':  curr = '\f'; break;      /* ff */
        case 'n':  curr = '\n'; break;      /* nl */
        case 'r':  curr = '\r'; break;      /* cr */
        case 't':  curr = '\t'; break;      /* ht */
        case 'v':  curr = '\v'; break;      /* vt */
        default: break;
        }
    }

    return curr;
}

static bool node_ComputeSets(SynNode node)
{
    SynFirst first = 0;

    inline bool allocate(CuFirstType type, bool set, unsigned count) {
        unsigned fullsize = sizeof(struct syn_first) + (sizeof(CuData) * count);

        first = malloc(fullsize);

        if (!first) {
            printf("malloc failed\n");
            return false;
        }

        memset(first, 0, fullsize);

        unsigned char *bitfield = 0;

        if (set) {
            bitfield = malloc(sizeof(unsigned char) * 32);
            if (!bitfield) {
                printf("malloc failed\n");
                free(first);
                return false;
            }
            memset(bitfield, 0, sizeof(unsigned char) * 32);
        }

        first->type     = type;
        first->bitfield = bitfield;
        first->count    = count;
        node.any->first = first;

        return true;
    }

    inline void merge(unsigned char *src) {
        assert(first);

        if (!src) return;
        if (!first->bitfield) return;

        unsigned char *bits = first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
        }
    }

    inline void concat(SynFirst before, SynFirst after) {
        unsigned inx = 0;
        unsigned jnx = 0;
        for ( ; inx < before->count ; ++inx) {
            first->name[inx] = before->name[inx];
        }
        for ( ; jnx < after->count ; ++jnx, ++inx) {
            first->name[inx] = after->name[jnx];
        }
    }

    inline void set(unsigned value) {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        bits[value >> 3] |=  (1 << (value & 7));
    }

    inline void clear(int value) {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        bits[value >> 3] &= ~(1 << (value & 7));
    }

    inline void set_all() {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] = 255;
        }
    }

    inline bool copy(SynFirst child) {
        if (!child) return false;

        bool     bits  = (0 != child->bitfield);
        unsigned total = child->count;

        if (!allocate(child->type, bits, total)) return false;

        if (bits) merge(child->bitfield);

        for ( ; total-- ; ) {
            first->name[total] = child->name[total];
        }

        return true;
    }

    //-----------------------------------------------------------

    // - %predicate
    // - dot
    inline bool do_opaque() {
        return allocate(pft_opaque, false , 0);
    }

    // - @name
    // - set state.begin
    // - set state.end
    // - %footer ...
    // - %header {...}
    // - %include "..." or  %include <...>
    // - {...}
    inline bool do_event() {
        return allocate(pft_event, false , 0);
    }

    // - 'chr
    inline bool do_char() {
        if (!allocate(pft_fixed, true, 0)) return false;
        set(node.character->value);
        return true;
    }

    // - [...]
    inline bool do_set() {
        if (!allocate(pft_fixed, true, 0)) return false;
        merge(makeBitfield(node.text->value));
        return true;
    }

    // - "..."
    inline bool do_string() {
        if (!allocate(pft_fixed, true, 0)) return false;
        set(data_FirstChar(node.text->value));
        return true;
    }

    // - name
    inline bool do_call() {
        if (!allocate(pft_opaque, false, 1)) return false;
        first->name[0] = node.text->value;
        return true;
    }

    // - e !
    inline bool do_not() {
         if (!node_ComputeSets(node.operator->value)) return false;
         if (!allocate(pft_opaque, false, 0)) return false;

         SynFirst child = node.operator->value.any->first;

         first->type = inWithout(child->type);

         return true;
    }


    // - e &
    // - e +
    inline bool do_check() {
        if (!node_ComputeSets(node.operator->value)) return false;

        SynFirst child = node.operator->value.any->first;

        // T(f&) = f, T(o&) = o, T(t&) = t, T(e&) = e
        // T(f+) = f, T(o+) = o, T(t+) = t, T(e+) = ERROR

        node.any->first = child;

        return true;
    }

    // - e ?
    // - e *
    inline bool do_question() {
        if (!node_ComputeSets(node.operator->value))  return false;

        SynFirst child = node.operator->value.any->first;

        if (!copy(child)) return false;

        // T(f?) = t, T(o?) = o, T(t?) = t, T(e?) = e
        // T(f*) = t, T(o*) = o, T(t*) = t, T(e+) = ERROR

        first->type = inOptional(child->type);

        return true;
    }

    // - e1 / e2
    inline bool do_choice() {
        if (!node_ComputeSets(node.tree->before))  return false;
        if (!node_ComputeSets(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;
        SynFirst after  = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);

        CuFirstType type = inChoice(before->type, after->type);

        if (!allocate(type, bits, total)) return false;

        merge(before->bitfield);
        merge(after->bitfield);

        concat(before, after);

        return true;
    }

    // - e1 e2
    inline bool do_sequence() {
        if (!node_ComputeSets(node.tree->before))  return false;
        if (!node_ComputeSets(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;
        SynFirst after  = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);

        // T(ff) = f, T(fo) = f, T(ft) = f, T(fe) = f
        // T(of) = o, T(oo) = o, T(ot) = o, T(oe) = o
        // T(tf) = f, T(to) = o, T(tt) = t, T(te) = e
        // T(ef) = f, T(eo) = o, T(et) = e, T(ee) = e

        CuFirstType type = inSequence(before->type, after->type);

        if (!allocate(type, bits, total)) return false;

        merge(before->bitfield);

        if (pft_fixed != before->type) {
            merge(after->bitfield);
        }

        concat(before, after);

        return true;
    }

    // - e1 ; e2
    inline bool do_loop() {
        if (!node_ComputeSets(node.tree->before))  return false;
        if (!node_ComputeSets(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;
        SynFirst after  = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);

        // T(ff) = f, T(fo) = f, T(ft) = f, T(fe) = f
        // T(of) = o, T(oo) = o, T(ot) = o, T(oe) = o
        // T(tf) = f, T(to) = o, T(tt) = t, T(te) = e
        // T(ef) = f, T(eo) = o, T(et) = e, T(ee) = e

        CuFirstType type = inSequence(before->type, after->type);

        if (!allocate(type, bits, total)) return false;

        merge(before->bitfield);

        if (pft_fixed != before->type) {
            merge(after->bitfield);
        }

        concat(before, after);

        return true;
    }

    // - identifier = ....
    inline bool do_rule() {
        if (!node_ComputeSets(node.define->value))  return false;
        node.any->first = node.operator->value.any->first;
        return true;
    }

    switch (node.any->type) {
    case syn_apply:     return do_event();    // - @name
    case syn_argument:  return do_opaque();   // - :[name]
    case syn_begin:     return do_event();    // - set state.begin
    case syn_call:      return do_call();     // - name
    case syn_char:      return do_char();     // - 'chr
    case syn_check:     return do_check();    // - e &
    case syn_choice:    return do_choice();   // - e1 | e2
    case syn_dot:       return do_opaque();   // - dot
    case syn_end:       return do_event();    // - set state.end
    case syn_loop:      return do_loop();     // - e1 ; e2
    case syn_not:       return do_not();      // - e !
    case syn_plus:      return do_check();    // - e +
    case syn_predicate: return do_opaque();   // - %predicate
    case syn_question:  return do_question(); // - e ?
    case syn_rule:      return do_rule();     // - identifier = ....
    case syn_sequence:  return do_sequence(); // - e1 e2
    case syn_set:       return do_set();      // - [...]
    case syn_star:      return do_question(); // - e *
    case syn_string:    return do_string();   // - "..."
        /* */
    case syn_void:
        break;
    }

    return do_opaque();
}

static void data_cooked_Write(CuData data, FILE* output)
{
    unsigned    count = data.length;
    const char* ptr   = (const char*) data.start;
    int         chr;

    fprintf(output, "\"");

    for ( ; count ; --count, ++ptr) {
        chr = *ptr;
        switch (chr) {
        case '\'':
        case '\"':
        case '\\':
            fprintf(output, "\\");
        default:
            break;
        }
        fprintf(output, "%c", chr);
    }

    fprintf(output, "\"");
}

static void data_Write(CuData data, FILE* output)
{
    unsigned    count = data.length;
    const char* ptr   = (const char*) data.start;

    fprintf(output, "\"");

    for ( ; count ; --count, ++ptr) {
        fprintf(output, "%c", *ptr);
    }

    fprintf(output, "\"");
}

static unsigned data_Count(CuData data)
{
    unsigned    size  = 0;
    unsigned    count = data.length;
    const char* ptr   = (const char*) data.start;
    int         chr;

    for ( ; count ; --count, ++ptr) {
        chr = *ptr;
        if ('\\' != chr) {
            size += 1;
            continue;
        }

        --count; ++ptr; size += 1;
        if ( !count ) return size;

        chr = *ptr;
        switch (chr) {
        case '0':
        case '1':
        case '2':
        case '3': {
            --count; ++ptr;
            if ( !count ) return size;
            --count; ++ptr;
            if ( !count ) return size;
        }

        default: continue;
        }
    }

    return size;
}

static void char_Write(unsigned char value, FILE* output)
{
    switch (value) {
    case '\a': fprintf(output, "\'%s\'", "\\a");  return; /* bel */
    case '\b': fprintf(output, "\'%s\'", "\\b");  return; /* bs */
    case '\e': fprintf(output, "\'%s\'", "\\e");  return; /* esc */
    case '\f': fprintf(output, "\'%s\'", "\\f");  return; /* ff */
    case '\n': fprintf(output, "\'%s\'", "\\n");  return; /* nl */
    case '\r': fprintf(output, "\'%s\'", "\\r");  return; /* cr */
    case '\t': fprintf(output, "\'%s\'", "\\t");  return; /* ht */
    case '\v': fprintf(output, "\'%s\'", "\\v");  return; /* vt */
    case '\'': fprintf(output, "\'%s\'", "\\'");  return; /* ' */
    case '\"': fprintf(output, "\'%s\'", "\\\""); return; /* " */
    case '\\': fprintf(output, "\'%s\'", "\\\\"); return; /* \ */
    default:   fprintf(output, "\'%c\'", value);  return;
    }
}

static void charclass_Write(unsigned char *bits, FILE* output)
{
   assert(0 != bits);

   fprintf(output, "\"");

   int inx = 0;
   for ( ; inx < 32;  ++inx) {
       fprintf(output, "\\%03o", bits[inx]);
   }

   fprintf(output, "\"");
}

static void namelist_Write(unsigned count, CuData name[], FILE* output)
{
    if (1 > count) return;

    unsigned index = 0;

    fprintf(output, "{ \"%s\"", convert(name[index]));

    for (++index ; index < count ; ++index) {
        fprintf(output, ", \"%s\"", convert(name[index]));
    }

    fprintf(output, " }");
}

static bool node_WriteSets(SynNode node, FILE* output)
{
    switch (type2kind(node.any->type)) {
    case syn_unknown: return false;
    case syn_define:
        if (!node_WriteSets(node.define->value, output))  return false;
        break;

    case syn_operator:
        if (!node_WriteSets(node.operator->value, output))  return false;
        break;

    case syn_tree:
        if (!node_WriteSets(node.tree->before, output))  return false;
        if (!node_WriteSets(node.tree->after, output))   return false;
        break;

    case syn_any:   break;
    case syn_text:  break;
    }

    SynFirst first = node.any->first;

    if (!first) return true;

    if (first->bitfield) {
        fprintf(output,
                "static struct cu_firstset  first_%.6x_set  = { ",
                node.any->id);

        charclass_Write(first->bitfield, output);

        fprintf(output, " };\n");
    }

    if (first->count) {
        fprintf(output,
                "static struct cu_firstlist first_%.6x_list = { %u, ",
                node.any->id,
                first->count);

        namelist_Write(first->count, first->name, output);

        fprintf(output, " };\n");
    }

    return true;
}

static bool node_FirstSet(SynNode node, FILE* output, const char **target)
{
    static char     buffer[600];
    static unsigned index = 0;

    char *start = buffer + (index * 100);
    char *ptr   = start;

    ++index;
    if (5 < index) index = 0;

    if (!node.any->first) {
        sprintf(ptr, "pft_opaque,0/*missing*/,0,\"%.6x\",0,", node.any->id);
        *target = start;
        return true;
    }

    SynFirst first = node.any->first;

    ptr += sprintf(ptr, "%s, ", first2name(first->type));

    if (!first->bitfield) {
        ptr += sprintf(ptr, "%u, ", 0);
    } else {
        ptr += sprintf(ptr, "&first_%.6x_set, ", node.any->id);
    }

    if (!first->count) {
         ptr += sprintf(ptr, "%u, ", 0);
    } else {
        ptr += sprintf(ptr, "&first_%.6x_list, ", node.any->id);
    }

    ptr += sprintf(ptr, "%u, ", 0);

    ptr += sprintf(ptr, "\"_%.6x\", ", node.any->id);

    *target = start;

    return true;
    (void) output;
}

static bool node_WriteTree(SynNode node, FILE* output)
{
    if (!node.any) return false;

    const char *first_sets = "0,0,0,0";

    inline bool do_thunk(const char* kind) {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s %s, no_argument };\n",
                node.any->id,
                first_sets,
                kind);

        return true;
    }

    inline bool do_action(const char* kind) {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s %s, (union cu_arg) ((CuName)",
                node.any->id,
                first_sets,
                kind);

        data_Write(node.text->value, output);

        fprintf(output,") };\n");

        return true;
    }

    inline bool do_branch(const char* kind) {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s %s, (union cu_arg) (&node_%.6x) };\n",
                node.any->id,
                first_sets,
                kind,
                node.operator->value.any->id);

        return true;
    }

    inline bool do_tree(const char* kind) {
        if (!node_WriteTree(node.tree->before, output)) return false;
        if (!node_WriteTree(node.tree->after, output))  return false;

        fprintf(output,
                "static struct cu_pair   pair_%.6x  = { &node_%.6x, &node_%.6x };\n",
                node.any->id,
                node.tree->before.any->id,
                node.tree->after.any->id);

        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s %s, (union cu_arg) (&pair_%.6x) };\n",
                node.any->id,
                first_sets,
                kind,
                node.any->id);

        return true;
    }

    inline bool do_char() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s cu_MatchChar, (union cu_arg) ((CuChar)",
                node.any->id,
                first_sets);

        char_Write(node.character->value, output);

        fprintf(output, ") };\n");

        return true;
    }

    inline bool do_set() {
        fprintf(output,
                "static struct cu_set    set_%.6x   = { ",
                node.any->id);

        data_cooked_Write(node.text->value, output);

        fprintf(output, ", ");

        charclass_Write(makeBitfield(node.text->value), output);

        fprintf(output, " };\n");

        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s cu_MatchSet, (union cu_arg) (&set_%.6x) };\n",
                node.any->id,
                first_sets,
                node.any->id);

        return true;
    }

    inline bool do_string() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct cu_string text_%.6x  = { %i, ",
                node.any->id,
                data_Count(node.text->value));

        data_Write(node.text->value, output);

        fprintf(output, " };\n");

        fprintf(output,
                "static struct cu_node   node_%.6x  = { %s cu_MatchText, (union cu_arg) (&text_%.6x) };\n",
                node.any->id,
                first_sets,
                node.any->id);

        return true;
    }

    inline bool do_node() {
        switch (node.any->type) {
        case syn_apply:     return do_action("cu_Apply");
        case syn_argument:  return do_action("cu_Argument");
        case syn_begin:     return do_thunk("cu_Begin");
        case syn_call:      return do_action("cu_MatchName");
        case syn_char:      return do_char();
        case syn_check:     return do_branch("cu_AssertTrue");
        case syn_choice:    return do_tree("cu_Choice");
        case syn_dot:       return do_thunk("cu_MatchDot");
        case syn_end:       return do_thunk("cu_End");
        case syn_loop:      return do_tree("cu_Loop");
        case syn_not:       return do_branch("cu_AssertFalse");
        case syn_plus:      return do_branch("cu_OneOrMore");
        case syn_predicate: return do_action("cu_Predicate");
        case syn_question:  return do_branch("cu_ZeroOrOne");
        case syn_sequence:  return do_tree("cu_Sequence");
        case syn_set:       return do_set();
        case syn_star:      return do_branch("cu_ZeroOrMore");
        case syn_string:    return do_string();
            /* */
        case syn_rule: break;
        case syn_void: break;
        }
        return false;
    }

    return do_node();
}

static bool writeTree(Copper file, CuCursor at, CuName name) {
    if (!empty(file)) {
        CU_ERROR("stack not empty ; %u\n", depth(file));
    }
    return true;
    (void) at;
    (void) name;
}

extern bool cu_GlobalInit(CuCallback callback) {
    callback->node      = copper_FindNode;
    callback->predicate = copper_FindPredicate;
    callback->event     = copper_FindEvent;

    callback->attach_node      = copper_SetNode;
    callback->attach_predicate = copper_SetPredicate;
    callback->attach_event     = copper_SetEvent;

    make_Hash(100, &(callback->node_table));
    make_Hash(100, &(callback->predicate_table));
    make_Hash(100, &(callback->event_table));

    return true;
}

extern bool file_ParserInit(Copper file) {
    CuCallback callback = theCallback(file);
    CuContext     local = theContext(file);

    cu_GlobalInit(callback);
    cu_LocalInit(local, 1024);

    inline bool attach(CuName name, CuEvent value) { return callback->attach_event(callback, name, value); }

    attach("writeTree", writeTree);
    attach("checkRule", checkRule);
    attach("defineRule", defineRule);
    attach("makeEnd", makeEnd);
    attach("makeBegin", makeBegin);
    attach("makeApply", makeApply);
    attach("makeArgument", makeArgument);
    attach("makePredicate", makePredicate);
    attach("makeDot", makeDot);
    attach("makeSet", makeSet);
    attach("makeString", makeString);
    attach("makeCall", makeCall);
    attach("makePlus", makePlus);
    attach("makeStar", makeStar);
    attach("makeQuestion", makeQuestion);
    attach("makeNot", makeNot);
    attach("makeCheck", makeCheck);
    attach("makeSequence", makeSequence);
    attach("makeChoice", makeChoice);
    attach("makeLoop", makeLoop);
    attach("bindTo", makeBinding);

    copper_graph(callback);

    return true;
}

extern bool file_WriteTree(Copper file, FILE* output, const char* function) {
    CuContext local = theContext(file);
    fprintf(output, "/*-*- mode: c;-*-*/\n");
    fprintf(output, "/* A recursive-descent parser generated by copper %s */\n", COPPER_VERSION);
    fprintf(output, "\n");
    fprintf(output, "/* ================================================== */\n");
    fprintf(output, "#include <copper.h>\n");
    fprintf(output, "/* ================================================== */\n");
    fprintf(output, "\n");
    fprintf(output, "\n");
    fprintf(output, "#define no_argument ((union cu_arg) ((CuName)0))\n\n");

    SynDefine rule = local->file_rules;
    for ( ; rule ; rule = rule->next ) {
        fprintf(output, "/* %s */\n", convert(rule->name));
        node_ComputeSets(rule->value);
        node_WriteSets(rule->value, output);
        node_WriteTree(rule->value, output);
        fprintf(output, "\n");
    }

    fprintf(output, "\n");
    fprintf(output, "extern bool %s(CuCallback input) {\n", function);
    fprintf(output, "    inline bool attach(CuName name, CuNode value) { return input->attach_node(input, name, value); }\n");
    fprintf(output, "\n");

    rule = local->file_rules;
    for ( ; rule ; rule = rule->next ) {
        fprintf(output, "    if (!attach(\"%s\", &node_%.6x)) return false;\n",
                convert(rule->name),
                rule->value.any->id);
    }
    fprintf(output, "\n");
    fprintf(output, "    return true;\n");
    fprintf(output, "}\n");
    fprintf(output, "\n");

    return true;
    (void)file;
    (void)local;
}
