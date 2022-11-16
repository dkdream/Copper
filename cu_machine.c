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
#include "copper.h"
#include "copper_inline.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */

static CuThread _free_threads_ = 0;

static bool make_Thread(const char* rule,
                        CuCursor at,
                        CuLabel  label,
                        CuNode   on,
                        CuThread *target)
{
    CuThread result = _free_threads_;

    if (!result) {
       result = malloc(sizeof(struct cu_thread));
    } else {
        _free_threads_ = result->next;
    }

    result->next  = 0;
    result->rule  = rule;
    result->at    = at;
    result->label = label;
    result->on    = on;

    *target = result;

    return true;
}

static bool free_Thread(CuThread *source) {
    if (!source)  return false;
    if (!*source) return false;

    CuThread thread = *source;

    thread->next   = _free_threads_;
    _free_threads_ = thread;

    *source = 0;

    return true;
}

static bool make_Queue(CuQueue *target) {
    if (!target) return false;

    CuQueue result = malloc(sizeof(struct cu_queue));

    result->free_list = 0;
    result->begin     = 0;
    result->end       = 0;

    *target = result;

    return true;
}

static bool queue_Event(CuQueue    queue,
                        const char* rule,
                        CuCursor   at,
                        CuLabel    label,
                        CuNode     on)
{
    if (!queue) return false;

    CuThread result = queue->free_list;

    if (!result) {
        make_Thread(rule, at, label, on, &result);
    } else {
        queue->free_list = result->next;
        result->next     = 0;
        result->rule     = rule;
        result->at       = at;
        result->label    = label;
        result->on       = on;
    }

    if (!queue->begin) {
        queue->begin = queue->end = result;
        return true;
    }

    queue->end->next = result;
    queue->end = result;

    return true;
}

