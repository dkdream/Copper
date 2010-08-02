# Copper Grammar for copper (vm)

# Parser syntax

grammar = - ( heading )? ( define-rule )+ end-of-file

heading = '%header' thunk { makeHeader(input); }

define-rule = identifier EQUAL       { checkRule(input); }
                         expression  { defineRule(input); }
                         SEMICOLON?

expression  = sequence ( BAR sequence  { makeAlternate(input); }  )*

sequence    = prefix ( prefix { makeSequence(input); } )*

prefix      = AND suffix { makePeekFor(input); }
            | NOT suffix { makePeekNot(input); }
            | suffix

suffix     = primary (QUESTION { makeQuery(input); }
                     | STAR    { makeStar(input)); }
                     | PLUS    { makePlus(input); }
                     )?

primary    = identifier !EQUAL     { makeName(input); }
           | OPEN expression CLOSE
           | literal               { makeString(input);    }
           | class                 { makeClass(input);     }
           | DOT                   { makeDot(input);       }
           | predicate             { makePredicate(input); }
           | event                 { makeEvent(input);     }
           | thunk                 { makeThunk(input);     }
           | BEGIN                 { makeBegin(input);     }
           | END                   { makeEnd(input);       }

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

