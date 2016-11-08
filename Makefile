CC = gcc

all: myfrm.o myfrmd.o

myfrm.o: myfrm.c
	gcc myfrm.c -o myfrm -lmhash

myfrmd.o: myfrmd.c
	gcc myfrmd.c -o myfrmd -lmhash

clean:
	$(RM) count *.o *~
	rm -f myfrm myfrmd
