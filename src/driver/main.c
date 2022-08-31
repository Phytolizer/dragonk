#include <dragon/lexer.h>
#include <stdio.h>

int main()
{
	Lexer lexer = lexer_new(
	                      str_lit("int main() { return 2; }"),
	                      str_lit("<memory>")
	              );
	for (Token tok = lexer_first(&lexer); !lexer_done(&lexer); tok = lexer_next(&lexer)) {
		printf("%s\n", TOKEN_STRINGS[tok.type]);
	}
	return 0;
}
