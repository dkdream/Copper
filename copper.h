/*-*- mode: c;-*-*/
#if !defined(_copper_vm_h_)
#define _copper_vm_h_

#include <stdio.h>
#include <stdbool.h>
#include <error.h>

#define COPPER_MAJOR    1
#define COPPER_MINOR    1
#define COPPER_LEVEL    0

/* parsing structure */
typedef struct copper* Copper;

/* a location in the current input */
struct cu_cursor {
    unsigned line_number; // line number in the current file
    unsigned char_offset; // char offset in the current line
    unsigned text_inx;    // char offset in the current buffer (copper->data)
};

typedef struct cu_cursor CuCursor;

/* user define event actions */
/* these are ONLY call after a sucessful parse completes */
typedef bool (*CuEvent)(Copper, CuCursor);

// if a predicate ALWAY return true then is it an action
// these are call during the parse to check if every can proceed
// example of use:
// name = < ([a-z][A-Z])+ > !%keyword
typedef bool (*CuPredicate)(Copper); // user defined predicate

enum cu_operator {
    cu_Apply,       // @name - an named event
    cu_AssertFalse, // e !
    cu_AssertTrue,  // e &
    cu_Begin,       // set state.begin
    cu_Choice,      // e1 e2 /
    cu_End,         // set state.end
    cu_MatchChar,   // 'chr
    cu_MatchDot,    // .
    cu_MatchName,   // name
    cu_MatchRange,  // begin-end
    cu_MatchSet,    // [...]
    cu_MatchText,   // "..."
    cu_OneOrMore,   // e +
    cu_Predicate,   // %predicate
    cu_Sequence,    // e1 e2 ;
    cu_Thunk,       // {...} - an unnamed event
    cu_ZeroOrMore,  // e *
    cu_ZeroOrOne,   // e ?
    cu_Void        // -nothing-
};

typedef enum cu_operator CuOperator;

static inline const char* oper2name(const CuOperator oper) {
    switch (oper) {
    case cu_Apply:       return "cu_Apply";       // @name - an named event
    case cu_AssertFalse: return "cu_AssertFalse"; // e !
    case cu_AssertTrue:  return "cu_AssertTrue";  // e &
    case cu_Begin:       return "cu_Begin";       // set state.begin
    case cu_Choice:      return "cu_Choice";      // e1 e2 /
    case cu_End:         return "cu_End";         // set state.end
    case cu_MatchChar:   return "cu_MatchChar";   // 'chr
    case cu_MatchDot:    return "cu_MatchDot";    // dot
    case cu_MatchName:   return "cu_MatchName";   // name
    case cu_MatchRange:  return "cu_MatchRange";  // begin-end
    case cu_MatchSet:    return "cu_MatchSet";    // [...]
    case cu_MatchText:   return "cu_MatchText";   // "..."
    case cu_OneOrMore:   return "cu_OneOrMore";   // e +
    case cu_Predicate:   return "cu_Predicate";   // %predicate
    case cu_Sequence:    return "cu_Sequence";    // e1 e2 ;
    case cu_Thunk:       return "cu_Thunk";       // { } - an unnamed event
    case cu_ZeroOrMore:  return "cu_ZeroOrMore";  // e *
    case cu_ZeroOrOne:   return "cu_ZeroOrOne";   // e ?
    case cu_Void:        break; // -nothing-
    }
    return "prs-unknown";
}

enum cu_first_type {
    pft_opaque,      // %predicate, e !, dot
    pft_fixed,       // "...", 'chr, [...], begin-end
    pft_transparent, // e *, e ?
    pft_event        // @name, set state.begin, set state.end {...}
};

typedef enum cu_first_type CuFirstType;

static inline const char* first2name(const CuFirstType first) {
    switch (first) {
    case pft_event:       return "pft_event";
    case pft_opaque:      return "pft_opaque";
    case pft_transparent: return "pft_transparent";
    case pft_fixed:       return "pft_fixed";
    }
    return "pft_opaque";
}

static inline CuFirstType inWithout(const CuFirstType child)
{
    // T(f!) = e, T(o&) = e, T(t!) = E, T(e!) = e
    return pft_event;
}

