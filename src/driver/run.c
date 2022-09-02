#include "dragon/driver/run.h"

#include <stdio.h>
#include <stdlib.h>

#include "dragon/ast.h"
#include "dragon/codegen.h"
#include "dragon/core/arg.h"
#include "dragon/core/buf.h"
#include "dragon/core/dir.h"
#include "dragon/core/file.h"
#include "dragon/core/process.h"
#include "dragon/core/str.h"
#include "dragon/parser.h"

int run(CArgBuf args, FILE* out, FILE* err)
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

	ArgParseErr argParseErr = argparser_parse(&parser, (int)args.len, args.ptr);
	if (helpArg.flagValue) {
		argparser_show_help(&parser, stdout);
		return 0;
	}

	if (argParseErr.present) {
		argparser_show_help(&parser, stderr);
		(void)fprintf(err, "ERROR: " STR_FMT "\n", STR_ARG(argParseErr.value));
		str_free(argParseErr.value);
		return 1;
	}

	SlurpFileResult slurpRes = slurp_file(fileArg.value);
	if (!slurpRes.ok) {
		(void)fprintf(err, "ERROR: " STR_FMT "\n", STR_ARG(slurpRes.get.error));
		str_free(slurpRes.get.error);
		return 1;
	}
	str inputContents = slurpRes.get.value;

	Parser p = parser_new(inputContents, fileArg.value);
	ProgramResult program_result = parser_parse(&p);
	parser_free(p);
	if (!program_result.ok) {
		(void)fprintf(err, "ERROR: " STR_FMT "\n", STR_ARG(program_result.get.error));
		str_free(program_result.get.error);
		return 1;
	}

	Program program = program_result.get.value;

	str outPath = outputArg.value;

	if (dumpAstArg.flagValue) {
		str s = program_to_str(program);
		(void)fprintf(out, STR_FMT, STR_ARG(s));
		str_free(s);
	} else if (assemblyArg.flagValue) {
		if (str_len(outPath) == 0) {
			outPath = str_lit("a.s");
		}
		codegen_program(program, outPath);
	} else {
		if (str_len(outPath) == 0) {
			outPath = str_lit("a.out");
		}
		char templ[] = "dragonk-XXXXXX";
		char* tempDir = mkdtemp(templ);
		str tempPath = path_join(str_ref(tempDir), str_lit("a.s"));
		codegen_program(program, tempPath);
		str objPath = path_join(str_ref(tempDir), str_lit("a.o"));
		// *INDENT-OFF*
		ProcessCreateResult nasmProcessResult = process_run(
			(ProcessCStrBuf)BUF_ARRAY(((const char* []) {
				"nasm",
				"-f",
				"elf64",
				tempPath.ptr,
				"-o",
				objPath.ptr
			})),
			PROCESS_OPTION_SEARCH_USER_PATH | PROCESS_OPTION_COMBINED_STDOUT_STDERR
		);
		// *INDENT-ON*
		if (!nasmProcessResult.present || nasmProcessResult.value.returnCode != 0) {
			(void)fprintf(err, "ERROR: running nasm failed\n");
			del_dir(str_ref(tempDir));
			return 1;
		}
		process_destroy(&nasmProcessResult.value);

		str_free(tempPath);

		// *INDENT-OFF*
		ProcessCreateResult ldProcessResult = process_run(
		        (ProcessCStrBuf)BUF_ARRAY(((const char* []) {
		                "ld",
		                objPath.ptr,
		                "-o",
		                outPath.ptr
		        })),
		        PROCESS_OPTION_SEARCH_USER_PATH | PROCESS_OPTION_COMBINED_STDOUT_STDERR
		);
		// *INDENT-ON*
		if (!ldProcessResult.present || ldProcessResult.value.returnCode != 0) {
			(void)fprintf(err, "ERROR: running ld failed\n");
			del_dir(str_ref(tempDir));
			return 1;
		}
		process_destroy(&ldProcessResult.value);

		str_free(objPath);
		del_dir(str_ref(tempDir));
	}

	program_free(program);
	str_free(inputContents);
	return 0;
}
