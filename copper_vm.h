/*-*- mode: c;-*-*/
#if !defined(_copper_vm_h_)
#define _copper_vm_h_

#include <stdbool.h>
#include <error.h>

enum prs_operator {
    prs_Sequence,    // e1 e2 ;
    prs_Choice,      // e1 e2 /
    prs_ZeroOrMore,  // e *
    prs_OneOrMore,   // e +
    prs_ZeroOrOne,   // e ?
    prs_AssertTrue,  // e &
    prs_AssertFalse, // e !
    prs_MatchDot,    // .
    prs_MatchChar,   // chr
    prs_MatchString, // "..."
    prs_MatchRange,  // begin-end
    prs_MatchSet,    // [...]
    prs_MatchName,   // @name
    prs_Predicate,   // &{ }
    prs_Action,      // %{ }
    prs_Event,       // { }
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
typedef void*              PrsMarker;

typedef unsigned char      PrsChar;
typedef struct prs_string* PrsString;
typedef struct prs_range*  PrsRange;
typedef struct prs_set*    PrsSet;
typedef void*              PrsName;
typedef struct prs_node*   PrsNode;
typedef struct prs_pair*   PrsPair;


typedef bool (*PrsPredicate)(PrsInput);
typedef void (*PrsAction)(PrsInput);
typedef bool (*PrsEvent)(PrsInput);

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

struct prs_node {
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



typedef bool (*CurrentChar)(PrsInput, PrsChar*);
typedef bool (*NextChar)(PrsInput);
typedef bool (*PushMark)(PrsInput);
typedef bool (*ReplaceMark)(PrsInput);
typedef bool (*PopMark)(PrsInput);
typedef bool (*SetChar)(PrsInput);
typedef bool (*FindNode)(PrsInput, PrsName, PrsNode*);
typedef bool (*FindPredicate)(PrsInput, PrsName, PrsPredicate*);
typedef bool (*FindAction)(PrsInput, PrsName, PrsAction*);
typedef bool (*PostEvent)(PrsInput, PrsEvent);
typedef bool (*AddName)(PrsInput, PrsName, PrsNode);
typedef bool (*SetPredicate)(PrsInput, PrsName, PrsPredicate);
typedef bool (*SetAction)(PrsInput, PrsName, PrsAction);
typedef bool (*SetEvent)(PrsInput, PrsName, PrsEvent);

struct prs_input {
    CurrentChar   current;
    NextChar      next;
    PushMark      push;
    ReplaceMark   replace;
    PopMark       pop;
    SetChar       reset;
    FindNode      node;
    FindPredicate predicate;
    FindAction    action;
    PostEvent     event;
    AddName       attach;
    SetPredicate  set_p;
    SetAction     set_a;
    SetEvent      set_e;
};

#endif