static inline CuFirstType inRequired(const CuFirstType child)
{
    // T(f&) = f, T(o&) = o, T(t&) = t, T(e&) = e
    // T(f+) = f, T(o+) = o, T(t+) = t, T(e+) = ERROR
    return child;
}

static inline CuFirstType inOptional(const CuFirstType child)
{
    // T(f?) = t, T(o?) = o, T(t?) = t, T(e?) = e
    // T(f*) = t, T(o*) = o, T(t*) = t, T(e+) = ERROR

     switch (child) {
     case pft_fixed:       return pft_transparent;
     case pft_transparent: return pft_transparent;
     case pft_opaque:      return pft_opaque;
     case pft_event:       return pft_event;
     }
     return pft_opaque;
}

static inline CuFirstType inChoice(const CuFirstType before,
                                    const CuFirstType after)
{
    // T(ff/) = f, T(fo/) = o, T(ft/) = t, T(fe/) = e
    // T(of/) = o, T(oo/) = o, T(ot/) = t, T(oe/) = o
    // T(tf/) = t, T(to/) = t, T(tt/) = t, T(te/) = t
    // T(ef/) = e, T(eo/) = o, T(et/) = t, T(ee/) = e

    if (pft_transparent == before) return pft_transparent;
    if (pft_transparent == after)  return pft_transparent;
    if (pft_opaque == before)      return pft_opaque;
    if (pft_opaque == after)       return pft_opaque;
    if (pft_event == before)       return pft_event;
    if (pft_event == after)        return pft_event;

    return pft_fixed;
}

static inline CuFirstType inSequence(const CuFirstType before,
                                      const CuFirstType after)
{
    // T(ff;) = f, T(fo;) = f, T(ft;) = f, T(fe;) = f
    // T(of;) = o, T(oo;) = o, T(ot;) = o, T(oe;) = o
    // T(tf;) = f, T(to;) = o, T(tt;) = t, T(te;) = e
    // T(ef;) = f, T(eo;) = o, T(et;) = t, T(ee;) = e

    switch (before) {
    case pft_transparent:
        return after;

    case pft_event:
        if (pft_transparent == after) return before;
        return after;

    case pft_opaque:
    case pft_fixed:
        return before;
    }

    return pft_opaque;
}

/* */
typedef struct cu_firstset  *CuFirstSet;
typedef struct cu_firstlist *CuFirstList;
typedef struct cu_metafirst *CuMetaFirst;
typedef struct cu_metaset   *CuMetaSet;

/* parse node arguments */
typedef unsigned char      CuChar;
typedef struct cu_string *CuString;
typedef struct cu_range  *CuRange;
typedef struct cu_set    *CuSet;
typedef const  char       *CuName;
typedef struct cu_node   *CuNode;
typedef struct cu_pair   *CuPair;
/* parse event queue */
typedef struct cu_thread *CuThread;
typedef struct cu_queue  *CuQueue;

/* parsing cache node */
typedef struct cu_point *CuPoint;
typedef struct cu_cache *CuCache;
typedef struct cu_tree  *CuTree;

struct cu_firstset {
    const unsigned char bitfield[32];
};

struct cu_firstlist {
    const unsigned count;
    const char* names[];
};

/* parsing data buffer */
struct cu_text {
    unsigned  size;   // size of buffer
    unsigned  limit;  // end of input
    char     *buffer;
};

typedef struct cu_text CuText;

struct cu_data {
    unsigned    length;
    const char* start; // this is NOT a zero terminated string
};

typedef struct cu_data CuData;

struct cu_string {
    unsigned length;
    CuChar  text[];
};

struct cu_range {
    CuChar begin;
    CuChar end;
};

struct cu_set {
    const char *label;
    const unsigned char bitfield[];
};

struct cu_pair {
    CuNode left;
    CuNode right;
};

struct cu_state {
    const char* rule;
    CuCursor   begin;
    CuCursor   end;
};

typedef struct cu_state CuState;

struct cu_label {
    CuEvent  function;
    CuName   name;
};

typedef struct cu_label CuLabel;

struct cu_thread {
    CuThread   next;
    const char* rule;
    CuCursor   at;
    CuLabel    label;
};

struct cu_slice {
    CuThread begin;
    CuThread end;
};

