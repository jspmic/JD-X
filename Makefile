compile: editor.c
	$(CC) editor.c -lm -o a.o -Wall -Wextra -pedantic

clean:
	rm -f a.o
