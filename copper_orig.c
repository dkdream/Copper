
#include "copper.inc"

/*-*- mode: c;-*-*/
/*-- begin of preamble --*/
#ifndef YY_PREAMBLE
#define YY_PREAMBLE

#define YY_SEND(method, args...) yySelf->method(yySelf, ## args)

#define	YYACCEPT yyAccept(yySelf, yystack)

#ifndef YY_CACHE_THUNKS
#define YY_CACHE_THUNKS 10
#endif

static inline void yyStateError() {
    fprintf(stderr, "ERROR IN PARSER STATE");
    exit(1);
}

static inline void yyFreeList(YYCacheNode* node) {
    YYCacheNode* next = 0;
    for (; node; node = next) {
        next = node->next;
        free(node);
    }
}

static inline void yyCheckDo(YYClass* yySelf) {
    int end = yySelf->thunkpos;

    if (1 > end) return;

    int pos = 0;
    for (pos = 0;  pos < end;  ++pos) {
        YYThunk *thunk = &(yySelf->thunks[pos]);
        if (thunk->action) continue;
        YY_SEND(debug_,"     yyCheckDo [%d] error\n", pos);
        yyStateError();
        break;
    }
}

static inline void yyResizeDo(YYClass* yySelf, int count) {
    int marker = yySelf->thunkpos + count;
    int old    = yySelf->thunkslen;

    if (marker < old) return;

    while (marker >= yySelf->thunkslen) {
        yySelf->thunkslen *= 2;
    }

    YY_SEND(debug_,"     yyResizeDo %d -> %d\n", old, yySelf->thunkslen);

    if (yySelf->thunks) {
        yySelf->thunks = realloc(yySelf->thunks, sizeof(YYThunk) * yySelf->thunkslen);
    } else {
        yySelf->thunks = malloc(sizeof(YYThunk) * yySelf->thunkslen);
    }

    yyCheckDo(yySelf);
}

static inline void yyDo(YYClass* yySelf, const char* actionname, YYAction action, int arg, YYState state) {

    yyResizeDo(yySelf,1);

    YYThunk *thunk = &(yySelf->thunks[yySelf->thunkpos]);

    YY_SEND(debug_,"     yyDo [%d] %p %s\n", yySelf->thunkpos, action, actionname);

    thunk->name     = actionname;
    thunk->begin    = yySelf->begin;
    thunk->end      = yySelf->end;
    thunk->state    = state;
    thunk->action   = action;
    thunk->argument = arg;

    ++yySelf->thunkpos;
}

static inline void yyAddNode(YYClass* yySelf, YYStack *yystack, int result) {
    if (!yySelf->cache) return;

    const char* name = yystack->name;
    YYRule  function = yystack->current;
    YYState    start = yystack->begin;

    int count = 0;

    if (result) {
        // if no chars are consumed then
        // this may the result of a YYACCEPT not of an optional rule
        // -- skip it
        if (start.pos == yySelf->pos) return;

        // if no thunks were added then check the length
        if (start.thunkpos != yySelf->thunkpos) {
            count = yySelf->thunkpos - start.thunkpos;
            // wrong direction
            if (0 > count) return;
            // to many
            if (YY_CACHE_THUNKS < count) {
                YY_SEND(debug_,"     not adding cache node (%s,@%d,%d) at %d for %s - to many thunks\n",
                         (result ? "ok" : "fail"), yySelf->pos, count,
                         start.pos, name);
                return;
            }
            // return;
        }
    }

    int index = start.pos % yySelf->cache->size;

    YYCacheNode* node =  malloc(sizeof(YYCacheNode) + (sizeof(YYCacheNode) * count));

    if (!node) return;

    node->next     = yySelf->cache->slot[index];
    node->location = start.pos;
    node->function = function;
    node->result   = result;

    if (result) {
        node->pos = yySelf->pos;
        node->count = count;
        YYThunk* to   = &(node->thunk[0]);
        YYThunk* from = &(yySelf->thunks[start.thunkpos]);
        for (; count--; ++to, ++from) {
            *to = *from;
        }
    } else {
        node->pos = start.pos;
        node->count = 0;
    }

    YY_SEND(debug_,"     adding cache node (%s,@%d,%d) at %d for %s\n",
             (node->result ? "ok" : "fail"), node->pos, node->count,
             node->location, name);

    yySelf->cache->slot[index] = node;
}

