/*
 * Skeleton code for Lab 2 - Shell processing
 * This file contains skeleton code for executing parsed commands.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include "cmdparse.h"
#include "cmdrun.h"

/* cmd_exec(cmd, pass_pipefd)
 *
 *   Execute the single command specified in the 'cmd' command structure.
 *
 *   The 'pass_pipefd' argument is used for pipes.
 *
 *     On input, '*pass_pipefd' is the file descriptor that the
 *     current command should use to read the output of the previous
 *     command. That is, it's the "read end" of the previous
 *     pipe, if there was a previous pipe; if there was not, then
 *     *pass_pipefd will equal STDIN_FILENO.
 *
 *     On output, cmd_exec should set '*pass_pipefd' to the file descriptor
 *     used for reading from THIS command's pipe (so that the next command
 *     can use it). If this command didn't have a pipe -- that is,
 *     if cmd->controlop != PIPE -- then this function should set
 *     '*pass_pipefd = STDIN_FILENO'.
 *
 *   Returns the process ID of the forked child, or < 0 if some system call
 *   fails.
 *
 *   Besides handling normal commands, redirection, and pipes, you must also
 *   handle three internal commands: "cd", "exit", and "our_pwd".
 *   (Why must "cd", "exit", and "our_pwd" (a version of "pwd") be implemented
 *   by the shell, versus simply exec()ing to handle them?)
 *
 *   Note that these special commands still have a status!
 *   For example, "cd DIR" should return status 0 if we successfully change
 *   to the DIR directory, and status 1 otherwise.
 *   Thus, "cd /tmp && echo /tmp exists" should print "/tmp exists" to stdout
 *      if and only if the /tmp directory exists.
 *   Not only this, but redirections should work too!
 *   For example, "cd /tmp > foo" should create an empty file named 'foo';
 *   and "cd /tmp 2> foo" should print any error messages to 'foo'.
 *
 *   Some specifications:
 *
 *       --subshells:
 *         the exit status should be either 0 (if the last
 *         command would have returned 0) or 5 (if the last command
 *         would have returned something non-zero). This is not the
 *         behavior of bash.
 *
 *       --cd:
 *
 *          this builtin takes exactly one argument besides itself (this
 *          is also not bash's behavior). if it is given fewer
 *          ("cd") or more ("cd foo bar"), that is a syntax error.  Any
 *          error (syntax or system call) should result in a non-zero
 *          exit status. Here is the specification for output:
 *
 *                ----If there is a syntax error, your shell should
 *                display the following message verbatim:
 *                   "cd: Syntax error! Wrong number of arguments!"
 *
 *                ----If there is a system call error, your shell should
 *                invoke perror("cd")
 *
 *       --our_pwd:
 *
 *          This stands for "our pwd", which prints the working
 *          directory to stdout, and has exit status 0 if successful and
 *          non-zero otherwise. this builtin takes no arguments besides
 *          itself. Handle errors in analogy with cd. Here, the syntax
 *          error message should be:
 *
 *              "pwd: Syntax error! Wrong number of arguments!"
 *
 *       --exit:
 *
 *          As noted in the lab, exit can take 0 or 1 arguments. If it
 *          is given zero arguments (besides itself), then the shell
 *          exits with command status 0. If it is given one numerical
 *          argument, the shell exits with that numerical argument. If it
 *          is given one non-numerical argument, do something sensible.
 *          If it is given more than one argument, print an error message,
 *          and do not exit.
 *
 *
 *   Implementation hints are given in the function body.
 */

/* Number of arguments in argv, including argv[0]. */
static int
cmd_argc(command_t *cmd)
{
	int argc = 0;
	while (cmd->argv[argc])
		argc++;
	return argc;
}

/* Parse the argument of "exit". Returns 0 and sets *status if 'arg'
 * is a whole number, else returns -1. */
static int
parse_exit_status(const char *arg, int *status)
{
	char *end;
	long val;

	errno = 0;
	val = strtol(arg, &end, 10);
	if (*arg == '\0' || *end != '\0' || errno != 0)
		return -1;
	*status = (int) (val & 0xff);
	return 0;
}

