
start	= (line | word | char)

line	= < (( '\n' '\r'* ) | ( '\r' '\n'* )) >	{ lines++;  chars += yyleng; }
word	= < [a-zA-Z]+ >				{ words++;  chars += yyleng;  printf("<%s>\n", yytext); }
char	= .					{ chars++; }
