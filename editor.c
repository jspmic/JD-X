#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h> // For reading terminal attributes
#include "functions.h"

#define CTRL_KEY(x) (x & 0x1f)

/* error handling */

void raise_error(const char* error_str){
	// Function that raises an error once called
	perror(error_str);
	refresh_screen();
	exit(1);
}

/* terminal */

int getWindowSize(int *rows, int *cols){
	// Calculates the window size of the terminal
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	}
	*cols = ws.ws_col;
	*rows  = ws.ws_row;
	return 0;
}

// Global Variable to store the terminal attributes and size
struct termConfig {
	struct termios original_terminal;
	int rows;
	int cols;
} termConfig;

void initEditor(void){
	// Initialize the script with the correct terminal size
	if (getWindowSize(&(termConfig.rows), &(termConfig.cols)) == -1)
			raise_error("getWindowSize");
}

void disableRawMode(void) {
	// Switch to canonical mode
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &(termConfig.original_terminal));
}
void enableRawMode(void) {
	// Switch to RAW mode

	// In raw mode, we see no characters on stdout
	// Ex: when writing `sudo ...` we don't see the output(the characters)
	if (tcgetattr(STDIN_FILENO, &(termConfig.original_terminal)) == -1) // Store terminal attributes
		raise_error("tcgetattr");

	atexit(disableRawMode);

	struct termios tty = termConfig.original_terminal;

	// Turning off IXON flag shuts down the C-s(to not send the output on screen) and C-q(to resend the output)
	// Turning off ICRNL flag disables the feature where carriage returns are transformed into new line(\r -> \n); (\n, 10)
	//  * ICRNL -> I: input, CR: Carriage Return, NL: New Line
	//  * Turning off ICRNL lets some letters return their own ASCII code(for instance `m`)
	tty.c_iflag &= ~(BRKINT|IXON|ICRNL|INPCK|ISTRIP);

	tty.c_cflag |= CS8; // tradition purposes

	// We disable the ECHO attribute so that no character may be displayed
	// Turning ICANON off enables us to read byte-per-bye not line-by-line
	// Turning ISIG off shuts down the C-c and C-z signals
	tty.c_lflag &= ~(ECHO|ICANON|ISIG);

	// Turning OPOST off disables the transformation \n -> \r\n
	tty.c_oflag &= ~(OPOST);

	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1){
		raise_error("tcsetattr");
	}
}

char keypress_read(void){
	char c;
	int actual_character;
	while ((actual_character = read(STDIN_FILENO, &c, 1)) != 1){
		if (actual_character == -1 && errno != EAGAIN)
			raise_error("read");
	}
	return c;
}

/* Key press handling */

void editorProcessKeypress(void) {
	char c = keypress_read();
	switch (c){
		case CTRL_KEY('q'):
			refresh_screen();
			exit(0);
			break;
	}
}

/* Output */

void draw_rows(void){
	// Draw the vim-style stars
	int y, rows = termConfig.rows;
	for (y=0; y < rows; y++){
		write(STDOUT_FILENO, "*", 1);
		if (y < (rows-1))
			write(STDOUT_FILENO, "\r\n", 2);
	}
}

void refresh_screen(void){
	// Clears the screen
	write(STDOUT_FILENO, "\x1b[2J", 4); // Clears all characters in the terminal
	write(STDOUT_FILENO, "\x1b[H", 3); // Reposition the cursor to top-left
	draw_rows();
	write(STDOUT_FILENO, "\x1b[H", 3);
}

/* Main function */

int main(void){
	enableRawMode();
	initEditor();
	while (1){
		refresh_screen();
		editorProcessKeypress();
	}
	return 0;
}
