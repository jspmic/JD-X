#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h> // For reading terminal attributes

/* error handling */

void raise_error(const char* error_str){
	perror(error_str);
	exit(1);
}

/* terminal */

struct termios original_terminal;

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
}
void enableRawMode() {
	// In raw mode, we see no characters on stdout
	// Ex: when writing `sudo ...` we don't see the output(the characters)
	if (tcgetattr(STDIN_FILENO, &original_terminal) == -1)
		raise_error("tcgetattr");

	atexit(disableRawMode);

	struct termios tty = original_terminal;

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
	tty.c_cc[VTIME] = 10;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty) == -1){
		raise_error("tcsetattr");
	}
}

int main(){
	enableRawMode();
	char c='\0';
	while (1) {
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
			raise_error("read");

		if (c=='q')
			break;
		else if (iscntrl(c))
			printf("%d\r\n", c);
		else
			printf("%d (%c)\r\n", c, c);
	}
	return 0;
}
