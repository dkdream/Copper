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

    return false;
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

static char *makeConstCChar(unsigned char value)
{
    static char text[4];

    switch (value) {
    case '\a': return "\\a"; /* bel */
    case '\b': return "\\b"; /* bs */
    case '\e': return "\\e"; /* esc */
    case '\f': return "\\f"; /* ff */
    case '\n': return "\\n"; /* nl */
    case '\r': return "\\r"; /* cr */
    case '\t': return "\\t"; /* ht */
    case '\v': return "\\v"; /* vt */
    case '\'': return "\\'"; /* ' */
    case '\"': return "\\\"";
    case '\\': return "\\\\";
    default:
        break;
    }

    text[0] = (char) value;
    text[1] = 0;
    return text;
}

static unsigned char *makeBitfield(PrsData data)
{
    static unsigned char bits[32];
    bool clear = false;

    const unsigned char *cclass = (const unsigned char *) convert(data);

    inline void set(int value) {
        if (clear) {
            bits[value >> 3] &= ~(1 << (value & 7));
        } else {
            bits[value >> 3] |=  (1 << (value & 7));
        }
    }

    int   curr;
    int   prev= -1;

    if ('^' != *cclass) {
        memset(bits, 0, 32);
    } else {
        memset(bits, 255, 32);
        clear = true;
        ++cclass;
    }

    for ( ; (curr = *cclass++) ; ) {
        if ('-' == curr && *cclass && prev >= 0) {
            for (curr = *cclass++;  prev <= curr;  ++prev) set(prev);
            prev = -1;
            continue;
        }

        if ('\\' == curr && *cclass) {
            switch (curr = *cclass++) {
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

static char *textCharClass(unsigned char *bits)
{
    assert(0 != bits);

    static char string[256];

    int   inx = 0;
    char *ptr = string;

    for ( ; inx < 32;  ++inx) {
        ptr += sprintf(ptr, "\\%03o", bits[inx]);
    }

    return string;
}

static char *makeConstCString(PrsData data)
{
    const unsigned char *cclass = (const unsigned char *) convert(data);

    static char string[256];
    int         chr;
    char*       ptr = string;

    while ((chr = *cclass++)) {
        switch (chr) {
        case '\'':
        case '\"':
        case '\\':
            *ptr++ = '\\';
        default:
            *ptr++ = chr;
        }
    }
    *ptr = 0;
    return string;
}

static bool node_ComputeFirst(SynNode node)
{
    SynFirst first = 0;

    inline bool allocate(PrsFirstType type, bool set, unsigned count) {
        unsigned fullsize = sizeof(struct syn_first) + (sizeof(PrsData) * count);

        first = malloc(fullsize);

        if (!first) return false;

        memset(first, 0, fullsize);

        unsigned char *bitfield = 0;

        if (set) {
            bitfield = malloc(sizeof(unsigned char) * 32);
            if (!bitfield) {
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

    inline void invert() {
        assert(first);
        assert(first->bitfield);

        unsigned char *bits = first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] ^= 255;
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

    //----

    inline bool do_call() {
        if (!allocate(pft_variable, false, 1)) return false;
        first->name[0] = node.text->value;
        return true;
    }

    inline bool do_char() {
        if (!allocate(pft_fixed, true, 0)) return false;
        set(node.character->value);
        return true;
    }

    inline bool do_check() {
        if (!node_ComputeFirst(node.operator->value)) return false;
        node.any->first = node.operator->value.any->first;
        return true;
    }

    inline bool do_choice() {
        if (!node_ComputeFirst(node.tree->before))  return false;
        if (!node_ComputeFirst(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;
        SynFirst after  = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);
        PrsFirstType type  = (0 < total ? pft_variable :  pft_fixed);

        if (pft_opaque == before->type) type = pft_opaque;
        if (pft_opaque == after->type)  type = pft_opaque;
        if (pft_transparent == before->type) type = pft_transparent;
        if (pft_transparent == after->type)  type = pft_transparent;

        if (!allocate(type, bits, total)) return false;

        if (before->bitfield) merge(before->bitfield);
        if (after->bitfield)  merge(after->bitfield);

        concat(before, after);

        return true;
    }

    inline bool do_dot() {
        if (allocate(pft_fixed, true, 0)) return false;
        invert();
        return true;
    }

    inline bool do_not() {
         if (!node_ComputeFirst(node.operator->value)) return false;

         SynFirst child = node.operator->value.any->first;

         bool         bits = (0 != child->bitfield);
         PrsFirstType type = (pft_fixed == child->type ? pft_fixed : pft_opaque);

         if (!allocate(type, bits, 0)) return false;
         if (!bits) return true;

         merge(child->bitfield);
         invert();

         return true;
    }

    inline bool do_question() {
        if (!node_ComputeFirst(node.operator->value))  return false;
        if (!copy(node.operator->value.any->first)) return false;
        first->type = pft_transparent;
        return true;
    }

    inline bool do_rule() {
        if (!node_ComputeFirst(node.define->value))  return false;
        node.any->first = node.operator->value.any->first;
        return true;
    }

    inline bool do_sequence() {
        if (!node_ComputeFirst(node.tree->before))  return false;
        if (!node_ComputeFirst(node.tree->after))   return false;

        SynFirst before = node.tree->before.any->first;

        if (pft_fixed == before->type) {
            node.any->first = before;
            return true;
        }

        if (pft_variable == before->type) {
            node.any->first = before;
            return true;
        }

        if (pft_opaque == before->type) {
            node.any->first = before;
            return true;
        }

        SynFirst after = node.tree->after.any->first;

        unsigned     total = before->count + after->count;
        bool         bits  = (0 != before->bitfield) || (0 != after->bitfield);
        PrsFirstType type  = (0 < total ? pft_variable :  pft_fixed);

        switch (after->type) {
        case pft_opaque:
            type = pft_opaque;
            break;

        case pft_fixed:
            break;

        case pft_variable:
            type = pft_variable;
            break;

        case pft_transparent:
            type = pft_transparent;
            break;
        }

        if (!allocate(type, bits, total)) return false;

        if (before->bitfield) merge(before->bitfield);
        if (after->bitfield)  merge(after->bitfield);

        concat(before, after);

        return true;
    }

    inline bool do_set() {
        if (!allocate(pft_fixed, true, 0)) return false;
        merge(makeBitfield(node.text->value));
        return true;
    }

    inline bool do_string() {
        if (!allocate(pft_fixed, true, 0)) return false;
         set(node.text->value.start[0]);
        return true;
    }

    switch (node.any->type) {
    case syn_apply:     return allocate(pft_transparent, false, 0);
    case syn_begin:     return allocate(pft_transparent, false, 0);
    case syn_call:      return do_call();
    case syn_char:      return do_char();
    case syn_check:     return do_check();
    case syn_choice:    return do_choice();
    case syn_dot:       return do_dot();
    case syn_end:       return allocate(pft_transparent, false, 0);
    case syn_footer:    return allocate(pft_transparent, false, 0);
    case syn_header:    return allocate(pft_transparent, false, 0);
    case syn_include:   return allocate(pft_transparent, false, 0);
    case syn_not:       return do_not();
    case syn_plus:      return do_check();
    case syn_predicate: return allocate(pft_opaque, false, 0);
    case syn_question:  return do_question();
    case syn_rule:      return do_rule();
    case syn_sequence:  return do_sequence();
    case syn_set:       return do_set();
    case syn_star:      return do_question();
    case syn_string:    return do_string();
    case syn_thunk:     return allocate(pft_transparent, false, 0);
        /* */
    case syn_void:
        break;
    }

    return allocate(pft_opaque, false, 0);
}

static bool node_WriteFirstSet(SynNode node, FILE* output, const char **target)
{
    static char buffer[64];

    if (!node.any->first) {
        *target = "pft_opaque,0,0,0,";
        return true;
    }

    char *ptr = buffer;

    SynFirst first = node.any->first;

    ptr += sprintf(ptr, "%s, ", first2name(first->type));

    if (!first->bitfield) {
        ptr += sprintf(ptr, "%u, ", 0);
    } else {
        fprintf(output,
                "static struct prs_firstset first_%x_set  = { \"%s\" };\n",
                (unsigned) node.any,
                textCharClass(first->bitfield));

        ptr += sprintf(ptr, "&first_%x_set, ", (unsigned) node.any);
    }

    if (!first->count) {
         ptr += sprintf(ptr, "%u, ", 0);
    } else {
        fprintf(output,
                "static struct prs_firstlist first_%x_list  = { %u, ",
                (unsigned) node.any,
                first->count);

        unsigned index = 0;

        if (index < first->count) {
            fprintf(output, "{ \"%s\"", convert(first->name[index]));
            for (++index ; index < first->count ; ++index) {
                fprintf(output, ", \"%s\"", convert(first->name[index]));
            }
            fprintf(output, " }");
        }

        fprintf(output, " };\n");

        ptr += sprintf(ptr, "&first_%x_list, ", (unsigned) node.any);
    }

    ptr += sprintf(ptr, "%u, ", 0);

    *target = buffer;

    return true;
}

static bool node_WriteTree(SynNode node, FILE* output)
{
    if (!node.any) return false;

    const char *first_sets = "0,0,0,0";

    inline bool do_apply() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Apply, (union prs_arg) ((PrsName)\"%s\") };\n",
                (unsigned) node.any,
                first_sets,
                makeConstCString(node.text->value));
        return true;
    }

    inline bool do_begin() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Begin, };\n",
                (unsigned) node.any,
                first_sets);
        return true;
    }

    inline bool do_call() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchName, (union prs_arg) ((PrsName)\"%s\") };\n",
                (unsigned) node.any,
                first_sets,
                convert(node.text->value));
        return true;
    }

    inline bool do_char() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchChar, (union prs_arg) ((PrsChar)\'%s\') };\n",
                (unsigned) node.any,
                first_sets,
                makeConstCChar(node.character->value));
        return true;
    }

    inline bool do_check() {
        if (!node_WriteTree(node.operator->value, output)) return false;
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
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Choice, (union prs_arg) (&pair_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);
        return true;
    }

    inline bool do_dot() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchDot };\n",
                (unsigned) node.any,
                first_sets);
        return true;
    }

    inline bool do_end() {
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
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_AssertFalse, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);
        return true;
    }

    inline bool do_plus() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_OneOrMore, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);
        return true;
    }

    inline bool do_predicate() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Predicate, (union prs_arg) ((PrsName)\"%s\") };\n",
                (unsigned) node.any,
                first_sets,
                convert(node.text->value));
        return true;
    }

    inline bool do_question() {
        if (!node_WriteTree(node.operator->value, output)) return false;
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
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_Sequence, (union prs_arg) (&pair_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);
        return true;
    }

    inline bool do_set() {
        fprintf(output,
                "static struct prs_set   set_%x   = { \"%s\", \"%s\" };\n",
                (unsigned) node.any,
                makeConstCString(node.text->value),
                textCharClass(makeBitfield(node.text->value)));
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchSet, (union prs_arg) (&set_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.any);
        return true;
    }

    inline bool do_star() {
        if (!node_WriteTree(node.operator->value, output)) return false;
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_ZeroOrMore, (union prs_arg) (&node_%x) };\n",
                (unsigned) node.any,
                first_sets,
                (unsigned) node.operator->value.any);
        return true;
    }

    inline bool do_string() {
        fprintf(output,
                "static struct prs_node  node_%x  = { %s prs_MatchText, (union prs_arg) ((PrsName)\"%s\") };\n",
                (unsigned) node.any,
                first_sets,
                convert(node.text->value));
        return true;
    }

    inline bool do_thunk() {
        fprintf(output,
                "static struct prs_label label_%x = { (&event_%x), \"event_%x\" };\n",
                (unsigned) node.any,
                (unsigned) node.any,
                (unsigned) node.any);
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

    if (!node_WriteFirstSet(node, output, &first_sets)) return false;

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
        node_ComputeFirst(rule->value);
        node_WriteTree(rule->value, output);
        fprintf(output, "\n");
    }

    fprintf(output, "extern bool %s(PrsInput input) {\n", function);
    fprintf(output, "\n");
    fprintf(output, "    inline bool attach(PrsName name, PrsNode value) { return input->attach(input, name, value); }\n");
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

