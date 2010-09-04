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
#include "compiler.h"

/* */
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */


static char buffer[4096];

static const char* convert(PrsData name) {
    strncpy(buffer, name.start, name.length);
    buffer[name.length] = 0;
    return buffer;
}

static bool make_Map(unsigned code,
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

static bool make_Hash(Hashcode encode,
                      Matchkey compare,
                      unsigned size,
                      struct prs_hash **target)
{
    unsigned fullsize = (sizeof(struct prs_hash)
                         + (size * sizeof(struct prs_map *)));

    struct prs_hash* result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->encode  = encode;
    result->compare = compare;
    result->size    = size;

    *target = result;
    return true;
}

static bool hash_Find(struct prs_hash *hash,
                      const void* key,
                      void** target)
{
    if (!key) return false;

    unsigned code  = hash->encode(key);
    unsigned index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!hash->compare(key, map->key)) continue;
        *target = map->value;
        return true;
    }

    return false;
}

static bool hash_Replace(struct prs_hash *hash,
                         const void* key,
                         void* value,
                         FreeValue release)
{
    if (!key)     return false;
    if (!value)   return false;
    if (!release) return false;

    unsigned long code  = hash->encode(key);
    unsigned      index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!hash->compare(key, map->key)) continue;
        if (release(map->value)) {
            return false;
        }
        map->value = value;
        return true;
    }

    if (!make_Map(code, key, value, hash->table[index], &map)) {
        return false;
    }

    hash->table[index] = map;

    return true;
}

static bool buffer_GetLine(struct prs_buffer *input, PrsInput base)
{
    if (input->cursor >= input->read) {
        int read = getline(&input->line, &input->allocated, input->file);
        if (read < 0) return false;
        input->cursor = 0;
        input->read   = read;
    }

    unsigned  count = input->read - input->cursor;
    const char *src = input->line + input->cursor;

    if (!cu_AppendData(base, count, src)) return false;

    input->cursor += count;

    return true;
}

static bool file_MoreData(PrsInput input) {
    struct prs_file *file = (struct prs_file *)input;
    return buffer_GetLine(&file->buffer, &file->base);
}

static bool file_FindNode(PrsInput input, PrsName name, PrsNode* target) {
    struct prs_file *file = (struct prs_file *)input;
    if (hash_Find(file->nodes, name, (void**)target)) {
        return true;
    }
    return false;
}

static bool noop_release(void* value) {
    return true;
}

static bool file_FindPredicate(PrsInput input, PrsName name, PrsPredicate* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->predicates, name, (void**)target);
}

static bool file_FindEvent(PrsInput input, PrsName name, PrsEvent* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->events, name, (void**)target);
}

static bool file_AddName(PrsInput input, PrsName name, PrsNode value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->nodes, (void*)name, value, noop_release);
}

/* -- */

extern bool file_SetPredicate(struct prs_file *file, PrsName name, PrsPredicate value) {
    return hash_Replace(file->predicates, (void*)name, value, noop_release);
}

extern bool file_SetEvent(struct prs_file *file, PrsName name, PrsEvent value) {
    return hash_Replace(file->events, (void*)name, value, noop_release);
}

static unsigned long encode_name(PrsName name) {
    const char *cursor = name;

    unsigned long result = 5381;

    for ( ; *cursor ; ++cursor ) {
        int val = *cursor;
        result = ((result << 5) + result) + val;
    }

    return result;
}

static unsigned compare_name(PrsName lname, PrsName rname) {
    const char *left  = lname;
    const char *right = rname;

    int result = strcmp(left, right);

    return (0 == result);
}

extern bool make_PrsFile(FILE* file, const char* filename, PrsInput *target) {
    struct prs_file *result = malloc(sizeof(struct prs_file));

    memset(result, 0, sizeof(struct prs_file));

    result->base.more      = file_MoreData;
    result->base.node      = file_FindNode;
    result->base.attach    = file_AddName;
    result->base.predicate = file_FindPredicate;
    result->base.event     = file_FindEvent;

    cu_InputInit(&result->base, 1024);

    /* */
    result->filename    = strdup(filename);
    result->buffer.file = file;

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->nodes);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->predicates);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->events);

    *target = (PrsInput) result;

    return true;
}

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

