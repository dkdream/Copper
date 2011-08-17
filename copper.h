/*-*- mode: c;-*-*/
#if !defined(_copper_vm_h_)
#define _copper_vm_h_

#include <stdio.h>
#include <stdbool.h>
#include <error.h>

/* parsing structure */
typedef struct copper*   Copper;
typedef struct cu_cursor CuCursor;
typedef enum   cu_signal CuSignal;
typedef struct cu_frame* CuFrame;

/**
    user define event actions are ONLY call after a
    path is found (cu_FoundPath) when parse completes
    and the user calls cu_RunQueue. if the events are
    use to construct a tree then the copper object must
    be extended to include the nessary data structures.

    if an event return false then the RunQueue stops on
    that event.
**/
typedef bool (*CuEvent)(Copper, CuCursor);

/**
   a predicate is called during the cu_Event phase to check if we can
   proceed.  they can be used to improve proformence or check dynamic
   parts of the language.  predicates can not consume input they can
   only check if its is valid.

   example of use:
        name = !%keyword < ([a-z][A-Z])+ >

   in the example the lookup ahead set for this rule is [a-z][A-Z]
   because a predicate is treated as transparent (just like actions)

   a predicate can look as far ahead in the input as the use allows.
   (within or across files)

   if a predicate ALWAY return true then it is (like) an action but
   is executed during the parse phase.
**/
typedef CuSignal (*CuPredicate)(Copper, CuFrame); // user defined predicate

enum cu_signal {
    cu_NeedData,
    cu_FoundPath,
    cu_NoPath,
    cu_Error
};

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
    cu_ZeroOrMore,  // e *
    cu_ZeroOrOne,   // e ?
    cu_Void        // -nothing-
};

enum cu_phase {
    cu_One,
    cu_Two,
    cu_Three,
    cu_Four
};

enum cu_first_type {
    pft_opaque,      // %predicate, e !, dot
    pft_fixed,       // "...", 'chr, [...], begin-end
    pft_transparent, // e *, e ?
    pft_event        // @name, set state.begin, set state.end {...}
};

typedef enum cu_operator   CuOperator;
typedef enum cu_phase      CuPhase;
typedef enum cu_first_type CuFirstType;

/* */
typedef struct cu_firstset  *CuFirstSet;
typedef struct cu_firstlist *CuFirstList;
typedef struct cu_metafirst *CuMetaFirst;
typedef struct cu_metaset   *CuMetaSet;

/* parse node arguments */
typedef unsigned char     CuChar;
typedef struct cu_string *CuString;
typedef struct cu_range  *CuRange;
typedef struct cu_set    *CuSet;
typedef const  char      *CuName;
typedef struct cu_node   *CuNode;
typedef struct cu_pair   *CuPair;

/* parse event queue */
typedef struct cu_thread *CuThread;
typedef struct cu_queue  *CuQueue;

/* parse node stack */
typedef struct cu_stack *CuStack;

/* parsing cache node */
typedef struct cu_point *CuPoint;
typedef struct cu_cache *CuCache;
typedef struct cu_tree  *CuTree;

/* */
typedef struct cu_text  CuText;
typedef struct cu_data  CuData;
typedef struct cu_state CuState;
typedef struct cu_label CuLabel;
typedef struct cu_slice CuSlice;

/* a location in the current input */
struct cu_cursor {
    unsigned line_number; // line number in the current file
    unsigned char_offset; // char offset in the current line
    unsigned text_inx;    // char offset in the current buffer (copper->data)
};

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

/* a sub-string of the data buffer */
struct cu_data {
    long       length; // -1 == eof, 0 == no data
    const char* start; // this is NOT a zero terminated string
};

/* parsing node member (begin) */
/* CuChar   letter (cu_MatchChar)*/
/* CuString string (cu_MatchText) */
struct cu_string {
    unsigned length;
    CuChar  text[];
};

/* CuRange range */
struct cu_range {
    CuChar begin;
    CuChar end;
};

/* CuSet set (cu_MatchSet)*/
struct cu_set {
    const char *label;
    const unsigned char bitfield[];
};

/* CuName name (cu_Apply, cu_MatchName, cu_Predicate) */
/* CuNode node (cu_AssertTrue, cu_AssertFalse, cu_OneOrMore, cu_ZeroOrOne, cu_ZeroOrMore)*/
/* CuPair pair (cu_Choice, cu_Sequence)*/
struct cu_pair {
    CuNode left;
    CuNode right;
};
/* (end) parsing node member */

struct cu_state {
    const char* rule;
    CuCursor   begin;
    CuCursor   end;
};

/* */
struct cu_label {
    CuEvent function;
    CuName  name;
};

/* a member of the event queue */
struct cu_thread {
    CuThread   next;
    const char* rule;
    CuCursor   at;
    CuLabel    label;
};

/* a section of the event queue */
struct cu_slice {
    CuThread begin;
    CuThread end;
};

struct cu_queue {
    CuThread free_list;
    CuThread begin;
    CuThread end;
};

struct cu_frame {
    CuFrame     next;
    CuNode      node;
    CuPhase     phase;
    bool        last;
    const char* rulename;
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
    bool          done;
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
#if 0
        CuEvent  event;
        CuLabel *label;
#endif
    } arg;
};

typedef bool (*MoreData)(Copper);                             // add more text to the data buffer (old)

/* parsing structure call back */
typedef bool (*FindNode)(Copper, CuName, CuNode*);            // find the CuNode labelled name
typedef bool (*AddName)(Copper, CuName, CuNode);              // add a CuNode to label name

typedef bool (*FindPredicate)(Copper, CuName, CuPredicate*);  // find the CuPredicate labelled name
typedef bool (*FindEvent)(Copper, CuName, CuEvent*);          // find the CuEvent labelled name

struct copper {
    /* call-backs */
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

extern bool     cu_InputInit(Copper input, unsigned cacheSize); // initials the copper parser
extern bool     cu_AddName(Copper input, CuName, CuNode);
extern bool     cu_FillMetadata(Copper input);
extern bool     cu_Start(const char* name, Copper input);
extern CuSignal cu_Event(Copper input, const CuData data);
extern bool     cu_MarkedText(Copper input, CuData *target);
extern bool     cu_RunQueue(Copper input);
extern void     cu_SyntaxError(FILE* error, Copper cu_input, const char* filename);

extern unsigned cu_global_debug;
extern void     cu_debug(const char *filename, unsigned int linenum, const char *format, ...);
extern void     cu_error(const char *filename, unsigned int linenum, const char *format, ...);
extern void     cu_error_part(const char *format, ...);

#endif