static inline int yyCheckNode(YYClass* yySelf, YYStack *yystack, int* result)
{
    if (!yySelf->cache) return 0;

    const char* name = yystack->name;
    YYRule  function = yystack->current;
    YYState    start = yystack->begin;

    int index = start.pos % yySelf->cache->size;

    YYCacheNode* node = yySelf->cache->slot[index];

    for (; node; node = node->next) {
        if (node->location != start.pos) continue;
        if (node->function != function) continue;
        *result = node->result;
        yySelf->pos   = node->pos;

        int count = node->count;

        if (0 < count) {
            YYThunk* from = &(node->thunk[0]);

            yyResizeDo(yySelf, count);

            YYThunk* to = &(yySelf->thunks[yySelf->thunkpos]);

            yySelf->thunkpos += count;
            for (; count--; ++to, ++from) {
                *to = *from;
            }
        }

        YY_SEND(debug_,"     using cache node (%s,@%d,%d) at %d for %s\n",
                 (node->result ? "ok" : "fail"), node->pos, node->count,
                 node->location, name);

        return 1;
    }

    return 0;
}

static inline void yyDone(YYClass* yySelf) {
    int pos;
    for (pos = 0;  pos < yySelf->thunkpos;  ++pos) {
        YYThunk *thunk = &yySelf->thunks[pos];

        YY_SEND(debug_,"DO [%d] %p %s : ", pos, thunk->action, thunk->name);

        if (thunk->action) {
            thunk->action(yySelf, yySelf->thunks[pos]);
        } else {
            YY_SEND(debug_,"\n");
        }
    }
    yySelf->thunkpos = 0;
}

static inline void yyResizeVars(YYClass* yySelf, int count) {
    int marker = yySelf->frame + count;

    if (0 > marker) return;

    while (marker >= yySelf->valslen) {
        yySelf->valslen *= 2;
    }

    YY_SEND(debug_,"do %s %d\n", "stack-resize", yySelf->valslen);

    if (yySelf->vals) {
        yySelf->vals = realloc(yySelf->vals, sizeof(YYSTYPE) * yySelf->valslen);
    } else {
        yySelf->vals = malloc(sizeof(YYSTYPE) * yySelf->valslen);
    }
}

static void yyPush(YYClass* yySelf, YYThunk thunk) {
    YY_SEND(debug_,"do %s %d\n", "push", thunk.argument);
    /* need to check if yySelf->frame + thunk.argument >= yySelf->valslen */
    yyResizeVars(yySelf, thunk.argument);
    yySelf->frame += thunk.argument;
}

static void yyPop(YYClass* yySelf, YYThunk thunk) {
    YY_SEND(debug_,"do %s %d\n", "pop", thunk.argument);
    yySelf->frame -= thunk.argument;
    /* need to check if yySelf->frame < 0 */
}

static void yySet(YYClass* yySelf, YYThunk thunk) {
    YY_SEND(debug_,"do %s v[%d] = %d\n", "set", thunk.argument,  yySelf->result);
    /* need to check if yySelf->frame + thunk.argument >= yySelf->valslen */
    yySelf->vals[yySelf->frame + thunk.argument] = yySelf->result;
}

static inline int yyThunkText(YYClass* yySelf, YYThunk thunk) {
    int begin  = thunk.begin;
    int end    = thunk.end;
    int length = end - begin;

    if (length <= 0)
        length = 0;
    else {
        char* target = yySelf->text;
        char* source = yySelf->buf + begin;
        int   limit  = yySelf->textlen;

        if (limit < (length - 1)) {
            while (limit < (length - 1)) {
                limit *= 2;
                target= realloc(target, limit);
            }

            yySelf->textlen = limit;
            yySelf->text    = target;
        }

        memcpy(target, source, length);
    }

    yySelf->text[length]= '\0';

    YY_SEND(debug_,"GET thunk text[%d,%d] = \'%s\'\n", begin, end,  yySelf->text);

    return length;
}

static inline int yyrefill(YYClass* yySelf) {
    YY_SEND(debug_,"     yyrefill\n");

    if (yySelf->buflen - yySelf->pos < 512) {
        while (yySelf->buflen - yySelf->pos < 512) {
            yySelf->buflen *= 2;
            yySelf->buf     = realloc(yySelf->buf, yySelf->buflen+2);
        }
        yySelf->buf[yySelf->limit] = 0;
    }

    //    int yyn = YY_INPUT(yySelf->buf + yySelf->pos, yySelf->buflen - yySelf->pos);
    int yyn = yySelf->input_(yySelf, yySelf->buf + yySelf->pos, yySelf->buflen - yySelf->pos);

    if (0 == yyn) return 0;

    yySelf->limit += yyn;
    yySelf->buf[yySelf->limit] = 0;

    return 1;
}

static inline int yymatchDot(YYClass* yySelf) {
    if (yySelf->pos >= yySelf->limit && !yyrefill(yySelf)) return 0;
    yySelf->pos += 1;
    return 1;
}

static inline int yymatchChar(YYClass* yySelf, int c) {
    if (yySelf->pos >= yySelf->limit && !yyrefill(yySelf)) return 0;

    if (yySelf->buf[yySelf->pos] == c) {
        yySelf->pos += 1;
        return 1;
    }

    return 0;
}

