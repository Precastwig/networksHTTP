all:
	gcc serverthreaded.c -o serverThreaded -D_POSIX_SOURCE -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE -pthread

run:
	./serverThreaded
