#ifndef YY_HEADER
typedef struct _yyclass YYClass;
typedef struct _yystack  YYStack;
#endif

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

