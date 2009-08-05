
start	= (line | word | char)

%define %add-char { chars++; }
%define %add-line { lines++; chars += yyleng; }
%define %add-word { words++;  chars += yyleng;  printf("<%s>\n", yytext); }

line	= < (( '\n' '\r'* ) | ( '\r' '\n'* )) >	%add-line
word	= < [a-zA-Z]+ >				%add-word
char	= .					%add-char