static bool makeChunk(struct prs_file *file, SynType type, SynChunk *target) {
    SynChunk chunk = 0;
    PrsData value;

    if (syn_chunk != type2kind(type)) return false;

    if (!cu_MarkedText((PrsInput) file, &value)) return false;

    if (!make_Any(type, &chunk)) return false;

    CU_DEBUG(1, "make %s\n", type2name(type));

    if (!file->chunks) {
        file->chunks = chunk;
    } else {
        SynChunk last = file->chunks;
        for ( ; last->next ; last = last->next) ;
        last->next = chunk;
    }

    chunk->value = value;

    if (target) {
        *target = chunk;
    }

    return true;
}

static unsigned depth(struct prs_file *file) {
    if (!file) return 0;

    struct prs_stack *stack = &file->stack;
    struct prs_cell  *top   = stack->top;

    unsigned result = 0;

    for ( ; top ; top = top->next) {
        result += 1;
    }

    return result;
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

static bool empty(struct prs_file *file) {
    if (!file) return true;

    struct prs_stack *stack = &file->stack;

    if (!stack->top) return true;

    return false;
}

static bool makeText(struct prs_file *file, SynType type) {
    SynText text = 0;
    PrsData value;

    if (syn_text != type2kind(type)) return false;

    if (!cu_MarkedText((PrsInput) file, &value)) return false;

    if (!make_Any(type, &text)) return false;

    CU_DEBUG(1, "make %s \"%s\"\n", type2name(type), convert(value));

    text->value = value;

    return push(file, text);
}

static bool makeOperator(struct prs_file *file, SynType type) {
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

static bool makeTree(struct prs_file *file, SynType type) {
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

extern bool checkRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    PrsData name;

    if (!cu_MarkedText((PrsInput) file, &name)) return false;

    SynDefine rule = file->rules;

    for ( ; rule ; rule = rule->next ) {
        if (rule->name.length != name.length) continue;
        if (strncmp(rule->name.start, name.start, name.length)) continue;
        return push(file, rule);
    }

    if (!make_Any(syn_rule, &rule)) return false;

    rule->next  = file->rules;
    file->rules = rule;
    rule->name  = name;

    return push(file, rule);
}

extern bool defineRule(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

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


static void data_Write(PrsData data, FILE* output);
static void char_Write(unsigned char value, FILE* output);
static void charclass_Write(unsigned char *bits, FILE* output);
static void namelist_Write(unsigned count, PrsData name[], FILE* output);

static unsigned char *makeBitfield(PrsData data)
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

    //    printf("make ->");
    //    charclass_Write(bits, stdout);
    //    printf("\n");

    return bits;
}

static int data_FirstChar(PrsData data)
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

    inline bool allocate(PrsFirstType type, bool set, unsigned count) {
        unsigned fullsize = sizeof(struct syn_first) + (sizeof(PrsData) * count);

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
        assert(src);
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
        }
        //        printf("merge ->");
        //        charclass_Write(bits, stdout);
        //        printf("\n");
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

        //        printf("set(%u) ->", value);
        //        charclass_Write(bits, stdout);
        //        printf("\n");
    }

    inline void clear(int value) {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        bits[value >> 3] &= ~(1 << (value & 7));

        //        printf("clear(%u) ->", value);
        //        charclass_Write(bits, stdout);
        //        printf("\n");
    }

    inline void invert() {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] ^= 255;
        }
        //        printf("invert ->");
        //        charclass_Write(bits, stdout);
        //        printf("\n");
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

    //----

    // - name
    inline bool do_call() {
        if (!allocate(pft_opaque, false, 1)) return false;
        first->name[0] = node.text->value;
        return true;
    }

    // - 'chr
    inline bool do_char() {
        if (!allocate(pft_fixed, true, 0)) return false;
        set(node.character->value);
        return true;
    }

    // - e &
    inline bool do_check() {
        if (!node_ComputeSets(node.operator->value)) return false;
        node.any->first = node.operator->value.any->first;
        return true;
    }

    // - e1 e2 |
    inline bool do_choice() {
        if (!node_ComputeSets(node.tree->before))  return false;
        if (!node_ComputeSets(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;
        SynFirst after  = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);
        PrsFirstType type  = pft_fixed;

        // F(*|transparent) -> F(transparent)
        // F(transparent|*) -> F(transparent)
        // F(fixed|opaque)  -> F(opaque)
        // F(opaque|fixed)  -> F(opaque)
        // F(fixed|fixed)   -> F(fixed)

        if (pft_opaque == before->type)      type = pft_opaque;
        if (pft_opaque == after->type)       type = pft_opaque;
        if (pft_transparent == before->type) type = pft_transparent;
        if (pft_transparent == after->type)  type = pft_transparent;

        if (!allocate(type, bits, total)) return false;

        if (before->bitfield) merge(before->bitfield);
        if (after->bitfield)  merge(after->bitfield);

        concat(before, after);

        return true;
    }

    // - dot
    inline bool do_dot() {
        if (!allocate(pft_fixed, true, 0)) return false;
        invert();
        return true;
    }

    // - e !
    inline bool do_not() {
         if (!node_ComputeSets(node.operator->value)) return false;
         if (!allocate(pft_opaque, false, 0)) return false;
         return true;
    }

    // - e +
    inline bool do_question() {
        if (!node_ComputeSets(node.operator->value))  return false;
        if (!copy(node.operator->value.any->first)) return false;
        first->type = pft_transparent;
        return true;
    }

    // - identifier = ....
    inline bool do_rule() {
        if (!node_ComputeSets(node.define->value))  return false;
        node.any->first = node.operator->value.any->first;
        return true;
    }

    // - e1 e2 ;
    inline bool do_sequence() {
        if (!node_ComputeSets(node.tree->before))  return false;
        if (!node_ComputeSets(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;

        if (pft_fixed == before->type) {
            node.any->first = before;
            return true;
        }

        SynFirst after = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);
        PrsFirstType type  = pft_fixed;

        // F(*,opaque)                -> F(opaque)
        // F(*,fixed)                 -> F(fixed)
        // F(opaque,transparent)      -> F(opaque)
        // F(fixed,transparent)       -> F(fixed)
        // F(transparent,transparent) -> F(transparent)

        switch (after->type) {
        case pft_opaque:
            type = pft_opaque;
            break;

        case pft_fixed:
            type = pft_fixed;
            break;

        case pft_transparent:
            type = before->type;
            break;
        }

        if (!allocate(type, bits, total)) return false;

        if (before->bitfield) merge(before->bitfield);

        if (after->bitfield) {
            if (pft_fixed != before->type) {
                merge(after->bitfield);
            }
        }

        concat(before, after);

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

    // - @name
    // - set state.begin
    // - set state.end
    // - %footer ...
    // - %header {...}
    // - %include "..." or  %include <...>
    // - %predicate
    // - {...}
    inline bool opaque() {
        return allocate(pft_opaque, false , 0);
    }

    switch (node.any->type) {
    case syn_apply:     return opaque();      // - @name
    case syn_begin:     return opaque();      // - set state.begin
    case syn_call:      return do_call();     // - name
    case syn_char:      return do_char();     // - 'chr
    case syn_check:     return do_check();    // - e &
    case syn_choice:    return do_choice();   // - e1 e2 |
    case syn_dot:       return do_dot();      // - .
    case syn_end:       return opaque();      // - set state.end
    case syn_footer:    return opaque();      // - %footer ...
    case syn_header:    return opaque();      // - %header {...}
    case syn_include:   return opaque();      // - %include "..." or  %include <...>
    case syn_not:       return do_not();      // - e !
    case syn_plus:      return do_check();    // - e +
    case syn_predicate: return opaque();      // - %predicate
    case syn_question:  return do_question(); // - e ?
    case syn_rule:      return do_rule();     // - identifier = ....
    case syn_sequence:  return do_sequence(); // - e1 e2 ;
    case syn_set:       return do_set();      // - [...]
    case syn_star:      return do_question(); // - e *
    case syn_string:    return do_string();   // - "..."
    case syn_thunk:     return opaque();      // - {...}
        /* */
    case syn_void:
        break;
    }

    return opaque();
}

static void data_cooked_Write(PrsData data, FILE* output)
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

static void data_Write(PrsData data, FILE* output)
{
    unsigned    count = data.length;
    const char* ptr   = (const char*) data.start;

    fprintf(output, "\"");

    for ( ; count ; --count, ++ptr) {
        fprintf(output, "%c", *ptr);
    }

    fprintf(output, "\"");
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

static void namelist_Write(unsigned count, PrsData name[], FILE* output)
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
    case syn_chunk: break;
    }

    SynFirst first = node.any->first;

    if (!first) return true;

    if (first->bitfield) {
        fprintf(output,
                "static struct prs_firstset  first_%x_set  = { ",
                (unsigned) node.any);

        charclass_Write(first->bitfield, output);

        fprintf(output, " };\n");
    }

    if (first->count) {
        fprintf(output,
                "static struct prs_firstlist first_%x_list = { %u, ",
                (unsigned) node.any,
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
        sprintf(ptr, "pft_opaque,0/*missing*/,0,\"%x\",0,", (unsigned) node.any);
        *target = start;
        return true;
    }

    SynFirst first = node.any->first;

    ptr += sprintf(ptr, "%s, ", first2name(first->type));

    if (!first->bitfield) {
        ptr += sprintf(ptr, "%u, ", 0);
    } else {
        ptr += sprintf(ptr, "&first_%x_set, ", (unsigned) node.any);
    }

    if (!first->count) {
         ptr += sprintf(ptr, "%u, ", 0);
    } else {
        ptr += sprintf(ptr, "&first_%x_list, ", (unsigned) node.any);
    }

    ptr += sprintf(ptr, "%u, ", 0);

    ptr += sprintf(ptr, "\"_%x\", ", (unsigned) node.any);

    *target = start;

    return true;
}

static bool node_WriteTree(SynNode node, FILE* output)
{
    if (!node.any) return false;

    const char *first_sets = "0,0,0,0";

    inline bool do_apply() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Apply, (union prs_arg) ((PrsName)",
                (unsigned) node.any,
                first_sets);

        data_Write(node.text->value, output);

        fprintf(output,") };\n");

        return true;
    }

    inline bool do_begin() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Begin, };\n",
                (unsigned) node.any,
                first_sets);

        return true;
    }

    inline bool do_call() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchName, (union prs_arg) ((PrsName)",
                (unsigned) node.any,
                first_sets);

        data_Write(node.text->value, output);

        fprintf(output, ") };\n");

        return true;
    }

    inline bool do_char() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchChar, (union prs_arg) ((PrsChar)",
                (unsigned) node.any,
                first_sets);

        char_Write(node.character->value, output);

        fprintf(output, ") };\n");

        return true;
    }

    inline bool do_check() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_AssertTrue, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);

        return true;
    }

    inline bool do_choice() {
        if (!node_WriteTree(node.tree->before, output)) return false;
        if (!node_WriteTree(node.tree->after, output))  return false;

        fprintf(output,
                "static struct prs_pair  pair_%x  = { &node_%x, &node_%x };\n",
                (unsigned) node.any,
                (unsigned) node.tree->before.any,
                (unsigned) node.tree->after.any);

        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Choice, (union prs_arg) (&pair_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);

        return true;
    }

    inline bool do_dot() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchDot };\n",
                (unsigned) node.any,
                first_sets);

        return true;
    }

    inline bool do_end() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_End, };\n",
                (unsigned) node.any,
                first_sets);

        return true;
    }

    inline bool do_footer() {
        return false;
    }

    inline bool do_header() {
        return false;
    }

    inline bool do_include() {
        return false;
    }

    inline bool do_not() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_AssertFalse, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);

        return true;
    }

    inline bool do_plus() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_OneOrMore, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);

        return true;
    }

    inline bool do_predicate() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Predicate, (union prs_arg) ((PrsName)",
                (unsigned) node.any,
                first_sets);

        data_Write(node.text->value, output);

        fprintf(output, ") };\n");

        return true;
    }

    inline bool do_question() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_ZeroOrOne, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);

        return true;
    }

    inline bool do_rule() {
        return false;
    }

    inline bool do_sequence() {
        if (!node_WriteTree(node.tree->before, output)) return false;
        if (!node_WriteTree(node.tree->after, output))  return false;

        fprintf(output,
                "static struct prs_pair  pair_%x  = { &node_%x, &node_%x };\n",
                (unsigned) node.any,
                (unsigned) node.tree->before.any,
                (unsigned) node.tree->after.any);

        if (!node_FirstSet(node, output, &first_sets))  return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Sequence, (union prs_arg) (&pair_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);

        return true;
    }

    inline bool do_set() {
        fprintf(output,
                "static struct prs_set   set_%x   = { ",
                (unsigned) node.any);

        data_cooked_Write(node.text->value, output);

        fprintf(output, ", ");

        charclass_Write(makeBitfield(node.text->value), output);

        fprintf(output, " };\n");

        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchSet, (union prs_arg) (&set_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);

        return true;
    }

    inline bool do_star() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        if (!node_FirstSet(node, output, &first_sets))     return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_ZeroOrMore, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);

        return true;
    }

    inline bool do_string() {
        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchText, (union prs_arg) ((PrsName)",
                (unsigned) node.any,
                first_sets);

        data_Write(node.text->value, output);

        fprintf(output, ") };\n");

        return true;
    }

    inline bool do_thunk() {
        fprintf(output,
                "static struct prs_label label_%x = { (&event_%x), \"event_%x\" };\n",
                (unsigned) node.any,
                (unsigned) node.any,
                (unsigned) node.any);

        if (!node_FirstSet(node, output, &first_sets)) return false;

        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Thunk, (union prs_arg) (&label_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);

        return true;
    }

    inline bool do_node() {
        switch (node.any->type) {
        case syn_apply:     return do_apply();
        case syn_begin:     return do_begin();
        case syn_call:      return do_call();
        case syn_char:      return do_char();
        case syn_check:     return do_check();
        case syn_choice:    return do_choice();
        case syn_dot:       return do_dot();
        case syn_end:       return do_end();
        case syn_footer:    return do_footer();
        case syn_header:    return do_header();
        case syn_include:   return do_include();
        case syn_not:       return do_not();
        case syn_plus:      return do_plus();
        case syn_predicate: return do_predicate();
        case syn_question:  return do_question();
        case syn_rule:      return do_rule();
        case syn_sequence:  return do_sequence();
        case syn_set:       return do_set();
        case syn_star:      return do_star();
        case syn_string:    return do_string();
        case syn_thunk:     return do_thunk();
            /* */
        case syn_void: break;
        }
        return false;
    }

    return do_node();
}

