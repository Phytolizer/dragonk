#include "dragon/driver/run.h"

#include <stdio.h>

#include "dragon/ast.h"
#include "dragon/codegen.h"
#include "dragon/core/arg.h"
#include "dragon/core/buf.h"
#include "dragon/core/file.h"
#include "dragon/core/str.h"
#include "dragon/parser.h"

int run(CArgBuf args)
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

	ArgParseErr err = argparser_parse(&parser, (int)args.len, args.ptr);
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

	Parser p = parser_new(inputContents, fileArg.value);
	ProgramResult program_result = parser_parse(&p);
	parser_free(p);
	if (!program_result.ok) {
		(void)fprintf(stderr, "ERROR: " STR_FMT "\n", STR_ARG(program_result.get.error));
		str_free(program_result.get.error);
		return 1;
	}

	Program program = program_result.get.value;

	str outPath = outputArg.value;

	if (dumpAstArg.flagValue) {
		str s = program_to_str(program);
		printf(STR_FMT, STR_ARG(s));
		str_free(s);
	} else if (assemblyArg.flagValue) {
		if (str_len(outPath) == 0) {
			outPath = str_lit("a.s");
		}
		codegen_program(program, outPath);
	}

	program_free(program);
	str_free(inputContents);
	return 0;
}
