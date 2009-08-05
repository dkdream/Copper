/* A recursive-descent parser generated by copper 1.1.0 */

/*-*- mode: c;-*-*/
#ifndef YY_HEADER
#define YY_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _yyclass  YYClass;
typedef struct _yystack  YYStack;
typedef struct _yystate  YYState;
typedef struct _yythunk  YYThunk;
typedef struct _yycache  YYCache;
typedef struct _yycache_node YYCacheNode;

typedef int  (*YYRule)(YYClass* yySelf, YYStack* stack);
typedef void (*YYAction)(YYClass* yySelf, YYThunk thunk);

typedef int  (*YYInput)(YYClass* yySelf, char* buffer, int max_size);
typedef void (*YYSave)(YYClass* yySelf, YYState* state);
typedef void (*YYRestore)(YYClass* yySelf, YYState* state);
typedef void (*YYBegin)(YYClass* yySelf, YYStack* stack);
typedef void (*YYEnd)(YYClass* yySelf, YYStack* stack);
typedef void (*YYDebug)(YYClass* yySelf, const char* format, ...);
typedef void (*YYMark)(YYClass* yySelf, const char* name);
typedef void (*YYCollect)(YYClass* yySelf, const char* name);

typedef int  (*YYApply)(YYClass* yySelf,
                        YYStack* yystack,
                        YYRule function,
                        const char* name);

#ifndef YYSTYPE
#define YYSTYPE	int
#endif

struct  _yyclass {
    char *    buf;
    int	      buflen;
    int	      pos;
    int	      limit;
    char *    text;
    int	      textlen;
    int	      begin;
    int	      end;
    int	      textmax;

    /* action list */
    YYThunk * thunks;
    int	      thunkslen;
    int       thunkpos;

    /* data stack */
    YYSTYPE   result;
    int       frame;
    int       valslen;
    YYSTYPE * vals;

    /**/
    YYCache * cache;

    /*method table*/
    YYInput   input_;
    YYSave    save_;
    YYRestore restore_;
    YYBegin   begin_;
    YYEnd     end_;
    YYDebug   debug_;
    YYMark    mark_;
    YYCollect collect_;
    YYApply   apply_;
};

struct _yystate {
    int pos;
    int thunkpos;
};

struct _yystack {
    YYStack*    next;
    const char* name;
    YYRule      current;
    YYState     begin;
};

struct _yythunk {
    const char* name;
    int         begin;
    int         end;
    YYState     state;
    YYAction    action;
    int         argument;
};

struct _yycache_node {
    YYCacheNode* next;
    int     location;
    void*   function;
    int     result;
    int     pos;
    int     count;
    YYThunk thunk[0];
};

struct _yycache {
    int size;
    YYCacheNode* slot[0];
};

extern void yyInit(YYClass* yySelf);
extern void yyClearCache(YYClass* yySelf);
extern int yyParseFrom(YYClass* yySelf, YYRule yystart, const char* name);
extern void yyCommit(YYClass* yySelf);
extern int yyAccept(YYClass* yySelf, YYStack* yystack);
#endif
/*-- end of header --*/



/* rules */
extern int yy_end_of_line (YYClass* yySelf, YYStack* yystack);
extern int yy_comment (YYClass* yySelf, YYStack* yystack);
extern int yy_space (YYClass* yySelf, YYStack* yystack);
extern int yy_braces (YYClass* yySelf, YYStack* yystack);
extern int yy_range (YYClass* yySelf, YYStack* yystack);
extern int yy_char (YYClass* yySelf, YYStack* yystack);
extern int yy_directive (YYClass* yySelf, YYStack* yystack);
extern int yy_COLLECT (YYClass* yySelf, YYStack* yystack);
extern int yy_MARK (YYClass* yySelf, YYStack* yystack);
extern int yy_END (YYClass* yySelf, YYStack* yystack);
extern int yy_BEGIN (YYClass* yySelf, YYStack* yystack);
extern int yy_DOT (YYClass* yySelf, YYStack* yystack);
extern int yy_class (YYClass* yySelf, YYStack* yystack);
extern int yy_literal (YYClass* yySelf, YYStack* yystack);
extern int yy_CLOSE (YYClass* yySelf, YYStack* yystack);
extern int yy_OPEN (YYClass* yySelf, YYStack* yystack);
extern int yy_COLON (YYClass* yySelf, YYStack* yystack);
extern int yy_PLUS (YYClass* yySelf, YYStack* yystack);
extern int yy_STAR (YYClass* yySelf, YYStack* yystack);
extern int yy_QUESTION (YYClass* yySelf, YYStack* yystack);
extern int yy_primary (YYClass* yySelf, YYStack* yystack);
extern int yy_NOT (YYClass* yySelf, YYStack* yystack);
extern int yy_suffix (YYClass* yySelf, YYStack* yystack);
extern int yy_AND (YYClass* yySelf, YYStack* yystack);
extern int yy_prefix (YYClass* yySelf, YYStack* yystack);
extern int yy_BAR (YYClass* yySelf, YYStack* yystack);
extern int yy_sequence (YYClass* yySelf, YYStack* yystack);
extern int yy_SEMICOLON (YYClass* yySelf, YYStack* yystack);
extern int yy_begin (YYClass* yySelf, YYStack* yystack);
extern int yy_end (YYClass* yySelf, YYStack* yystack);
extern int yy_expression (YYClass* yySelf, YYStack* yystack);
extern int yy_EQUAL (YYClass* yySelf, YYStack* yystack);
extern int yy_action (YYClass* yySelf, YYStack* yystack);
extern int yy_macro (YYClass* yySelf, YYStack* yystack);
extern int yy_identifier (YYClass* yySelf, YYStack* yystack);
extern int yy_RPERCENT (YYClass* yySelf, YYStack* yystack);
extern int yy_end_of_file (YYClass* yySelf, YYStack* yystack);
extern int yy_trailer (YYClass* yySelf, YYStack* yystack);
extern int yy_define_rule (YYClass* yySelf, YYStack* yystack);
extern int yy_define_macro (YYClass* yySelf, YYStack* yystack);
extern int yy_exportation (YYClass* yySelf, YYStack* yystack);
extern int yy_declaration (YYClass* yySelf, YYStack* yystack);
extern int yy_heading (YYClass* yySelf, YYStack* yystack);
extern int yy__ (YYClass* yySelf, YYStack* yystack);
extern int yy_grammar (YYClass* yySelf, YYStack* yystack);
