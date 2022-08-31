#include <dragon/core/arg.h>
#include <dragon/core/file.h>
#include <dragon/lexer.h>

#include <stdio.h>

int main(int argc, char** argv)
{
	Arg fileArg =
	        ARG_POS(str_lit("FILE"), str_lit("The file to compile"));
	Arg assemblyArg =
	        ARG_FLAG(
	                .shortname = 'S',
	                .longname = str_lit("assembly"),
	                .help = str_lit("Output assembly instead of executable")
	        );
	Arg dumpAstArg =
	        ARG_FLAG(
	                .longname = str_lit("dump-ast"),
	                .help = str_lit("Dump the AST to stdout, don't compile"),
	        );
	Arg helpArg =
	        ARG_FLAG(
	                .shortname = 'h',
	                .longname = str_lit("help"),
	                .help = str_lit("Show this help message")
	        );
	Arg outputArg =
	        ARG_OPT(
	                .shortname = 'o',
	                .longname = str_lit("output"),
	                .help = str_lit("The output file"),
	        );
	Arg* acceptedOptions[] = {
		&fileArg,
		&assemblyArg,
		&dumpAstArg,
		&helpArg,
		&outputArg,
	};

	ArgParser parser = argparser_new(
	                           str_lit("dragonk"),
	                           str_lit("C compiler"),
	                           (ArgBuf)BUF_ARRAY(acceptedOptions)
	                   );

	ArgParseErr err = argparser_parse(&parser, argc, argv);
	if (helpArg.flagValue) {
		argparser_show_help(&parser, stdout);
		return 0;
	}

	if (err.present) {
		argparser_show_help(&parser, stderr);
		(void)fprintf(stderr, "ERROR: " STR_FMT "\n", STR_ARG(err.value));
		str_free(err.value);
		return 1;
	}

	SlurpFileResult slurpRes = slurp_file(fileArg.value);
	if (!slurpRes.ok) {
		(void)fprintf(stderr, "ERROR: " STR_FMT "\n", STR_ARG(slurpRes.get.error));
		str_free(slurpRes.get.error);
		return 1;
	}
	str inputContents = slurpRes.get.value;

	Lexer lexer =
	        lexer_new(
	                str_ref(inputContents),
	                str_ref(fileArg.value)
	        );
	for (
	        Token tok = lexer_first(&lexer);
	        !lexer_done(&lexer);
	        tok = lexer_next(&lexer)
	) {
		if (tok.type == TT_ERROR) {
			(void)fprintf(
			        stderr,
			        SOURCE_LOCATION_FMT ": unrecognized token: '" STR_FMT "'\n",
			        SOURCE_LOCATION_ARG(tok.location),
			        STR_ARG(tok.text)
			);
			token_free(tok);
			str_free(inputContents);
			return 1;
		}
		printf(
		        SOURCE_LOCATION_FMT ": %s\n",
		        SOURCE_LOCATION_ARG(tok.location),
		        TOKEN_STRINGS[tok.type]
		);
		token_free(tok);
	}

	str_free(inputContents);
	return 0;
}
