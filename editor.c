#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h> // For reading terminal attributes
#include "functions.h"

#define CTRL_KEY(x) (x & 0x1f)
#define ABUF_INIT {NULL, 0}
#define VERSION "1.0"
#define WELCOME_LEN 100

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

// Global Variable to store the terminal configuration
struct termConfig {
	struct termios original_terminal;
	int rows; // Terminal rows
	int cols; // Terminal columns
	int cursor_x;
	int cursor_y;
} termConfig;

void initEditor(void){
	termConfig.cursor_x = 0;
	termConfig.cursor_y = 0;
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

void editorMoveCursor(char key){
	switch (key){
		case 'h':
			if (termConfig.cursor_x > 0)
				termConfig.cursor_x--;
			break;
		case 'j':
			if (termConfig.cursor_y < termConfig.rows-1)
				termConfig.cursor_y++;
			break;
		case 'k':
			if (termConfig.cursor_y > 0)
				termConfig.cursor_y--;
			break;
		case 'l':
			if (termConfig.cursor_x < termConfig.cols-1)
				termConfig.cursor_x++;
			break;
	}
}

void editorProcessKeypress(void) {
	char c = keypress_read();
	switch (c){
		case CTRL_KEY('q'):
			refresh_screen();
			exit(0);
			break;
		case 'h':
		case 'j':
		case 'k':
		case 'l':
			editorMoveCursor(c);
			break;
	}
}

/* Append String to Buffer */

void buffer_append(buffer *buf, const char* content, int len){
	// Append the content to the end of the buffer
	char *new = realloc(buf->buf, buf->len+len);
	if (new == NULL)
		return;
	memcpy(&new[buf->len], content, len);
	buf->buf = new;
	buf->len+=len;
}

void free_buffer(buffer *buf){
	free(buf->buf);
	buf->len = 0;
}

/* Output */

void draw_rows(buffer *buf){
	// Draw the vim-style tilds

	int y, rows = termConfig.rows, cols=termConfig.cols;
	for (y=0; y < rows; y++){
		if (y == rows/2){
			// Places welcome message at the center of the screen
			char welcome[WELCOME_LEN];
			int welcome_len = snprintf(welcome, sizeof(welcome), "JD-X Editor -- version %s\r\n", VERSION);
			int padding = (cols - welcome_len) / 2;
			if (padding) {
				buffer_append(buf, "~", 1);
				padding--;
			}
			while (padding--) buffer_append(buf, " ", 1);
			if (welcome_len > cols)
				welcome_len = cols;
			buffer_append(buf, welcome, welcome_len);
		}
		buffer_append(buf, "~", 1);
		buffer_append(buf, "\x1b[K", 3); // Clears one line on each call
		if (y < (rows-1))
			buffer_append(buf, "\r\n", 2);
	}
}

void refresh_screen(void){
	// Clears the screen

	buffer buf = ABUF_INIT;
	buffer_append(&buf, "\x1b[?25l", 6); // Hides the cursor

	buffer_append(&buf, "\x1b[H", 3); // Reposition the cursor to the top-left
	draw_rows(&buf);
	
	char buffer[32];
	// String to reposition the cursor to the top-left
	snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", termConfig.cursor_y + 1, termConfig.cursor_x + 1);
	buffer_append(&buf, buffer, strlen(buffer));
	/* buffer_append(&buf, "\x1b[H", 3); */

	buffer_append(&buf, "\x1b[?25h", 6); // Shows the cursor
	write(STDOUT_FILENO, buf.buf, buf.len); // Writes all the changes to the buffer
	free_buffer(&buf);
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
