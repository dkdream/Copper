/*-*- mode: c;-*-*/
#if !defined(_copper_vm_h_)
#define _copper_vm_h_

#include <stdbool.h>
#include <error.h>

enum prs_operator {
    prs_Action,      // %action
    prs_AssertFalse, // e !
    prs_AssertTrue,  // e &
    prs_Begin,       // set state.begin
    prs_Choice,      // e1 e2 /
    prs_End,         // set state.end
    prs_Event,       // { }
    prs_MatchChar,   // 'chr
    prs_MatchDot,    // .
    prs_MatchName,   // @name
    prs_MatchRange,  // begin-end
    prs_MatchSet,    // [...]
    prs_MatchString, // "..."
    prs_OneOrMore,   // e +
    prs_Predicate,   // &predicate
    prs_Sequence,    // e1 e2 ;
    prs_ZeroOrMore,  // e *
    prs_ZeroOrOne,   // e ?
    prs_Void         // -nothing-
};

typedef enum prs_operator PrsOperator;

/* parsing structure */
typedef struct prs_input* PrsInput;

/* parse node arguments */
typedef unsigned char      PrsChar;
typedef struct prs_string *PrsString;
typedef struct prs_range  *PrsRange;
typedef struct prs_set    *PrsSet;
typedef void              *PrsName;
typedef struct prs_node   *PrsNode;
typedef struct prs_pair   *PrsPair;

/* parse event queue */
typedef struct prs_thread *PrsThread;
typedef struct prs_queue  *PrsQueue;

/* parsing cache node */
typedef struct prs_point *PrsPoint;
typedef struct prs_cache *PrsCache;

/* parsing data buffer */
struct prs_text {
    unsigned  size;   // size of buffer
    unsigned  limit;  // end of input
    char     *buffer;
};

typedef struct prs_text PrsText;

struct prs_data {
    unsigned length;
    const char* start;
};

typedef struct prs_data PrsData;

struct prs_string {
    unsigned length;
    PrsChar  text[];
};

struct prs_range {
    PrsChar begin;
    PrsChar end;
};

struct prs_set {
    const char *label;
    const unsigned char bitfield[];
};

struct prs_map {
    unsigned        code;
    void*           key;
    void*           value;
    struct prs_map *next;
};

struct prs_pair {
    PrsNode left;
    PrsNode right;
};

struct prs_cursor {
    unsigned line_number;
    unsigned char_offset;
    unsigned text_inx;
};

typedef struct prs_cursor PrsCursor;

struct prs_state {
    PrsCursor begin;
    PrsCursor end;
};

typedef struct prs_state PrsState;

/* user define event actions */
/* these are ONLY call after a sucessful parse completes */
typedef bool (*PrsEvent)(PrsInput, PrsCursor);

struct prs_thread {
    PrsThread next;
    PrsCursor at;
    PrsEvent  function;
};

struct prs_slice {
    PrsThread begin;
    PrsThread end;
};

typedef struct prs_slice PrsSlice;

struct prs_queue {
    PrsThread free_list;
    PrsThread begin;
    PrsThread end;
};

struct prs_point {
    PrsPoint  next;
    PrsNode   node;
    PrsState  cursor;
    bool      match;
    PrsSlice  segment;
};

struct prs_cache {
    PrsPoint free_list;
    unsigned size;
    PrsPoint table[];
};

struct prs_node {
    PrsSet      first;  // the first set of node
    PrsSet      follow; // the follow set of node
    PrsOperator oper;
    union prs_arg {
        PrsChar   letter;
        PrsString string;
        PrsRange  range;
        PrsSet    set;
        PrsName   name;
        PrsNode   node;
        PrsPair   pair;
        PrsEvent  event;
    } arg;
};

typedef bool (*PrsPredicate)(PrsInput);         // user defined predicate
typedef bool (*PrsFirstSet)(PrsInput, PrsSet*); // compute first set for user defined predicate/action

// do we need these ?
// if a predicate ALWAY return true then is it an action
typedef void (*PrsAction)(PrsInput);            // user defined parsing action

/* parsing structure call back */
typedef bool (*CurrentChar)(PrsInput, PrsChar*);                  // return the char at the cursor location
typedef bool (*NextChar)(PrsInput);                               // move the cursor by on char
typedef bool (*GetCursor)(PrsInput, PrsCursor*);                  // get the cursor location
typedef bool (*SetCursor)(PrsInput, PrsCursor);                   // set the cursor location
typedef bool (*FindNode)(PrsInput, PrsName, PrsNode*);            // find the PrsNode labelled name
typedef bool (*FindPredicate)(PrsInput, PrsName, PrsPredicate*);  // find the PrsPredicate labelled name
typedef bool (*FindAction)(PrsInput, PrsName, PrsAction*);        // find the PrsAction labelled name
typedef bool (*AddName)(PrsInput, PrsName, PrsNode);              // add a PrsNode to label name
typedef bool (*SetAction)(PrsInput, PrsName, PrsAction);          // assign the PrsAction to label name
typedef bool (*SetEvent)(PrsInput, PrsName, PrsEvent);            // assign the PrsEvent to label name
typedef bool (*SetPredicate)(PrsInput, PrsName, PrsPredicate);    // assign the PrsPredicate to label name

struct prs_input {
    /* call-backs */
    CurrentChar   current;
    NextChar      next;
    GetCursor     fetch;
    SetCursor     reset;
    FindNode      node;
    FindPredicate predicate;
    FindAction    action;    // do we need these ?
    AddName       attach;
    SetPredicate  set_p;
    SetAction     set_a;     // do we need these ?
    SetEvent      set_e;
    /* data */
    PrsText  data;
    PrsCache cache;
    PrsQueue queue;
    PrsState slice;
};

extern bool text_Extend(struct prs_text *text, const unsigned room);
extern bool input_Parse(char* name, PrsInput input);
extern bool input_Text(PrsInput input, PrsData *target);

#endif

