
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "scc.h"

int
	main(int argc, char *argv[])
{
	static char inbuf[8182];

	static char verse[512];
	static char lemma[512];
	static char vrdgs[512];

	int nuverse = NO;
	int nulemma = NO;
	int found = NO;

	while (fgets(inbuf, sizeof inbuf, stdin)) {
		switch (inbuf[0]) {
		case '@':
			strncpy(verse, inbuf, sizeof verse);
			nuverse = YES;
			break;
		case '>':
			strncpy(lemma, inbuf+6, sizeof lemma);
			nulemma = YES;
			break;
		case '^':
			if (found)
				fputs("\n", stdout);
			int segno = atoi(inbuf+1);
			found = NO;
			for (char **vn = argv+1; *vn; vn++) {
				int seg = atoi(*vn);
				if (seg != 0 && seg == segno) {
					found = YES;
					break;
				}
			}
			if (found) {
				if (nuverse) {
					fputs(verse, stdout);
					nuverse = NO;
				}
				if (nulemma) {
					fputs(lemma, stdout);
					nulemma = NO;
				}
				fputs(inbuf+1, stdout);
			}
			break;
		case '=':
			if (found)
				fputs(inbuf+1, stdout);
			break;
		case '\n':
			if (found)
				fputs(inbuf, stdout);
			found = NO;
			break;
		}
	}
	return OK;
}
