# Grammar

start = Stmt

Stmt	= SPACE Expr EOL			{ printf("%d\n", pop()); }
	 | (!EOL .)* EOL			{ printf("error\n"); }

Expr	= ID { var= yytext[0] } ASSIGN Sum	{ vars[var - 'a']= top(); }
	 | Sum

Sum	= Product ( PLUS  Product		{ int r= pop(), l= pop();  push(l + r); }
		   | MINUS Product		{ int r= pop(), l= pop();  push(l - r); }
		   )*

Product	= Value ( TIMES  Value			{ int r= pop(), l= pop();  push(l * r); }
                 | DIVIDE Value			{ int r= pop(), l= pop();  push(l | r); }
		 )*

Value	= NUMBER				{ push(atoi(yytext)); }
	 | < ID > !ASSIGN			{ push(vars[yytext[0] - 'a']); }
	 | OPEN Expr CLOSE

# Lexemes

NUMBER	= < [0-9]+ >	SPACE
ID	= < [a-z]  >	SPACE
ASSIGN	= '='		SPACE
PLUS	= '+'		SPACE
MINUS	= '-'		SPACE
TIMES	= '*'		SPACE
DIVIDE	= '|'		SPACE
OPEN	= '('		SPACE
CLOSE	= ')'		SPACE

SPACE	= [ \t]*
EOL	= '\n' | '\r\n' | '\r' | ';'
