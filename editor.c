#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h> // For reading terminal attributes

struct termios original_terminal;

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
}
void enableRawMode() {
	// In raw mode, we see no characters on stdout
	// Ex: when writing `sudo ...` we don't see the output(the characters)
	tcgetattr(STDIN_FILENO, &original_terminal);
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

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty);
}

int main(){
	enableRawMode();
	char c;
	while (1) {
		read(STDIN_FILENO, &c, 1);
		if (c=='q')
			break;
		else if (iscntrl(c)) // iscntrl defined in ctype.h
			printf("%d\r\n", c);
		else
			printf("%d (%c)\r\n", c, c); // \n for the letter to be displayed(\n = <ENTER>)
	}
	return 0;
}
