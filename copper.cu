# Copper Grammar for copper

%{
#include "copper.inc"
%}

# Hierarchical syntax

grammar=	- ( declaration | definition )+ trailer? end-of-file

declaration=	'%{' < ( !'%}' . )* > RPERCENT		{ makeHeader(yytext); }

trailer=	'%%' < .* >				{ makeTrailer(yytext); }

definition=	identifier 				{ checkRule(yytext); }
			EQUAL ( expression end        	{ defineRule(rule_with_end); }
                              | expression              { defineRule(simple_rule); }
                              | begin expression end    { defineRule(rule_with_both); }
                              | begin expression        { defineRule(rule_with_begin); }
                              ) SEMICOLON?

begin=          '%begin' - action                       { push(makePredicate(yytext)); }
end=            '%end'   - action                       { push(makePredicate(yytext)); }

expression=	sequence (BAR sequence			{ Node *f= pop();  push(Alternate_append(pop(), f)); }
			    )*

sequence=	prefix (prefix				{ Node *f= pop();  push(Sequence_append(pop(), f)); }
			  )*

prefix=		AND action				{ push(makePredicate(yytext)); }
|		AND suffix				{ push(makePeekFor(pop())); }
|		NOT suffix				{ push(makePeekNot(pop())); }
|		    suffix

suffix=		primary (QUESTION			{ push(makeQuery(pop())); }
			     | STAR			{ push(makeStar (pop())); }
			     | PLUS			{ push(makePlus (pop())); }
			   )?

primary=	identifier				{ push(makeVariable(yytext)); }
			COLON identifier !EQUAL		{ Node *name= makeName(findRule(yytext));  name->name.variable= pop();  push(name); }
|		identifier !EQUAL			{ push(makeName(findRule(yytext))); }
|		OPEN expression CLOSE
|		literal					{ push(makeString(yytext)); }
|		class					{ push(makeClass(yytext)); }
|		DOT					{ push(makeDot()); }
|		action					{ push(makeAction(yytext)); }
|		BEGIN					{ push(makeMark("YY_SEND(begin_, yystack)")); }
|		END					{ push(makeMark("YY_SEND(end_, yystack)")); }
|		MARK					{ push(makeAction("YY_SEND(mark_, yyrulename);")); }
|		COLLECT					{ push(makeAction("YY_SEND(collect_, yyrulename);")); }

# Lexical syntax

identifier=	< [-a-zA-Z_][-a-zA-Z_0-9]* > -

literal=	['] < ( !['] char )* > ['] -
|		["] < ( !["] char )* > ["] -

class=		'[' < ( !']' range )* > ']' -

range=		char '-' char | char

char=		'\\' [abefnrtv'"\[\]\\]
|		'\\' [0-3][0-7][0-7]
|		'\\' [0-7][0-7]?
|		!'\\' .

action=		'{' < braces* > '}' -

braces=		'{' (!'}' .)* '}'
|		!'}' .

EQUAL=		'=' -
COLON=		':' -
SEMICOLON=	';' -
BAR=		'|' -
AND=		'&' -
NOT=		'!' -
QUESTION=	'?' -
STAR=		'*' -
PLUS=		'+' -
OPEN=		'(' -
CLOSE=		')' -
DOT=		'.' -
BEGIN=		'<' -
END=		'>' -
MARK=           '@' -
COLLECT=        '$' -
RPERCENT=	'%}' -

-=		(space | comment)*
space=		' ' | '\t' | end-of-line
comment=	'#' (!end-of-line .)* end-of-line
end-of-line=	'\r\n' | '\n' | '\r'
end-of-file=	!.

%%
