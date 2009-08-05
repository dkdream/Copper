# Copper Grammar for copper

%export grammar

%{
#include "copper.inc"
%}

# Hierarchical syntax

grammar     =  - ( heading
                 | declaration
                 | exportation
                 | define-macro 
                 | define-rule
                 )+ trailer? end-of-file

heading     =  '%{' < ( !'%}' . )* > RPERCENT { makeHeader(yytext); }
declaration =  '%declare' - identifier        { declareRule(yytext); }
exportation =  '%export'  - identifier        { exportRule(yytext); }
trailer     =  '%%' < .* >                    { makeTrailer(yytext); }

define-macro =  '%define' - macro  { checkMacro(yytext);   }
                            action { defineMacro(yytext);  }

define-rule =  identifier                   { checkRule(yytext); }
               EQUAL ( expression end       { defineRule(rule_with_end); }
                     | expression           { defineRule(simple_rule); }
                     | begin expression end { defineRule(rule_with_both); }
                     | begin expression     { defineRule(rule_with_begin); }
                     ) SEMICOLON?

begin       = '%begin' - ( action { push(makePredicate(yytext)); }
                         | macro  { push(makePredicate(fetchMacro(yytext))); }
                         )
end         = '%end'   - ( action { push(makePredicate(yytext)); }
                         | macro  { push(makePredicate(fetchMacro(yytext))); }
                         )

expression  = sequence (BAR sequence  { Node *f= pop();  push(Alternate_append(pop(), f)); }  )*

sequence    = prefix (prefix          { Node *f= pop();  push(Sequence_append(pop(), f)); }   )*

prefix      = AND action    { push(makePredicate(yytext)); }
            | AND macro     { push(makePredicate(fetchMacro(yytext))); }
            | AND suffix    { push(makePeekFor(pop())); }
            | NOT suffix    { push(makePeekNot(pop())); }
            | suffix

suffix     = primary (QUESTION { push(makeQuery(pop())); }
                     | STAR    { push(makeStar (pop())); }
                     | PLUS    { push(makePlus (pop())); }
                     )?

primary    = identifier                 { push(makeVariable(yytext)); }
                COLON identifier !EQUAL { Node *name= makeName(findRule(yytext));  name->name.variable= pop();  push(name); }
           | identifier !EQUAL          { push(makeName(findRule(yytext))); }
           | OPEN expression CLOSE
           | literal                    { push(makeString(yytext)); }
           | class                      { push(makeClass(yytext)); }
           | DOT                        { push(makeDot()); }
           | action                     { push(makeAction(yytext)); }
           | macro                      { push(makeAction(fetchMacro(yytext))); }
           | BEGIN                      { push(makeMark("YY_SEND(begin_, yystack)")); }
           | END                        { push(makeMark("YY_SEND(end_, yystack)")); }
           | MARK                       { push(makeAction("YY_SEND(mark_, yyrulename);")); }
           | COLLECT                    { push(makeAction("YY_SEND(collect_, yyrulename);")); }

# Lexical syntax

macro      = !directive '%' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

directive  = '%{' | '%define' | '%declare' | '%export' | '%%' | '%begin' | '%end' | '%}'

identifier =  < [-a-zA-Z_][-a-zA-Z_0-9]* > -

literal    = ['] < ( !['] char )* > ['] -
           | ["] < ( !["] char )* > ["] -

class      = '[' < ( !']' range )* > ']' -

range      = char '-' char
           | char

char       = '\\' [abefnrtv'"\[\]\\]
           | '\\' [0-3][0-7][0-7]
           | '\\' [0-7][0-7]?
           | !'\\' .

action     = '{' < braces* > '}' -

braces     =    '{' (!'}' .)* '}'
           |    !'}' .

EQUAL     = '=' -
COLON     = ':' -
SEMICOLON = ';' -
BAR       = '|' -
AND       = '&' -
NOT       = '!' -
QUESTION  = '?' -
STAR      = '*' -
PLUS      = '+' -
OPEN      = '(' -
CLOSE     = ')' -
DOT       = '.' -
BEGIN     = '<' -
END       = '>' -
MARK      = '@' -
COLLECT   = '$' -
RPERCENT  = '%}' -

-           = (space | comment)*
space       = ' ' | '\t' | end-of-line
comment     = '#' (!end-of-line .)* end-of-line
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.
