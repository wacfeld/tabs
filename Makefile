CC = gcc
CFLAGS = -g -MMD -Wall
EXEC = $(shell basename $(CURDIR)).out
LIBS = 
OBJECTS = main.o midi.o tabs.o
DEPENDS = ${OBJECTS:.o=.d}
${EXEC}: ${OBJECTS}
	${CC} ${OBJECTS} ${CFLAGS} -o ${EXEC} ${LIBS}
-include ${DEPENDS}
.PHONY: clean
clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}

