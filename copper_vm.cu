# Copper Grammar for copper (vm)
# Parser syntax

%{
#include "syntax.h"
%}

grammar = - ( heading )? ( define-rule )+ end-of-file

heading = '%header' thunk { makeHeader(input,cursor); }

define-rule = identifier EQUAL       { checkRule(input,cursor); }
                         expression  { defineRule(input,cursor); }
                         SEMICOLON?

expression  = sequence ( BAR sequence  { makeChoice(input,cursor); }  )*

sequence    = prefix ( prefix { makeSequence(input,cursor); } )*

prefix      = AND suffix { makeCheck(input,cursor); }
            | NOT suffix { makeNot(input,cursor); }
            | suffix

suffix     = primary (QUESTION { makeQuestion(input,cursor); }
                     | STAR    { makeStar(input,cursor); }
                     | PLUS    { makePlus(input,cursor); }
                     )?

primary    = identifier !EQUAL     { makeCall(input,cursor); }
           | OPEN expression CLOSE
           | literal               { makeString(input,cursor);    }
           | class                 { makeSet(input,cursor);     }
           | DOT                   { makeDot(input,cursor);       }
           | predicate             { makePredicate(input,cursor); }
           | event                 { makeApply(input,cursor);     }
           | thunk                 { makeThunk(input,cursor);     }
           | BEGIN                 { makeBegin(input,cursor);     }
           | END                   { makeEnd(input,cursor);       }

# Lexer syntax

predicate  = !directive  '%' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

event      = '@' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

directive  = '%header' | '%begin' | '%end'

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

thunk      = '{' < braces* > '}' -

braces     = '{' braces* '}'
           |  !'}' .

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

-           = (space | comment)*
space       = ' ' | '\t' | end-of-line
comment     = '#' (!end-of-line .)* end-of-line
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.

