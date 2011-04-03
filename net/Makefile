


all:
		gcc -c net.c -o net.o -DNET_HAVE_MAIN -std=gnu99
		gcc -o net net.o ../logging/logging.o -DNET_HAVE_MAIN -std=gnu99 -lrt


clean:
		rm -f net net.o


