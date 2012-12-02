# Copper Grammar for copper (vm)
# Parser syntax

grammar = ( - define-rule )+
          end-of-file @writeTree

define-rule = identifier       @checkRule
              EQUAL expression @defineRule

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
           | START expression SEMICOLON expression STOP @makeLoop
           | OPEN expression CLOSE
           | literal
           | class
           | DOT
           | predicate ( argument )?
           | event     ( argument )?
           | BEGIN                 
           | END

# Lexer syntax

predicate  = '%'  < [-a-zA-Z_][-a-zA-Z_0-9]* >     - @makePredicate
event      = '@'  < [-a-zA-Z_][-a-zA-Z_0-9]* >     - @makeApply
argument   = ':[' < [-a-zA-Z_][-a-zA-Z_0-9]* > ']' - @makeArgument
identifier =      < [-a-zA-Z_][-a-zA-Z_0-9]* >     -

literal    = ['] < ( !['] char )* > ['] - @makeString
           | ["] < ( !["] char )* > ["] - @makeString

class      = '[' < ( !']' range )* > ']' - @makeSet

range      = char '-' char
           | char

char       = '\\' [abefnrtv'"\[\]\\]
           | '\\' [0-3][0-7][0-7]
           | '\\' [0-7][0-7]?
           | !'\\' !end-of-line .

EQUAL     = '=' -
BAR       = '|' -
AND       = '&' -
NOT       = '!' -
QUESTION  = '?' - 
STAR      = '*' - 
PLUS      = '+' -
OPEN      = '(' -
CLOSE     = ')' -
DOT       = '.' - @makeDot
BEGIN     = '<' - @makeBegin
END       = '>' - @makeEnd

START     = '{' -
SEMICOLON = ';' -
STOP      = '}' -


-           = (space | comment)*
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' (!end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.

