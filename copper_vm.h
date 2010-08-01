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
    prs_MatchChar,   // chr
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

enum prs_result {
    prs_Error,
    prs_Falure,
    prs_Success
};

typedef enum prs_operator  PrsOperator;
typedef enum prs_result    PrsResult;

typedef struct prs_input*  PrsInput;

typedef unsigned char      PrsChar;
typedef struct prs_string* PrsString;
typedef struct prs_range*  PrsRange;
typedef struct prs_set*    PrsSet;
typedef void*              PrsName;
typedef struct prs_node*   PrsNode;
typedef struct prs_pair*   PrsPair;

typedef bool (*PrsPredicate)(PrsInput);
typedef bool (*PrsFirstSet)(PrsInput, PrsSet*);
typedef void (*PrsAction)(PrsInput);

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
typedef bool (*PrsEvent)(PrsInput, PrsState);

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
    FindAction    action;
    AddName       attach;
    SetPredicate  set_p;
    SetAction     set_a;
    SetEvent      set_e;
    /* data */
    PrsState state;

};

extern bool make_PrsFile(const char* filename, PrsInput *input);
extern bool copper_vm(PrsNode start, PrsInput input);

#endif

