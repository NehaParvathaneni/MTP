run: lib user1.c user2.c initmsocket.c
	gcc initmsocket.c -L. -lmsocket -o init
	gcc user1.c -L. -lmsocket -o user1
	gcc user2.c -L. -lmsocket -o user2
	gcc try1.c -L. -lmsocket -o try1
	gcc try2.c -L. -lmsocket -o try2

lib: msocket.c msocket.h msocket.o
	gcc -c msocket.c
	ar rcs libmsocket.a msocket.o

clean:
	rm -f *.o *.a init user1 user2 try1 try2 Frankenstein_dup.txt
