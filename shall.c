/* Author: Robbert van Renesse 2015
 *
 * Main code.
 */
//make build from sources
// h contains definitions
//parse+token:compliling stuff
//reader:input happens here
//shall the main function where we setup

#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>//print,scanf
#include <stdlib.h>//standard library
#include <unistd.h>//system calls!
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>//while testing,easier to understand
#include "shall.h"

static void arg_append(command_t command, char *arg){
	command->argv = realloc(command->argv, (command->argc + 1) * sizeof(*command->argv));
	command->argv[command->argc++] = arg;
}

static void redir_append(command_t command, element_t elt){
	command->redirs = realloc(command->redirs,
				(command->nredirs + 1) * sizeof(command->redirs));
	command->redirs[command->nredirs++] = elt;
}

/* Display the next prompt.
 */
static void display_prompt(){
	fprintf(stderr, "-> ");
}

static void gotline(command_t command, int background){
	if (command->argc > 0) {
		arg_append(command, 0);
		perform(command, background);
	}

	int i;
	for (i = 0; i < command->argc; i++) {
		free(command->argv[i]);
	}
	command->argc = 0;
	for (i = 0; i < command->nredirs; i++) {
		element_free(command->redirs[i]);
	}
	command->nredirs = 0;
}

void interpret(reader_t reader, int interactive){
	struct command command;

	memset(&command, 0, sizeof(command));

	tokenizer_t tokenizer = tokenizer_create(reader);
	parser_t parser = parser_create(tokenizer);

	if (interactive) {
		display_prompt();
	}


	int more = 1;
	while (more) {
		element_t elt = parser_next(parser);
		switch (elt->type) {
		case ELEMENT_ARG:
			arg_append(&command, elt->u.arg.string);
			elt->u.arg.string = 0;
			element_free(elt);
			break;
		case ELEMENT_REDIR_FILE_IN:
		case ELEMENT_REDIR_FILE_OUT:
		case ELEMENT_REDIR_FILE_APPEND:
		case ELEMENT_REDIR_FD_IN:
		case ELEMENT_REDIR_FD_OUT:
			redir_append(&command, elt);
			break;
		case ELEMENT_EOLN:
			element_free(elt);
			gotline(&command, 0);
			if (interactive) {
				display_prompt();
			}
			break;
		case ELEMENT_SEMI:
			element_free(elt);
			gotline(&command, 0);
			break;
		case ELEMENT_BACKGROUND:
			element_free(elt);
			gotline(&command, 1);
			break;
		case ELEMENT_ERROR:
			element_free(elt);
			if (interactive) {
				display_prompt();
			}
			break;
		case ELEMENT_EOF:
			element_free(elt);
			if (interactive) {
				fprintf(stderr, "EOF\n");
			}
			gotline(&command, 0);
			more = 0;
			break;
		default:
			assert(0);
		}
	}

	parser_free(parser);
	tokenizer_free(tokenizer);
	free(command.argv);
	free(command.redirs);
}

/* Main code.  If interactive, print prompts.  Read pipelines from input
 * and execute them.
 */

 //return int, take argument count, pointer to a character
 //pointer to characters: string is represented by characters, terminated by a null character
 //char* is the first character in string hello
 //argv is a pointer that points to an another array of pointers!:
 //char ** is a pointer to pointer to character:
 //1. pointer to  ./shall
 //2. pointer to x
 //3. pointer y
 //./shall x y :main start executing

 //when pointer is null, it equals 0 ; two same things


int main(int argc, char **argv){
	interrupts_catch();
	reader_t reader = reader_create(0);//allocate the resources of the reader
	interpret(reader, isatty(0));
	reader_free(reader);//release reader
	return 0;
}
