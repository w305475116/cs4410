/* Author: Robbert van Renesse 2018
 *
 * The interface is as follows:
 *	reader_t reader_create(int fd);
 *		Create a reader that reads characters from the given file descriptor.
 *
 *	char reader_next(reader_t reader):
 *		Return the next character or -1 upon EOF (or error...)
 *
 *	void reader_free(reader_t reader):
 *		Release any memory allocated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "shall.h"
#include <sys/errno.h>
struct reader {
	int fd;
};
// struct reader z,*p
// z.fd equals (*p).zf equals p->zf;//read the int fd in the struct
reader_t reader_create(int fd){                                 //must be with a *, which what reader points to//if without *, it would just be a 8byte reader
	reader_t reader = (reader_t) calloc(1, sizeof(*reader));//allocate size to the reader,allocate 8bytes,multiply these 
	reader->fd = fd;//difference betweenn malloca,calloc: calloc set the variable to 0,cleaner; malloc
	return reader;
}
//reader->token->parser->elements->shall
char reader_next(reader_t reader){//return the next chracter
    char c;
    for (;;) {
        extern int errno;
        int n = read(reader->fd, &c, 1);//read the file(could be keyboard)//system call, takes fd, pointer to charc c//-1:error, 0:
        if (n > 0) {//& takes the object, return the poitner,equals **//* take the pointer, return the object. //int *p
            return c;//number of charcters
        }
        if (n == 0 || errno != EINTR) {
            return EOF;//end of file:-1
        }
    }
}

//homework:use a buffer, use less system call: after buffer is clean, call system call

void reader_free(reader_t reader){
	free(reader);
}
