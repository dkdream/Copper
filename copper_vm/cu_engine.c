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

static bool make_Point(PrsNode  node,
                       PrsState cursor,
                       bool     match,
                       PrsSlice segment,
                       PrsPoint next,
                       PrsPoint *target)
{
    struct prs_point *result = malloc(sizeof(struct prs_point));

    result->next    = next;
    result->node    = node;
    result->cursor  = cursor;
    result->match   = match;
    result->segment = segment;
    result->segment = segment;

    *target = result;

    return true;
}

static bool make_Thread(PrsCursor at,
                        PrsLabel  label,
                        PrsThread *target)
{
    struct prs_thread *result = malloc(sizeof(struct prs_thread));

    result->next  = 0;
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

static bool make_Cache(unsigned size, PrsCache *target) {
    unsigned fullsize = (sizeof(struct prs_cache) + (size * sizeof(PrsPoint)));

    struct prs_cache *result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size = size;

    *target = result;

    return true;
}

static void queue_Check(PrsQueue  queue,
                        const char *filename,
                        unsigned int linenumber)
{
    if (!queue) return;

    if (!queue->begin) {
        if (!queue->end) return;
        CU_ERROR_AT(filename,  linenumber,
                    "invalid queue depth 0 end %x",
                    (unsigned) queue->end);
    }

    unsigned depth = 0;

    PrsThread current = queue->begin;
    PrsThread end     = queue->end;

    bool found = false;

    for ( ; current ; current = current->next) {
        if (end == current) found = true;
        depth += 1;
    }

    if (!found) {
        CU_ERROR_AT(filename,  linenumber,
                    "invalid queue depth %u end %x",
                    depth, (unsigned) queue->end);
    }
}

static unsigned queue_Count(PrsQueue  queue) {
    if (!queue)        return 0;
    if (!queue->begin) return 0;

    unsigned result = 0;

    PrsThread current = queue->begin;
    PrsThread end     = queue->end;

    bool found = false;

    for ( ; current ; current = current->next) {
        if (end == current) found = true;
        result += 1;
    }

    return result;
}

static bool queue_Event(PrsQueue  queue,
                        PrsCursor at,
                        PrsLabel  label)
{
    if (!queue) return true;

    struct prs_thread *result = queue->free_list;

    if (!result) {
        make_Thread(at, label, &result);
    } else {
        queue->free_list = result->next;
        result->next     = 0;
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

static bool queue_CloneEvent(PrsQueue   queue,
                             PrsThread  value,
                             PrsThread *target)
{
    if (!queue) return false;
    if (!value) return false;

    struct prs_thread *result = queue->free_list;

    if (!result) {
        return make_Thread(value->at, value->label, target);
    }

    queue->free_list = result->next;

    result->next  = 0;
    result->at    = value->at;
    result->label = value->label;

    *target = result;

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

// (begin-end] or [begin->next-end]
// start after begin upto and including end
// the slice = { clone(begin->next), clone(end) }
static bool queue_Slice(PrsQueue   queue,
                        PrsThread  begin,
                        PrsThread  end,
                        PrsSlice  *slice)
{
    if (!slice) return false;
    if (!begin) {
        return (0 == end);
    }

    if (slice->begin) {
        queue_FreeList(queue, slice->begin);
    }

    if (begin == end) {
        slice->begin = 0;
        slice->end   = 0;
        return true;
    }

    PrsThread last;
    PrsThread node = begin->next;

    queue_CloneEvent(queue, node, &last);

    slice->begin = last;

    for ( ; node ; ) {
        if (node == end) {
            slice->end = last;
            return true;
        }
        node = node->next;
        queue_CloneEvent(queue, node, &last->next);
        last = last->next;
    }

    queue_FreeList(queue, slice->begin);

    slice->begin = 0;
    slice->end   = 0;

    return false;
}

static bool queue_CloneSlice(PrsQueue  queue,
                             PrsSlice  segment,
                             PrsSlice *slice)
{
    if (!queue)         return true;
    if (!segment.begin) return true;
    if (!segment.end)   return false;

    if (slice->begin) {
        queue_FreeList(queue, slice->begin);
    }

    PrsThread last;
    PrsThread node = segment.begin;

    queue_CloneEvent(queue, node, &last);

    slice->begin = last;

    for ( ; node ; ) {
        if (node == segment.end) {
            slice->end = last;
            return true;
        }
        node = node->next;
        queue_CloneEvent(queue, node, &last->next);
        last = last->next;
    }

    queue_FreeList(queue, slice->begin);

    slice->begin = 0;
    slice->end   = 0;

    return false;
}

static bool queue_AppendSlice(PrsQueue queue,
                              PrsSlice segment)
{
    if (!queue)         return true;
    if (!segment.begin) return true;
    if (!segment.end)   return false;

    PrsSlice slice = { 0, 0 };

    if (!queue_CloneSlice(queue, segment, &slice)) return false;

    queue->end->next = slice.begin;
    queue->end       = slice.end;

    return true;
}

static bool queue_Clear(PrsQueue queue) {
    if (!queue) return true;

    PrsThread node = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    return queue_FreeList(queue, node);
}

#if 0
static bool queue_Free(PrsQueue *target) {
    if (!target) return true;

    PrsQueue queue = *target;

    if (!queue_Clear(queue)) return false;

    PrsThread free_list = queue->free_list;

    queue->free_list = 0;

    for ( ; free_list ; ) {
        PrsThread next = free_list->next;
        free(free_list);
        free_list = next;
    }

    free(queue);

    *target = 0;

    return true;
}
#endif

static bool cache_Point(PrsCache cache,
                        PrsNode  node,
                        PrsState cursor,
                        bool     match,
                        PrsSlice segment,
                        PrsPoint next,
                        PrsPoint *target)
{
    if (!cache) {
        return make_Point(node, cursor, match, segment, next, target);
    }

    struct prs_point *result = cache->free_list;

    if (!result) {
        return make_Point(node, cursor, match, segment, next, target);
    }

    cache->free_list = result->next;

    result->next    = next;
    result->node    = node;
    result->cursor  = cursor;
    result->match   = match;
    result->segment = segment;

    *target = result;

     return true;
}

static bool cache_Find(PrsCache  cache,
                       PrsNode   node,
                       PrsCursor cursor,
                       PrsPoint *target)
{
    assert(0 != cache);
    assert(0 < cache->size);

    unsigned code  = (unsigned)node;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node != list->node) continue;
        if (cursor.text_inx != list->cursor.begin.text_inx) continue;
        *target = list;
        return true;
    }

    return false;
}

static bool input_CacheInsert(PrsInput  input,
                              unsigned  depth,
                              PrsNode   node,
                              PrsState  cursor,
                              bool      match,
                              PrsThread begin,
                              PrsThread end)
{
    assert(0 != input);

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    assert(0 != cache);
    assert(0 != queue);
    assert(0 < cache->size);

    PrsSlice segment = { 0, 0 };

    if (match) {
        if (!queue_Slice(queue, begin, end, &segment)) return false;
        unsigned check = queue_Count(input->queue);

        if (depth > check) CU_ERROR("invalid change in queue %u -> %u",
                                    depth, check);
    } else {
        if (!queue_TrimTo(input->queue, begin)) return false;

        unsigned check = queue_Count(input->queue);

        if (depth > check) CU_ERROR("invalid change in queue %u -> %u  %x",
                                    depth, check,
                                    (unsigned) begin);

    }


    unsigned code  = (unsigned)node;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node != list->node) continue;
        if (cursor.begin.text_inx != list->cursor.begin.text_inx) continue;

        // found
        // free the old segment
        queue_FreeList(queue, list->segment.begin);

        list->cursor.end = cursor.end;
        list->match      = match;
        list->segment    = segment;
        return true;
    }

    if (!cache_Point(cache,
                     node,
                     cursor,
                     match,
                     segment,
                     cache->table[index],
                     &list))
        return false;

    cache->table[index] = list;

    return true;
}

