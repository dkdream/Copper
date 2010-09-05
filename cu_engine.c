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
#include "copper.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */

static bool text_Extend(struct prs_text *text, const unsigned room) {
    if (!text) return false;

    const unsigned point  = text->limit;
    const unsigned length = text->size;

    int space = length - point;

    if (space > room) return true;

    unsigned newsize = (0 < length) ? length : (room << 1) ;

    while ((newsize - point) < room) {
        newsize = newsize << 1;
    }

    const unsigned overflow = newsize + 2;

    if (text->buffer) {
        text->buffer = realloc(text->buffer, overflow);
        text->buffer[point] = 0;
    } else {
        text->buffer = malloc(overflow);
        memset(text->buffer, 0, overflow);
    }

    text->size = newsize;

    return true;
}

static bool make_Thread(const char* rule,
                        PrsCursor at,
                        PrsLabel  label,
                        PrsThread *target)
{
    struct prs_thread *result = malloc(sizeof(struct prs_thread));

    result->next  = 0;
    result->rule  = rule;
    result->at    = at;
    result->label = label;

    *target = result;

    return true;
}

static bool make_Queue(PrsQueue *target) {
    struct prs_queue *result = malloc(sizeof(struct prs_queue));

    result->free_list = 0;
    result->begin     = 0;
    result->end       = 0;

    *target = result;

    return true;
}

static bool queue_Event(PrsQueue    queue,
                        const char* rule,
                        PrsCursor   at,
                        PrsLabel    label)
{
    if (!queue) return true;

    struct prs_thread *result = queue->free_list;

    if (!result) {
        make_Thread(rule, at, label, &result);
    } else {
        queue->free_list = result->next;
        result->next     = 0;
        result->rule     = rule;
        result->at       = at;
        result->label    = label;
    }

    if (!queue->begin) {
        queue->begin = queue->end = result;
        return true;
    }

    queue->end->next = result;
    queue->end = result;

    return true;
}

static bool queue_Run(PrsQueue queue,
                      PrsInput input)
{
    if (!queue)        return true;
    if (!queue->begin) return true;
    if (!input)        return false;

    PrsThread current = queue->begin;

    queue->begin = 0;

    unsigned index = 0;

    for ( ; current ; ) {
        PrsLabel label = current->label;

        input->context.rule = current->rule;

        if (!label.function(input, current->at)) {
            queue->begin = current;
            return false;
        }

        index += 1;

        PrsThread next   = current->next;
        current->next    = queue->free_list;
        queue->free_list = current;
        current = next;
    }

    return true;
}

static bool queue_FreeList(PrsQueue queue,
                           PrsThread list)
{
    if (!list) return true;

    if (!queue) {
        for ( ; list ; ) {
            PrsThread next = list->next;
            free(list);
            list = next;
        }
        return true;
    }

    for ( ; list ; ) {
        PrsThread next = list->next;
        list->next = queue->free_list;
        queue->free_list = list;
        list = next;
    }

    return true;
}

static bool queue_TrimTo(PrsQueue  queue,
                         PrsThread to)
{
    if (!queue) return false;
    if (!to) {
        to = queue->begin;
        queue->begin = 0;
        queue->end   = 0;
        return  queue_FreeList(queue, to);
    }

    if (!queue->begin) return false;

    if (to == queue->end) return true;

    PrsThread current = queue->begin;

    for ( ; current ; current = current->next ) {
        if (current == to) break;

    }

    if (!current) return false;

    current = to->next;

    to->next   = 0;
    queue->end = to;

    return queue_FreeList(queue, current);

}

static bool queue_Clear(PrsQueue queue) {
    if (!queue) return true;

    PrsThread node = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    return queue_FreeList(queue, node);
}

static bool mark_begin(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->context.begin = at;
    return true;
}

static struct prs_label begin_label = { &mark_begin, "set.begin" };

static bool mark_end(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->context.end = at;
    return true;
}

static struct prs_label end_label = { &mark_end, "set.end" };

/* this is use only in copper_vm for debugging (SINGLE THEADED ALERT) */
static char *char2string(unsigned char value)
{
    static unsigned index = 0;
    static char buffer[60];
    char *text = buffer + (index * 6);

    // allow up to 10 call before reusing the same buffer section
    index = (index + 1) % 10;

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
        if (value < 32) {
            int len = sprintf(text, "\\%03o", value);
            text[len] = 0;
            return text;
        }
        if (126 < value) {
            int len = sprintf(text, "\\%03o", value);
            text[len] = 0;
            return text;
        }
        break;
    }

    text[0] = (char) value;
    text[1] = 0;
    return text;
}

