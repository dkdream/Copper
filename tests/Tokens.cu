
KEYWORD = Ambiguous_kw
        | As_kw
        | AssignForward_kw
        | Assign_kw
        | Begin_kw
        | Case_kw
        | Catch_kw
        | Comma_kw
        | Context_kw
        | Continuation_kw
        | Continue_kw
        | Declare_kw
        | Define_kw
        | Directive_kw
        | Do_kw
        | Done_kw
        | Else_kw
        | Empty_kw
        | End_kw
        | Ensure_kw
        | Extends_kw
        | Equal_kw
        | Finally_kw
        | For_kw
        | Fork_kw
        | If_kw
        | In_kw
        | Is_kw
        | Join_kw
        | Let_kw
        | Loop_kw
        | Meet_kw
        | Next_kw
        | On_kw
        | Otherwise_kw
        | Prototype_kw
        | Reply_kw
        | Rest_kw
        | Return_kw
        | Select_kw
        | Signal_kw
        | Skip_kw
        | Start_kw
        | Stop_kw
        | Takes_kw
        | Throw_kw
        | Trap_kw
        | Type_kw
        | Typeof_kw
        | Unless_kw
        | When_kw
        | With_kw
        | VoidType_kw
        | BooleanType_kw
        | ImmutableType_kw
        | SymbolType_kw
        | TypeType_kw
        | ListType_kw
        | ArrayType_kw
        | HashType_kw
        | DelayType_kw
        | LambdaType_kw
        | SetType_kw

#
# values
#
Text_variable  = dquote < (!dquote char)* > dquote             Spacing @push_text_variable
Text_constant  = squote < (!squote char)* > squote             Spacing @push_text_constant
Regex_constant = percent 'r' Open < (!Close char)* > Close     Spacing @push_regex
Meta_constant  = dollor < letter ( letter | digit | break )* > Spacing @push_meta

Number_constant = < ( digit | break )+ > Spacing @push_number

Hex      = < '0x' ( digit | [_abcdefABCDEF] )+ >        Spacing @push_hexinteger
Oct      = < '0o' ( [_0-7] )+ >                         Spacing @push_octinteger
Binary   = < '0b' ( [_01] )+ >                          Spacing @push_bininteger
Integer  = < ( digit )+ >                                       @push_integer
Rational = Integer slash Integer                        Spacing @push_rational
Float    = < ( digit )+ dot ( digit )+ 'e' ( digit )+ > Spacing @push_float
         | < ( digit )+ dot ( digit )+ >                Spacing @push_float

Number = Hex | Oct | Binary | Float | Number_constant

#
# labels
#
Name_label     = !KEYWORD < letter ( letter | digit | break )* > !colon             Spacing @push_name
Operator_label = !KEYWORD < ( operator | equal | letter | digit | break )+ > !colon Spacing @push_operator
Access_label   = colon < letter ( letter | break )* >                               Spacing @push_access
Member_label   = at < ( letter | break ) ( letter | digit | break )* >              Spacing @push_member
Tag_label      = < letter ( letter | digit | break )* > colon                       Spacing @push_tag

Symbol_label  = colon < letter ( letter | digit | break | colon )* > Spacing @push_symbol
QSymbol_label = colon squote < (!squote char)* > squote              Spacing @push_symbol
Send_label    = dot < letter ( letter | digit | break )* >           Spacing @push_unary

#
# structural tokens
#

Open        = popen        AnySpace # (
Close       = pclose       Spacing  # )
OpenIndex   = sopen        AnySpace # [
CloseIndex  = sclose       Spacing  # ]
OpenSet     = less         AnySpace # < maybe
CloseSet    = greater      Spacing  # > maybe
OpenDelay   = copen        AnySpace # {
CloseDelay  = cclose       Spacing  # }

Comma_kw = comma     AnySpace @push_keyword
Rest_kw  = semicolon AnySpace @push_keyword

