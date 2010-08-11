/*-*- mode: c;-*-*/
#if !defined(_copper_vm_h_)
#define _copper_vm_h_

#include <stdbool.h>
#include <error.h>

#define COPPER_MAJOR    1
#define COPPER_MINOR    1
#define COPPER_LEVEL    0

enum prs_operator {
    prs_Apply,       // @name - an named event
    prs_AssertFalse, // e !
    prs_AssertTrue,  // e &
    prs_Begin,       // set state.begin
    prs_Choice,      // e1 e2 /
    prs_End,         // set state.end
    prs_MatchChar,   // 'chr
    prs_MatchDot,    // .
    prs_MatchName,   // @name
    prs_MatchRange,  // begin-end
    prs_MatchSet,    // [...]
    prs_MatchText,   // "..."
    prs_OneOrMore,   // e +
    prs_Predicate,   // %predicate
    prs_Sequence,    // e1 e2 ;
    prs_Thunk,       // { } - an unnamed event
    prs_ZeroOrMore,  // e *
    prs_ZeroOrOne,   // e ?
    prs_Void        // -nothing-
};

static inline const char* oper2name(enum prs_operator oper) {
    switch (oper) {
    case prs_Apply:       return "prs_Apply";       // @name - an named event
    case prs_AssertFalse: return "prs_AssertFalse"; // e !
    case prs_AssertTrue:  return "prs_AssertTrue";  // e &
    case prs_Begin:       return "prs_Begin";       // set state.begin
    case prs_Choice:      return "prs_Choice";      // e1 e2 /
    case prs_End:         return "prs_End";         // set state.end
    case prs_MatchChar:   return "prs_MatchChar";   // 'chr
    case prs_MatchDot:    return "prs_MatchDot";    // .
    case prs_MatchName:   return "prs_MatchName";   // @name
    case prs_MatchRange:  return "prs_MatchRange";  // begin-end
    case prs_MatchSet:    return "prs_MatchSet";    // [...]
    case prs_MatchText:   return "prs_MatchText";   // "..."
    case prs_OneOrMore:   return "prs_OneOrMore";   // e +
    case prs_Predicate:   return "prs_Predicate";   // %predicate
    case prs_Sequence:    return "prs_Sequence";    // e1 e2 ;
    case prs_Thunk:       return "prs_Thunk";       // { } - an unnamed event
    case prs_ZeroOrMore:  return "prs_ZeroOrMore";  // e *
    case prs_ZeroOrOne:   return "prs_ZeroOrOne";   // e ?
    case prs_Void:        break; // -nothing-
    }
    return "prs-unknown";
}

typedef enum prs_operator PrsOperator;

/* parsing structure */
typedef struct prs_input* PrsInput;

/* parse node arguments */
typedef unsigned char      PrsChar;
typedef struct prs_string *PrsString;
typedef struct prs_range  *PrsRange;
typedef struct prs_set    *PrsSet;
typedef const  char       *PrsName;
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
    unsigned    length;
    const char* start; // this is NOT a zero terminated string
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

struct prs_label {
    PrsEvent  function;
    PrsName   name;
};

typedef struct prs_label PrsLabel;

struct prs_thread {
    PrsThread next;
    PrsCursor at;
    PrsLabel  label;
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
        PrsLabel *label;
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
typedef bool (*MoreData)(PrsInput);                               // add more text to the data buffer

typedef bool (*FindNode)(PrsInput, PrsName, PrsNode*);            // find the PrsNode labelled name
typedef bool (*AddName)(PrsInput, PrsName, PrsNode);              // add a PrsNode to label name

typedef bool (*FindPredicate)(PrsInput, PrsName, PrsPredicate*);  // find the PrsPredicate labelled name
typedef bool (*FindEvent)(PrsInput, PrsName, PrsEvent*);          // find the PrsEvent labelled name

struct prs_input {
    /* call-backs */
    MoreData      more;
    FindNode      node;
    AddName       attach;
    FindPredicate predicate;
    FindEvent     event;

    /* data */
    PrsText   data;
    PrsCursor cursor;
    PrsCursor reach;

    PrsCache cache;
    PrsQueue queue;
    PrsState slice;
};

extern bool cu_InputInit(PrsInput input, unsigned cacheSize);
extern bool cu_Parse(char* name, PrsInput input);
extern bool cu_AppendData(PrsInput input, const unsigned count, const char *src);
extern bool cu_RunQueue(PrsInput input);
extern bool cu_MarkedText(PrsInput input, PrsData *target);

extern unsigned cu_global_debug;
extern void     cu_debug(const char *filename, unsigned int linenum, const char *format, ...);
extern void     cu_error(const char *filename, unsigned int linenum, const char *format, ...);

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
#define CU_ERROR_AT(args...) cu_error(args)
#else
#define CU_ERROR_AT(args...) cu_noop()
#endif

#endif