static bool make_Point(PrsPoint *target)
{
    unsigned fullsize = sizeof(struct prs_point);

    struct prs_point *result = malloc(fullsize);
    memset(result, 0, fullsize);

    *target = result;

    return true;
}

static bool cache_MorePoints(PrsCache value, unsigned count) {
    if (!value) return false;

    PrsPoint list = value->free_list;

    for ( ; count ; --count) {
        PrsPoint head;

        if (!make_Point(&head)) return false;

        head->next = list;
        value->free_list = head;
        list = head;
    }

    return true;
}


static bool make_Cache(unsigned size, PrsCache *target) {
    unsigned fullsize = (sizeof(struct prs_cache) + (size * sizeof(PrsPoint)));

    struct prs_cache *result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size = size;

    if (!cache_MorePoints(result, size)) return false;

    *target = result;

    return true;
}

static bool cache_Point(PrsCache cache, PrsNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    if (!cache->free_list) {
        if (!cache_MorePoints(cache, cache->size)) return false;
    }

    PrsPoint head = cache->free_list;

    head->node   = node;
    head->offset = offset;

    cache->free_list = head->next;

    head->next = cache->table[index];

    cache->table[index] = head;

    return true;
}

static bool cache_Find(PrsCache cache, PrsNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    return false;
}

static bool cache_Remove(PrsCache cache, PrsNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    if (node   == list->node) {
        if (offset == list->offset) {
            cache->table[index] = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            return true;
        }
    }

    PrsPoint prev = list;

    for ( ; list->next ; ) {
        prev = list;
        list = list->next;
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;

        prev->next = list->next;
        list->next = cache->free_list;
        cache->free_list = list;

        return true;
    }

    return true;
}

static bool cache_Clear(PrsCache cache) {
    if (!cache) return false;

    unsigned index   = 0;
    unsigned columns = cache->size;

    for ( ; index < columns ; ++index ) {
        PrsPoint list = cache->table[index];
        cache->table[index] = 0;
        for ( ; list ; ) {
            PrsPoint next = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            list = next;
        }
    }

    return true;
}


