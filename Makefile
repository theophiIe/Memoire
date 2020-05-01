CC = gcc
CFLAGS = -Wall -g -pthread
LENOM = MOLINATTI_Theophile

run: compile
	./main

compile: clean
	$(CC) $(CFLAGS) main.c -L. -liof -o main

valgrind: compile
	valgrind ./main

clean:
	rm -f main
	rm -f /tmp/FIFO1
	rm -f /tmp/FIFO2
	rm -f ${LENOM}.tar
	ls

tar:
	rm -f ${LENOM}.tar
	mkdir ${LENOM}
	cp Makefile ${LENOM}
	cp *.h *.c *.cfg ${LENOM}
	cp libiof.a ${LENOM}
	cp README.md ${LENOM}
	tar -zcvf ${LENOM}.tar ${LENOM}
	rm -rf ${LENOM}
