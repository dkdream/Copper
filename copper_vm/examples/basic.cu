# A 'syntax-directed interpreter' (all execution is a side-effect of parsing).
# Inspired by Dennis Allison's original Tiny BASIC grammar, circa 1975.
# 
# Copyright (c) 2007 by Ian Piumarta
# All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the 'Software'),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, provided that the above copyright notice(s) and this
# permission notice appear in all copies of the Software.  Acknowledgement
# of the use of this Software in supporting documentation would be
# appreciated but is not required.
# 
# THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
# 
# Last edited: 2007-05-14 11:32:49 by piumarta on emilia

start = line

line = - s:statement CR
|      - n:number < ( !CR . )* CR >			{ accept(n.number, yytext); }
|      - CR
|      - < ( !CR . )* CR >				{ epc= pc;  error("syntax error"); }
|      - !.						{ exit(0); }

statement = 'print'- expr-list
|           'if'- e1:expression r:relop e2:expression	{ if (!r.binop(e1.number, e2.number)) yythunkpos= 0; }
		'then'- statement
|           'goto'- e:expression			{ epc= pc;  if ((pc= findLine(e.number, 0)) < 0) error("no such line"); }
|           'input'- var-list
|           'let'- v:var EQUAL e:expression		{ variables[v.number]= e.number; }
|           'gosub'- e:expression			{ epc= pc;  if (sp < 1024) stack[sp++]= pc, pc= findLine(e.number, 0); else error("too many gosubs");
							  if (pc < 0) error("no such line"); }
|           'return'-					{ epc= pc;  if ((pc= sp ? stack[--sp] : -1) < 0) error("no gosub"); }
|           'clear'-					{ while (numLines) accept(lines->number, "\n"); }
|           'list'-					{ int i;  for (i= 0;  i < numLines;  ++i) printf("%5d %s", lines[i].number, lines[i].text); }
|           'run'- s:string				{ load(s.string);  pc= 0; }
|           'run'-					{ pc= 0; }
|           'end'-					{ pc= -1;  if (batch) exit(0); }
|           'rem'- ( !CR . )*
|           ('bye'|'quit'|'exit')-			{ exit(0); }
|           'save'- s:string				{ save(s.string); }
|           'load'- s:string				{ load(s.string); }
|           'type'- s:string				{ type(s.string); }
|           'dir'-					{ system("ls *.bas"); }
|           'help'-					{ fprintf(stderr, "%s", help); }

expr-list = ( e:string					{ printf("%s", e.string); }
            | e:expression				{ printf("%d", e.number); }
            )? ( COMMA ( e:string			{ printf("%s", e.string); }
                       | e:expression			{ printf("%d", e.number); }
                       )
               )* ( COMMA
	          | !COMMA				{ printf("\n"); }
		  )

var-list = v:var					{ variables[v.number]= input(); }
           ( COMMA v:var				{ variables[v.number]= input(); }
           )*

expression = ( PLUS? l:term
             | MINUS l:term				{ l.number = -l.number }
             ) ( PLUS  r:term				{ l.number += r.number }
               | MINUS r:term				{ l.number -= r.number }
               )*					{ $$.number = l.number }

term = l:factor ( STAR  r:factor			{ l.number *= r.number }
                | SLASH r:factor			{ l.number /= r.number }
                )*					{ $$.number = l.number }

factor = v:var						{ $$.number = variables[v.number] }
|        n:number
|        OPEN expression CLOSE

var = < [a-z] > -					{ $$.number = yytext[0] - 'a' }

number = < digit+ > -					{ $$.number = atoi(yytext); }

digit = [0-9]

string = '"' < [^\"]* > '"' -				{ $$.string = yytext; }

relop = '<=' -						{ $$.binop= lessEqual; }
|       '<>' -						{ $$.binop= notEqual; }
|       '<'  -						{ $$.binop= lessThan; }
|       '>=' -						{ $$.binop= greaterEqual; }
|       '>'  -						{ $$.binop= greaterThan; }
|       '='  -						{ $$.binop= equalTo; }

EQUAL  = '=' -  CLOSE  = ')' -  OPEN   = '(' -
SLASH  = '/' -  STAR   = '*' -  MINUS  = '-' -
PLUS   = '+' -  COMMA  = ',' -

- = [ \t]*

CR = '\n' | '\r' | '\r\n'

