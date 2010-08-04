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
#include "syntax.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* */

extern bool text_Extend(struct prs_text *text, const unsigned room) {
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
                        PrsEvent function,
                        PrsThread *target)
{
    struct prs_thread *result = malloc(sizeof(struct prs_thread));

    result->next     = 0;
    result->at       = at;
    result->function = function;

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

static bool queue_Event(PrsQueue  queue,
                        PrsCursor at,
                        PrsEvent function)
{
    if (!queue) return true;

    struct prs_thread *result = queue->free_list;

    if (!result) {
        make_Thread(at, function, &result);
    } else {
        queue->free_list = result->next;
        result->next     = 0;
        result->at       = at;
        result->function = function;
    }

    if (!queue->begin) {
        queue->begin = queue->end = result;
        return true;
    }

    queue->end->next = result;
    queue->end = result;

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
        return make_Thread(value->at, value->function, target);
    }

    queue->free_list = result->next;

    result->next     = 0;
    result->at       = value->at;
    result->function = value->function;

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

// (begin-end] or [begin->next-end]
// start after begin upto and including end
static bool queue_Slice(PrsQueue   queue,
                        PrsThread  begin,
                        PrsThread  end,
                        PrsSlice  *slice)
{
    if (!slice) return false;
    if (!begin) {
        return (0 == end);
    }

    if (begin == end) {
        queue_FreeList(queue, slice->begin);
        slice->begin = 0;
        slice->end   = 0;
        return true;
    }

    PrsSlice hold = *slice;

    PrsThread last;
    PrsThread node = begin->next;

    queue_CloneEvent(queue, node, &last);

    slice->begin = last;

    for ( ; node ; ) {
        if (node == end) {
            queue_FreeList(queue, hold.begin);
            return true;
        }
        node = node->next;
        queue_CloneEvent(queue, node, &last->next);
        last = slice->end = last->next;
    }

    queue_FreeList(queue, slice->begin);

    *slice = hold;

    return false;
}

static bool queue_AppendSlice(PrsQueue queue,
                              PrsSlice segment)
{
    if (!queue)         return true;
    if (!segment.begin) return true;
    if (!segment.end)   return false;

    PrsSlice slice;

    if (!queue_Slice(queue,
                     segment.begin,
                     segment.end,
                     &slice))
        return false;

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

static bool make_Cache(unsigned size, PrsCache *target) {
    unsigned fullsize = (sizeof(struct prs_cache) + (size * sizeof(PrsPoint)));

    struct prs_cache *result = malloc(fullsize);
    memset(result, 0, fullsize);

    result->size = size;

    *target = result;

    return true;
}

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

static bool input_CacheInsert(PrsInput input,
                              PrsNode  node,
                              PrsState cursor,
                              bool     match,
                              PrsSlice segment)
{
    if (!input) return false;

    PrsCache cache = input->cache;
    PrsQueue queue = input->queue;

    if (!cache) return false;

    unsigned code  = (unsigned)node;
    unsigned index = code  % cache->size;
    PrsPoint list  = cache->table[index];

    for ( ; list ; list = list->next) {
        if (node != list->node) continue;
        if (cursor.begin.text_inx != list->cursor.begin.text_inx) continue;
        list->cursor.end = cursor.end;
        list->match      = match;
        queue_FreeList(queue, list->segment.begin);
        list->segment = segment;
        return true;
    }

    if (!cache_Point(cache,
                     node,
                     cursor,
                     match,
                     segment,
                     cache->table[index],
                     &list)) {
        return false;
    }

    cache->table[index] = list;

    return true;
}

static bool input_CacheClear(PrsInput input) {
    if (!input) return false;

     PrsCache cache = input->cache;
     PrsQueue queue = input->queue;

    if (!cache) return true;

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

static bool make_Map(unsigned code,
                     void* key,
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
                      void* key,
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

#if 0
static bool hash_Add(struct prs_hash *hash,
                     void* key,
                     void* value)
{
    if (!key)   return false;
    if (!value) return false;

    unsigned code  = hash->encode(key);
    unsigned index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!hash->compare(key, map->key)) continue;
        return false;
    }

    if (!make_Map(code, key, value, hash->table[index], &map)) {
        return false;
    }

    hash->table[index] = map;

    return true;
}
#endif

static bool hash_Replace(struct prs_hash *hash,
                         void* key,
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

#if 0
static bool hash_Remove(struct prs_hash *hash,
                         void* key,
                         FreeValue release)
{
    if (!key)     return false;
    if (!release) return false;

    unsigned code  = hash->encode(key);
    unsigned index = code % hash->size;

    struct prs_map *map = hash->table[index];

    for ( ; map ; map = map->next) {
        if (code != map->code) continue;
        if (!hash->compare(key, map->key)) continue;
        if (release(map->value)) return false;
        break;
    }

    return true;
}
#endif

static unsigned buffer_GetLine(struct prs_buffer *input, struct prs_text *data)
{
    if (input->cursor >= input->read) {
        int read = getline(&input->line, &input->allocated, input->file);
        if (read < 0) return 0;
        input->cursor = 0;
        input->read   = read;
    }

    unsigned count = input->read - input->cursor;

    text_Extend(data, count);

    char *dest = data->buffer + data->limit;
    char *src  = input->line  + input->cursor;

    memcpy(dest, src, count);

    input->cursor += count;
    data->limit   += count;

    data->buffer[data->limit] = 0;

    return count;
}

static bool file_CurrentChar(PrsInput input, PrsChar* target) {
    struct prs_file *file = (struct prs_file *)input;

    unsigned point = file->cursor.text_inx;
    unsigned limit = file->base.data.limit;

    if (point >= limit) {
        if (0 >= buffer_GetLine(&file->buffer, &file->base.data))
            return false;
    }

    *target = file->base.data.buffer[point];

    return true;
}

static bool file_NextChar(PrsInput input) {
    struct prs_file *file = (struct prs_file *)input;

    file->cursor.text_inx    += 1;
    file->cursor.char_offset += 1;

    unsigned point = file->cursor.text_inx;
    unsigned limit = file->base.data.limit;

    if (point >= limit) {
        if (0 >= buffer_GetLine(&file->buffer, &file->base.data))
            return false;
    }

    if ('\n' == file->base.data.buffer[point]) {
        file->cursor.line_number += 1;
        file->cursor.char_offset  = 0;
    }

    return true;
}

static bool file_GetCursor(PrsInput input, PrsCursor* target) {
    struct prs_file *file = (struct prs_file *)input;
    (*target) = file->cursor;
    return true;
}

static bool file_SetCursor(PrsInput input, PrsCursor value) {
    struct prs_file *file = (struct prs_file *)input;
    file->cursor = value;
    return true;
}

static bool file_FindNode(PrsInput input, PrsName name, PrsNode* target) {
    struct prs_file *file = (struct prs_file *)input;
    if (hash_Find(file->nodes, name, (void**)target)) {
        return true;
    }
    return false;
}

static bool file_FindPredicate(PrsInput input, PrsName name, PrsPredicate* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->predicates, name, (void**)target);
}

static bool file_FindAction(PrsInput input, PrsName name, PrsAction* target) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Find(file->actions, name, (void**)target);
}

static bool noop_release(void* value) {
    return true;
}

#if 0
static bool malloc_release(void* value) {
    free(value);
    return true;
}
#endif

static bool file_AddName(PrsInput input, PrsName name, PrsNode value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->nodes, name, value, noop_release);
}