static inline int yymatchString(YYClass* yySelf, char *s) {
    int yysav= yySelf->pos;
    while (*s) {
        if (yySelf->pos >= yySelf->limit && !yyrefill(yySelf)) {
            yySelf->pos= yysav;
            return 0;
        }
        if (yySelf->buf[yySelf->pos] != *s) {
            yySelf->pos= yysav;
            return 0;
        }
        ++s;
        yySelf->pos += 1;
    }
    return 1;
}

static inline int yymatchClass(YYClass* yySelf, const char* cls, unsigned char *bits) {
    int chr;

    if (yySelf->pos >= yySelf->limit && !yyrefill(yySelf)) return 0;

    chr = yySelf->buf[yySelf->pos];

    if (bits[chr >> 3] & (1 << (chr & 7))) {
        ++yySelf->pos;
        return 1;
    }

    return 0;
}

static inline void yy_no_warings() {
    /* this is here to stop 'defined but not used' warnings */

    // for references ONLY
    (void)yyPush;
    (void)yyPop;
    (void)yySet;
    (void)yymatchDot;
    (void)yymatchChar;
    (void)yymatchString;
    (void)yymatchClass;
    (void)yyDo;
    (void)yyAccept;
}

#endif
/*-- end of preamble --*/


/* actions */
static void yy_11_primary (YYClass* yySelf, YYThunk thunk);
static void yy_10_primary (YYClass* yySelf, YYThunk thunk);
static void yy_9_primary (YYClass* yySelf, YYThunk thunk);
static void yy_8_primary (YYClass* yySelf, YYThunk thunk);
static void yy_7_primary (YYClass* yySelf, YYThunk thunk);
static void yy_6_primary (YYClass* yySelf, YYThunk thunk);
static void yy_5_primary (YYClass* yySelf, YYThunk thunk);
static void yy_4_primary (YYClass* yySelf, YYThunk thunk);
static void yy_3_primary (YYClass* yySelf, YYThunk thunk);
static void yy_2_primary (YYClass* yySelf, YYThunk thunk);
static void yy_1_primary (YYClass* yySelf, YYThunk thunk);
static void yy_3_suffix (YYClass* yySelf, YYThunk thunk);
static void yy_2_suffix (YYClass* yySelf, YYThunk thunk);
static void yy_1_suffix (YYClass* yySelf, YYThunk thunk);
static void yy_3_prefix (YYClass* yySelf, YYThunk thunk);
static void yy_2_prefix (YYClass* yySelf, YYThunk thunk);
static void yy_1_prefix (YYClass* yySelf, YYThunk thunk);
static void yy_1_sequence (YYClass* yySelf, YYThunk thunk);
static void yy_1_expression (YYClass* yySelf, YYThunk thunk);
static void yy_1_end (YYClass* yySelf, YYThunk thunk);
static void yy_1_begin (YYClass* yySelf, YYThunk thunk);
static void yy_5_definition (YYClass* yySelf, YYThunk thunk);
static void yy_4_definition (YYClass* yySelf, YYThunk thunk);
static void yy_3_definition (YYClass* yySelf, YYThunk thunk);
static void yy_2_definition (YYClass* yySelf, YYThunk thunk);
static void yy_1_definition (YYClass* yySelf, YYThunk thunk);
static void yy_1_trailer (YYClass* yySelf, YYThunk thunk);
static void yy_1_declaration (YYClass* yySelf, YYThunk thunk);