Ambiguous_kw     = 'ambiguous'    !LABEL AnySpace    @push_keyword
As_kw            = 'as'           !LABEL AnySpace    @push_keyword
AssignForward_kw = '->'           !OPERATOR AnySpace @push_keyword
Assign_kw        = '<-'           !OPERATOR AnySpace @push_keyword
Begin_kw         = 'begin'        !LABEL AnySpace    @push_keyword
Case_kw          = 'case'         !LABEL AnySpace    @push_keyword
Catch_kw         = 'catch'        !LABEL AnySpace    @push_keyword
Context_kw       = 'context'      !LABEL AnySpace    @push_keyword
Continuation_kw  = 'continuation' !LABEL AnySpace    @push_keyword
Continue_kw      = 'continue'     !LABEL AnySpace    @push_keyword
Declare_kw       = 'declare'      !LABEL AnySpace    @push_keyword
Define_kw        = 'define'       !LABEL AnySpace    @push_keyword
Directive_kw     = 'directive'    !LABEL AnySpace    @push_keyword
Do_kw            = 'do'           !LABEL AnySpace    @push_keyword
Done_kw          = 'done'         !LABEL AnySpace    @push_keyword
Else_kw          = 'else'         !LABEL AnySpace    @push_keyword
Empty_kw         = 'empty'        !LABEL AnySpace    @push_keyword
End_kw           = 'end'          !LABEL AnySpace    @push_keyword
Ensure_kw        = 'ensure'       !LABEL AnySpace    @push_keyword
Equal_kw         = '='            !OPERATOR AnySpace @push_keyword
Extends_kw       = 'extends'      !LABEL AnySpace    @push_keyword
For_kw           = 'for'          !LABEL AnySpace    @push_keyword
Finally_kw       = 'finally'      !LABEL AnySpace    @push_keyword
Fork_kw          = 'fork'         !LABEL AnySpace    @push_keyword
If_kw            = 'if'           !LABEL AnySpace    @push_keyword
In_kw            = 'in'           !LABEL AnySpace    @push_keyword
Is_kw            = 'is'           !LABEL AnySpace    @push_keyword
Join_kw          = 'join'         !LABEL AnySpace    @push_keyword
Let_kw           = 'let'          !LABEL AnySpace    @push_keyword
Loop_kw          = 'loop'         !LABEL AnySpace    @push_keyword
Meet_kw          = 'meet'         !LABEL AnySpace    @push_keyword
Next_kw          = 'next'         !LABEL AnySpace    @push_keyword
On_kw            = 'on'           !LABEL AnySpace    @push_keyword
Otherwise_kw     = 'otherwise'    !LABEL AnySpace    @push_keyword
Prototype_kw     = 'prototype'    !LABEL AnySpace    @push_keyword
Reply_kw         = 'reply'        !LABEL AnySpace    @push_keyword
Return_kw        = 'return'       !LABEL AnySpace    @push_keyword
Select_kw        = 'select'       !LABEL AnySpace    @push_keyword
Signal_kw        = 'Signal'       !LABEL AnySpace    @push_keyword
Skip_kw          = '__'           !OPERATOR AnySpace @push_keyword
Start_kw         = 'start'        !LABEL AnySpace    @push_keyword
Stop_kw          = 'stop'         !LABEL AnySpace    @push_keyword
Takes_kw         = 'takes'        !LABEL AnySpace    @push_keyword
Throw_kw         = 'throw'        !LABEL AnySpace    @push_keyword
Trap_kw          = 'trap'         !LABEL AnySpace    @push_keyword
Type_kw          = 'type'         !LABEL AnySpace    @push_keyword
Typeof_kw        = 'typeof'       !LABEL AnySpace    @push_keyword
Unless_kw        = 'unless'       !LABEL AnySpace    @push_keyword
When_kw          = 'when'         !LABEL AnySpace    @push_keyword
With_kw          = 'with'         !LABEL AnySpace    @push_keyword
void_kw          = 'void'         !LABEL AnySpace    @push_keyword
true_kw          = 'true'         !LABEL AnySpace    @push_keyword
false_kw         = 'false'        !LABEL AnySpace    @push_keyword
VoidType_kw      = 'Void'         !LABEL AnySpace    @push_keyword
BooleanType_kw   = 'Boolean'      !LABEL AnySpace    @push_keyword
ImmutableType_kw = 'Immutable'    !LABEL AnySpace    @push_keyword
SymbolType_kw    = 'Symbol'       !LABEL AnySpace    @push_keyword
TypeType_kw      = 'Type'         !LABEL AnySpace    @push_keyword
ListType_kw      = 'Tuple'        !LABEL AnySpace    @push_keyword
ArrayType_kw     = 'Array'        !LABEL AnySpace    @push_keyword
HashType_kw      = 'Map'          !LABEL AnySpace    @push_keyword
DelayType_kw     = 'Delay'        !LABEL AnySpace    @push_keyword
LambdaType_kw    = 'Lambda'       !LABEL AnySpace    @push_keyword
SetType_kw       = 'Set'          !LABEL AnySpace    @push_keyword

_ = AnySpace

Spacing = (Space | BlockComment | LineComment)*

AnySpace = (Space | BlockComment | LineComment | EndOfLine )*

LineEnd = EndOfLine Spacing

LineComment  = '//' (!EndOfLine .)*

BlockComment = BeginComment (!EndComment .)* EndComment

BeginComment = '/' '*'
EndComment   = '*' '/'
Space        = ' ' | '\t'
EndOfLine    = creturn newline | newline | creturn

char = bslash [abefnrtv'"\[\]\\/]
     | bslash dollor
     | bslash [0-3][0-7][0-7]
     | bslash [0-7][0-7]?
     | !bslash .

#
# character token 
#

LABEL    = letter | digit | break
OPERATOR = operator | equal | break

letter   = [a-zA-Z]
digit    = [0-9]
operator = plus | minus | star | slash | bang | or | and | less | greater | tilde | hat | hash | btic

plus      = '+'
minus     = '-'
star      = '*'
slash     = '/'
equal     = '='
bang      = '!'
or        = '|'
and       = '&'
less      = '<'
greater   = '>'
tilde     = '~'
hat       = '^'
break     = '_'
hash      = '#'
btic      = '`'

dot       = '.'
colon     = ':'
popen     = '('
pclose    = ')'
sopen     = '['
sclose    = ']'
copen     = '{'
cclose    = '}'
bslash    = '\\'
at        = '@'
dollor    = '$'
percent   = '%'
comma     = ','
semicolon = ';'
squote    = [']
dquote    = ["]
newline   = '\n'
creturn   = '\r'
