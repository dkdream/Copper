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

#include "copper_vm.h"

/* */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* */

struct prs_list {
    void            *value;
    struct prs_list *next;
};

static bool make_List(void* value,
                      struct prs_list *next,
                      struct prs_list** target)
{
    struct prs_list *result = malloc(sizeof(struct prs_list));

    result->value = value;
    result->next  = next;

    *target = result;
    return true;
}

struct prs_map {
    unsigned        code;
    void*           key;
    void*           value;
    struct prs_map *next;
};

static bool make_Map(unsigned code,
                     void* key,
                     void* value,
                     struct prs_map *next,
                     struct prs_map **target)
{
    struct prs_map *result = malloc(sizeof(struct prs_map));

    result->code  = code;
    result->key   = value;
    result->value = value;
    result->next  = next;

    *target = result;
    return true;
}

typedef unsigned long (*Hashcode)(void*);
typedef bool     (*Matchkey)(void*, void*);
typedef bool     (*FreeValue)(void*);

struct prs_hash {
    Hashcode encode;
    Matchkey compare;
    unsigned size;
    struct prs_map *table[];
};

static bool hash_Make(Hashcode encode,
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

    if (!make_Map(code, key, value, hash->table[index], &map)) return false;

    hash->table[index] = map;

    return true;
}

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
        if (release(map->value)) return false;
        map->value = value;
        return true;
    }

    if (!make_Map(code, key, value, hash->table[index], &map)) return false;

    hash->table[index] = map;

    return true;
}

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

struct prs_text {
    unsigned  size;   // size of buffer
    unsigned  limit;  // end of input
    char     *buffer;
};

static bool text_Extend(struct prs_text *text, const unsigned room) {
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

struct prs_buffer {
    FILE     *file;
    unsigned  cursor;
    ssize_t   read;
    size_t    allocated;
    char     *line;
};

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

struct prs_file {
    struct prs_input   base;
    const char        *filename;
    struct prs_buffer  buffer;
    struct prs_text    data;
    struct prs_cursor  cursor;
    struct prs_cursor  reach;
    struct prs_hash   *nodes;
    struct prs_hash   *predicates;
    struct prs_hash   *actions;
    struct prs_hash   *events;
};

static bool file_CurrentChar(PrsInput input, PrsChar* target) {
    struct prs_file *file = (struct prs_file *)input;

    unsigned point = file->cursor.text_inx;
    unsigned limit = file->data.limit;

    if (point >= limit) {
        if (0 >= buffer_GetLine(&file->buffer, &file->data))
            return false;
    }

    *target = file->data.buffer[point];

    return true;
}

static bool file_NextChar(PrsInput input) {
    struct prs_file *file = (struct prs_file *)input;

    unsigned point = file->cursor.text_inx + 1;
    unsigned limit = file->data.limit;

    if (point >= limit) {
        if (0 >= buffer_GetLine(&file->buffer, &file->data))
            return false;
    }

    file->cursor.text_inx = point;

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
    return hash_Find(file->nodes, name, (void**)target);
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

static bool malloc_release(void* value) {
    free(value);
    return true;
}

static bool file_AddName(PrsInput input, PrsName name, PrsNode value) {
    struct prs_file *file = (struct prs_file *)input;
    return hash_Replace(file->actions, name, value, noop_release);
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

    /* */
    result->filename    = strdup(filename);
    result->buffer.file = fopen(filename, "r");

    hash_Make((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->nodes);

    hash_Make((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->predicates);

    hash_Make((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->actions);

    hash_Make((Hashcode) encode_name,
              (Matchkey) compare_name,
              100,
              &result->events);

    *target = (PrsInput) result;

    return true;
}

extern bool copper_vm(PrsNode start, PrsInput input) {
    PrsCursor at;

    inline bool hold()    { return input->fetch(input, &at); }
    inline bool reset()   { return input->reset(input, at);  }
    inline bool check()   { return true;                     }
    inline bool next()    { return input->next(input);       }

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
        return true;
    }

    inline bool event(PrsEvent action) {
        return true;
    }

    inline bool prs_and() {
        hold();
        if (!copper_vm(start->arg.pair->left, input))  {
            reset();
            return false;
        }
        if (!copper_vm(start->arg.pair->right, input)) {
            reset();
            return false;
        }
        check();
        return true;
    }

    inline bool prs_choice() {
        hold();
        if (copper_vm(start->arg.pair->left, input)) {
            check();
            return true;
        }
        reset();
        if (copper_vm(start->arg.pair->right, input)) {
            check();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_zero_plus() {
        hold();
        while (copper_vm(start->arg.node, input)) {
            hold();
        }
        reset();
        return true;
    }

    inline bool prs_one_plus() {
        bool result = false;
        hold();
        while (copper_vm(start->arg.node, input)) {
            result = true;
            hold();
        }
        reset();
        return result;
    }

    inline bool prs_optional() {
        hold();
        if (copper_vm(start->arg.node, input)) {
            check();
            return true;
        }
        reset();
        return true;
    }

    inline bool prs_assert_true() {
        hold();
        if (copper_vm(start->arg.node, input)) {
            reset();
            return true;
        }
        reset();
        return false;
    }

    inline bool prs_assert_false() {
        hold();
        if (copper_vm(start->arg.node, input)) {
            reset();
            return false;
        }
        reset();
        return true;
    }

    inline bool prs_char() {
        PrsChar chr = 0;
        if (!current(&chr)) {
            return false;
        }
        if (chr != start->arg.letter) {
            return false;
        }
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
        printf("running %s", (char*)start->arg.name);
        return copper_vm(value, input);
    }

    inline bool prs_dot() {
        PrsChar chr;
        if (!current(&chr)) return false;
        next();
        return true;
    }

    inline bool prs_string() {
        PrsString string = start->arg.string;
        unsigned  length = string->length;
        PrsChar   chr  = 0;
        unsigned  inx  = 0;

        hold();
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
        return input->fetch(input, &input->state.begin);
    }

    inline bool prs_end() {
        return input->fetch(input, &input->state.end);
    }

    inline bool prs_predicate() {
        return predicate(start->arg.name);
    }

    inline bool prs_action() {
        return action(start->arg.name);
    }

    inline bool prs_event() {
        return event(start->arg.event);
    }

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

