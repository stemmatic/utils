
#include <string.h>
#include <stdio.h>
#include "scc.h"

int
	main(int argc, char *argv[])
{
	static char inbuf[8182];
	static char utbuf[8182];
	static char vunit[8182];
	char needle[64];
	char **ms, *mss[] = { "05D", "038Th", 0 };
	char *ut;
	int found, len;
	int nuseg = NO;

	while (fgets(inbuf, sizeof inbuf, stdin)) {
		switch (inbuf[0]) {
		case '@':
			fputs(inbuf, stdout);
			break;
		case '>':
			fputs(inbuf+6, stdout);
			break;
		case '^':
			strcpy(vunit, inbuf+1);
			nuseg = YES;
			break;
		case '=':
			found = NO;
			ut = utbuf;
			ut += sprintf(ut, "      %c", inbuf[1]);
			for (ms = argv+1; *ms; ms++) {
				len = sprintf(needle, " %s ", *ms);
				if (strstr(inbuf, needle)) {
					found = YES;
					ut += sprintf(ut, " %s", *ms);
				} else
					ut += sprintf(ut, "%*c", len-1, ' ');
			}
			ut += sprintf(ut, "\n");
			if (found) {
				if (nuseg && inbuf[1] == '?' && argc > 2)
					continue;
				if (nuseg)
					fputs(vunit, stdout);
				fputs(utbuf, stdout);
				nuseg = NO;
			}
		}
	}
	return 0;
}
