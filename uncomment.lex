%{
/*
 * uncomment.lex - strip '#'-to-newline comments and blank lines
 */
#include <stdio.h>
%}

%%
[ \t]*\#[^\n]*\n  { }
[\n]+             { printf("\n"); }
.                 { ECHO; }
%%
