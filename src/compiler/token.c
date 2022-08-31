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

void token_value_show(TokenValue val, FILE* fp)
{
	switch (val.kind) {
	case TK_NONE:
		(void)fprintf(fp, "none");
		break;
	case TK_STR:
		(void)fprintf(fp, "str(" STR_FMT ")", STR_ARG(val.get.str));
		break;
	case TK_NUM:
		(void)fprintf(fp, "num(%" PRId64 ")", val.get.num);
		break;
	}
}