static bool input_CacheClear(PrsInput input) {
    assert(0 != input);

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    assert(0 != cache);
    assert(0 != queue);
    assert(0 < cache->size);

    unsigned  size  = cache->size;
    PrsPoint *table = cache->table;

    for ( ; size-- ; ) {
        PrsPoint value = table[size];
        table[size] = 0;
        for ( ; value ; ) {
            PrsPoint next = value->next;
            queue_FreeList(queue, value->segment.begin);

            value->node          = 0;
            value->segment.begin = 0;
            value->segment.end   = 0;
            value->next          = cache->free_list;
            cache->free_list     = value;

            value = next;
        }
    }

    return true;
}

#if 0
static bool input_CacheFree(PrsInput input) {
    if (!input) return true;

    PrsCache cache = input->cache;

    if (!cache) return true;

    if (!input_CacheClear(input)) return false;

    input->cache = 0;

    PrsPoint free_list = cache->free_list;

    cache->free_list = 0;

    for ( ; free_list ; ) {
        PrsPoint next = free_list->next;
        free(free_list);
        free_list = next;
    }

    free(cache);

    return true;
}
#endif

static bool mark_begin(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->slice.begin = at;
    return true;
}

static struct prs_label begin_label = { &mark_begin, "set.begin" };

static bool mark_end(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->slice.end = at;
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

static bool copper_vm(PrsNode start, unsigned level, PrsInput input) {
    assert(0 != input);
    assert(0 != start);

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    assert(0 != cache);
    assert(0 != queue);
    assert(0 < cache->size);

    PrsThread begin  = 0;
    PrsThread mark   = 0;
    bool      match  = false;
    PrsState  cursor;
    PrsCursor at;
    unsigned  depth;

    inline bool hold() {
        mark = queue->end;
        at   = input->cursor;
        return true;
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
        input->cursor.text_inx    += 1;
        input->cursor.char_offset += 1;

        unsigned point = input->cursor.text_inx;
        unsigned limit = input->data.limit;

        if (point >= limit) {
            if (!more()) return false;
        }

        if ('\n' == input->data.buffer[point]) {
            input->cursor.line_number += 1;
            input->cursor.char_offset  = 0;
        }

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
        return input->node(input, name, target);
    }

    inline bool predicate(PrsName name) {
        PrsPredicate test = 0;
        if (input->predicate(input, name, &test)) {
            return test(input);
        }
        return false;
    }

    inline void indent() {
        CU_ON_DEBUG(2,
                    { unsigned inx = level;
                        for ( ; inx ; --inx) {
                            CU_DEBUG(2, " |");
                        }
                    });
    }

    inline bool add_event(PrsLabel label) {
        indent(); CU_DEBUG(2, "event %s(%x) at (%u,%u)\n",
                           label.name,
                           (unsigned) label.function,
                           at.line_number + 1,
                           at.char_offset);

        return queue_Event(queue,
                           at,
                           label);
    }

    inline bool cache_begin() {
        if (!hold()) return false;

        indent(); CU_DEBUG(2, "%s (%x) at (%u,%u)\n",
                           oper2name(start->oper),
                           (unsigned) start,
                           at.line_number + 1,
                           at.char_offset);

        depth        = queue_Count(queue);
        begin        = mark;
        cursor.begin = at;

        return true;;
    }

    inline void queue_check(const char *filename, unsigned int linenumber) {
        unsigned check = queue_Count(queue);

        if (depth > check)  CU_ERROR_AT(filename, linenumber,
                                        "invalid change in queue %u -> %u",
                                        depth, check);

        queue_Check(queue, filename, linenumber);
    }

    inline bool cache_check() {
        PrsPoint point = 0;
        if (!cache_Find(cache, start, cursor.begin, &point)) return false;

        match = point->match;

        if (match) {
            input->cursor = point->cursor.end;
            if (!queue_AppendSlice(queue, point->segment)) return false;

            indent(); CU_DEBUG(2, "%s (%x) at (%u,%u) to (%u,%u) using cache result %s\n",
                               oper2name(start->oper),
                               (unsigned) start,
                               at.line_number + 1,
                               at.char_offset,
                               point->cursor.end.line_number + 1,
                               point->cursor.end.char_offset,
                               "passed");
        } else {
            indent(); CU_DEBUG(2, "%s (%x) at (%u,%u) using cache result %s\n",
                               oper2name(start->oper),
                               (unsigned) start,
                               at.line_number + 1,
                               at.char_offset,
                               "failed");
        }

        return true;
    }

    inline bool cache_end() {
        cursor.end = input->cursor;

        if (match) {
            indent(); CU_DEBUG(2, "%s (%x) at (%u,%u) to (%u,%u) result %s\n",
                               oper2name(start->oper),
                               (unsigned) start,
                               cursor.begin.line_number + 1,
                               cursor.begin.char_offset,
                               cursor.end.line_number + 1,
                               cursor.end.char_offset,
                               "passed");
        } else {
            indent(); CU_DEBUG(2, "%s (%x) at (%u,%u) result %s\n",
                               oper2name(start->oper),
                               (unsigned) start,
                               cursor.begin.line_number + 1,
                               cursor.begin.char_offset,
                               "failed");
        }

       return input_CacheInsert(input,
                                depth,
                                start,
                                cursor,
                                match,
                                begin,
                                queue->end);
    }

    inline bool prs_and() {
        if (!copper_vm(start->arg.pair->left, level+1, input))  {
            reset();
            return false;
        }
        if (!copper_vm(start->arg.pair->right, level+1, input)) {
            reset();
            return false;
        }

        return true;
    }

    inline bool prs_choice() {
        if (copper_vm(start->arg.pair->left, level+1, input)) {
            return true;
        }
        reset();
        if (copper_vm(start->arg.pair->right, level+1, input)) {
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_zero_plus() {
        while (copper_vm(start->arg.node, level+1, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        while (copper_vm(start->arg.node, level+1, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool prs_optional() {
        if (copper_vm(start->arg.node, level+1, input)) {
            return true;
        }
        reset();
        return true;
    }

    inline bool prs_assert_true() {
        if (copper_vm(start->arg.node, level+1, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_assert_false() {
        if (copper_vm(start->arg.node, level+1, input)) {
            reset();
            return false;
        }
        reset();
        return true;
    }

    inline bool prs_char() {
        PrsChar chr = 0;
        if (!current(&chr))           return false;
        if (chr != start->arg.letter) return false;
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
        if (!node(start->arg.name, &value)) return false;

        indent(); CU_DEBUG(2, "%s at (%u,%u)\n",
                           start->arg.name,
                           at.line_number + 1,
                           at.char_offset);

        // note the same name maybe call from two or more uncached nodes
        bool result = copper_vm(value, level+1, input);

        indent(); CU_DEBUG(2, "%s at (%u,%u) result %s\n",
                           start->arg.name,
                           at.line_number + 1,
                           at.char_offset,
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
        PrsLabel label = { 0, 0 };
        PrsEvent event = 0;

        indent(); CU_DEBUG(2, "fetching event %s at (%u,%u)\n",
                           start->arg.name,
                           at.line_number + 1,
                           at.char_offset);

        if (!input->event(input, start->arg.name, &event)) return false;

        label.name     = start->arg.name;
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
                indent(); CU_DEBUG(2, "match \'%s\' to end-of-file at (%u,%u)\n",
                                   char2string(*text),
                                   at.line_number + 1,
                                   at.char_offset);
                reset();
                return false;
            }

            indent(); CU_DEBUG(2, "match \'%s\' to \'%s\' at (%u,%u)\n",
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

    if (!cache_begin()) {
        return run_node();
    }

    if (cache_check()) {
        return match;
    }

    match = run_node();

    cache_end();

    return match;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool cu_InputInit(PrsInput input, unsigned cacheSize) {
    assert(0 != input);

    CU_DEBUG(3, "making cache %u\n", cacheSize);
    if (!make_Cache(cacheSize, &(input->cache))) return false;

    CU_DEBUG(3, "making queue\n");
    if (!make_Queue(&(input->queue))) return false;

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    assert(0 != cache);
    assert(0 != queue);
    assert(0 < cache->size);

    CU_DEBUG(3, "InputInit done (cache %x queue %x)\n", (unsigned) cache, (unsigned) queue);
    return true;
}

extern bool cu_Parse(char* name, PrsInput input) {
    assert(0 != input);

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    assert(0 != cache);
    assert(0 != queue);
    assert(0 < cache->size);

    PrsNode start = 0;

    CU_DEBUG(3, "requesting start node %s\n", name);
    if (!input->node(input, name, &start)) return false;

    CU_DEBUG(3, "clearing cache %x\n", (unsigned) cache);
    if (!input_CacheClear(input))          return false;

    CU_DEBUG(3, "clearing queue %x\n", (unsigned) queue);
    if (!queue_Clear(input->queue))        return false;

    CU_DEBUG(3, "starting parce\n");
    return copper_vm(start, 0, input);
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

    unsigned text_begin = input->slice.begin.text_inx;
    unsigned text_end   = input->slice.end.text_inx;

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


