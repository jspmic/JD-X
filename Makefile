SCR = editor.c
OBJ = a.o
CFLAGS = -Wall -Wextra -pedantic -lm
BIN = editor

$(BIN): $(SCR)
	$(CC) $(CFLAGS) $(SCR) -o $(OBJ)

clean:
	rm -f $(OBJ)
