/*-*- mode: c;-*-*/
#if !defined(_static_table_h_)
#define _static_table_h_
/***************************
 **
 ** Purpose
 **   a non-gc hash, this is used by
 **   the parser to store
 **     o. (name -> node)  maps
 **     o. (name -> event) maps
 **     o. (name -> predicate) maps
 **   all these maps are static after parser init.
 **/


/* */
#include <string.h>
#include <stdlib.h>

/* */
typedef void* StaticValue;
typedef struct static_map   *StaticMap;
typedef struct static_table *StaticTable;

struct static_map {
    unsigned long code;
    size_t        length;
    const char   *name;
    StaticValue   value;
    StaticMap     next;
};

struct static_table {
    unsigned   entries; // number of entries
    unsigned   count;   // number of columns
    StaticMap *column;
};

static inline unsigned long stable_Code(const char* name) {
    unsigned long result = 5381;

    for ( ; *name ; ++name ) {
        int val = *name;
        result = ((result << 5) + result) + val;
    }

    return result;
}

static inline bool stable_Init(unsigned count, StaticTable table) {
    if (!table) return false;

    memset(table, 0, sizeof(struct static_table));

    StaticMap *columns = malloc(sizeof(StaticMap) * count);

    if (!columns) return false;

    memset(columns, 0, sizeof(StaticMap) * count);

    table->count  = count;
    table->column = columns;

    return true;
}

static inline bool stable_Find(StaticTable table, const char* name, StaticValue *target) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;
    if (!table->entries) return false;

    unsigned long code   = stable_Code(name);
    unsigned      index  = code % table->count;
    size_t        length = strlen(name);

    StaticMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->code != code) continue;
        if (at->length != length) continue;
        if (0 != strcmp(at->name, name)) continue;
        *target = at->value;
        return true;
    }

    return false;
}

static inline bool stable_Replace(StaticTable table, const char* name, StaticValue value) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;

    unsigned long code   = stable_Code(name);
    unsigned      index  = code % table->count;
    size_t        length = strlen(name);

    StaticMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->code != code) continue;
        if (at->length != length) continue;
        if (0 != strcmp(at->name, name)) continue;
        at->value = value;
        return true;
    }

    at = malloc(sizeof(struct static_map));

    if (!at) return false;

    memset(at, 0, sizeof(struct static_map));

    at->code   = code;
    at->length = length;
    at->name   = name;
    at->value  = value;
    at->next   = table->column[index];

    table->column[index]  = at;
    table->entries       += 1;

    return true;
}

static inline void stable_Release(StaticTable table) {
    if (!table) return;

    unsigned index = table->count;

    for ( ; index-- ; ) {
        StaticMap   at = table->column[index];
        StaticMap next = 0;
        for ( ; at ; at = next) {
            next = at->next;
            free(at);
        }
    }

    free(table->column);

    memset(table, 0, sizeof(struct static_table));
}


/***************************
 ** end of file
 **************************/
#endif
