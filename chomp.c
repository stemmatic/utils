
#include <stdio.h>

int
	main(int argc, char *argv[])
{
	int ch;

	static char linebuf[256];
	char *p = linebuf;

	while ((ch = getchar()) != EOF) {
		switch (ch) {
		case '\n':
			puts(linebuf);
		case '\r':
			*p = '\0';
			p = linebuf;
			break;
		default:
			*p++ = ch;
		}
	}
	return 0;
}
