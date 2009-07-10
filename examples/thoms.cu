
start = Spacing body(EndOfLine|EndOfFile){ printf(". "); }|EndOfFile { printf("DONE "); }

body = field{ printf("(field)") }(Dot_kw field{ printf(".field\n") })*

field = a1 | a2 | a3 | a4 | a5

a1 = %begin { printf("begin %s\n", yyrulename); }
      A1_kw { printf("a1 "); } { printf("[%s]", yytext); }
      B_kw  { printf("ab1 "); } { printf("[%s]", yytext); }
     %end   { printf("end %s\n", yyrulename); }


a2 = A2_kw  { printf("a2 "); } { printf("[%s]", yytext); }
      C_kw  { printf("ac2 "); } { printf("[%s]", yytext); }

a3 =  A3_kw { printf("a3 "); }
            ( D_kw  { printf("ad3 "); }
            | E_kw  { printf("ae3 "); } )

a4 =  A_kw { printf("a4 "); }
            ( F_kw { printf("af4a "); }
              G_kw { printf("afg4 "); }
	    | F_kw { printf("af4b "); }
              H_kw { printf("afh4 "); } )

a5 =  A_kw { printf("a5 "); $$=1; }
            ( F_kw &{ doMatch("af5a", 'i') } { printf("af5a ") } 
              I_kw &{ printf("afi5? ") }     { printf("afi5 ") }
	    | F_kw &{ doMatch("af5b", 'j') } { printf("af5b ") }
              J_kw &{ printf("afj5? ") }     { printf("afj5 ") } )

# Lexical syntax
A_kw         = 'a' { printf("(a)"); }
A1_kw        = 'a' { printf("(a1)"); }
A2_kw        = 'a' { printf("(a2)"); }
A3_kw        = 'a' { printf("(a3)"); }
B_kw         = 'b'
C_kw         = 'c'
D_kw         = 'd'
E_kw         = 'e'
F_kw         = 'f'
G_kw         = 'g'
H_kw         = 'h'
I_kw         = 'i'
J_kw         = 'j'
Dot_kw       = '.'
Spacing      = (Space | LineComment | BlockComment)*
LineComment  = '#' (!EndOfLine .)* EndOfLine
BlockComment = BeginComment (!EndComment .)* EndComment
Space        = ' ' | '\t' | EndOfLine
EndOfLine    = '\r\n' | '\n' | '\r'
EndOfFile    = !. { YY_DONE; }
BeginComment = '/' '*'
EndComment   = '*' '/'