static unsigned queue_Count(CuQueue queue)
{
    if (!queue) return 0;

    CuThread result = queue->begin;

    unsigned count = 0;
    for (; result ; ++count) {
        result = result->next;
    }

    return count;
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

static bool queue_Run(CuQueue queue,
                      Copper input)
{
    if (!queue)        return true;
    if (!queue->begin) return true;
    if (!input)        return false;

    CuContext  local = theContext(input);
    CuThread current = queue->begin;

    queue->begin = 0;

    for ( ; current ; ) {
        CuLabel label = current->label;

        local->context.rule = current->rule;
        local->context.on   = current->on;

        if (!label.function(input, current->at, label.name)) {
            queue_FreeList(queue, current);
            return false;
        }

        CuThread next = current->next;

        /* free this node */
        current->next    = queue->free_list;
        queue->free_list = current;

        current = next;
    }

    return true;
}

static bool queue_WillAdjust(CuQueue  queue,
                             CuThread to)
{
    if (!queue) return false;

    if (!to) {
        if (queue->begin) return true;
        if (queue->end)   return true;
        return false;
    }

    if (to == queue->end) return false;

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
        return queue_FreeList(queue, to);
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
    if (!queue) return false;

    CuThread node = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    return queue_FreeList(queue, node);
}

static bool free_Queue(CuQueue *source) {
    if (!source) return false;

    CuQueue  queue = *source;
    CuThread at    = queue->begin;

    queue->begin = 0;
    queue->end   = 0;

    for (; at ;) {
        CuThread hold = at->next;
        if (!free_Thread(&at)) {
            queue->begin = at;
            return false;
        }
        at = hold;
    }

    at = queue->free_list;

    queue->free_list = 0;

    for (; at ;) {
        CuThread hold = at->next;
        if (!free_Thread(&at)) {
            queue->free_list = at;
            return false;
        }
        at = hold;
    }

    free(queue);

    *source = 0;

    return true;
}

static CuFrame _free_frames_ = 0;

static bool make_Frame(CuFrame next, CuFrame *target) {
    CuFrame result = _free_frames_;

    if (!result) {
        result = malloc(sizeof(struct cu_frame));
    } else {
        _free_frames_ = result->next;
    }

    memset(result, 0, sizeof(struct cu_frame));

    result->next = next;

    *target = result;

    return true;
}

static bool free_Frame(CuFrame *source) {
    if (!source)  return false;
    if (!*source) return false;

    CuFrame frame =  *source;

    frame->next   = _free_frames_;
    _free_frames_ = frame;

    *source = 0;

    return true;
}

static bool make_Stack(CuStack *target) {
    struct cu_stack *result = malloc(sizeof(struct cu_stack));

    memset(result, 0, sizeof(struct cu_stack));

    int inx = 10;
    for(; inx-- ;) {
        if (!make_Frame(result->free_list, &(result->free_list))) return false;
    }

    *target = result;

    return true;
}

static bool stack_Extend(CuStack target) {
    int inx=10;
    for(; inx-- ;) {
        if (!make_Frame(target->free_list, &(target->free_list))) return false;
    }
    return true;
}

static bool free_Stack(CuStack *source) {
    if (!source)  return false;
    if (!*source) return false;

    CuStack stack = *source;
    CuFrame at    = stack->top;

    stack->top = 0;

    for (; at ;) {
        CuFrame hold = at->next;
        if (!free_Frame(&at)) {
            stack->top = at;
            return false;
        }
        at = hold;
    }

    at = stack->free_list;

    stack->free_list = 0;

    for (; at ;) {
        CuFrame hold = at->next;
        if (!free_Frame(&at)) {
            stack->free_list = at;
            return false;
        }
        at = hold;
    }

    free(stack);

    *source = 0;

    return true;
}

static CuPoint _free_points_ = 0;

static bool make_Point(CuPoint *target)
{
    CuPoint result = _free_points_;

    if (!result) {
        result = malloc(sizeof(struct cu_point));
    } else {
        _free_points_ = result->next;
    }

    memset(result, 0, sizeof(struct cu_point));

    *target = result;

    return true;
}

static bool free_Point(CuPoint *source) {
    if (!source)  return false;
    if (!*source) return false;

    CuPoint point = *source;

    point->next   = _free_points_;
    _free_points_ = point;

    *source = 0;

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

static bool cache_Point(CuCache cache, CuFrame frame) {
    if (!cache) return false;

    CuNode   node   = frame->node;
    unsigned offset = frame->at.text_inx;
    unsigned code   = offset;
    unsigned index  = code  % cache->size;
    CuPoint  list   = cache->table[index];

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

static bool free_Cache(CuCache *source) {
    if (!source)  return false;
    if (!*source) return false;

    CuCache cache = *source;
    CuPoint at;

    unsigned index   = 0;
    unsigned columns = cache->size;

    for ( ; index < columns ; ++index ) {
        if (!cache->table[index]) continue;

        at = cache->table[index];

        cache->table[index] = 0;

        for (; at ;) {
            CuPoint hold = at->next;
            if (!free_Point(&at)) {
                cache->table[index] = at;
                return false;
            }
            at = hold;
        }
    }

    at = cache->free_list;

    cache->free_list = 0;

    for (; at ;) {
        CuPoint hold = at->next;
        if (!free_Point(&at)) {
            cache->free_list = at;
            return false;
        }
        at = hold;
    }

    free(*source);
    *source = 0;

    return true;
}

/*****************************************************************
 *****************************************************************/

// event to mark the begin of text '<'
static bool mark_begin(Copper input, CuCursor at, CuName name) {
    if (!input) return false;
    CuContext local = theContext(input);
    local->context.begin = at;
    local->context.argument = (CuNode)0;
    return true;
    (void) name;  // to remove: unused parameter warning
}

static struct cu_label begin_label = { (CuEvent) &mark_begin, "set.begin" };

// event to mark the end of text '>'
static bool mark_end(Copper input, CuCursor at, CuName name) {
    if (!input) return false;
    CuContext local = theContext(input);
    local->context.end = at;
    return true;
    (void) name;  // to remove: unused parameter warning
}

static struct cu_label end_label = { (CuEvent) &mark_end, "set.end" };

// ????
static bool mark_argument(Copper input, CuCursor at, CuName name) {
    if (!input) return false;
    CuContext local = theContext(input);
    local->context.argument = local->context.on;
    return true;
    (void) at; // to remove: unused parameter warning
    (void) name;  // to remove: unused parameter warning
}

static struct cu_label argument_label = { (CuEvent) &mark_argument, "set.argument" };

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

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

static void cu_append(CuContext input, CuData *text)
{
    struct cu_text *data = &input->data;

    inline void extend() {
        const unsigned point  = data->limit;
        const unsigned length = data->size;
        const unsigned extend = text->length;

        unsigned space = (point < length) ? length - point : 0;

        if (space > extend) return;

        unsigned newsize = (0 < length) ? length : (extend << 1) ;

        while ((newsize - point) < extend) {
            newsize = newsize << 1;
        }

        const unsigned overflow = newsize + 2;

        if (data->buffer) {
            data->buffer = realloc(data->buffer, overflow);
            data->buffer[point] = 0;
        } else {
            data->buffer = malloc(overflow);
            memset(data->buffer, 0, overflow);
        }

        data->size = newsize;
    }

    inline void append() {
        char *dest = data->buffer + data->limit;

        memcpy(dest, text->start, text->length);

        data->limit   += text->length;
        data->buffer[data->limit] = 0;
        text->length = 0;
    }

    extend();
    append();
}

extern bool cu_LocalEnter(CuContext input, unsigned cacheSize) {
    assert(0 != input);

    CU_DEBUG(3, "making stack\n");
    if (!make_Stack(&(input->stack))) return false;

    CU_DEBUG(3, "making buffer\n");
    struct cu_text *data = &input->data;
    const unsigned size = 1024;
    data->buffer = malloc(size);
    memset(data->buffer, 0, size);
    data->size = size;

    CU_DEBUG(3, "making queue\n");
    if (!make_Queue(&(input->queue))) return false;

    CuQueue queue = input->queue;
    assert(0 != queue);

    CU_DEBUG(3, "making cache\n");
    if (!make_Cache(cacheSize, &(input->cache))) return false;

    CuCache cache = input->cache;
    assert(0 != cache);
    assert(cacheSize == cache->size);

    CU_DEBUG(3, "InputInit done (queue %lx) (cache %lx)\n",
             (unsigned long) queue,
             (unsigned long) cache);

    return true;
}

extern bool cu_MarkedText(CuContext input, CuData *target) {
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

extern bool cu_ArgumentText(CuContext input, CuData *target) {
    if (!input)  return false;
    if (!target) return false;

    CuNode argument = input->context.argument;

    if (!argument) return false;

    CuName name = argument->arg.name;

    if (!name) return false;

    target->length = strlen(name);
    target->start  = name;

    input->context.argument = 0;

    return true;
}

extern bool cu_InputFinit(CuContext input) {
    if (!input)  return false;

    if (!free_Cache(&(input->cache))) return false;

    if (!free_Queue(&(input->queue))) return false;

    struct cu_text *data = &input->data;
    if (data->buffer) {
        free(data->buffer);
        data->buffer = 0;
        data->size   = 0;
    }

    if (!free_Stack(&(input->stack))) return false;

    return true;
}

/*****************************************************************
 *****************************************************************
 *****************************************************************
 *****************************************************************/

intptr_t cu_global_debug = 0;

extern void cu_debug(const char *filename,
                     unsigned int linenum,
                     const char *format,
                     ...)
{
    va_list ap; va_start (ap, format);

    //    printf("file %s line %u :: ", filename, linenum);
    vprintf(format, ap);
    fflush(stdout);

    (void) filename;
    (void) linenum;
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


/*****************************************************************
 *****************************************************************
 *****************************************************************
 *****************************************************************/

extern bool cu_Start(const char* name, Copper input) {
    assert(0 != input);

    CuCallback callback = theCallback(input);
    CuContext  local    = theContext(input);

    CuNode start = 0;

    assert(0 != callback->node);

    CU_DEBUG(3, "requesting start node %s\n", name);
    if (!callback->node(callback, name, &start)) return false;

    assert(0 != start);
    assert(0 != local->queue);
    assert(0 != local->cache);
    assert(0 != local->stack);

    CU_DEBUG(3, "clearing queue %lx\n", (unsigned long) local->queue);
    if (!queue_Clear(local->queue)) return false;

    CU_DEBUG(3, "clearing cache %lx\n", (unsigned long) local->cache);
    if (!cache_Clear(local->cache)) return false;

    CU_DEBUG(2, "start rule \"%s\"\n", name);

    CuStack stack = local->stack;

    CU_DEBUG(3, "clearing parse stack %lx\n", (unsigned long) stack);

    while (stack->top) {
        CuFrame top = stack->top;
        stack->top  = top->next;
        top->next   = stack->free_list;
        stack->free_list = top;
    }

    if (!stack->free_list) {
        if (!stack_Extend(stack)) return false;
    }

    CuFrame next = stack->free_list;

    stack->free_list = next->next;

    next->next     = (CuFrame) 0;
    next->node     = start;
    next->phase    = cu_One;
    next->last     = false;
    next->rulename = name;         // set the rulename
    next->level    = 0;
    next->mark     = 0;
    next->at       = local->cursor;

    stack->top = next;

    return true;
}

extern CuSignal cu_Event(Copper input, CuData *data)
{
    assert(0 != input);

    CuCallback callback = theCallback(input);
    CuContext  local    = theContext(input);

    assert(0 != local->stack);
    assert(0 != local->stack->top);
    assert(0 != local->queue);
    assert(0 != data);

    const bool end_of_file = (data->length < 0);

    if (0 < data->length) {
        assert(0 != data->start);
        cu_append(local, data);
    }

    CuStack stack = local->stack;
    CuQueue queue = local->queue;
    CuCache cache = local->cache;
    CuFrame frame = stack->top;
    CuNode  start = frame->node;

    /************* for debugging (begin) *******************/
    char buffer[20];
    inline const char* node_label(CuNode node) {
        if (node->label) return node->label;
        sprintf(buffer, "%lx__", (unsigned long) node);
        return buffer;
    }

    inline void echo(intptr_t debug, unsigned from, unsigned to) {
        CU_ON_DEBUG(debug,
                    {
                        const unsigned limit = local->data.limit;
                        CU_DEBUG(debug, "\"");
                        unsigned inx = from;
                        for ( ; inx < limit; ++inx) {
                            if (inx >= to) break;
                            CU_DEBUG(debug, "%s", char2string(local->data.buffer[inx]));
                        }

                        CU_DEBUG(debug, "\"");
                    });
    }

    inline void indent(intptr_t debug) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = 0;
                        for ( ; inx < frame->level; ++inx) {
                            if (0 == (inx & 1)) {
                                CU_DEBUG(debug, "  |");
                            } else {
                                CU_DEBUG(debug, "  :");
                            }
                        }
                        CU_DEBUG(debug, "_");
                    });
    }

    inline void debug_charclass(intptr_t debug, const unsigned char *bits) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = 0;
                        for ( ; inx < 32;  ++inx) {
                            CU_DEBUG(debug, "\\%03o", bits[inx]);
                        }
                    });
    }
    /************* debugging (end) *******************/

    // push a new rule node
    inline bool push_rule(const char* rulename, CuNode node, CuPhase phase) {
        if (!stack->free_list) {
            if (!stack_Extend(stack)) return false;
        }

        CuFrame next = stack->free_list;

        stack->free_list = next->next;

        next->next     = frame;
        next->node     = node;
        next->phase    = cu_One;
        next->last     = false;
        next->rulename = rulename;         // set the rulename
        next->level    = frame->level + 1; // increase the indent level
        next->mark     = queue->end;
        next->at       = local->cursor;

        // shift to next phase;
        frame->phase = phase;

        stack->top = next;
        frame      = next;
        start      = node;

        return true;
    }

    // push a new internal node
    inline bool push_node(CuNode node, CuPhase phase) {
        if (!stack->free_list) {
            if (!stack_Extend(stack)) return false;
        }

        CuFrame next = stack->free_list;

        stack->free_list = next->next;

        next->next     = frame;
        next->node     = node;
        next->phase    = cu_One;
        next->last     = false;
        next->rulename = frame->rulename; // keep the current rulename
        next->level    = frame->level;    // keep the current indent level
        next->mark     = queue->end;
        next->at       = local->cursor;

        // shift to next phase;
        frame->phase = phase;

        stack->top = next; // push the stack
        frame      = next; // set the current frame
        start      = node; // set the current node

        return true;
    }

    inline bool pop(bool matched) {
        CuFrame next = frame->next;

        // add the last top to the free list
        frame->next = stack->free_list;
        stack->free_list = frame;

        // are we done?
        if (!next) return false;

        next->last = matched; // set the results
        stack->top = next;    // pop the stack
        frame = next;         // set the current frame
        start = frame->node;  // set the current node

        return true;
    }

    inline bool cursorMoved() {
        return local->cursor.text_inx > frame->at.text_inx;
    }

    inline bool mark() {
         frame->mark = queue->end;
         frame->at   = local->cursor;
         return true;
    }

    inline bool reset() {
        bool adjust = queue_WillAdjust(queue, frame->mark);

        if (adjust) {
            indent(3);
            CU_DEBUG(3, "reset at (%u,%u) ",
                     frame->at.line_number + 1,
                     frame->at.char_offset);
        }

        if (!frame->mark) {
            if (!queue_Clear(queue)) return false;
        } else {
            if (!queue_TrimTo(queue, frame->mark)) return false;
        }

        local->cursor = frame->at;

        if (adjust) {
            CU_ON_DEBUG(3, {
                    CU_DEBUG(3, "queue.size=%d\n",
                             queue_Count(queue));
                });
        };

        return true;
    }

    inline bool eot() {
        const unsigned point = local->cursor.text_inx;
        const unsigned limit = local->data.limit;
        return (point >= limit);
    }

    inline bool lookahead(const unsigned count) {
        const unsigned point     = local->cursor.text_inx;
        const unsigned limit     = local->data.limit;
        const unsigned lookahead = limit - point;
        return (lookahead < count);
    }

    inline bool token(CuChar chr) {
        const unsigned point = local->cursor.text_inx;
        const CuChar   token = local->data.buffer[point];
        return (token == chr);
    }

    inline bool token_range(CuChar from, CuChar to) {
        const unsigned point = local->cursor.text_inx;
        const CuChar   token = local->data.buffer[point];

        if (token < from) return false;
        if (token > to)   return false;
        return true;
    }

    inline bool token_member(const unsigned char *bits) {
        const unsigned point = local->cursor.text_inx;
        const CuChar   token = local->data.buffer[point];
        const unsigned  binx = token;

        if (bits[binx >> 3] & (1 << (binx & 7))) return true;
        return false;
    }

    inline void consume() {
        const unsigned point = local->cursor.text_inx;
        const CuChar   token = local->data.buffer[point];

        if ('\n' == token) {
            local->cursor.line_number += 1;
            local->cursor.char_offset  = 0;
        } else {
            local->cursor.char_offset += 1;
        }

        local->cursor.text_inx = point + 1;

        if ((point + 1) > local->reach.text_inx) {
            local->reach = local->cursor;
        }
    }

    inline bool match_string(const CuChar *text) {
        for ( ; 0 != *text ; ++text) {
            const unsigned point = local->cursor.text_inx;
            const CuChar     chr = *text;
            const CuChar   token = local->data.buffer[point];

            if (chr != token) return false;

            consume();
        }
        return true;
    }

    inline bool node(CuName name, CuNode* target) {

        indent(3); CU_DEBUG(3, "fetching node %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!callback->node(callback, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool predicate(CuName name, CuPredicate *test) {
        indent(3); CU_DEBUG(3, "fetching predicate %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!callback->predicate(callback, name, test)) {
            CU_ERROR("predicate %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool event(CuLabel *label) {
        CuName name = label->name;

        indent(3); CU_DEBUG(3, "fetching event %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!callback->event(callback, name, &(label->function))) {
            CU_ERROR("event %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool add_event(CuLabel label) {
        CU_ON_DEBUG(3,
                    {
                        indent(3);
                        CU_DEBUG(3, "event %s(%lx) at (%u,%u) queue.size=%d\n",
                                 label.name,
                                 (unsigned long) label.function,
                                 frame->at.line_number + 1,
                                 frame->at.char_offset,
                                 queue_Count(queue) + 1);
                    });

        return queue_Event(queue,
                           frame->rulename,
                           frame->at,
                           label,
                           start);
    }

    // return true if MisMatch
    // if the (node,inx) is in the cache then it is a mismatch
    inline bool checkCache(CuNode cnode) {
        bool result = cache_Find(cache, cnode, local->cursor.text_inx);
        return result;
    }

    // return true if MisMatch
    inline bool checkFirstSet(CuNode cnode) {
        if (eot()) return false;

        if (cu_MatchName    == cnode->oper) return false;
        if (pft_opaque      == cnode->type) return false;
        if (pft_event       == cnode->type) return false;
        if (pft_transparent == cnode->type) return false;

        CuFirstSet  first = cnode->first;
        CuFirstList list  = cnode->start;

        if (!first) return false;

        if (list) {
            if (0 < list->count) {
                indent(3); CU_DEBUG(3, "check (%s) at (%u,%u) first (bypass call nodes)\n",
                                    node_label(cnode),
                                    frame->at.line_number + 1,
                                    frame->at.char_offset);
                return false;
            }
        }

        const CuChar         token  = local->data.buffer[local->cursor.text_inx];
        const unsigned       binx   = token;
        const unsigned char *bits   = first->bitfield;
        bool                 result = (bits[binx >> 3] & (1 << (binx & 7)));

        indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) first %s",
                            node_label(cnode),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset,
                            (result ? "continue" : "skip"));

        CU_DEBUG(6, " (");
        debug_charclass(6, bits);
        CU_DEBUG(6, ")");
        CU_DEBUG(4, "\n");

        return !result;
    }

    // return true if MisMatch
    inline bool checkMetadata(CuNode cnode) {
        if (eot()) return false;

        if (cu_MatchName == cnode->oper) return false;

        CuMetaFirst meta = cnode->metadata;

        if (!meta)       return false;
        if (!meta->done) return false;

        if (pft_opaque      == meta->type)  return false;
        if (pft_event       == meta->type)  return false;
        if (pft_transparent == meta->type)  return false;

        CuMetaSet first = meta->first;

        if (!first) return false;

        const CuChar         token  = local->data.buffer[local->cursor.text_inx];
        const unsigned       binx   = token;
        const unsigned char *bits   = first->bitfield;
        bool                 result = (bits[binx >> 3] & (1 << (binx & 7)));

        indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) meta %s ",
                            node_label(cnode),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset,
                            (result ? "continue" : "skip"));
        CU_DEBUG(5, "(");
        debug_charclass(5, bits);
        CU_DEBUG(5, ")");
        CU_DEBUG(4, "\n");

        return !result;
    }

 do_Continue: {
        int level = 3;
        if (cu_MatchName == start->oper) {
            level = 2;
        }

        CU_ON_DEBUG(level, {
                indent(level); CU_DEBUG(level, "check (%s) [%d] %s",
                                    node_label(start),
                                    frame->phase,
                                    oper2name(start->oper));

                if (cu_MatchName == start->oper) {
                    CU_DEBUG(level, " %s", start->arg.name);
                }

                if (cu_MatchChar == start->oper) {
                    CU_DEBUG(level, " match(\'%s\')", char2string(start->arg.letter));
                }

                if (cu_MatchRange == start->oper) {
                    CU_DEBUG(level,  " between(\'%s\',\'%s\')",
                             char2string(start->arg.range->begin),
                             char2string(start->arg.range->end));
                }

                if (cu_MatchText == start->oper) {
                    const CuString string = start->arg.string;
                    CU_DEBUG(level, " match(%d, \"%s\")",
                             string->length,
                             string->text);
                }

                if (cu_MatchSet == start->oper) {
                    CU_DEBUG(level,  " set(%s)", start->arg.set->label);
                }

                CU_DEBUG(level, " at (%u,%u) ",
                         local->cursor.line_number + 1,
                         local->cursor.char_offset);

                const unsigned point = local->cursor.text_inx;

                echo(level, point, point + 10);

                CU_DEBUG(level, "\n");
            });
    }

    if (cu_One == frame->phase) {
        if (checkCache(start))    goto do_MisMatch_nocache;
        if (checkFirstSet(start)) goto do_MisMatch;
        if (checkMetadata(start)) goto do_MisMatch;
    }

 do_Switch:
    switch (start->oper) {
    case cu_Apply:       goto do_Apply;
    case cu_Argument:    goto do_Argument;
    case cu_AssertFalse: goto do_AssertFalse;
    case cu_AssertTrue:  goto do_AssertTrue;
    case cu_Begin:       goto do_Begin;
    case cu_Choice:      goto do_Choice;
    case cu_End:         goto do_End;
    case cu_Loop:        goto do_Loop;
    case cu_MatchChar:   goto do_MatchChar;
    case cu_MatchDot:    goto do_MatchDot;
    case cu_MatchName:   goto do_MatchName;
    case cu_MatchRange:  goto do_MatchRange;
    case cu_MatchSet:    goto do_MatchSet;
    case cu_MatchText:   goto do_MatchText;
    case cu_OneOrMore:   goto do_OneOrMore;
    case cu_Predicate:   goto do_Predicate;
    case cu_Sequence:    goto do_Sequence;
    case cu_ZeroOrMore:  goto do_ZeroOrMore;
    case cu_ZeroOrOne:   goto do_ZeroOrOne;
    case cu_Void:        goto do_Void;
    }
    goto do_PhaseError;

 do_Apply: {     // @name - an named event
        CuLabel label = { 0, start->arg.name };
        if (!event(&label))    goto do_Error;
        if (!add_event(label)) goto do_Error;
        goto do_Match;
    }
    goto do_PhaseError;

 do_Argument: {     // :[name] - an argument name
        // set set.argument
        if (add_event(argument_label)) {
            local->context.argument = start;
        }
        goto do_Match;
    }
    goto do_PhaseError;

 do_AssertFalse: // e !
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (frame->last) goto do_MisMatch;
        reset();
        goto do_Match;
    }
    goto do_PhaseError;

 do_AssertTrue:  // e &
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (!frame->last) goto do_MisMatch;
        reset();
        goto do_Match;
    }
    goto do_PhaseError;

 do_Begin: {
        // set state.begin
        if (add_event(begin_label)) {
            local->context.begin = frame->at;
            goto do_Match;
        }
        goto do_Error;
    }
    goto do_PhaseError;

 do_Choice:      // e1 / e2
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (frame->last) goto do_Match;
        frame->phase = cu_Three;
        goto do_Continue; // ???

    case cu_Three:
        if (!push_node(start->arg.pair->right, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four:
        if (frame->last) goto do_Match;
        goto do_MisMatch;
    }
    goto do_PhaseError;

 do_End: {
        // set state.end
        if (add_event(end_label)) {
            local->context.end = frame->at;
            goto do_Match;
        }
        goto do_Error;
    }
    goto do_PhaseError;

 do_Loop:  // e1 ; e2
    switch (frame->phase) {
    case cu_One: // call(e1)
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two: // return(e1) have we match the first one (once)?
        if (!frame->last) goto do_MisMatch;
        frame->phase = cu_Three;
        goto do_Continue; // ???

    case cu_Three: // call(e2)
        if (!push_node(start->arg.pair->right, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four: // return(e2) have we match the next one?
        if (!frame->last) goto do_Match;
        frame->phase = cu_Five;
        goto do_Continue; // ???

    case cu_Five: // call(e1)
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Six: // return(e1)  have we match the first one (again)?
        if (!frame->last) goto do_MisMatch;
        frame->phase = cu_Three;
        goto do_Loop;
    }
    goto do_PhaseError;

 do_MatchChar:   // 'chr
    if (lookahead(1))  goto do_MoreTokens;
    if (!token(start->arg.letter)) goto do_MisMatch;
    consume();
    goto do_Match;


 do_MatchDot:    // .
    if (lookahead(1))  goto do_MoreTokens;
    consume();
    goto do_Match;

 do_MatchName: {  // name
        const char *name = start->arg.name;
        CuNode value;

        switch (frame->phase) {
        default: break;
        case cu_One:
            if (!node(name, &value)) goto do_Error;
            if (!push_rule(name, value, cu_Two)) goto do_Error;
            goto do_Continue;

        case cu_Two:
            // note the same name maybe call from two or more uncached nodes
            if (frame->last) goto do_Match;
            goto do_MisMatch;
        }
    }
    goto do_PhaseError;

 do_MatchRange:  // begin-end
    if (lookahead(1)) goto do_MoreTokens;
    if (!token_range(start->arg.range->begin,
                     start->arg.range->end))
        goto do_MisMatch;

    consume();
    goto do_Match;

 do_MatchSet: {   // [...]
        if (lookahead(1))  goto do_MoreTokens;
        if (!token_member(start->arg.set->bitfield)) {
            goto do_MisMatch;
        }
        consume();
    }
    goto do_Match;


 do_MatchText: {  // "..."
        const CuString string = start->arg.string;

        if (lookahead(string->length)) goto do_MoreTokens;

        if (!match_string(string->text)) goto do_MisMatch;

        goto do_Match;
    }
    goto do_PhaseError;


 do_OneOrMore:   // e +
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two: // have we match the first one?
        if (!frame->last) goto do_MisMatch;
        frame->phase = cu_Three;
        goto do_Continue; // ??

    case cu_Three:
        if (!cursorMoved()) goto do_Match;
        mark();
        if (!push_node(start->arg.node, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four: // have we match the next one?
        if (!frame->last) goto do_Match;
        frame->phase = cu_Three;
        goto do_OneOrMore;
    }
    goto do_PhaseError;

 do_Predicate: { // %predicate
        local->context.rule = frame->rulename;

        // a predicate dosent consume input but may look ahead
        // a predicate dosent have phases
        CuPredicate test;
        if (!predicate(start->arg.name, &test)) goto do_Error;
        switch (test(input, frame, start->arg.name)) {
        case cu_FoundPath:
            reset();
            goto do_Match;

        case cu_NoPath:
            reset();
            goto do_MisMatch;

        case cu_Error:
            goto do_Error;

        case cu_NeedData:
            goto do_MoreTokens;
        }
    }
    goto do_PhaseError;

 do_Sequence:    // e1 e2
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two: // have we match the first one?
        if (!frame->last) goto do_MisMatch;
        frame->phase = cu_Three;
        goto do_Continue; // ???

    case cu_Three:
        if (!push_node(start->arg.pair->right, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four: // have we match the next one?
        if (frame->last) goto do_Match;
        goto do_MisMatch;
    }
    goto do_PhaseError;

 do_ZeroOrMore:  // e *
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (!frame->last) {
            reset();
            goto do_Match;
        }
        if (!cursorMoved()) goto do_Match;
        mark();
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;
    }
    goto do_PhaseError;

 do_ZeroOrOne:   // e ?
    switch (frame->phase) {
    default: break;
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (!frame->last) reset();
        goto do_Match;
    }
    goto do_PhaseError;

 do_Void:        // -nothing-
    goto do_PhaseError;

 do_MoreTokens:
    if (end_of_file) goto do_MisMatch;
    return cu_NeedData;

 do_Match: {
        int level = 3;
        if (cu_MatchName == start->oper) {
            level = 1;
        }

        CU_ON_DEBUG(level, {
                indent(level); CU_DEBUG(level, "pass  (%s) [%d] %s",
                                    node_label(start),
                                    frame->phase,
                                    oper2name(start->oper));

                if (cu_MatchName == start->oper) {
                    const char *name = start->arg.name;
                    CU_DEBUG(level, " %s", name);
                }

                CU_DEBUG(level, " at (%u,%u)",
                         frame->at.line_number + 1,
                         frame->at.char_offset);

                CU_DEBUG(level, " to (%u,%u) ",
                         local->cursor.line_number + 1,
                         local->cursor.char_offset);

                echo(level, frame->at.text_inx, local->cursor.text_inx);

                CU_DEBUG(level, "\n");
            });

        CuFrame next = frame->next;

        // are we done?
        if (!next) return cu_FoundPath;

        // add the last top to the free list
        frame->next = stack->free_list;
        stack->free_list = frame;

        next->last = true;   // set the results
        stack->top = next;   // pop the stack
        frame = next;        // set the current frame
        start = frame->node; // set the current node
    }
    goto do_Continue;

 do_MisMatch:
    if (!cache_Point(cache, frame)) goto do_Error;

 do_MisMatch_nocache: {
        int level = 3;
        if (cu_MatchName == start->oper) {
            level = 1;
        }

        CU_ON_DEBUG(level, {
                indent(level); CU_DEBUG(level, "fail  (%s) [%d] %s",
                                    node_label(start),
                                    frame->phase,
                                    oper2name(start->oper));

                if (cu_MatchName == start->oper) {
                    const char *name = start->arg.name;
                    CU_DEBUG(level, " %s", name);
                }

                CU_DEBUG(level, " at (%u,%u) ",
                         frame->at.line_number + 1,
                         frame->at.char_offset);

                echo(level, frame->at.text_inx, frame->at.text_inx + 10);

                CU_DEBUG(level, "\n");
            });

        reset();

        CuFrame next = frame->next;

        // are we done?
        if (!next) return cu_NoPath; // mismatch

        // add the last top to the free list
        frame->next = stack->free_list;
        stack->free_list = frame;

        next->last = false;  // set the results
        stack->top = next;   // pop the stack
        frame = next;        // set the current frame
        start = frame->node; // set the current node
    }
    goto do_Switch;

 do_Error: // process error (like malloc)
    indent(2);  CU_DEBUG(2, "ERROR\n");
    return cu_Error;

 do_PhaseError: // program error (bug in code)
    indent(2);  CU_DEBUG(2, "PHASE-ERROR\n");
    return cu_Error;

    (void)cache_Remove;
}

extern bool cu_RunQueue(Copper input) {
    if (!input) return false;

    CuContext local = theContext(input);

    return queue_Run(local->queue, input);
}
