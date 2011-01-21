# Copper Grammar for copper (vm)
# Parser syntax

grammar = ( - heading )?
          ( - define-rule )+
          end-of-file @writeTree

heading = '%header' - thunk @makeHeader

define-rule = identifier       @checkRule
              EQUAL expression @defineRule
              SEMICOLON?

expression  = sequence ( BAR expression @makeChoice )?

sequence    = prefix ( sequence @makeSequence )?

prefix      = AND suffix @makeCheck
            | NOT suffix @makeNot
            | suffix

suffix     = primary (QUESTION @makeQuestion
                     | STAR    @makeStar
                     | PLUS    @makePlus
                     )?

primary    = identifier !EQUAL     @makeCall
           | OPEN expression CLOSE
           | literal               @makeString
           | class                 @makeSet
           | DOT                   @makeDot
           | predicate             @makePredicate
           | event                 @makeApply
           | thunk                 @makeThunk
           | BEGIN                 @makeBegin
           | END                   @makeEnd

# Lexer syntax

predicate  = !directive  '%' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

event      = '@' < [-a-zA-Z_][-a-zA-Z_0-9]* > -

directive  = '%header'

identifier = < [-a-zA-Z_][-a-zA-Z_0-9]* > -

literal    = ['] < ( !['] char )* > ['] -
           | ["] < ( !["] char )* > ["] -

class      = '[' < ( !']' range )* > ']' -

range      = char '-' char
           | char

char       = '\\' [abefnrtv'"\[\]\\]
           | '\\' [0-3][0-7][0-7]
           | '\\' [0-7][0-7]?
           | !'\\' !end-of-line .

thunk      = '{' < braces* > '}' - 

braces     = '{' braces* '}'
           |  !'}' .

EQUAL     = '=' -
COLON     = ':' -
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
SEMICOLON = ';' -

-           = (space | comment)*
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' (!end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.

