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
#include "copper_inline.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>

/* */

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

        CuThread next    = current->next;
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

static bool make_Frame(CuFrame next, CuFrame *target) {
    struct cu_frame *result = malloc(sizeof(struct cu_frame));

    memset(result, 0, sizeof(struct cu_frame));

    result->next = next;

    *target = result;

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

static void cu_append(Copper input, const CuData text)
{
    struct cu_text *data = &input->data;

    inline void extend() {
        const unsigned point  = data->limit;
        const unsigned length = data->size;

        int space = length - point;

        if (space > text.length) return;

        unsigned newsize = (0 < length) ? length : (text.length << 1) ;

        while ((newsize - point) < text.length) {
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

        memcpy(dest, text.start, text.length);

        data->limit   += text.length;
        data->buffer[data->limit] = 0;
    }

    extend();
    append();
}

extern CuSignal cu_Event(Copper input, const CuData data)
{
    assert(0 != input);
    assert(0 != data.start);
    assert(0 != input->stack);
    assert(0 != input->stack->top);
    assert(0 != input->queue);

    CuStack stack = input->stack;
    CuQueue queue = input->queue;
    CuCache cache = input->cache;

    cu_append(input, data);

    bool end_of_file = (data.length < 1);

    unsigned point         = input->cursor.text_inx;
    unsigned limit         = input->data.limit;
    bool     end_of_tokens = (point >= limit);
    unsigned lookahead     = limit - point;
    CuChar   token         = input->data.buffer[point];

    CuFrame frame = stack->top;
    CuNode  start = frame->node;

    //    const char* rulename = frame->rulename;
    //    unsigned    level    = frame->level;

    //    CuThread mark = 0;
    //    CuCursor at;

    /************* for debugging (begin) *******************/
    char buffer[10];
    inline const char* node_label(CuNode node) {
        if (node->label) return node->label;
        sprintf(buffer, "%x__", (unsigned) node);
        return buffer;
    }

    inline void indent(unsigned debug) {
        CU_ON_DEBUG(debug,
                    { unsigned inx = frame->level;
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
        next->at       = input->cursor;

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
        next->at       = input->cursor;

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
        return point > frame->at.text_inx;
    }

    inline bool reset() {
        if (!frame->mark) {
            if (!queue_Clear(queue)) return false;
        } else {
            if (!queue_TrimTo(queue, frame->mark)) return false;
        }

        input->cursor = frame->at;

        point         = input->cursor.text_inx;
        end_of_tokens = (point >= limit);
        lookahead     = limit - point;
        token         = input->data.buffer[point];

        return true;
    }

    inline void consume() {
        if ('\n' == token) {
            input->cursor.line_number += 1;
            input->cursor.char_offset  = 0;
        } else {
            input->cursor.char_offset += 1;
        }

        input->cursor.text_inx += 1;

        point         = input->cursor.text_inx;
        end_of_tokens = (point >= limit);
        lookahead     = limit - point;
        token         = input->data.buffer[point];

        if (point > input->reach.text_inx) {
            input->reach = input->cursor;
        }
    }

    inline bool node(CuName name, CuNode* target) {

        indent(2); CU_DEBUG(2, "fetching node %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!input->node(input, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool predicate(CuName name, CuPredicate *test) {
        indent(2); CU_DEBUG(2, "fetching predicate %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!input->predicate(input, name, test)) {
            CU_ERROR("predicate %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool event(CuLabel *label) {
        CuName name = label->name;

        indent(2); CU_DEBUG(2, "fetching event %s at (%u,%u)\n",
                            name,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (!input->event(input, name, &(label->function))) {
            CU_ERROR("event %s not found\n", name);
            return false;
        }

        return true;
    }

    inline bool add_event(CuLabel label) {
        indent(2); CU_DEBUG(2, "event %s(%x) at (%u,%u)\n",
                            label.name,
                            (unsigned) label.function,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        return queue_Event(queue,
                           frame->rulename,
                           frame->at,
                           label);
    }

    // return true if MisMatch
    // if the (node,inx) is in the cache then it is a mismatch
    inline bool checkCache(CuNode cnode) {
        return cache_Find(cache, cnode, point);
    }

    // return true if MisMatch
    inline bool checkFirstSet(CuNode cnode) {
        if (end_of_tokens) return false;

        if (cu_MatchName    == cnode->oper) return false;
        if (pft_opaque      == cnode->type) return false;
        if (pft_event       == cnode->type) return false;
        if (pft_transparent == cnode->type) return false;

        CuFirstSet  first = cnode->first;
        CuFirstList list  = cnode->start;

        if (!first) return false;

        if (list) {
            if (0 < list->count) {
                indent(2); CU_DEBUG(2, "check (%s) at (%u,%u) first (bypass call nodes)\n",
                                    node_label(cnode),
                                    frame->at.line_number + 1,
                                    frame->at.char_offset);
                return false;
            }
        }

        unsigned             binx   = token;
        const unsigned char *bits   = first->bitfield;
        bool                 result = (bits[binx >> 3] & (1 << (binx & 7)));

        indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) first %s",
                            node_label(cnode),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset,
                            (result ? "continue" : "skip"));

        CU_DEBUG(4, " (");
        debug_charclass(4, bits);
        CU_DEBUG(4, ")\n");

        return !result;
    }

    // return true if MisMatch
    inline bool checkMetadata(CuNode cnode) {
        if (end_of_tokens) return false;

        if (cu_MatchName == cnode->oper) return false;

        CuMetaFirst meta = cnode->metadata;

        if (!meta)       return false;
        if (!meta->done) return false;

        if (pft_opaque      == meta->type)  return false;
        if (pft_event       == meta->type)  return false;
        if (pft_transparent == meta->type)  return false;

        CuMetaSet first = meta->first;

        if (!first) return false;

        unsigned             binx   = token;
        const unsigned char *bits   = first->bitfield;
        bool                 result = (bits[binx >> 3] & (1 << (binx & 7)));

        indent(4); CU_DEBUG(4, "check (%s) to cursor(\'%s\') at (%u,%u) meta %s (",
                            node_label(cnode),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset,
                            (result ? "continue" : "skip"));
        debug_charclass(4, bits);
        CU_DEBUG(4, ")\n");

        return !result;
    }

 do_Continue:

    CU_ON_DEBUG(3, {
            indent(3); CU_DEBUG(3, "check (%s) %s",
                                node_label(start),
                                oper2name(start->oper));

            if (cu_MatchName == start->oper) {
                const char *name = start->arg.name;
                CU_DEBUG(3, " %s", name);
            }
            CU_DEBUG(3, " at (%u,%u)\n",
                     frame->at.line_number + 1,
                     frame->at.char_offset);
        });

    if (checkCache(start))    goto do_MisMatch_nocache;
    if (checkFirstSet(start)) goto do_MisMatch;
    if (checkMetadata(start)) goto do_MisMatch;

    switch (start->oper) {
    case cu_Apply:       goto do_Apply;
    case cu_AssertFalse: goto do_AssertFalse;
    case cu_AssertTrue:  goto do_AssertTrue;
    case cu_Begin:       goto do_Begin;
    case cu_Choice:      goto do_Choice;
    case cu_End:         goto do_End;
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

 do_AssertFalse: // e !
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        reset();
        if (frame->last) {
            goto do_MisMatch;
        } else {
            goto do_Match;
        }
    default: break;
    }
    goto do_PhaseError;

 do_AssertTrue:  // e &
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        reset();
        if (frame->last) {
            goto do_Match;
        } else {
            goto do_MisMatch;
        }
    default: break;
    }
    goto do_PhaseError;

 do_Begin: {     // set state.begin
        if (add_event(begin_label)) goto do_Match;
        goto do_Error;
    }
    goto do_PhaseError;

 do_Choice:      // e1 e2 /
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (frame->last) goto do_Match;
        reset();
        frame->phase = cu_Three;

    case cu_Three:
        if (!push_node(start->arg.pair->right, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four:
        if (frame->last) goto do_Match;
        reset();
        goto do_MisMatch;
    }
    goto do_PhaseError;

 do_End: {       // set state.end
        if (add_event(end_label)) goto do_Match;
        goto do_Error;
    }
    goto do_PhaseError;

 do_MatchChar:   // 'chr
    if (end_of_tokens) {
        if (!end_of_file) goto do_MoreTokens;

        indent(2); CU_DEBUG(2, "match(\'%s\') to end-of-file at (%u,%u)\n",
                            char2string(start->arg.letter),
                            frame->at.line_number + 1,
                            frame->at.char_offset);
        goto do_MisMatch;
    } else {
        indent(2); CU_DEBUG(2, "match(\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                            char2string(start->arg.letter),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (token != start->arg.letter) goto do_MisMatch;

        consume();

        goto do_Match;
    }
    goto do_PhaseError;

 do_MatchDot:    // .
    if (end_of_tokens) {
        if (!end_of_file) goto do_MoreTokens;
        goto do_MisMatch;
    } else {
        consume();
        goto do_Match;
    }
    goto do_PhaseError;


 do_MatchName: {  // name
        const char *name = start->arg.name;
        CuNode value;

        switch (frame->phase) {
        case cu_One:
            if (!node(name, &value)) goto do_Error;

            if (cache_Find(input->cache, value, frame->at.text_inx)) {
                indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) (cached result failed\n",
                                    name,
                                    frame->at.line_number + 1,
                                    frame->at.char_offset);
                goto do_MisMatch;
            }

            if (checkFirstSet(value)) goto do_MisMatch;
            if (checkMetadata(value)) goto do_MisMatch;

            indent(2); CU_DEBUG(2, "rule \"%s\" at (%u,%u)\n",
                                name,
                                frame->at.line_number + 1,
                                frame->at.char_offset);

            if (!push_rule(name, value, cu_Two)) goto do_Error;
            goto do_Continue;

        case cu_Two:
            // note the same name maybe call from two or more uncached nodes
            indent(2); CU_DEBUG(1, "rule \"%s\" at (%u,%u) to (%u,%u) result %s\n",
                                name,
                                frame->at.line_number + 1,
                                frame->at.char_offset,
                                input->cursor.line_number + 1,
                                input->cursor.char_offset,
                                (frame->last ? "passed" : "failed"));
            if (frame->last) goto do_Match;
            goto do_MisMatch;

        default: break;
        }
    }
    goto do_PhaseError;

 do_MatchRange:  // begin-end
    if (end_of_tokens) {
        if (!end_of_file) goto do_MoreTokens;
        indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to end-of-file at (%u,%u)\n",
                            char2string(start->arg.range->begin),
                            char2string(start->arg.range->end),
                            frame->at.line_number + 1,
                            frame->at.char_offset);
        goto do_MisMatch;
    } else {
        indent(2); CU_DEBUG(2, "between(\'%s\',\'%s\') to cursor(\'%s\') at (%u,%u)\n",
                            char2string(start->arg.range->begin),
                            char2string(start->arg.range->end),
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        if (token < start->arg.range->begin) goto do_MisMatch;
        if (token > start->arg.range->end)   goto do_MisMatch;

        consume();

        goto do_Match;
    }
    goto do_PhaseError;

 do_MatchSet:    // [...]
    if (end_of_tokens) {
        if (!end_of_file) goto do_MoreTokens;

        indent(2); CU_DEBUG(2, "set(%s) to end-of-file at (%u,%u)\n",
                            start->arg.set->label,
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        goto do_MisMatch;
    } else {
        indent(2); CU_DEBUG(2, "set(%s) to cursor(\'%s\') at (%u,%u)\n",
                            start->arg.set->label,
                            char2string(token),
                            frame->at.line_number + 1,
                            frame->at.char_offset);

        unsigned             binx = token;
        const unsigned char *bits = start->arg.set->bitfield;

        if (bits[binx >> 3] & (1 << (binx & 7))) {
            consume();
            goto do_Match;
        }

        goto do_MisMatch;
    }
    goto do_PhaseError;


 do_MatchText:   // "..."
    if (end_of_tokens) {
        if (!end_of_file) goto do_MoreTokens;
    } else {
        CuString string = start->arg.string;

        if (lookahead < string->length) {
            if (end_of_file) goto do_MisMatch;
            goto do_MoreTokens;
        }

        const CuCursor hold = input->cursor;
        const CuChar  *text = string->text;

        for ( ; 0 != *text ; ++text) {
            if (token != *text) {
                input->cursor = hold;
                goto do_MisMatch;
            }
            consume();
        }

        goto do_Match;
    }
    goto do_PhaseError;


 do_OneOrMore:   // e +
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two: // have we match the first one?
        if (frame->last) {
            frame->phase = cu_Three;
        } else {
            reset();
            goto do_MisMatch;
        }

    case cu_Three:
        if (!push_node(start->arg.node, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four: // have we match the next one?
        if (frame->last) {
            frame->phase = cu_Three;
            goto do_OneOrMore;
        } else {
            reset();
            goto do_Match;
        }
    }
    goto do_PhaseError;

 do_Predicate: { // %predicate
        // a predicate dosent consume input but may look ahead
        // a predicate dosent have phases
        CuPredicate test;
        if (!predicate(start->arg.name, &test)) goto do_Error;
    }
    goto do_PhaseError;

 do_Sequence:    // e1 e2 ;
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.pair->left, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two: // have we match the first one?
        if (frame->last) {
            frame->phase = cu_Three;
        } else {
            reset();
            goto do_MisMatch;
        }

    case cu_Three:
        if (!push_node(start->arg.pair->right, cu_Four)) goto do_Error;
        goto do_Continue;

    case cu_Four: // have we match the next one?
        if (frame->last) goto do_Match;
        reset();
        goto do_MisMatch;
    }
    goto do_PhaseError;

 do_ZeroOrMore:  // e *
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (frame->last) {
            if (!cursorMoved()) goto do_Match;
            if (!push_node(start->arg.node, cu_Two)) goto do_Error;
            goto do_Continue;
        } else {
            reset();
            goto do_Match;
        }
    default: break;
    }
    goto do_PhaseError;

 do_ZeroOrOne:   // e ?
    switch (frame->phase) {
    case cu_One:
        if (!push_node(start->arg.node, cu_Two)) goto do_Error;
        goto do_Continue;

    case cu_Two:
        if (frame->last) {
            goto do_Match;
        } else {
            reset();
            goto do_Match;
        }
    default: break;
    }
    goto do_PhaseError;

 do_Void:        // -nothing-
    goto do_PhaseError;

 do_MoreTokens:
    return cu_NeedData;

 do_Match: {
        CuFrame next = frame->next;

        // are we done?
        if (!next) return cu_FoundPath; // mismatch

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
    if (!cache_Point(cache, start, point)) goto do_Error;

 do_MisMatch_nocache: {
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
    goto do_Continue;

 do_Error: // process error (like malloc)
    return cu_Error;

 do_PhaseError: // program error (bug in code)
    return cu_Error;

    (void)cache_Remove;
}

/*************************************************************************************
 *************************************************************************************
 *************************************************************************************
 *************************************************************************************/

extern bool cu_InputInit(Copper input, unsigned cacheSize) {
    assert(0 != input);

    CU_DEBUG(3, "making stack\n");
    if (!make_Stack(&(input->stack))) return false;

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

extern bool cu_Start(const char* name, Copper input) {
    assert(0 != input);

    CuNode start = 0;

    assert(0 != input->node);

    CU_DEBUG(3, "requesting start node %s\n", name);
    if (!input->node(input, name, &start)) return false;

    assert(0 != start);
    assert(0 != input->queue);
    assert(0 != input->cache);
    assert(0 != input->stack);

    CU_DEBUG(3, "clearing queue %x\n", (unsigned) input->queue);
    if (!queue_Clear(input->queue)) return false;

    CU_DEBUG(3, "clearing cache %x\n", (unsigned) input->cache);
    if (!cache_Clear(input->cache)) return false;

    CU_DEBUG(2, "start rule \"%s\"\n", name);

    CuStack stack = input->stack;

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
    next->at       = input->cursor;

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