extern bool writeTree(PrsInput input, PrsCursor at) {
    struct prs_file *file = (struct prs_file *)input;

    if (!empty(file)) {
        CU_ERROR("stack not empty ; %u\n", depth(file));
    }

    return true;
}


extern bool file_WriteTree(PrsInput input, FILE* output, const char* function) {
    struct prs_file *file = (struct prs_file *)input;

    fprintf(output, "/*-*- mode: c;-*-*/\n");
    fprintf(output, "/* A recursive-descent parser generated by copper %d.%d.%d */\n", COPPER_MAJOR, COPPER_MINOR, COPPER_LEVEL);
    fprintf(output, "\n");
    fprintf(output, "/* ================================================== */\n");
    fprintf(output, "#include <copper.h>\n");
    fprintf(output, "/* ================================================== */\n");

    SynChunk chunk = file->chunks;
    for ( ; chunk ; chunk = chunk->next ) {
        switch (chunk->type) {
        default: break;
        case syn_header:
            fprintf(output, "%s\n", convert(chunk->value));
            break;
        case syn_include:
            fprintf(output, "#include %s\n", convert(chunk->value));
            break;

        case syn_thunk:
            fprintf(output, "static bool event_%x(PrsInput input, PrsCursor cursor) {\n"
                    "    %s\n"
                    "    return true;"
                    "\n}"
                    "\n",
                    (unsigned)chunk,
                    convert(chunk->value));
            break;
        }
    }
    fprintf(output, "\n");

    SynDefine rule = file->rules;
    for ( ; rule ; rule = rule->next ) {
        fprintf(output, "/* %s */\n", convert(rule->name));
        node_ComputeSets(rule->value);
        node_WriteSets(rule->value, output);
        node_WriteTree(rule->value, output);
        fprintf(output, "\n");
    }

    fprintf(output, "extern bool %s(PrsInput input) {\n", function);
    fprintf(output, "\n");
    fprintf(output, "    inline bool attach(PrsName name, PrsNode value) { return cu_AddName(input, name, value); }\n");
    fprintf(output, "\n");

    rule = file->rules;
    for ( ; rule ; rule = rule->next ) {
        fprintf(output, "    if (!attach(\"%s\", &node_%x)) return false;\n",
                convert(rule->name),
                (unsigned) rule->value.any);
    }
    fprintf(output, "\n");
    fprintf(output, "    return true;\n");
    fprintf(output, "}\n");
    fprintf(output, "\n");

    chunk = file->chunks;
    for ( ; chunk ; chunk = chunk->next ) {
        switch (chunk->type) {
        default: break;
        case syn_footer:
            fprintf(output, "%s\n", convert(chunk->value));
            break;
        }
    }

    return true;
}