static bool file_SetPredicate(PrsInput input, PrsName name, PrsPredicate value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->predicates, name, value, noop_release);
}

static bool file_SetAction(PrsInput input, PrsName name, PrsAction value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->actions, name, value, noop_release);
}

static bool file_SetEvent(PrsInput input, PrsName name, PrsEvent value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->events, name, value, noop_release);
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

extern bool make_PrsFile(const char* filename, PrsInput *target) {
    struct prs_file *result = malloc(sizeof(struct prs_file));

    memset(result, 0, sizeof(struct prs_file));

    result->base.current   = file_CurrentChar;
    result->base.next      = file_NextChar;
    result->base.fetch     = file_GetCursor;
    result->base.reset     = file_SetCursor;
    result->base.node      = file_FindNode;
    result->base.predicate = file_FindPredicate;
    result->base.action    = file_FindAction;
    result->base.attach    = file_AddName;
    result->base.set_p     = file_SetPredicate;
    result->base.set_a     = file_SetAction;
    result->base.set_e     = file_SetEvent;

    make_Cache(1024, &result->base.cache);
    make_Queue(&result->base.queue);

    /* */
    result->filename    = strdup(filename);
    result->buffer.file = fopen(filename, "r");

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
              &result->actions);

    make_Hash((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->events);

    *target = (PrsInput) result;

    return true;
}

static bool mark_begin(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->slice.begin = at;
    return true;
}

static bool mark_end(PrsInput input, PrsCursor at) {
    if (!input) return false;
    input->slice.end = at;
    return true;
}

