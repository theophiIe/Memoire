CC = gcc
CFLAGS = -Wall -g -pthread

run: compile
	./main

compile:
	$(CC) $(CFLAGS) main.c -L. -liof -o main
	
clean:
	rm -f main
	rm -f ${LENOM}.zip
	rm -f ${LENOM}.tar
	ls
	
LENOM=MOLINATTI_Theophile
zip:
	rm -rf ${LENOM}
	rm -rf ${LENOM}.zip
	mkdir ${LENOM}
	cp Makefile ${LENOM}
	cp *.h *.c *.cfg ${LENOM}
	cp libiof.a ${LENOM}
	cp README.md ${LENOM}
	zip -r ${LENOM}.zip ${LENOM}
	rm -rf ${LENOM}

tar:
	rm -f ${LENOM}.tar
	mkdir ${LENOM}
	cp Makefile ${LENOM}
	cp *.h *.c *.cfg ${LENOM}
	cp libiof.a ${LENOM}
	cp README.md ${LENOM}
	tar -zcvf ${LENOM}.tar ${LENOM}
	rm -rf ${LENOM}
