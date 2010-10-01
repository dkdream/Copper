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

static bool text_Extend(struct cu_text *text, const unsigned room) {
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
                        CuCursor at,
                        CuLabel  label,
                        CuThread *target)
{
    struct cu_thread *result = malloc(sizeof(struct cu_thread));

    result->next  = 0;
    result->rule  = rule;
    result->at    = at;
    result->label = label;

    *target = result;

    return true;
}

static bool make_Queue(CuQueue *target) {
    struct cu_queue *result = malloc(sizeof(struct cu_queue));

    result->free_list = 0;
    result->begin     = 0;
    result->end       = 0;

    *target = result;

    return true;
}

static bool queue_Event(CuQueue    queue,
                        const char* rule,
                        CuCursor   at,
                        CuLabel    label)
{
    if (!queue) return true;

    struct cu_thread *result = queue->free_list;

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

static bool queue_Run(CuQueue queue,
                      Copper input)
{
    if (!queue)        return true;
    if (!queue->begin) return true;
    if (!input)        return false;

    CuThread current = queue->begin;

    queue->begin = 0;

    unsigned index = 0;

    for ( ; current ; ) {
        CuLabel label = current->label;

        input->context.rule = current->rule;

        if (!label.function(input, current->at)) {
            queue->begin = current;
            return false;
        }

        index += 1;

        CuThread next   = current->next;
        current->next    = queue->free_list;
        queue->free_list = current;
        current = next;
    }

    return true;
}

static bool queue_FreeList(CuQueue queue,
                           CuThread list)
{
    if (!list) return true;

    if (!queue) {
        for ( ; list ; ) {
            CuThread next = list->next;
            free(list);
            list = next;
        }
        return true;
    }

    for ( ; list ; ) {
        CuThread next = list->next;
        list->next = queue->free_list;
        queue->free_list = list;
        list = next;
    }

    return true;
}

static bool queue_TrimTo(CuQueue  queue,
                         CuThread to)
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

    CuThread current = queue->begin;

    for ( ; current ; current = current->next ) {
        if (current == to) break;

    }

    if (!current) return false;

    current = to->next;

    to->next   = 0;
    queue->end = to;

    return queue_FreeList(queue, current);

}

static bool queue_Clear(CuQueue queue) {
    if (!queue) return true;

    CuThread node = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    return queue_FreeList(queue, node);
}

static bool mark_begin(Copper input, CuCursor at) {
    if (!input) return false;
    input->context.begin = at;
    return true;
}

static struct cu_label begin_label = { &mark_begin, "set.begin" };

static bool mark_end(Copper input, CuCursor at) {
    if (!input) return false;
    input->context.end = at;
    return true;
}

static struct cu_label end_label = { &mark_end, "set.end" };

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

static bool make_Point(CuPoint *target)
{
    unsigned fullsize = sizeof(struct cu_point);

    struct cu_point *result = malloc(fullsize);
    memset(result, 0, fullsize);

    *target = result;

    return true;
}

static bool cache_MorePoints(CuCache value, unsigned count) {
    if (!value) return false;

    CuPoint list = value->free_list;

    for ( ; count ; --count) {
        CuPoint head;

        if (!make_Point(&head)) return false;

        head->next = list;
        value->free_list = head;
        list = head;
    }

    return true;
}


static bool make_Cache(unsigned size, CuCache *target) {
    unsigned fullsize = (sizeof(struct cu_cache) + (size * sizeof(CuPoint)));

    struct cu_cache *result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size = size;

    if (!cache_MorePoints(result, size)) return false;

    *target = result;

    return true;
}

static bool cache_Point(CuCache cache, CuNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    CuPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    if (!cache->free_list) {
        if (!cache_MorePoints(cache, cache->size)) return false;
    }

    CuPoint head = cache->free_list;

    head->node   = node;
    head->offset = offset;

    cache->free_list = head->next;

    head->next = cache->table[index];

    cache->table[index] = head;

    return true;
}

static bool cache_Find(CuCache cache, CuNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    CuPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node   != list->node)   continue;
        if (offset != list->offset) continue;
        return true;
    }

    return false;
}