static bool copper_vm(PrsNode start, PrsInput input) {
    PrsState  cursor;
    PrsSlice  segment;
    bool      match = false;
    PrsCursor at;
    PrsThread begin = 0;

    inline bool hold()    {
        if (input->queue) {
            begin = input->queue->end;
        }
        return input->fetch(input, &at);
    }
    inline bool reset()   {
        if (begin) {
            if (!queue_FreeList(input->queue, begin->next))
                return false;
            begin->next = 0;
        }
        if (input->queue) {
            input->queue->end = begin;
        }
        return input->reset(input, at);
    }

    inline bool next() {
        return input->next(input);
    }

    inline bool current(PrsChar *target) {
        return input->current(input, target);
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

    inline bool action(PrsName name) {
        PrsAction thunk = 0;
        if (input->action(input, name, &thunk)) {
            thunk(input);
            return true;
        }
        return true;
    }

    inline bool cache_begin() {
        segment.begin = begin;
        segment.end   = 0;
        return input->fetch(input, &cursor.begin);
    }

    inline bool cache_check() {
        PrsPoint point = 0;
        if (cache_Find(input->cache, start, cursor.begin, &point)) {
            if (input->reset(input, point->cursor.end)) {
                if (input->queue) {
                    if (!queue_AppendSlice(input->queue,
                                           point->segment))
                        return false;
                }
                match = point->match;
                return true;
            }
        }
        return false;

    }

    inline bool cache_end() {
        if (input->fetch(input, &cursor.end)) {
            if (!match) {
                segment.begin = 0;
                segment.end   = 0;
            } else {
                if (segment.begin) {
                    segment.end = input->queue->end;
                } else {
                    segment.end = 0;
                }
            }
            return input_CacheInsert(input,
                                     start,
                                     cursor,
                                     match,
                                     segment);
        }
        return false;
    }

    inline bool prs_and() {
        if (!copper_vm(start->arg.pair->left, input))  {
            reset();
            return false;
        }
        if (!copper_vm(start->arg.pair->right, input)) {
            reset();
            return false;
        }
        return true;
    }

    inline bool prs_choice() {
        if (copper_vm(start->arg.pair->left, input)) {
            return true;
        }
        reset();
        if (copper_vm(start->arg.pair->right, input)) {
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_zero_plus() {
        while (copper_vm(start->arg.node, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        while (copper_vm(start->arg.node, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool prs_optional() {
        if (copper_vm(start->arg.node, input)) {
            return true;
        }
        reset();
        return true;
    }

    inline bool prs_assert_true() {
        if (copper_vm(start->arg.node, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_assert_false() {
        if (copper_vm(start->arg.node, input)) {
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
        if (!node(start->arg.name, &value)) {
            return false;
        }
        // note the same node maybe call from two or more uncached nodes
        bool result = copper_vm(value, input);

        printf("running %s (%u,%u) %s\n",
               (char*) start->arg.name,
               at.line_number,
               at.char_offset,
               (result ? "match" : "no match"));

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

    inline bool prs_string() {
        PrsString string = start->arg.string;
        unsigned  length = string->length;
        PrsChar   chr  = 0;
        unsigned  inx  = 0;

        for ( ; inx < length ; ++inx) {
            if (!current(&chr)) {
                reset();
                return false;
            }
            if (chr != string->text[inx]) {
                reset();
                return false;
            }
            next();
        }

        return true;
    }

    inline bool prs_begin() {
        return queue_Event(input->queue,
                           at,
                           mark_begin);
    }

    inline bool prs_end() {
        return queue_Event(input->queue,
                           at,
                           mark_end);
    }

    inline bool prs_predicate() {
        if (!predicate(start->arg.name)) {
            reset();
            return false;
        }
        return true;
    }

    inline bool prs_action() {
        return action(start->arg.name);
    }

    inline bool prs_event() {
        return queue_Event(input->queue,
                           at,
                           start->arg.event);
    }

    inline bool run_node() {
        hold();
        switch (start->oper) {
        case prs_Action:      return prs_action();
        case prs_AssertFalse: return prs_assert_false();
        case prs_AssertTrue:  return prs_assert_true();
        case prs_Choice:      return prs_choice();
        case prs_Event:       return prs_event();
        case prs_MatchChar:   return prs_char();
        case prs_MatchDot:    return prs_dot();
        case prs_MatchName:   return prs_name();
        case prs_MatchRange:  return prs_between();
        case prs_MatchSet:    return prs_in();
        case prs_MatchString: return prs_string();
        case prs_OneOrMore:   return prs_one_plus();
        case prs_Predicate:   return prs_predicate();
        case prs_Sequence:    return prs_and();
        case prs_ZeroOrMore:  return prs_zero_plus();
        case prs_ZeroOrOne:   return prs_optional();
        case prs_Begin:       return prs_begin();
        case prs_End:         return prs_end();
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

extern bool input_Parse(char* name, PrsInput input) {
    PrsNode start = 0;

    if (!input->node(input, name, &start)) return false;
    if (!input_CacheClear(input))          return false;
    if (!queue_Clear(input->queue))        return false;

    printf("running %s\n", name);

    return copper_vm(start, input);
}

extern bool input_Text(PrsInput input, PrsData *target) {
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


