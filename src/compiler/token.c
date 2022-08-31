#include "dragon/token.h"

const char* TOKEN_STRINGS[] = {
#define X(x) #x,
#include "dragon/token_type.def"
#undef X
};

void token_free(Token tok)
{
	str_free(tok.text);
	if (tok.value.kind == TK_STR) {
		str_free(tok.value.get.str);
	}
	str_free(tok.location.filename);
}