static void yy_11_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_11_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeAction("YY_SEND(collect_, yyrulename);")); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_10_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_10_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeAction("YY_SEND(mark_, yyrulename);")); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_9_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_9_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeMark("YY_SEND(end_, yystack)")); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_8_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_8_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeMark("YY_SEND(begin_, yystack)")); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_7_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_7_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeAction(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_6_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_6_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeDot()); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_5_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_5_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeClass(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_4_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_4_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeString(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_3_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_3_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeName(findRule(yytext))); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_2_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_2_primary (%s) '%s'\n", yyrulename, yytext);

   Node *name= makeName(findRule(yytext));  name->name.variable= pop();  push(name); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_primary(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "primary";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_primary (%s) '%s'\n", yyrulename, yytext);

   push(makeVariable(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_3_suffix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "suffix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_3_suffix (%s) '%s'\n", yyrulename, yytext);

   push(makePlus (pop())); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_2_suffix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "suffix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_2_suffix (%s) '%s'\n", yyrulename, yytext);

   push(makeStar (pop())); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_suffix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "suffix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_suffix (%s) '%s'\n", yyrulename, yytext);

   push(makeQuery(pop())); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_3_prefix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "prefix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_3_prefix (%s) '%s'\n", yyrulename, yytext);

   push(makePeekNot(pop())); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_2_prefix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "prefix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_2_prefix (%s) '%s'\n", yyrulename, yytext);

   push(makePeekFor(pop())); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_prefix(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "prefix";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_prefix (%s) '%s'\n", yyrulename, yytext);

   push(makePredicate(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_sequence(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "sequence";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_sequence (%s) '%s'\n", yyrulename, yytext);

   Node *f= pop();  push(Sequence_append(pop(), f)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_expression(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "expression";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_expression (%s) '%s'\n", yyrulename, yytext);

   Node *f= pop();  push(Alternate_append(pop(), f)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_end(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "end";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_end (%s) '%s'\n", yyrulename, yytext);

   push(makePredicate(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_begin(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "begin";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_begin (%s) '%s'\n", yyrulename, yytext);

   push(makePredicate(yytext)); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_5_definition(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "definition";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_5_definition (%s) '%s'\n", yyrulename, yytext);

   defineRule(rule_with_begin); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_4_definition(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "definition";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_4_definition (%s) '%s'\n", yyrulename, yytext);

   defineRule(rule_with_both); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_3_definition(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "definition";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_3_definition (%s) '%s'\n", yyrulename, yytext);

   defineRule(simple_rule); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_2_definition(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "definition";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_2_definition (%s) '%s'\n", yyrulename, yytext);

   defineRule(rule_with_end); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_definition(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "definition";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_definition (%s) '%s'\n", yyrulename, yytext);

   checkRule(yytext); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_trailer(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "trailer";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_trailer (%s) '%s'\n", yyrulename, yytext);

   makeTrailer(yytext); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}
static void yy_1_declaration(YYClass* yySelf, YYThunk thunk)
{
  static const char* yyrulename = "declaration";
  int   yyleng = yyThunkText(yySelf, thunk);
  char* yytext = yySelf->text;
  int   yypos  = yySelf->pos;

#define yy yySelf->result
#define yythunkpos yySelf->thunkpos
  YY_SEND(debug_, "do yy_1_declaration (%s) '%s'\n", yyrulename, yytext);

   makeHeader(yytext); ;
#undef yy
#undef yythunkpos

  // for references ONLY
  (void)yyrulename;
  (void)yyleng;
  (void)yytext;
  (void)yypos;
}

int yy_end_of_line(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "end_of_line";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate2;
  YY_SEND(save_, &yystate2);  if (!yymatchString(yySelf, "\r\n")) goto l3; goto l2;
  l3:;	
  YY_SEND(restore_, &yystate2);  if (!yymatchChar(yySelf, '\n')) goto l4; goto l2;
  l4:;	
  YY_SEND(restore_, &yystate2);  if (!yymatchChar(yySelf, '\r')) goto failed;
  l2:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_comment(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "comment";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '#')) goto failed;
  l5:;	
  YYState yystate6;
  YY_SEND(save_, &yystate6);
  YYState yystate7;
  YY_SEND(save_, &yystate7);  if (!YY_SEND(apply_, yystack, &yy_end_of_line, "end_of_line")) goto l7; goto l6;
  l7:;	
  YY_SEND(restore_, &yystate7);  if (!yymatchDot(yySelf)) goto l6; goto l5;
  l6:;	
  YY_SEND(restore_, &yystate6);  if (!YY_SEND(apply_, yystack, &yy_end_of_line, "end_of_line")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_space(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "space";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate8;
  YY_SEND(save_, &yystate8);  if (!yymatchChar(yySelf, ' ')) goto l9; goto l8;
  l9:;	
  YY_SEND(restore_, &yystate8);  if (!yymatchChar(yySelf, '\t')) goto l10; goto l8;
  l10:;	
  YY_SEND(restore_, &yystate8);  if (!YY_SEND(apply_, yystack, &yy_end_of_line, "end_of_line")) goto failed;
  l8:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_braces(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "braces";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate11;
  YY_SEND(save_, &yystate11);  if (!yymatchChar(yySelf, '{')) goto l12;
  l13:;	
  YYState yystate14;
  YY_SEND(save_, &yystate14);
  YYState yystate15;
  YY_SEND(save_, &yystate15);  if (!yymatchChar(yySelf, '}')) goto l15; goto l14;
  l15:;	
  YY_SEND(restore_, &yystate15);  if (!yymatchDot(yySelf)) goto l14; goto l13;
  l14:;	
  YY_SEND(restore_, &yystate14);  if (!yymatchChar(yySelf, '}')) goto l12; goto l11;
  l12:;	
  YY_SEND(restore_, &yystate11);
  YYState yystate16;
  YY_SEND(save_, &yystate16);  if (!yymatchChar(yySelf, '}')) goto l16; goto failed;
  l16:;	
  YY_SEND(restore_, &yystate16);  if (!yymatchDot(yySelf)) goto failed;
  l11:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_range(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "range";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate17;
  YY_SEND(save_, &yystate17);  if (!YY_SEND(apply_, yystack, &yy_char, "char")) goto l18;  if (!yymatchChar(yySelf, '-')) goto l18;  if (!YY_SEND(apply_, yystack, &yy_char, "char")) goto l18; goto l17;
  l18:;	
  YY_SEND(restore_, &yystate17);  if (!YY_SEND(apply_, yystack, &yy_char, "char")) goto failed;
  l17:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_char(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "char";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate19;
  YY_SEND(save_, &yystate19);  if (!yymatchChar(yySelf, '\\')) goto l20;  if (!yymatchClass(yySelf, "abefnrtv\'\"\\[\\]\\\\", (unsigned char *)"\000\000\000\000\204\000\000\000\000\000\000\070\146\100\124\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l20; goto l19;
  l20:;	
  YY_SEND(restore_, &yystate19);  if (!yymatchChar(yySelf, '\\')) goto l21;  if (!yymatchClass(yySelf, "0-3", (unsigned char *)"\000\000\000\000\000\000\017\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l21;  if (!yymatchClass(yySelf, "0-7", (unsigned char *)"\000\000\000\000\000\000\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l21;  if (!yymatchClass(yySelf, "0-7", (unsigned char *)"\000\000\000\000\000\000\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l21; goto l19;
  l21:;	
  YY_SEND(restore_, &yystate19);  if (!yymatchChar(yySelf, '\\')) goto l22;  if (!yymatchClass(yySelf, "0-7", (unsigned char *)"\000\000\000\000\000\000\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l22;
  YYState yystate23;
  YY_SEND(save_, &yystate23);  if (!yymatchClass(yySelf, "0-7", (unsigned char *)"\000\000\000\000\000\000\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l23; goto l24;
  l23:;	
  YY_SEND(restore_, &yystate23);
  l24:;	 goto l19;
  l22:;	
  YY_SEND(restore_, &yystate19);
  YYState yystate25;
  YY_SEND(save_, &yystate25);  if (!yymatchChar(yySelf, '\\')) goto l25; goto failed;
  l25:;	
  YY_SEND(restore_, &yystate25);  if (!yymatchDot(yySelf)) goto failed;
  l19:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_COLLECT(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "COLLECT";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '$')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_MARK(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "MARK";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '@')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_END(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "END";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '>')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_BEGIN(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "BEGIN";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '<')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_DOT(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "DOT";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '.')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_class(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "class";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '[')) goto failed;  { YY_SEND(begin_, yystack); }
  l26:;	
  YYState yystate27;
  YY_SEND(save_, &yystate27);
  YYState yystate28;
  YY_SEND(save_, &yystate28);  if (!yymatchChar(yySelf, ']')) goto l28; goto l27;
  l28:;	
  YY_SEND(restore_, &yystate28);  if (!YY_SEND(apply_, yystack, &yy_range, "range")) goto l27; goto l26;
  l27:;	
  YY_SEND(restore_, &yystate27);  { YY_SEND(end_, yystack); }  if (!yymatchChar(yySelf, ']')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_literal(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "literal";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate29;
  YY_SEND(save_, &yystate29);  if (!yymatchClass(yySelf, "\'", (unsigned char *)"\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l30;  { YY_SEND(begin_, yystack); }
  l31:;	
  YYState yystate32;
  YY_SEND(save_, &yystate32);
  YYState yystate33;
  YY_SEND(save_, &yystate33);  if (!yymatchClass(yySelf, "\'", (unsigned char *)"\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l33; goto l32;
  l33:;	
  YY_SEND(restore_, &yystate33);  if (!YY_SEND(apply_, yystack, &yy_char, "char")) goto l32; goto l31;
  l32:;	
  YY_SEND(restore_, &yystate32);  { YY_SEND(end_, yystack); }  if (!yymatchClass(yySelf, "\'", (unsigned char *)"\000\000\000\000\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l30;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto l30; goto l29;
  l30:;	
  YY_SEND(restore_, &yystate29);  if (!yymatchClass(yySelf, "\"", (unsigned char *)"\000\000\000\000\004\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto failed;  { YY_SEND(begin_, yystack); }
  l34:;	
  YYState yystate35;
  YY_SEND(save_, &yystate35);
  YYState yystate36;
  YY_SEND(save_, &yystate36);  if (!yymatchClass(yySelf, "\"", (unsigned char *)"\000\000\000\000\004\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l36; goto l35;
  l36:;	
  YY_SEND(restore_, &yystate36);  if (!YY_SEND(apply_, yystack, &yy_char, "char")) goto l35; goto l34;
  l35:;	
  YY_SEND(restore_, &yystate35);  { YY_SEND(end_, yystack); }  if (!yymatchClass(yySelf, "\"", (unsigned char *)"\000\000\000\000\004\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  l29:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_CLOSE(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "CLOSE";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, ')')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_OPEN(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "OPEN";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '(')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_COLON(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "COLON";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, ':')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_PLUS(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "PLUS";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '+')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_STAR(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "STAR";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '*')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_QUESTION(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "QUESTION";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '?')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_primary(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "primary";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate37;
  YY_SEND(save_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_identifier, "identifier")) goto l38;  yyDo(yySelf, " yy_1_primary", yy_1_primary, 0, yystate0);  if (!YY_SEND(apply_, yystack, &yy_COLON, "COLON")) goto l38;  if (!YY_SEND(apply_, yystack, &yy_identifier, "identifier")) goto l38;
  YYState yystate39;
  YY_SEND(save_, &yystate39);  if (!YY_SEND(apply_, yystack, &yy_EQUAL, "EQUAL")) goto l39; goto l38;
  l39:;	
  YY_SEND(restore_, &yystate39);  yyDo(yySelf, " yy_2_primary", yy_2_primary, 0, yystate0); goto l37;
  l38:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_identifier, "identifier")) goto l40;
  YYState yystate41;
  YY_SEND(save_, &yystate41);  if (!YY_SEND(apply_, yystack, &yy_EQUAL, "EQUAL")) goto l41; goto l40;
  l41:;	
  YY_SEND(restore_, &yystate41);  yyDo(yySelf, " yy_3_primary", yy_3_primary, 0, yystate0); goto l37;
  l40:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_OPEN, "OPEN")) goto l42;  if (!YY_SEND(apply_, yystack, &yy_expression, "expression")) goto l42;  if (!YY_SEND(apply_, yystack, &yy_CLOSE, "CLOSE")) goto l42; goto l37;
  l42:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_literal, "literal")) goto l43;  yyDo(yySelf, " yy_4_primary", yy_4_primary, 0, yystate0); goto l37;
  l43:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_class, "class")) goto l44;  yyDo(yySelf, " yy_5_primary", yy_5_primary, 0, yystate0); goto l37;
  l44:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_DOT, "DOT")) goto l45;  yyDo(yySelf, " yy_6_primary", yy_6_primary, 0, yystate0); goto l37;
  l45:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_action, "action")) goto l46;  yyDo(yySelf, " yy_7_primary", yy_7_primary, 0, yystate0); goto l37;
  l46:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_BEGIN, "BEGIN")) goto l47;  yyDo(yySelf, " yy_8_primary", yy_8_primary, 0, yystate0); goto l37;
  l47:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_END, "END")) goto l48;  yyDo(yySelf, " yy_9_primary", yy_9_primary, 0, yystate0); goto l37;
  l48:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_MARK, "MARK")) goto l49;  yyDo(yySelf, " yy_10_primary", yy_10_primary, 0, yystate0); goto l37;
  l49:;	
  YY_SEND(restore_, &yystate37);  if (!YY_SEND(apply_, yystack, &yy_COLLECT, "COLLECT")) goto failed;  yyDo(yySelf, " yy_11_primary", yy_11_primary, 0, yystate0);
  l37:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_NOT(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "NOT";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '!')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_suffix(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "suffix";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!YY_SEND(apply_, yystack, &yy_primary, "primary")) goto failed;
  YYState yystate50;
  YY_SEND(save_, &yystate50);
  YYState yystate52;
  YY_SEND(save_, &yystate52);  if (!YY_SEND(apply_, yystack, &yy_QUESTION, "QUESTION")) goto l53;  yyDo(yySelf, " yy_1_suffix", yy_1_suffix, 0, yystate0); goto l52;
  l53:;	
  YY_SEND(restore_, &yystate52);  if (!YY_SEND(apply_, yystack, &yy_STAR, "STAR")) goto l54;  yyDo(yySelf, " yy_2_suffix", yy_2_suffix, 0, yystate0); goto l52;
  l54:;	
  YY_SEND(restore_, &yystate52);  if (!YY_SEND(apply_, yystack, &yy_PLUS, "PLUS")) goto l50;  yyDo(yySelf, " yy_3_suffix", yy_3_suffix, 0, yystate0);
  l52:;	 goto l51;
  l50:;	
  YY_SEND(restore_, &yystate50);
  l51:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_AND(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "AND";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '&')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_prefix(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "prefix";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate55;
  YY_SEND(save_, &yystate55);  if (!YY_SEND(apply_, yystack, &yy_AND, "AND")) goto l56;  if (!YY_SEND(apply_, yystack, &yy_action, "action")) goto l56;  yyDo(yySelf, " yy_1_prefix", yy_1_prefix, 0, yystate0); goto l55;
  l56:;	
  YY_SEND(restore_, &yystate55);  if (!YY_SEND(apply_, yystack, &yy_AND, "AND")) goto l57;  if (!YY_SEND(apply_, yystack, &yy_suffix, "suffix")) goto l57;  yyDo(yySelf, " yy_2_prefix", yy_2_prefix, 0, yystate0); goto l55;
  l57:;	
  YY_SEND(restore_, &yystate55);  if (!YY_SEND(apply_, yystack, &yy_NOT, "NOT")) goto l58;  if (!YY_SEND(apply_, yystack, &yy_suffix, "suffix")) goto l58;  yyDo(yySelf, " yy_3_prefix", yy_3_prefix, 0, yystate0); goto l55;
  l58:;	
  YY_SEND(restore_, &yystate55);  if (!YY_SEND(apply_, yystack, &yy_suffix, "suffix")) goto failed;
  l55:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_BAR(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "BAR";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '|')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_sequence(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "sequence";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!YY_SEND(apply_, yystack, &yy_prefix, "prefix")) goto failed;
  l59:;	
  YYState yystate60;
  YY_SEND(save_, &yystate60);  if (!YY_SEND(apply_, yystack, &yy_prefix, "prefix")) goto l60;  yyDo(yySelf, " yy_1_sequence", yy_1_sequence, 0, yystate0); goto l59;
  l60:;	
  YY_SEND(restore_, &yystate60);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_action(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "action";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '{')) goto failed;  { YY_SEND(begin_, yystack); }
  l61:;	
  YYState yystate62;
  YY_SEND(save_, &yystate62);  if (!YY_SEND(apply_, yystack, &yy_braces, "braces")) goto l62; goto l61;
  l62:;	
  YY_SEND(restore_, &yystate62);  { YY_SEND(end_, yystack); }  if (!yymatchChar(yySelf, '}')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_SEMICOLON(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "SEMICOLON";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, ';')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_begin(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "begin";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchString(yySelf, "%begin")) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;  if (!YY_SEND(apply_, yystack, &yy_action, "action")) goto failed;  yyDo(yySelf, " yy_1_begin", yy_1_begin, 0, yystate0);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_end(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "end";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchString(yySelf, "%end")) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;  if (!YY_SEND(apply_, yystack, &yy_action, "action")) goto failed;  yyDo(yySelf, " yy_1_end", yy_1_end, 0, yystate0);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_expression(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "expression";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!YY_SEND(apply_, yystack, &yy_sequence, "sequence")) goto failed;
  l63:;	
  YYState yystate64;
  YY_SEND(save_, &yystate64);  if (!YY_SEND(apply_, yystack, &yy_BAR, "BAR")) goto l64;  if (!YY_SEND(apply_, yystack, &yy_sequence, "sequence")) goto l64;  yyDo(yySelf, " yy_1_expression", yy_1_expression, 0, yystate0); goto l63;
  l64:;	
  YY_SEND(restore_, &yystate64);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_EQUAL(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "EQUAL";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchChar(yySelf, '=')) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_identifier(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "identifier";

  YYState yystate0 = yystack->begin;

  start_rule:;
  { YY_SEND(begin_, yystack); }  if (!yymatchClass(yySelf, "-a-zA-Z_", (unsigned char *)"\000\000\000\000\000\040\000\000\376\377\377\207\376\377\377\007\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto failed;
  l65:;	
  YYState yystate66;
  YY_SEND(save_, &yystate66);  if (!yymatchClass(yySelf, "-a-zA-Z_0-9", (unsigned char *)"\000\000\000\000\000\040\377\003\376\377\377\207\376\377\377\007\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000")) goto l66; goto l65;
  l66:;	
  YY_SEND(restore_, &yystate66);  { YY_SEND(end_, yystack); }  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_RPERCENT(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "RPERCENT";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchString(yySelf, "%}")) goto failed;  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_end_of_file(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "end_of_file";

  YYState yystate0 = yystack->begin;

  start_rule:;

  YYState yystate67;
  YY_SEND(save_, &yystate67);  if (!yymatchDot(yySelf)) goto l67; goto failed;
  l67:;	
  YY_SEND(restore_, &yystate67);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_trailer(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "trailer";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchString(yySelf, "%%")) goto failed;  { YY_SEND(begin_, yystack); }
  l68:;	
  YYState yystate69;
  YY_SEND(save_, &yystate69);  if (!yymatchDot(yySelf)) goto l69; goto l68;
  l69:;	
  YY_SEND(restore_, &yystate69);  { YY_SEND(end_, yystack); }  yyDo(yySelf, " yy_1_trailer", yy_1_trailer, 0, yystate0);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_definition(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "definition";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!YY_SEND(apply_, yystack, &yy_identifier, "identifier")) goto failed;  yyDo(yySelf, " yy_1_definition", yy_1_definition, 0, yystate0);  if (!YY_SEND(apply_, yystack, &yy_EQUAL, "EQUAL")) goto failed;
  YYState yystate70;
  YY_SEND(save_, &yystate70);  if (!YY_SEND(apply_, yystack, &yy_expression, "expression")) goto l71;  if (!YY_SEND(apply_, yystack, &yy_end, "end")) goto l71;  yyDo(yySelf, " yy_2_definition", yy_2_definition, 0, yystate0); goto l70;
  l71:;	
  YY_SEND(restore_, &yystate70);  if (!YY_SEND(apply_, yystack, &yy_expression, "expression")) goto l72;  yyDo(yySelf, " yy_3_definition", yy_3_definition, 0, yystate0); goto l70;
  l72:;	
  YY_SEND(restore_, &yystate70);  if (!YY_SEND(apply_, yystack, &yy_begin, "begin")) goto l73;  if (!YY_SEND(apply_, yystack, &yy_expression, "expression")) goto l73;  if (!YY_SEND(apply_, yystack, &yy_end, "end")) goto l73;  yyDo(yySelf, " yy_4_definition", yy_4_definition, 0, yystate0); goto l70;
  l73:;	
  YY_SEND(restore_, &yystate70);  if (!YY_SEND(apply_, yystack, &yy_begin, "begin")) goto failed;  if (!YY_SEND(apply_, yystack, &yy_expression, "expression")) goto failed;  yyDo(yySelf, " yy_5_definition", yy_5_definition, 0, yystate0);
  l70:;	
  YYState yystate74;
  YY_SEND(save_, &yystate74);  if (!YY_SEND(apply_, yystack, &yy_SEMICOLON, "SEMICOLON")) goto l74; goto l75;
  l74:;	
  YY_SEND(restore_, &yystate74);
  l75:;	
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_declaration(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "declaration";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!yymatchString(yySelf, "%{")) goto failed;  { YY_SEND(begin_, yystack); }
  l76:;	
  YYState yystate77;
  YY_SEND(save_, &yystate77);
  YYState yystate78;
  YY_SEND(save_, &yystate78);  if (!yymatchString(yySelf, "%}")) goto l78; goto l77;
  l78:;	
  YY_SEND(restore_, &yystate78);  if (!yymatchDot(yySelf)) goto l77; goto l76;
  l77:;	
  YY_SEND(restore_, &yystate77);  { YY_SEND(end_, yystack); }  if (!YY_SEND(apply_, yystack, &yy_RPERCENT, "RPERCENT")) goto failed;  yyDo(yySelf, " yy_1_declaration", yy_1_declaration, 0, yystate0);
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy__(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "_";

  YYState yystate0 = yystack->begin;

  start_rule:;

  l79:;	
  YYState yystate80;
  YY_SEND(save_, &yystate80);
  YYState yystate81;
  YY_SEND(save_, &yystate81);  if (!YY_SEND(apply_, yystack, &yy_space, "space")) goto l82; goto l81;
  l82:;	
  YY_SEND(restore_, &yystate81);  if (!YY_SEND(apply_, yystack, &yy_comment, "comment")) goto l80;
  l81:;	 goto l79;
  l80:;	
  YY_SEND(restore_, &yystate80);
  goto passed;

  passed:
  return 1;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}

int yy_grammar(YYClass* yySelf, YYStack* yystack)
{
  static const char* yyrulename = "grammar";

  YYState yystate0 = yystack->begin;

  start_rule:;
  if (!YY_SEND(apply_, yystack, &yy__, "_")) goto failed;
  YYState yystate85;
  YY_SEND(save_, &yystate85);  if (!YY_SEND(apply_, yystack, &yy_declaration, "declaration")) goto l86; goto l85;
  l86:;	
  YY_SEND(restore_, &yystate85);  if (!YY_SEND(apply_, yystack, &yy_definition, "definition")) goto failed;
  l85:;	
  l83:;	
  YYState yystate84;
  YY_SEND(save_, &yystate84);
  YYState yystate87;
  YY_SEND(save_, &yystate87);  if (!YY_SEND(apply_, yystack, &yy_declaration, "declaration")) goto l88; goto l87;
  l88:;	
  YY_SEND(restore_, &yystate87);  if (!YY_SEND(apply_, yystack, &yy_definition, "definition")) goto l84;
  l87:;	 goto l83;
  l84:;	
  YY_SEND(restore_, &yystate84);
  YYState yystate89;
  YY_SEND(save_, &yystate89);  if (!YY_SEND(apply_, yystack, &yy_trailer, "trailer")) goto l89; goto l90;
  l89:;	
  YY_SEND(restore_, &yystate89);
  l90:;	  if (!YY_SEND(apply_, yystack, &yy_end_of_file, "end_of_file")) goto failed;
  goto passed;

  passed:
  return 1;
  goto failed;

  failed:
  return 0;
  // for references ONLY
  (void)yyrulename;
  (void)yystate0;
  if (0) goto start_rule;
}