static bool copper_vm(const char* rulename,
                      PrsNode start,
                      unsigned level,
                      PrsInput input)
{
    assert(0 != input);
    assert(0 != start);

    PrsQueue queue = input->queue;
    assert(0 != queue);

    PrsThread mark   = 0;
    PrsCursor at;

    char buffer[10];

    inline const char* node_label(PrsNode node) {
        if (node->label) return node->label;
        sprintf(buffer, "%x__", (unsigned) node);
        return buffer;
    }

    inline void hold() {
        mark = queue->end;
        at   = input->cursor;
        if (at.text_inx > input->reach.text_inx) {
            input->reach = at;
        }
    }

    inline bool reset()   {
        if (!mark) {
            if (!queue_Clear(queue)) return false;
        } else {
            if (!queue_TrimTo(queue, mark)) return false;
        }
        input->cursor = at;
        return true;
    }

    inline bool more() {
        return input->more(input);
    }

    inline bool next() {
        unsigned point = input->cursor.text_inx;

        if ('\n' == input->data.buffer[point]) {
            input->cursor.line_number += 1;
            input->cursor.char_offset  = 0;
        } else {
            input->cursor.char_offset += 1;
        }

        input->cursor.text_inx += 1;

        //unsigned limit = input->data.limit;

        return true;
    }

    inline bool current(PrsChar *target) {
        unsigned point = input->cursor.text_inx;
        unsigned limit = input->data.limit;

        if (point >= limit) {
            if (!more()) return false;
        }

        *target = input->data.buffer[point];

        return true;
    }

    inline bool node(PrsName name, PrsNode* target) {
        if (!input->node(input, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }
        return true;
    }

    inline bool predicate(PrsName name) {
        PrsPredicate test = 0;
        if (!input->predicate(input, name, &test)) {
            CU_ERROR("predicate %s not found\n", name);
            return false;
        }
        return test(input);
    }

    inline void indent(unsigned debug) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = level;
                        for ( ; inx ; --inx) {
                            CU_DEBUG(debug, " |");
                        }
                    });
    }

    inline void debug_charclass(unsigned debug, const unsigned char *bits) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = 0;
                        for ( ; inx < 32;  ++inx) {
                            CU_DEBUG(debug, "\\%03o", bits[inx]);
                        }
                    });
    }

    inline bool add_event(PrsLabel label) {
        indent(2); CU_DEBUG(2, "event %s(%x) at (%u,%u)\n",
                           label.name,
                           (unsigned) label.function,
                           at.line_number + 1,
                           at.char_offset);

        return queue_Event(queue,
                           rulename,
                           at,
                           label);
    }

    inline bool checkFirstSet(PrsNode cnode, bool *target) {
        if (pft_opaque == cnode->type) return false;

        PrsFirstSet  first = cnode->first;
        PrsFirstList list  = cnode->start;

        if (!first) return false;

        if (list) {
            if (0 < list->count) {
                indent(2); CU_DEBUG(2, "check (%s) at (%u,%u) first (bypass call nodes)\n",
                                    node_label(cnode),
                                    at.line_number + 1,
                                    at.char_offset);
                return false;
            }
        }

        PrsChar chr = 0;
        if (!current(&chr)) return false;

        unsigned             binx   = chr;
        const unsigned char *bits   = first->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) first %s",
                                node_label(cnode),
                                char2string(chr),
                                at.line_number + 1,
                                at.char_offset,
                                "continue");
            CU_DEBUG(4, " (");
            debug_charclass(4, bits);
            CU_DEBUG(4, ")\n");
            return false;
        }

        bool result;

        if (pft_transparent == cnode->type) {
            *target = result = true;
        } else {
            *target = result = false;
        }

        indent(2); CU_DEBUG(1, "check (%s) to cursor(\'%s\') at (%u,%u) first %s result %s",
                            node_label(cnode),
                            char2string(chr),
                            at.line_number + 1,
                            at.char_offset,
                            "skip",
                            (result ? "passed" : "failed"));
        CU_DEBUG(4, " (");
        debug_charclass(4, bits);
        CU_DEBUG(4, ")");
        CU_DEBUG(1, "\n");
        return true;
    }

    inline bool checkMetadata(PrsNode cnode, bool *target) {
        if (pft_opaque == cnode->type) return false;

        PrsMetaFirst meta  = cnode->metadata;

        if (!meta) return false;

        if (!meta->done) {
            indent(2); CU_DEBUG(1, "check (%s) using unfinish meta data\n",
                                node_label(cnode));
        }

        PrsMetaSet first = meta->first;

        if (!first) return false;

        PrsChar chr = 0;
        if (!current(&chr)) return false;

        unsigned             binx = chr;
        const unsigned char *bits = first->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) meta %s (",
                                node_label(cnode),
                                char2string(chr),
                                at.line_number + 1,
                                at.char_offset,
                                "continue");
            debug_charclass(4, bits);
            CU_DEBUG(4, ")\n");
            return false;
        }


        bool result;

        if (pft_transparent == cnode->type) {
            *target = result = true;
        } else {
            *target = result = false;
        }

        indent(2); CU_DEBUG(1, "check (%s) to cursor(\'%s\') at (%u,%u) %s meta %s result %s",
                            node_label(cnode),
                            char2string(chr),
                            at.line_number + 1,
                            at.char_offset,
                            oper2name(cnode->oper),
                            "skip",
                            (result ? "passed" : "failed"));
        CU_DEBUG(4, " (");
        debug_charclass(4, bits);
        CU_DEBUG(4, ")");
        CU_DEBUG(1, "\n");

        return true;
    }

    inline bool prs_and() {
        if (!copper_vm(rulename, start->arg.pair->left, level, input))  {
            reset();
            return false;
        }
        if (!copper_vm(rulename, start->arg.pair->right, level, input)) {
            reset();
            return false;
        }

        return true;
    }

    inline bool prs_choice() {
        if (copper_vm(rulename, start->arg.pair->left, level, input)) {
            return true;
        }
        reset();
        if (copper_vm(rulename, start->arg.pair->right, level, input)) {
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_zero_plus() {
        while (copper_vm(rulename, start->arg.node, level, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        while (copper_vm(rulename, start->arg.node, level, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool prs_optional() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            return true;
        }
        reset();
        return true;
    }

    inline bool prs_assert_true() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_assert_false() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return false;
        }
        reset();
        return true;
    }

    inline bool prs_char() {
        PrsChar chr = 0;
        if (!current(&chr)) {
            indent(2); CU_DEBUG(2, "match(\'%s\') to end-of-file at (%u,%u)\n",
                               char2string(start->arg.letter),
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "match(\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                           char2string(start->arg.letter),
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

        if (chr != start->arg.letter) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_between() {
        PrsChar chr = 0;
        if (!current(&chr)) {
            indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to end-of-file at (%u,%u)\n",
                               char2string(start->arg.range->begin),
                               char2string(start->arg.range->end),
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                           char2string(start->arg.range->begin),
                           char2string(start->arg.range->end),
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

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
            indent(2); CU_DEBUG(2, "set(%s) to end-of-file at (%u,%u)\n",
                               start->arg.set->label,
                               at.line_number + 1,
                               at.char_offset);
            return false;
        }

        indent(2); CU_DEBUG(2, "set(%s) to cursor(\'%s\') at (%u,%u)\n",
                           start->arg.set->label,
                           char2string(chr),
                           at.line_number + 1,
                           at.char_offset);

        unsigned             binx = chr;
        const unsigned char *bits = start->arg.set->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            next();
            return true;
        }

        return false;
    }

    inline bool prs_name() {
        const char *name = start->arg.name;
        PrsNode     value;

        if (!node(name, &value)) return false;

        bool result;

        if (checkFirstSet(value, &result)) {
            return result;
        }

        if (checkMetadata(value, &result)) {
            return result;
        }

        if (cache_Find(input->cache, value, at.text_inx)) {
            indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) (cached result failed\n",
                                name,
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }


        indent(2); CU_DEBUG(2, "rule \"%s\" at (%u,%u)\n",
                           name,
                           at.line_number + 1,
                           at.char_offset);

        // note the same name maybe call from two or more uncached nodes
        result = copper_vm(name, value, level+1, input);

        indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) to (%u,%u) result %s\n",
                            name,
                            at.line_number + 1,
                            at.char_offset,
                            input->cursor.line_number + 1,
                            input->cursor.char_offset,
                            (result ? "passed" : "failed"));

        return result;
    }

    inline bool prs_dot() {
        PrsChar chr;
        if (!current(&chr)) {
            return false;
        }
        next();
        return true;
    }

    inline bool prs_begin() {
        return add_event(begin_label);
    }

    inline bool prs_end() {
        return add_event(end_label);
    }

    inline bool prs_predicate() {
        if (!predicate(start->arg.name)) {
            reset();
            return false;
        }
        return true;
    }

    inline bool prs_apply() {
        const char *name  = start->arg.name;
        PrsLabel    label = { 0, name };
        PrsEvent    event = 0;

        indent(2); CU_DEBUG(2, "fetching event %s at (%u,%u)\n",
                           name,
                           at.line_number + 1,
                           at.char_offset);

        if (!input->event(input, name, &event)) {
            CU_ERROR("event %s not found\n", name);
            return false;
        }

        label.function = event;

        add_event(label);

        return true;
    }
    inline bool prs_thunk() {
        return add_event(*(start->arg.label));
    }

    inline bool prs_text() {
        const PrsChar *text = (const PrsChar *) start->arg.name;

        for ( ; 0 != *text ; ++text) {
            PrsChar chr = 0;

            if (!current(&chr)) {
                indent(2); CU_DEBUG(2, "match(\"%s\") to end-of-file at (%u,%u)\n",
                                   char2string(*text),
                                   at.line_number + 1,
                                   at.char_offset);
                reset();
                return false;
            }

            indent(2); CU_DEBUG(2, "match(\"%s\") to cursor(\'%s\') at (%u,%u)\n",
                               char2string(*text),
                               char2string(chr),
                               at.line_number + 1,
                               at.char_offset);

            if (chr != *text) {
                reset();
                return false;
            }

            next();
        }

        return true;
    }

    inline bool run_node() {
        hold();
        switch (start->oper) {
        case prs_Apply:       return prs_apply();
        case prs_AssertFalse: return prs_assert_false();
        case prs_AssertTrue:  return prs_assert_true();
        case prs_Begin:       return prs_begin();
        case prs_Choice:      return prs_choice();
        case prs_End:         return prs_end();
        case prs_MatchChar:   return prs_char();
        case prs_MatchDot:    return prs_dot();
        case prs_MatchName:   return prs_name();
        case prs_MatchRange:  return prs_between();
        case prs_MatchSet:    return prs_in();
        case prs_MatchText:   return prs_text();
        case prs_OneOrMore:   return prs_one_plus();
        case prs_Predicate:   return prs_predicate();
        case prs_Sequence:    return prs_and();
        case prs_Thunk:       return prs_thunk();
        case prs_ZeroOrMore:  return prs_zero_plus();
        case prs_ZeroOrOne:   return prs_optional();
        case prs_Void:        return false;
        }
        return false;
    }

    bool result;

    hold();

    indent(4); CU_DEBUG(4, "check (%s) %s\n", node_label(start), oper2name(start->oper));

    if (checkFirstSet(start, &result)) {
        return result;
    }

    if (checkMetadata(start, &result)) {
        return result;
    }

    if (cache_Find(input->cache, start, at.text_inx)) return false;

    result = run_node();

    if (!result) {
        cache_Point(input->cache, start, at.text_inx);
    }

    return result;

    (void)cache_Remove;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool cu_InputInit(PrsInput input, unsigned cacheSize) {
    assert(0 != input);

    CU_DEBUG(3, "making queue\n");
    if (!make_Queue(&(input->queue))) return false;

    PrsQueue queue = input->queue;
    assert(0 != queue);

    CU_DEBUG(3, "making cache\n");
    if (!make_Cache(cacheSize, &(input->cache))) return false;

    PrsCache cache = input->cache;
    assert(0 != cache);
    assert(cacheSize == cache->size);

    CU_DEBUG(3, "InputInit done (queue %x) (cache %x)\n", (unsigned) queue, (unsigned) cache);

    return true;
}

extern bool cu_Parse(const char* name, PrsInput input) {
    assert(0 != input);

    PrsNode start = 0;

    CU_DEBUG(3, "requesting start node %s\n", name);
    if (!input->node(input, name, &start)) return false;

    assert(0 != input->queue);
    CU_DEBUG(3, "clearing queue %x\n", (unsigned) input->queue);
    if (!queue_Clear(input->queue)) return false;

    assert(0 != input->cache);
    CU_DEBUG(3, "clearing cache %x\n", (unsigned) input->cache);
    if (!cache_Clear(input->cache)) return false;

    CU_DEBUG(3, "starting parce\n");

    CU_DEBUG(2, "rule \"%s\" begin\n", name);

    bool result = copper_vm(name, start, 1, input);

    CU_DEBUG(2, "rule \"%s\" result %s\n",
             name, (result ? "passed" : "failed"));

    return result;
}

extern bool cu_AppendData(PrsInput input,
                          const unsigned count,
                          const char *src)
{
    if (!input)    return false;
    if (1 > count) return true;
    if (!src)      return false;

    struct prs_text *data = &input->data;

    text_Extend(data, count);

    char *dest = data->buffer + data->limit;

    memcpy(dest, src, count);

    data->limit   += count;
    data->buffer[data->limit] = 0;

    return true;
}

extern bool cu_RunQueue(PrsInput input) {
    if (!input) return false;
    return queue_Run(input->queue, input);
}

extern bool cu_MarkedText(PrsInput input, PrsData *target) {
    if (!input)  return false;
    if (!target) return false;

    unsigned text_begin = input->context.begin.text_inx;
    unsigned text_end   = input->context.end.text_inx;

    if (text_begin > text_end) return false;

    if (text_begin == text_end) {
        target->length = 0;
        target->start  = 0;
        return true;
    }

    if (text_begin > input->data.limit) return false;
    if (text_end   > input->data.limit) return false;

    target->length = text_end - text_begin;
    target->start  = input->data.buffer + text_begin;

    return true;
}

unsigned cu_global_debug = 0;

extern void cu_debug(const char *filename,
                     unsigned int linenum,
                     const char *format,
                     ...)
{
    va_list ap; va_start (ap, format);

    //    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    fflush(stdout);
}

extern void cu_error(const char *filename,
                     unsigned int linenum,
                     const char *format,
                     ...)
{
    va_list ap; va_start (ap, format);

    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    exit(1);
}

extern void cu_error_part(const char *format, ...)
{
    va_list ap; va_start (ap, format);
    vprintf(format, ap);
}


