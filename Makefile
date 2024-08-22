SCR = editor.c
OBJ = a.o
CFLAGS = -Wall -Wextra -pedantic -lm
BIN = install

$(BIN): $(SCR)
	$(CC) $(CFLAGS) $(SCR) -o $(OBJ)

clean:
	rm -f $(OBJ)