typedef struct cu_slice CuSlice;

struct cu_queue {
    CuThread free_list;
    CuThread begin;
    CuThread end;
};

struct cu_point {
    CuPoint next;
    CuNode  node;
    unsigned offset;
};

struct cu_cache {
    CuPoint free_list;
    unsigned size;
    CuPoint table[];
};

struct cu_tree {
    const char* name;
    CuNode     node;
    CuTree     left;
    CuTree     right;
};

struct cu_metaset {
    unsigned char bitfield[32];
};

struct cu_metafirst {
    CuMetaFirst   next;
    bool           done;
    CuNode        node;
    CuFirstType   type;
    CuMetaSet     first;
    CuMetaFirst   left;
    CuMetaFirst   right;
};

struct cu_node {
    CuFirstType type;
    CuFirstSet  first;    // the static first set of node (if known)
    CuFirstList start;    // the static list of name that start this note (if any)
    CuMetaFirst metadata; // dynamically allocated state use by the engine.
    CuName      label;    // the compiler assigned label (for debugging)
    CuOperator  oper;
    union cu_arg {
        CuChar   letter;
        CuString string;
        CuRange  range;
        CuSet    set;
        CuName   name;
        CuNode   node;
        CuPair   pair;
        CuEvent  event;
        CuLabel *label;
    } arg;
};


// do we need these ?
//typedef void (*CuAction)(Copper);            // user defined parsing action

/* parsing structure call back */
typedef bool (*MoreData)(Copper);                             // add more text to the data buffer

typedef bool (*FindNode)(Copper, CuName, CuNode*);            // find the CuNode labelled name
typedef bool (*AddName)(Copper, CuName, CuNode);              // add a CuNode to label name

typedef bool (*FindPredicate)(Copper, CuName, CuPredicate*);  // find the CuPredicate labelled name
typedef bool (*FindEvent)(Copper, CuName, CuEvent*);          // find the CuEvent labelled name

struct copper {
    /* call-backs */
    MoreData      more;      // add more text to the data buffer
    FindNode      node;      // find the CuNode for rule name
    AddName       attach;    // add CuNode as rule name
    FindPredicate predicate; // find CuPredicate by name
    FindEvent     event;     // find CuEvent by name

    /* data */
    CuText   data;   // the text in the current parse phase
    CuCursor cursor; // the location in the current parse phase
    CuCursor reach;  // the maximum location reached in the current parse phase

    /* parse phase markers */
    unsigned begin_inx; // the char offset when the last begin event was added
    unsigned end_inx;   // the char offset when the last end event was added

    CuCache cache;   // parser state cache (stores parse failures: (node, text_inx))
    CuTree  map;     // map rule name to first-set
    CuQueue queue;   // the current event queue
    CuState context; // the current context while the event queue is running
};

extern bool cu_InputInit(Copper input, unsigned cacheSize); // initials the copper parser
extern bool cu_AddName(Copper input, CuName, CuNode);
extern bool cu_FillMetadata(Copper input);
extern bool cu_Parse(const char* name, Copper input);
extern bool cu_AppendData(Copper input, const unsigned count, const char *src);
extern bool cu_RunQueue(Copper input);
extern bool cu_MarkedText(Copper input, CuData *target);
extern void cu_SyntaxError(FILE* error, Copper cu_input, const char* filename);

extern unsigned cu_global_debug;
extern void     cu_debug(const char *filename, unsigned int linenum, const char *format, ...);
extern void     cu_error(const char *filename, unsigned int linenum, const char *format, ...);
extern void     cu_error_part(const char *format, ...);

static inline void cu_noop() __attribute__((always_inline));
static inline void cu_noop() { return; }

#if 1
#define CU_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) cu_debug(__FILE__,  __LINE__, args); })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) arg; })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ERROR(args...) cu_error(__FILE__,  __LINE__, args)
#else
#define CU_ERROR(args...) cu_noop()
#endif

#if 1
#define CU_ERROR_PART(args...) cu_error_part(args)
#else
#define CU_ERROR_PART(args...) cu_noop()
#endif

#if 1
#define CU_ERROR_AT(args...) cu_error(args)
#else
#define CU_ERROR_AT(args...) cu_noop()
#endif

#endif