static pid_t
cmd_exec(command_t *cmd, int *pass_pipefd)
{
	pid_t pid = -1;		// process ID for child
	int pipefd[2];		// file descriptors for this process's pipe
	int fd;
	int argc = cmd_argc(cmd);
	// Builtins should only change the shell itself when they are not
	// part of a pipeline or run in the background (same as bash:
	// "cd /tmp | cat" does not change the directory).
	int in_shell = (cmd->controlop != CMD_PIPE
			&& cmd->controlop != CMD_BACKGROUND
			&& *pass_pipefd == STDIN_FILENO);

	/* EXERCISE 4: Complete this function!
	 * We've written some of the skeleton for you, but feel free to
	 * change it.
	 */

	// Create a pipe, if this command is the left-hand side of a pipe.
	// Return -1 if the pipe fails.
	if (cmd->controlop == CMD_PIPE) {
		/* Your code here*/
		if (pipe(pipefd) < 0) {
			perror("pipe");
			return -1;
		}
	}


	// Fork the child and execute the command in that child.
	// You will handle all redirections by manipulating file descriptors.
	//
	// This section is fairly long.  It is probably best to implement this
	// part in stages, checking it after each step.  For example:
        //
	//   --First implement just the fork and the execute in the
	//   child (as we saw in class).  This should allow you to
	//   execute simple commands like 'ls'. Some of the tests in
	//   "make test" ought to pass.
	//   --Next, add support for redirections: as a simple test,
	//   'ls > foo' followed by 'cat < foo' should display the
	//   directory listing. A few more of the tests in "make test"
	//   should pass.
	//   --Next, support parentheses (a few more tests should pass,
	//     etc.)
	//   --Then pipes (if you use our pseudocode from class as
	//   inspiration, NOTE that the logic is a bit different in this lab)
	//   --And finally the internal commands 'cd', 'exit', and
	//   'our_pwd'.
	//
	//  Throughout, you will be using system calls: fork(), dup2(),
	//  close(), open(), execvp() or execve(), exit(), chdir(), etc.
	//
	//  You can also use perror() to handle error conditions
	//  concisely (type "$ man 3 perror" in a Unix terminal to get
	//  documentation).
	//
	//  You will need to do real C programming here.
	//  Tools like gdb (the debugger) may be useful.
	//
	//  You will also find functions like strcmp() and strtol()
	//  useful. Type "man strcmp", etc. for more information.
	//
	// In the child, you should:
	//    1. Set up stdout to point to this command's pipe, if necessary;
	//       close some file descriptors, if necessary (which ones?)
	//    2. Set up stdin to point to the PREVIOUS command's pipe (that
	//       is, *pass_pipefd), if appropriate; close a file
	//       descriptor (which one?)
	//    3. Set up redirections. Use the open() system call.
	//       Hints:
	//          --For input redirections, the oflag should be O_RDONLY.
	//          --For output redirections (stdout and stderr), what
	//          oflags do you want?
	//          --Set the mode argument of open() to be 0666
	//    4. Execute the command.
	//       There are some special cases:
	//       a. Parentheses.  Execute cmd->subshell. How? Also,
	//          you won't be invoking exec() in this special case,
	//          so think carefully about how this path "ends". (What
	//          would happen if we were to return from the current
	//          function?)
	//       b. A null command (no subshell, no arguments).
	//          Exit with status 0.
	//       c. "exit".
	//       d. "cd".
	//       e. "our_pwd".
	//       f. Remember to make your implementation
	//       consistent with the specification above!
	//
	// In the parent, you should:
	//    1. Close some file descriptors.  Hint: Consider the write end
	//       of this command's pipe, and one other fd as well.
	//    2. Handle the special "exit", "cd", and "our_pwd" commands.
	//    3. Set *pass_pipefd as appropriate.
	//
	// "cd","exit","our_pwd" Hints:
	//    Recall from the comments earlier that you need to return a status,
	//    and do redirections. How can you do these things for a
	//    command executed in the parent shell?
	//    Answer: It's easest if you fork a child ANYWAY!
        //    You should divide functionality between the parent and the child.
        //    Some functions will be executed in each process. For example,
        //         --in the case of "exit", both parent and child
        //         need to exit, but only the parent's exit code "matters",
        //         --in the case of "cd", both parent and child should
        //         try to change directory (use chdir()), but only the child
        //         should print error messages
        //         --in the case of "our_pwd", see "man getcwd"
	//
	//    For the "cd" command, you should change directories AFTER
	//    the fork(), not before it.  Why?
	//    Design some tests with 'bash' that will tell you the answer.
	//    For example, try "cd /tmp ; cd $HOME > foo".  In which directory
	//    does foo appear, /tmp or $HOME?  If you chdir() BEFORE the fork,
	//    in which directory would foo appear, /tmp or $HOME?
	//
	/* Your code here */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		if (cmd->controlop == CMD_PIPE) {
			close(pipefd[0]);
			close(pipefd[1]);
		}
		return -1;
	}

	if (pid == 0) {
		// Child.
		// Note: the child uses _exit() and not exit(). exit() would
		// flush stdio buffers copied from the parent, which can make
		// the shell read input lines twice when stdin is a file.

		// 1. stdout goes to this command's pipe.
		if (cmd->controlop == CMD_PIPE) {
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[0]);
			close(pipefd[1]);
		}

		// 2. stdin comes from the previous command's pipe.
		if (*pass_pipefd != STDIN_FILENO) {
			dup2(*pass_pipefd, STDIN_FILENO);
			close(*pass_pipefd);
		}

		// 3. Redirections. Index of redirect_filename[] is the fd.
		// These come after the pipe setup so "a > f | b" writes to f.
		for (fd = 0; fd < 3; fd++) {
			int flags, newfd;
			if (!cmd->redirect_filename[fd])
				continue;
			if (fd == STDIN_FILENO)
				flags = O_RDONLY;
			else
				flags = O_WRONLY | O_CREAT | O_TRUNC;
			newfd = open(cmd->redirect_filename[fd], flags, 0666);
			if (newfd < 0) {
				perror(cmd->redirect_filename[fd]);
				_exit(1);
			}
			if (newfd != fd) {
				dup2(newfd, fd);
				close(newfd);
			}
		}

		// Subshell: run the inner command line in this child, then
		// exit. We must not return, or the child would carry on as a
		// second copy of the shell. Status is 0 or 5 as per the spec.
		if (cmd->subshell) {
			int status = cmd_line_exec(cmd->subshell);
			_exit(status == 0 ? 0 : 5);
		}

		// Null command: nothing to run.
		if (!cmd->argv[0])
			_exit(0);

		// cd: exactly one argument. The child does the error
		// reporting and gives the exit status; the parent does the
		// real chdir() below.
		if (strcmp(cmd->argv[0], "cd") == 0) {
			if (argc != 2) {
				fprintf(stderr, "cd: Syntax error! Wrong number of arguments!\n");
				_exit(1);
			}
			if (chdir(cmd->argv[1]) < 0) {
				perror("cd");
				_exit(1);
			}
			_exit(0);
		}

		// our_pwd: no arguments, print the current directory.
		if (strcmp(cmd->argv[0], "our_pwd") == 0) {
			char buf[PATH_MAX];
			if (argc != 1) {
				fprintf(stderr, "pwd: Syntax error! Wrong number of arguments!\n");
				_exit(1);
			}
			if (!getcwd(buf, sizeof(buf))) {
				perror("pwd");
				_exit(1);
			}
			printf("%s\n", buf);
			fflush(stdout);
			_exit(0);
		}

		// exit: the child only works out the status (and prints any
		// error). The parent waits for it and then exits.
		if (strcmp(cmd->argv[0], "exit") == 0) {
			int status = 0;
			if (argc > 2) {
				fprintf(stderr, "exit: Syntax error! Too many arguments!\n");
				_exit(1);
			}
			if (argc == 2 && parse_exit_status(cmd->argv[1], &status) < 0) {
				// Like bash: complain, then exit with status 2.
				fprintf(stderr, "exit: %s: numeric argument required\n",
					cmd->argv[1]);
				_exit(2);
			}
			_exit(status);
		}

		execvp(cmd->argv[0], cmd->argv);
		// execvp() only returns on error.
		perror(cmd->argv[0]);
		_exit(1);
	}

	// Parent.
	// Close the write end of our pipe (only the child writes to it)
	// and the read end of the previous pipe (the child has its copy).
	// If we keep the write end open, the reader never sees EOF.
	if (cmd->controlop == CMD_PIPE)
		close(pipefd[1]);
	if (*pass_pipefd != STDIN_FILENO)
		close(*pass_pipefd);

	// Pass our read end to the next command.
	if (cmd->controlop == CMD_PIPE)
		*pass_pipefd = pipefd[0];
	else
		*pass_pipefd = STDIN_FILENO;

	// Builtins that change the shell itself. These run AFTER the fork,
	// so the child opens its redirection files in the old directory
	// ("cd /tmp ; cd $HOME > foo" creates /tmp/foo, same as bash).
	if (in_shell && !cmd->subshell && cmd->argv[0]) {
		if (strcmp(cmd->argv[0], "cd") == 0 && argc == 2) {
			// Errors are ignored here; the child already printed them.
			int r = chdir(cmd->argv[1]);
			(void) r;
		} else if (strcmp(cmd->argv[0], "exit") == 0 && argc <= 2) {
			// With more than one argument we print an error (in the
			// child) and do not exit.
			int status = 0;
			if (argc == 2 && parse_exit_status(cmd->argv[1], &status) < 0)
				status = 2;
			// Let the child finish printing any message first.
			waitpid(pid, NULL, 0);
			exit(status);
		}
	}

	// return the child process ID
	return pid;
}


