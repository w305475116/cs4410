/* Author: Robbert van Renesse 2018
 *
 * Architecture-dependent code.
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "shall.h"

/* This is a simple signal handler that prints the signal number.
 */
static void sighandler(int sig){
	printf("got signal %d\n", sig);//if interrupt, sig=2, etc
}

/* Disable interrupts.
 */
void interrupts_disable(){
// BEGIN
	/* This code is already finished.  It's an example of what you need
	 * to do with the other BEGIN/END sections in this file.
	 */
	signal(SIGINT, SIG_IGN);//ignore it
// END
}

/* Enable interrupts, causing the process to terminate.
 */
void interrupts_enable(){
// BEGIN
	signal(SIGINT, SIG_DFL);//call the signal function,
// END
}

/* Enable interrupts, and cause the process to invoke sighandler() when
 * an interrupt occurs.
 */
void interrupts_catch(){//
// BEGIN
	struct sigaction sa;
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);//when interrupt arrives, invoke handler,
// END
}

/* This implements '{fd1} > {fd2}' directives.  That is, any output
 * produced by file descriptor fd1 should go to the same place as fd2,
 * or in other words, fd1 is to become a copy of fd2.  If redirection
 * fails, _exit(1) causes the process to fail before executing the command.
 */
static void redir_fd(int fd1, int fd2){
// BEGIN
	printf("REDIRECT %d TO %d\n", fd1, fd2);	// replace this line
	int newfd = dup2(fd2,fd1);
	if(newfd<0){
		_exit(1);
	}
// END
}

/* Redirect file descriptor fd to file 'name'.  flags are for the
 * open() system call and specify if the file should be opened for
 * reading, writing, etc.  If the file is to be created, mode 0644//mode 0644 is the rw permission
 * is used.
 */
static void redir_file(char *name, int fd, int flags){
// BEGIN
	int newfd = open(name,flags,0644);
	redir_fd(fd,newfd);//fd is 0(stdin),1(stdout),2(stderr)
	close(newfd);//close should follow open
// END
}

/* Handle the I/O redirections in the command in the order given.
 */
static void redir(command_t command){
	int i;
	for (i = 0; i < command->nredirs; i++) {
		element_t elt = command->redirs[i];
		switch (elt->type) {
		case ELEMENT_REDIR_FILE_IN:
			redir_file(elt->u.redir_file.name, elt->u.redir_file.fd, O_RDONLY);
			break;
		case ELEMENT_REDIR_FILE_OUT:
			redir_file(elt->u.redir_file.name, elt->u.redir_file.fd, O_WRONLY | O_CREAT | O_TRUNC);
			break;
		case ELEMENT_REDIR_FILE_APPEND:
			redir_file(elt->u.redir_file.name, elt->u.redir_file.fd, O_WRONLY | O_APPEND);
			break;
		case ELEMENT_REDIR_FD_IN:
		case ELEMENT_REDIR_FD_OUT:
			redir_fd(elt->u.redir_fd.fd1, elt->u.redir_fd.fd2);
			break;
		default:
			assert(0);
		}
	}
}

/* Try to execute the given argument vector (the first of which
 * indicates the executable itself.
 */
static void do_exec(char **argv){
	if (strchr(argv[0], '/') == 0) {
		char *path = getenv("PATH");
		int proglen = strlen(argv[0]);

		if (path == 0) {
			path = "";
		}

		for (;;) {
			char *r = strchr(path, ':');
			int len;

			if (r == 0) {
				len = strlen(path);
			}
			else {
				len = r - path;
			}
			if (len == 0) {
				execv(argv[0], argv);
			}
			else {
				char *file = malloc(proglen + len + 2);
				sprintf(file, "%.*s/%s", len, path, argv[0]);
				execv(file, argv);
				free(file);
			}
			if (r == 0) {
				break;
			}
			path = r + 1;
		}
		fprintf(stderr, "%s: command not found\n", argv[0]);
		exit(1);
	}
	else {
		execv(argv[0], argv);
		perror(argv[0]);
		_exit(1);
	}
}

/* This function can be used by spawn() to execute the command after
 * forking and redirecting I/O.
 */
static void execute(command_t command){
	do_exec(command->argv);
}

/* Spawn the given command.  Run in the background if argument 'background'
 * is true (non-zero).  Otherwise wait for the command to finish.  Also
 * print information about abnormally ending processes or terminated
 * processes that ran in the background.
 */
static void spawn(command_t command, int background){
// BEGIN
	int pid = fork();
	if(pid < 0){
		fprintf(stderr, "fork failed\n");
	}
	else if (pid == 0) {
		if(background){
			interrupts_disable();
			printf("process %i running in background:\n",getpid());
		}
		redir(command);
		execute(command);
	}
	else {
		if(!background){//run in foreground
			int status;
			int endpid = wait(&status); //child pid
			if(WIFEXITED(status)){
	            printf("process:%d terminated with status %d\n",endpid, WEXITSTATUS(status));
	        }
	        if(WIFSIGNALED(status)){
	        	printf("process:%d terminated with signal %d\n",endpid,WTERMSIG(status));   
	    	}
		}
	}
}

/* Change the current working directory to command->argv[1], or to
 * the directory in environment variable $HOME if command->argv[1] = null.
 */
static void cd(command_t command){
	if (command->argc > 3) {
		fprintf(stderr, "Usage: cd [directory]\n");
		return;
	}
	
// BEGIN
	char *dir = command->argv[1];
	if(dir==0){
		chdir(getenv("HOME"));
	}
	else{
		int success = chdir(dir);
		if(success==-1){
			fprintf(stderr, "No such file or directory\n");
		}
	}
// END
}

/* Read commands from the specified files in the list of arguments.
 * of the command.
 */
static void source(command_t command){
	int i;
	for (i = 1; command->argv[i] != 0; i++) {
		char *file = command->argv[i];
// BEGIN
		int fd = open(file,O_RDONLY);
		reader_t reader = reader_create(fd);
		interpret(reader, 0);
		reader_free(reader);
		close(fd);
// END
	}
}

/* Exit the shall.
 */
static void do_exit(command_t command){
	if (command->argc > 3) {
		fprintf(stderr, "Usage: exit [status]\n");
		return;
	}
	char *status = command->argv[1];
	exit(status == 0 ? 0 : atoi(status));
}

/* Exec the given command, replacing the shall with it.
 */
static void exec(command_t command){
	redir(command);
	if (command->argc > 2) {
		do_exec(&command->argv[1]);
	}
}

/* Builtin commands cannot run in background and I/O cannot be redirected.
 */
static int builtin_check(command_t command, int background){
	if (background) {
		fprintf(stderr, "can't run builtin commands in background\n");
		return 0;
	}
	if (command->nredirs > 0) {
		fprintf(stderr, "can't redirect I/O for builtin commands\n");
		return 0;
	}
	return 1;
}

/* Perform the command in the arguments list.
 */
void perform(command_t command, int background){
	if (strcmp(command->argv[0], "cd") == 0) {
		if (builtin_check(command, background)) {
			cd(command);
		}
	}
	else if (strcmp(command->argv[0], "source") == 0) {
		if (builtin_check(command, background)) {
			source(command);
		}
	}
	else if (strcmp(command->argv[0], "exit") == 0) {
		if (builtin_check(command, background)) {
			do_exit(command);
		}
	}
	else if (strcmp(command->argv[0], "exec") == 0) {
		if (background) {
			fprintf(stderr, "can't exec in background\n");
		}
		else {
			exec(command);
		}
	}
	else {
		spawn(command, background);
	}
}
