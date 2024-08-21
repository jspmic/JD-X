#ifndef UTILS_H
#define UTILS_H

#define CTRL_KEY(x) (x & 0x1f)
#define ABUF_INIT {NULL, 0}
#define VERSION "1.0"
#define WELCOME_LEN 100

enum editor_keys{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	PG_UP,
	PG_DOWN
};

typedef struct buffer buffer;
struct buffer{
	char *buf;
	int len;
};

void refresh_screen(void);
#endif
