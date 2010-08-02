# Copper Grammar for copper (vm)

grammar = - heading ( define-rule )+ end-of-file

heading =  '%header' action { makeHeader(input); }

define-rule = identifier EQUAL       { checkRule(input); }
                         expression  { defineRule(input); }
                         SEMICOLON?

expression  = sequence ( BAR sequence  { makeAlternate(input); }  )*

sequence    = prefix ( prefix { makeSequence(input); } )*

prefix      = AND suffix { makePeekFor(input); }
            | NOT suffix { makePeekNot(input); }
            | suffix

suffix     = macro             { makePredicate(input); }
           | primary (QUESTION { makeQuery(input); }
                     | STAR    { makeStar(input)); }
                     | PLUS    { makePlus(input); }
                     )?

primary    = identifier !EQUAL     { makeName(input); }
           | OPEN expression CLOSE
           | literal               { makeString(input); }
           | class                 { makeClass(input); }
           | DOT                   { makeDot(input); }
           | macro                 { makeAction(input); }
           | action                { makeEvent(input); }
           | BEGIN                 { makeBegin(input); }
           | END                   { makeEnd(input); }

# Lexical syntax

macro      = !directive '%' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

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

action     = '{' < braces* > '}' -

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