static bool cache_Remove(CuCache cache, CuNode node, unsigned offset) {
    if (!cache) return false;

    unsigned code  = offset;
    unsigned index = code  % cache->size;
    CuPoint list  = cache->table[index];

    if (node   == list->node) {
        if (offset == list->offset) {
            cache->table[index] = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            return true;
        }
    }

    CuPoint prev = list;

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

static bool cache_Clear(CuCache cache) {
    if (!cache) return false;

    unsigned index   = 0;
    unsigned columns = cache->size;

    for ( ; index < columns ; ++index ) {
        CuPoint list = cache->table[index];
        cache->table[index] = 0;
        for ( ; list ; ) {
            CuPoint next = list->next;
            list->next = cache->free_list;
            cache->free_list = list;
            list = next;
        }
    }

    return true;
}


static bool copper_vm(const char* rulename,
                      CuNode start,
                      unsigned level,
                      Copper input)
{
    assert(0 != input);
    assert(0 != start);

    CuQueue queue = input->queue;
    assert(0 != queue);

    CuThread mark   = 0;
    CuCursor at;

    char buffer[10];

    inline const char* node_label(CuNode node) {
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

        return true;
    }

    inline bool current(CuChar *target) {
        unsigned point = input->cursor.text_inx;
        unsigned limit = input->data.limit;

        if (point >= limit) {
            if (!more()) return false;
        }

        *target = input->data.buffer[point];

        return true;
    }

    inline bool node(CuName name, CuNode* target) {
        if (!input->node(input, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }
        return true;
    }

    inline bool predicate(CuName name) {
        CuPredicate test = 0;
        if (!input->predicate(input, name, &test)) {
            CU_ERROR("predicate %s not found\n", name);
            return false;
        }
        return test(input);
    }

    inline bool set_cache(CuNode cnode) {
        return cache_Point(input->cache, cnode, at.text_inx);
    }

    inline bool check_cache(CuNode cnode) {
        if (cache_Find(input->cache, cnode, at.text_inx)) return true;
        if (cu_MatchName != cnode->oper) return false;
        const char *name = cnode->arg.name;
        CuNode value;
        if (!node(name, &value)) return false;
        if (!check_cache(value)) return false;
        set_cache(value);
        return true;
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

    inline bool add_event(CuLabel label) {
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

    inline bool checkFirstSet(CuNode cnode, bool *target) {
        if (!target) return false;

        if (cu_MatchName == cnode->oper) {
            const char *name = cnode->arg.name;
            CuNode value;
            if (!node(name, &value)) return false;
            if (!checkFirstSet(value, target)) return false;
            if (!*target) set_cache(value);
            return true;
        }

        if (pft_opaque == cnode->type) return false;
        if (pft_event  == cnode->type) return false;

        CuFirstSet  first = cnode->first;
        CuFirstList list  = cnode->start;

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

        CuChar chr = 0;
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
            cache_Point(input->cache, cnode, at.text_inx);
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

    inline bool checkMetadata(CuNode cnode, bool *target) {
        if (!target) return false;

        CuMetaFirst meta  = cnode->metadata;

        if (!meta) return false;

        if (pft_opaque == meta->type) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass opaque)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        if (pft_event == meta->type) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass event)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        if (cu_MatchName == cnode->oper) {
            const char *name = cnode->arg.name;
            CuNode value;
            if (!node(name, &value)) return false;
            if (!checkMetadata(value, target)) return false;
            if (!*target) set_cache(value);
            return true;
        }

        if (!meta->done) {
            indent(2); CU_DEBUG(1, "check (%s) using unfinish meta data\n",
                                node_label(cnode));
        }

        CuMetaSet first = meta->first;

        if (!first) {
            indent(4); CU_DEBUG(4, "check (%s) at (%u,%u) meta (bypass no first)\n",
                                node_label(cnode),
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        CuChar chr = 0;
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

        if (pft_transparent == meta->type) {
            *target = result = true;
        } else {
            *target = result = false;
            cache_Point(input->cache, cnode, at.text_inx);
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

    inline bool cu_and() {
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

    inline bool cu_choice() {
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

    inline bool cu_zero_plus() {
        while (copper_vm(rulename, start->arg.node, level, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool cu_one_plus() {
        bool result = false;
        while (copper_vm(rulename, start->arg.node, level, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool cu_optional() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            return true;
        }
        reset();
        return true;
    }

    inline bool cu_assert_true() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool cu_assert_false() {
        if (copper_vm(rulename, start->arg.node, level, input)) {
            reset();
            return false;
        }
        reset();
        return true;
    }

    inline bool cu_char() {
        CuChar chr = 0;
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

    inline bool cu_between() {
        CuChar chr = 0;
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

    inline bool cu_in() {
        CuChar chr = 0;

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

    inline bool cu_name() {
        const char *name = start->arg.name;
        CuNode     value;

        if (!node(name, &value)) return false;

        if (cache_Find(input->cache, value, at.text_inx)) {
            indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) (cached result failed\n",
                                name,
                                at.line_number + 1,
                                at.char_offset);
            return false;
        }

        bool result;

        if (checkFirstSet(value, &result)) {
            return result;
        }

        if (checkMetadata(value, &result)) {
            return result;
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

    inline bool cu_dot() {
        CuChar chr;
        if (!current(&chr)) {
            return false;
        }
        next();
        return true;
    }

    inline bool cu_begin() {
        return add_event(begin_label);
    }

    inline bool cu_end() {
        return add_event(end_label);
    }

    inline bool cu_predicate() {
        if (!predicate(start->arg.name)) {
            reset();
            return false;
        }
        return true;
    }

    inline bool cu_apply() {
        const char *name  = start->arg.name;
        CuLabel    label = { 0, name };
        CuEvent    event = 0;

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
    inline bool cu_thunk() {
        return add_event(*(start->arg.label));
    }

    inline bool cu_text() {
        const CuChar *text = (const CuChar *) start->arg.name;

        for ( ; 0 != *text ; ++text) {
            CuChar chr = 0;

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
        bool result;

        if (checkFirstSet(start, &result)) {
            return result;
        }

        if (checkMetadata(start, &result)) {
            return result;
        }

        switch (start->oper) {
        case cu_Apply:       return cu_apply();
        case cu_AssertFalse: return cu_assert_false();
        case cu_AssertTrue:  return cu_assert_true();
        case cu_Begin:       return cu_begin();
        case cu_Choice:      return cu_choice();
        case cu_End:         return cu_end();
        case cu_MatchChar:   return cu_char();
        case cu_MatchDot:    return cu_dot();
        case cu_MatchName:   return cu_name();
        case cu_MatchRange:  return cu_between();
        case cu_MatchSet:    return cu_in();
        case cu_MatchText:   return cu_text();
        case cu_OneOrMore:   return cu_one_plus();
        case cu_Predicate:   return cu_predicate();
        case cu_Sequence:    return cu_and();
        case cu_Thunk:       return cu_thunk();
        case cu_ZeroOrMore:  return cu_zero_plus();
        case cu_ZeroOrOne:   return cu_optional();
        case cu_Void:        return false;
        }
        return false;
    }

    hold();

    CU_ON_DEBUG(3, {
            indent(3); CU_DEBUG(3, "check (%s) %s",
                                node_label(start),
                                oper2name(start->oper));

            if (cu_MatchName == start->oper) {
                const char *name = start->arg.name;
                CU_DEBUG(3, " %s", name);
            }
            CU_DEBUG(3, " at (%u,%u)\n",
                     at.line_number + 1,
                     at.char_offset);
        });

    if (check_cache(start)) return false;

    bool result = run_node();

    if (!result) {
        set_cache(start);
    }

    return result;

    (void)cache_Remove;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool cu_InputInit(Copper input, unsigned cacheSize) {
    assert(0 != input);

    CU_DEBUG(3, "making queue\n");
    if (!make_Queue(&(input->queue))) return false;

    CuQueue queue = input->queue;
    assert(0 != queue);

    CU_DEBUG(3, "making cache\n");
    if (!make_Cache(cacheSize, &(input->cache))) return false;

    CuCache cache = input->cache;
    assert(0 != cache);
    assert(cacheSize == cache->size);

    CU_DEBUG(3, "InputInit done (queue %x) (cache %x)\n", (unsigned) queue, (unsigned) cache);

    return true;
}

extern bool cu_Parse(const char* name, Copper input) {
    assert(0 != input);

    CuNode start = 0;

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

extern bool cu_AppendData(Copper input,
                          const unsigned count,
                          const char *src)
{
    if (!input)    return false;
    if (1 > count) return true;
    if (!src)      return false;

    struct cu_text *data = &input->data;

    text_Extend(data, count);

    char *dest = data->buffer + data->limit;

    memcpy(dest, src, count);

    data->limit   += count;
    data->buffer[data->limit] = 0;

    return true;
}

extern bool cu_RunQueue(Copper input) {
    if (!input) return false;
    return queue_Run(input->queue, input);
}

extern bool cu_MarkedText(Copper input, CuData *target) {
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


