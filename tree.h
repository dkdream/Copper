/* Copyright (c) 2007 by Ian Piumarta
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 *
 * Last edited: 2007-05-15 10:32:05 by piumarta on emilia
 */

#include <stdio.h>

enum { Unknown= 0, Rule, Macro, Variable, Name, Dot, Character, String, Class, Action, Predicate, Mark, Alternate, Sequence, PeekFor, PeekNot, Query, Star, Plus };

enum {
  RuleUsed	= 1<<0,
  RuleReached	= 1<<1,
  LeftRecursion = 1<<2,
  RuleDeclared  = 1<<3,
  RuleExported  = 1<<4,
};

typedef union Node Node;

struct Rule	 { int type;  Node *next;   char *name;	 char *scope; Node *variables;  Node *expression;  int id;  int flags; char* begin; char* end; };
struct Macro     { int type;  Node *next;   char *name;  char *value;                                                   };
struct Variable	 { int type;  Node *next;   char *name;  Node *value;  int offset;					};
struct Name	 { int type;  Node *next;   Node *rule;  Node *variable;						};
struct Dot	 { int type;  Node *next;										};
struct Character { int type;  Node *next;   char *value;								};
struct String	 { int type;  Node *next;   char *value;								};
struct Class	 { int type;  Node *next;   unsigned char *value;							};
struct Action	 { int type;  Node *next;   char *text;	  Node *list;  char *name;  Node *rule;				};
struct Predicate { int type;  Node *next;   char *text;									};
struct Mark      { int type;  Node *next;   char *text;									};
struct Alternate { int type;  Node *next;   Node *first;  Node *last;							};
struct Sequence	 { int type;  Node *next;   Node *first;  Node *last;							};
struct PeekFor	 { int type;  Node *next;   Node *element;								};
struct PeekNot	 { int type;  Node *next;   Node *element;								};
struct Query	 { int type;  Node *next;   Node *element;								};
struct Star	 { int type;  Node *next;   Node *element;								};
struct Plus	 { int type;  Node *next;   Node *element;								};
struct Any	 { int type;  Node *next;										};

union Node
{
    int			type;
    struct Rule		rule;
    struct Macro	macro;
    struct Variable	variable;
    struct Name		name;
    struct Dot		dot;
    struct Character	character;
    struct String	string;
    struct Class	cclass;
    struct Action	action;
    struct Predicate	predicate;
    struct Mark   	mark;
    struct Alternate	alternate;
    struct Sequence	sequence;
    struct PeekFor	peekFor;
    struct PeekNot	peekNot;
    struct Query	query;
    struct Star		star;
    struct Plus		plus;
    struct Any		any;
};

extern Node *actions;
extern Node *rules;
extern Node *macros;
extern Node *start;

extern int ruleCount;

extern FILE *output;
extern FILE *include;

extern Node *makeRule(char *name);
extern Node *findRule(char *name);
extern Node *beginRule(Node *rule);
extern void  Rule_setExpression(Node *rule, Node *expression);
extern Node *Rule_beToken(Node *rule);
extern Node *makeMacro(char *name);
extern Node *findMacro(char *name);
extern void  Macro_setValue(Node *macro, char *value);
extern Node *makeVariable(char *name);
extern Node *makeName(Node *rule);
extern Node *makeDot(void);
extern Node *makeCharacter(char *text);
extern Node *makeString(char *text);
extern Node *makeClass(char *text);
extern Node *makeAction(char *text);
extern Node *makePredicate(char *text);
extern Node *makeMark(char *text);
extern Node *makeAlternate(Node *e);
extern Node *Alternate_append(Node *e, Node *f);
extern Node *makeSequence(Node *e);
extern Node *Sequence_append(Node *e, Node *f);
extern Node *makePeekFor(Node *e);
extern Node *makePeekNot(Node *e);
extern Node *makeQuery(Node *e);
extern Node *makeStar(Node *e);
extern Node *makePlus(Node *e);
extern Node *push(Node *node);
extern Node *top(void);
extern Node *pop(void);

extern void Rule_compile_c_heading(FILE* ofile);
extern void Rule_compile_c_preamble(FILE* ofile);
extern void Rule_compile_c_declare(FILE* ofile, Node *node);
extern void Rule_compile_c(Node *node, int export_all);
extern void Rule_compile_c_footing(FILE* ofile);

extern void Node_print(Node *node);
extern void Rule_print(Node *node);

extern void Rule_compile_vm(FILE* ofile);