/* cmd_line_exec(cmdlist)
 *
 *   Execute the command list.
 *
 *   Execute each individual command with 'cmd_exec'.
 *   String commands together depending on the 'cmdlist->controlop' operators.
 *   Returns the exit status of the entire command list, which equals the
 *   exit status of the last completed command.
 *
 *   The operators have the following behavior:
 *
 *      CMD_END, CMD_SEMICOLON
 *                        Wait for command to exit.  Proceed to next command
 *                        regardless of status.
 *      CMD_AND           Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status 0.  Otherwise
 *                        exit the whole command line.
 *      CMD_OR            Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status != 0.
 *                        Otherwise exit the whole command line.
 *      CMD_BACKGROUND, CMD_PIPE
 *                        Do not wait for this command to exit.  Pretend it
 *                        had status 0, for the purpose of returning a value
 *                        from cmd_line_exec.
 */
int
cmd_line_exec(command_t *cmdlist)
{
	int cmd_status = 0;	    // status of last command executed
	int pipefd = STDIN_FILENO;  // read end of last pipe
	controlop_t prev_op = CMD_END;  // operator before the current command
	int skipped = 0;		// was the previous command skipped?

	while (cmdlist) {
		int wp_status;	    // Use for waitpid's status argument!
				    // Read the manual page for waitpid() to
				    // see how to get the command's exit
				    // status (cmd_status) from this value.
		int skip;
		pid_t pid;

		// EXERCISE 4: Fill out this function!
		// If an error occurs in cmd_exec, feel free to abort().

		/* Your code here */

		// Decide if this command runs, like bash does:
		//   "a && b" runs b only if a succeeded,
		//   "a || b" runs b only if a failed,
		//   "a | b"  skips b if a was skipped (same pipeline).
		// A skipped command keeps the previous status, so in
		// "false && echo x || echo y" the "echo y" still runs.
		if (prev_op == CMD_AND)
			skip = (cmd_status != 0);
		else if (prev_op == CMD_OR)
			skip = (cmd_status == 0);
		else if (prev_op == CMD_PIPE)
			skip = skipped;
		else
			skip = 0;

		if (!skip) {
			pid = cmd_exec(cmdlist, &pipefd);
			if (pid < 0)
				abort();

			switch (cmdlist->controlop) {
			case CMD_END:
			case CMD_SEMICOLON:
			case CMD_AND:
			case CMD_OR:
				while (waitpid(pid, &wp_status, 0) < 0) {
					if (errno != EINTR)
						abort();
				}
				if (WIFEXITED(wp_status))
					cmd_status = WEXITSTATUS(wp_status);
				else
					cmd_status = 128 + WTERMSIG(wp_status);
				break;
			case CMD_BACKGROUND:
			case CMD_PIPE:
				// Do not wait; pretend it succeeded.
				cmd_status = 0;
				break;
			}
		}

		skipped = skip;
		prev_op = cmdlist->controlop;
		cmdlist = cmdlist->next;
	}

        while (waitpid(0, 0, WNOHANG) > 0);

done:
	return cmd_status;
}
