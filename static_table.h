/*-*- mode: c;-*-*/
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

static inline unsigned long stable_NCode(const char* name, size_t length) {
    unsigned long result = 5381;

    if (0 >= length) return result;

    for ( ; *name ; ++name ) {
        int val = *name;
        result = ((result << 5) + result) + val;
        --length;
        if (0 >= length) return result;
    }

    return result;
}

static inline unsigned long stable_Code(const char* name) {
    size_t length = strlen(name);
    return stable_NCode(name, length);
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

static inline bool stable_NFind(StaticTable table, const char* name, size_t length, StaticValue *target) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;
    if (!table->entries) return false;

    unsigned long code   = stable_NCode(name, length);
    unsigned      index  = code % table->count;

    StaticMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->code != code) continue;
        if (at->length != length) continue;
        if (0 != strncmp(at->name, name, length)) continue;
        *target = at->value;
        return true;
    }

    return false;
}

static inline bool stable_Find(StaticTable table, const char* name, StaticValue *target) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;
    if (!table->entries) return false;

    size_t length = strlen(name);

    return stable_NFind(table, name, length, target);
}

static inline bool stable_NReplace(StaticTable table, const char* name,  size_t length, StaticValue value) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;

    unsigned long code   = stable_NCode(name, length);
    unsigned      index  = code % table->count;

    StaticMap at = table->column[index];

    for ( ; at ; at = at->next) {
        if (at->code != code) continue;
        if (at->length != length) continue;
        if (0 != strncmp(at->name, name, length)) continue;
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

static inline bool stable_Replace(StaticTable table, const char* name, StaticValue value) {
    if (!table)          return false;
    if (!name)           return false;
    if (!table->count)   return false;

    size_t length = strlen(name);

    return stable_NReplace(table, name, length, value);
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
