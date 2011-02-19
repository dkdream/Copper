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

enum cu_first_type {
    pft_opaque,      // %predicate, e !, dot
    pft_fixed,       // "...", 'chr, [...], begin-end
    pft_transparent, // e *, e ?
    pft_event        // @name, set state.begin, set state.end {...}
};

typedef enum cu_first_type CuFirstType;

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
/* parse node stack */
typedef struct cu_thread *CuFrame;
typedef struct cu_queue  *CuStack;

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

struct cu_frame {
    CuFrame     next;
    const char* rulename;
    CuNode      start;
    unsigned    level;
    CuThread    mark;
    CuCursor    at;
};

struct cu_stack {
    CuFrame free_list;
    CuFrame top;
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
    CuStack  stack;  // the parse node stack
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

#endif

