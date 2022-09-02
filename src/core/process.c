#include "dragon/core/process.h"

#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

ProcessCreateResult process_create(ProcessCStrBuf commandLine, ProcessOption options)
{
	int stdinfd[2];
	int stdoutfd[2];
	int stderrfd[2];
	pid_t child;

	if (pipe(stdinfd) != 0) {
		return (ProcessCreateResult)NOTHING;
	}
	if (pipe(stdoutfd) != 0) {
		close(stdinfd[0]);
		close(stdinfd[1]);
		return (ProcessCreateResult)NOTHING;
	}

	if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
		if (pipe(stderrfd) != 0) {
			close(stdinfd[0]);
			close(stdinfd[1]);
			close(stdoutfd[0]);
			close(stdoutfd[1]);
			return (ProcessCreateResult)NOTHING;
		}
	} else {
		stderrfd[0] = stdoutfd[0];
		stderrfd[1] = stdoutfd[1];
	}

	posix_spawn_file_actions_t actions;
	if (posix_spawn_file_actions_init(&actions) != 0) {
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	if (posix_spawn_file_actions_addclose(&actions, stdinfd[1]) != 0) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	if (posix_spawn_file_actions_adddup2(&actions, stdinfd[0], STDIN_FILENO) != 0) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	if (posix_spawn_file_actions_addclose(&actions, stdoutfd[0]) != 0) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	if (posix_spawn_file_actions_adddup2(&actions, stdoutfd[1], STDOUT_FILENO) != 0) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR)
	    == PROCESS_OPTION_COMBINED_STDOUT_STDERR) {
		if (posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO) != 0) {
			posix_spawn_file_actions_destroy(&actions);
			close(stdinfd[0]);
			close(stdinfd[1]);
			close(stdoutfd[0]);
			close(stdoutfd[1]);
			return (ProcessCreateResult)NOTHING;
		}
	} else {
		if (posix_spawn_file_actions_addclose(&actions, stderrfd[0]) != 0) {
			posix_spawn_file_actions_destroy(&actions);
			close(stdinfd[0]);
			close(stdinfd[1]);
			close(stdoutfd[0]);
			close(stdoutfd[1]);
			close(stderrfd[0]);
			close(stderrfd[1]);
			return (ProcessCreateResult)NOTHING;
		}

		if (posix_spawn_file_actions_adddup2(&actions, stderrfd[1], STDERR_FILENO) != 0) {
			posix_spawn_file_actions_destroy(&actions);
			close(stdinfd[0]);
			close(stdinfd[1]);
			close(stdoutfd[0]);
			close(stdoutfd[1]);
			close(stderrfd[0]);
			close(stderrfd[1]);
			return (ProcessCreateResult)NOTHING;
		}
	}

	typedef int (*PosixSpawnFunc)(pid_t*, const char*, const posix_spawn_file_actions_t*,
	                              const posix_spawnattr_t*, char* const*, char* const*);
	PosixSpawnFunc spawnFunc = posix_spawn;
	if ((options & PROCESS_OPTION_SEARCH_USER_PATH) == PROCESS_OPTION_SEARCH_USER_PATH) {
		spawnFunc = posix_spawnp;
	}
	extern char** environ;

	const char** argv = malloc(sizeof(const char*) * (commandLine.len + 1));
	if (argv == NULL) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		return (ProcessCreateResult)NOTHING;
	}

	for (int i = 0; i < commandLine.len; i++) {
		argv[i] = commandLine.ptr[i];
	}
	argv[commandLine.len] = NULL;

	if (spawnFunc(
	            &child,
	            argv[0],
	            &actions,
	            NULL,
	            (char* const*)argv,
	            environ
	    ) != 0) {
		posix_spawn_file_actions_destroy(&actions);
		close(stdinfd[0]);
		close(stdinfd[1]);
		close(stdoutfd[0]);
		close(stdoutfd[1]);
		if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR) == 0) {
			close(stderrfd[0]);
			close(stderrfd[1]);
		}
		free(argv);
		return (ProcessCreateResult)NOTHING;
	}

	free(argv);

	Process process = {0};

	close(stdinfd[0]);
	process.stdinFile = fdopen(stdinfd[1], "wb");
	close(stdoutfd[1]);
	process.stdoutFile = fdopen(stdoutfd[0], "rb");
	if ((options & PROCESS_OPTION_COMBINED_STDOUT_STDERR)
	    == PROCESS_OPTION_COMBINED_STDOUT_STDERR) {
		process.stderrFile = process.stdoutFile;
	} else {
		close(stderrfd[1]);
		process.stderrFile = fdopen(stderrfd[0], "rb");
	}
	process.child = child;
	process.alive = true;

	posix_spawn_file_actions_destroy(&actions);

	return (ProcessCreateResult)JUST(process);
}

ProcessJoinResult process_join(Process* process)
{
	int status;
	if (process->stdinFile != NULL) {
		(void)fclose(process->stdinFile);
		process->stdinFile = NULL;
	}

	if (process->child != 0) {
		if (waitpid(process->child, &status, 0) != process->child) {
			return (ProcessJoinResult)NOTHING;
		}

		process->child = 0;

		if (WIFEXITED(status)) {
			process->returnCode = WEXITSTATUS(status);
		} else {
			process->returnCode = EXIT_FAILURE;
		}

		process->alive = false;
	}

	return (ProcessJoinResult)JUST(process->returnCode);
}

void process_destroy(Process* process)
{
	if (process->stdinFile != NULL) {
		(void)fclose(process->stdinFile);
		process->stdinFile = NULL;
	}

	if (process->stdoutFile != NULL) {
		(void)fclose(process->stdoutFile);

		if (process->stderrFile != process->stdoutFile) {
			(void)fclose(process->stderrFile);
		}

		process->stdoutFile = NULL;
		process->stderrFile = NULL;
	}
}

ProcessCreateResult process_run(ProcessCStrBuf commandLine, ProcessOption options)
{
	ProcessCreateResult result = process_create(commandLine, options);
	if (result.present) {
		ProcessJoinResult joinResult = process_join(&result.value);
		if (joinResult.present) {
			FILE* stdoutFile = result.value.stdoutFile;
			if (stdoutFile) {
				char buf[1024];
				size_t nread = fread(buf, 1, sizeof(buf), stdoutFile);
				while (nread > 0) {
					(void)fwrite(buf, 1, nread, stderr);
					nread = fread(buf, 1, sizeof(buf), stdoutFile);
				}
			}
			return (ProcessCreateResult)JUST(result.value);
		}
	}

	return (ProcessCreateResult)NOTHING;
}
