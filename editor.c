#include <unistd.h>
#include <termios.h> // For reading terminal attributes

void enableRawMode() {
	// In raw mode, we see no characters on stdout
	// Ex: when writing `sudo ...` we don't see the output(the characters)
	struct termios raw;
	tcgetattr(STDIN_FILENO, &raw);
	raw.c_lflag &= ~(ECHO); // We disable the echo attribute so that no character may be displayed
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
	enableRawMode();
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c!='q');
	return 0;
}
