
CFLAGS = -g -Wall
OBJECTS = shall.o exec.o reader.o token.o parser.o

shall: $(OBJECTS)
	$(CC) -o shall $(OBJECTS)

$(OBJECTS): shall.h

clean:
	rm -f shall $(OBJECTS)
