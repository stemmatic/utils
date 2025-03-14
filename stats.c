/*
	stats - print out basic phenetic states
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#include "scc.h"

#define MAXNAME 128

#define RATE(a, t) ((t) != 0 ? (double) (a)/(t) : 0.0)

#define STATES "?0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-"
#define MAXSTATES (sizeof(STATES) - 1)

// Witnesses: a Structure of Arrays (SoA)

typedef struct witnesses {
	int nLeafs;						// Number of extant witnesses
	int nSites;						// Number of sites

	char **names;                   // [nLeafs][strlen] Name aliases for nodes
	unsigned char **rdgs;           // [nLeafs][nSites] Original readings

	Flag *inset;					// [nLeafs] In selection set
	unsigned char *maxrdgs;         // [nSites] Maximum reading

	unsigned char *majRdgs;         // [nSites] Majority readings
	unsigned char *agrSets;         // [nSites] Bit vector for agreement sets

	int A;							// -1 .. nLeafs :: Override archetype
	int B;							// -1 .. nLeafs :: Override Byzantine
} Wits;

extern Wits *
	wtNew(int nLeafs, int nSites)
{
	Wits *wt = 0;

	ASSERT("nLeafs must be positive", nLeafs > 0);

	NEWMEM(wt, 1);
	wt->nLeafs = nLeafs;
	wt->nSites = nSites;

	wt->names = 0;
	NEWMEM(wt->names, nLeafs);

	wt->rdgs = 0;
	if (nSites == 0) {
		NEWMEM(wt->rdgs, nLeafs);
		for (int ww = 0; ww < nLeafs; ww++)
			wt->rdgs[ww] = 0;
	} else
		NEWMAT(wt->rdgs, nLeafs, nSites);

	// Set up selection set for group stats
	NEWMEM(wt->inset, nLeafs);
	ZERO(wt->inset, nLeafs);

	// Set up maxrdgs[]
	NEWMEM(wt->maxrdgs, nSites);
	ASET(wt->maxrdgs, '0', nSites);

	// Set up majRdgs[]
	NEWMEM(wt->majRdgs, nSites);
	ASET(wt->majRdgs, '0', nSites);

	// Set up agrSets[]
	NEWMEM(wt->agrSets, nSites);
	ASET(wt->agrSets, 0, nSites);

	wt->A = wt->B = ERR;

	return wt;
}

Wits *
	wtScan(FILE *fp)
{
	int nLeafs, nSites;
	int rc;					// fscanf return code

	rc = fscanf(fp, "%d %d", &nLeafs, &nSites);
	ASSERT("Bad tx header <nLeafs> <nSites>", rc == 2);

	Wits *wt = wtNew(nLeafs, nSites);

	for (int ww = 0; ww < nLeafs; ww++) {
		static char name[MAXNAME];

		rc = fscanf(fp, "%s %s", name, wt->rdgs[ww]);
		ASSERT("EOF or Bad tx line <name> <rdgs>", rc == 2);

		wt->names[ww] = strdup(name);
	}

	// Pre-compute maximum readings
	for (int ss = 0; ss < nSites; ss++) {
		for (int ll = 0; ll < nLeafs; ll++) {
			char rdg = wt->rdgs[ll][ss];
			if (strchr("0123456789", rdg) && rdg > wt->maxrdgs[ss])
				wt->maxrdgs[ss] = rdg;
		}
	}

	// Calculate Majority readings
	for (int ss = 0; ss < nSites; ss++) {
		char maxRdg = '0';
		char maxCnt = 0;
		static int stCounts[10];
		ASET(stCounts, 0, dimof(stCounts));

		for (int ll = 0; ll < nLeafs; ll++) {
			char rdg = wt->rdgs[ll][ss];
			if (!strchr("0123456789", rdg))
				continue;
			int idx = rdg - '0';
			if (++stCounts[idx] > maxCnt) {
				maxCnt = stCounts[idx];
				maxRdg = rdg;
			}
		}

		wt->majRdgs[ss] = maxRdg;
	}

	return wt;
}

#define _O  0x01
#define _A  0x02
#define _B  0x04
#define _AB 0x08

struct st {
	unsigned int agrO,  totO;		// type-O agreements;
	unsigned int agrA,  totA;		// type-A agreements;
	unsigned int agrB,  totB;		// type-B agreements;
	unsigned int agrAB, totAB;		// type-AB agreements;
};

double
	wtSim(Wits *wt, int ms, int ll, struct st result[1])
{
	struct st acc = {0, 0, 0, 0, 0, };

	ASET(wt->agrSets, 0, wt->nSites);

	for (int rr = 0; rr < wt->nSites; rr++) {
		char msRdg = wt->rdgs[ms][rr];
		char llRdg = wt->rdgs[ll][rr];

		const char rdgA = (wt->A >= 0 && wt->A < wt->nLeafs) ? wt->rdgs[wt->A][rr] : '0';
		const char rdgB = (wt->B >= 0 && wt->B < wt->nLeafs) ? wt->rdgs[wt->B][rr] : wt->majRdgs[rr];

		// Skip lacunae
		if (msRdg == '?' || llRdg == '?')
			continue;

		// Handle Type-O agreements
		if (msRdg == msRdg) {
			acc.totO++;
			if (msRdg == llRdg) {
				acc.agrO++;
				wt->agrSets[rr] |= _O;
			}
		}

		// Handle Type-A agreements
		if (msRdg != rdgA) {
			acc.totA++;
			if (msRdg == llRdg) {
				acc.agrA++;
				wt->agrSets[rr] |= _A;
			}
		}

		// Handle Type-B agreements
		if (msRdg != rdgB) {
			acc.totB++;
			if (msRdg == llRdg) {
				acc.agrB++;
				wt->agrSets[rr] |= _B;
			}
		}

		// Handle Type-AB agreements
		if (msRdg != rdgA && msRdg != rdgB) {
			acc.totAB++;
			if (msRdg == llRdg) {
				acc.agrAB++;
				wt->agrSets[rr] |= _AB;
			}
		}
	}

	if (result)
		*result = acc;
	return RATE(acc.agrO, acc.totO);
}

int
	wtFind(Wits *wt, const char *msName)
{
	int ll;

	if (!msName)
		return ERR;

	for (ll = 0; ll < wt->nLeafs; ll++) {
		if (strcmp(msName, wt->names[ll]) == 0)
			break;
	}
	return (ll < wt->nLeafs) ? ll : ERR;
}

double
	wtDistSum(Wits *wt, double **dist, int ms)
{
	double sumD = 0.0;

	for (int ll = 0; ll < wt->nLeafs; ll++)
		sumD += dist[ms][ll];

	return sumD;
}

void
	wtDistInit(Wits *wt, double **dist)
{
	for (int ms = 0; ms < wt->nLeafs; ms++) {
		dist[ms][ms] = 0.0;

		for (int ll = 0; ll < ms; ll++) {
			double rate = wtSim(wt, ms, ll, (struct st *) 0);
			dist[ms][ll] = dist[ll][ms] = 1.0 - rate;
		}
	}
}

// Confusion matrix possible calculations:
struct cm {
	int TP;
	int FP;
	int FN;
	int TN;

	double TPR;		// Total positive rate, recall, sensitivity
	double TNR;		// True negative rate, specificity
	double DOR;		// Diagnostic odds ratio
	double MCC;		// Matthews correlation coefficient, phi
};

// Calculate binary classication stats
double
	wtBinClass(Wits *wt, int site, int variant, struct cm *cm)
{
	int tp = 0, fp = 0, fn = 0, tn = 0;		// init {true, false} {positive, negative}

	// Fill out confusion matrix, skipping MISSING ('?')
	for (int ll = 0; ll < wt->nLeafs; ll++) {
		int rdg = wt->rdgs[ll][site];
		if (rdg == '?')
			continue;
		if (rdg == variant) {
			if (wt->inset[ll])
				tp++;
			else
				fp++;
		} else {
			if (wt->inset[ll])
				fn++;
			else
				tn++;
		}

	}

	// Save off the observations
	cm->TP = tp;
	cm->FP = fp;
	cm->FN = fn;
	cm->TN = tn;

	// Totals:
	const double pos = tp + fn;
	const double neg = fp + tn;
	const double pp  = tp + fp;
	const double pn  = fn + tn;

	const double pop = pos + neg;

	// Calculate basic rates
	cm->TPR = tp / pos;
	cm->TNR = tn / neg;

	// Diagnostic Odds Ratio
	cm->DOR = (tp + 0.5) * (tn + 0.5) / (fp + 0.5) / (fn + 0.5);

	// Matthews Correlation Coefficient (MCC, phi)
	int sq = ((tp+fn)*(fp+tn)*(tp+fp)*(fn+tn));
	cm->MCC = (sq > 0) ? (tp*tn - fp*fn) / sqrt(sq) : 0.0;

	return pop;
}

// Calculate Waltz's relationship number, between ms and ll
// RN = SUM @r: # of mss at site -1 / size of set agreeing with
double
	wtRN(Wits *wt, int ms, int ll)
{
	double rn = 0.0;
	int nRdgs = 0;

	for (int rr = 0; rr < wt->nSites; rr++) {
		char msRdg = wt->rdgs[ms][rr];
		char llRdg = wt->rdgs[ll][rr];

		// Skip lacunae
		if (msRdg == '?' || llRdg == '?')
			continue;

		nRdgs++;

		// If they do not agree, rn is 0.0
		if (msRdg != llRdg)
			continue;

		// Count all ms attested at site and those which agree
		int nAll = 0;
		int nAgr = 0;
		for (int mm = 0; mm < wt->nLeafs; mm++) {
			char mmRdg = wt->rdgs[mm][rr];

			if (msRdg == '?')
				continue;
			nAll++;
			if (mmRdg == msRdg)
				nAgr++;
		}
		rn += (nAll - 1.0) / nAgr;
	}

	return rn / nRdgs;
}

int
	agrPrint(Wits *wt, int mask, const char *ms1name, const char *ms2name)
{
	
	const int ms1 = wtFind(wt, ms1name);
	if (ms1 == ERR) {
		fprintf(stderr, "%s not found\n", (ms1name) ? ms2name : "[NULL]");
		return ERR;
	}

	const int ms2 = wtFind(wt, ms2name);
	if (ms2 == ERR) {
		fprintf(stderr, "%s not found\n", (ms2name) ? ms2name : "[NULL]");
		return ERR;
	}
		
	// Find site agreements (saved in wt->agrSets[]).
	(void) wtSim(wt, ms1, ms2, (struct st *) 0);

	// Ready the VARS file: if input is not redirected try SOLN.VARS
	FILE *fpin = stdin;
	if (isatty(0)) {
		fpin = fopen("SOLN.VARS", "r");
		if (!fpin) {
			fprintf(stderr, "Cannot find SOLN.VARS. If your has another name, pipe or redirect it in with <FILE-NAME.\n");
			return ERR;
		}
	}

	// Print variants when the mask hits.
	{
		static char inbuf[8182];

		static char verse[512];
		static char lemma[512];
		static char vrdgs[512];

		int nuverse = NO;
		int nulemma = NO;
		int found = NO;
		int segno = 0;

		while (fgets(inbuf, sizeof inbuf, fpin)) {
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
				if (found) {
					if (wt->rdgs[ms1][segno] == '0')
						fprintf(stdout, "0  %s %s rell.\n", ms1name, ms2name);
					fputs("\n", stdout);
				}
				segno = atoi(inbuf+1);
				found = (wt->agrSets[segno] & mask);
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
				if (found) {
					// Explicitly print agreements in 0
					if (wt->rdgs[ms1][segno] == '0')
						fprintf(stdout, "0  %s %s rell.\n", ms1name, ms2name);
					fputs(inbuf, stdout);
				}
				found = NO;
				break;
			}
		}
	}
	return OK;
}

int
	main(int argc, const char *argv[])
{
	int ll;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <mss>.tx [opt] <ms-name>\n", argv[0]);
		return ERR;
	}

	// Scan the tx file
	const char *mss_tx = argv[1];
	FILE *fpMss = fopen(mss_tx, "r");
	if (!fpMss) {
		perror("Cannot read mss file.");
		return ERR;
	}
	Wits *wt = wtScan(fpMss);

	wt->A = wtFind(wt, getenv("A"));
	wt->B = wtFind(wt, getenv("B"));

	const char *msName = argv[2];
	const int ms = wtFind(wt, msName);
	if (ms != ERR)
		;
	else if (strcmp(msName, "-med") == 0) {
		// Calculate distances for medoids.
		int nLeafs = wt->nLeafs;
		double **dist;						// [nLeafs][nLeafs] Distance Matrix

		NEWMAT(dist, nLeafs, nLeafs);

		wtDistInit(wt, dist);

		for (int ms = 0; ms < wt->nLeafs; ms++) {
			double sumD = wtDistSum(wt, dist, ms);
			printf("%-12s %8.04f\n", wt->names[ms], sumD);
		}
		return OK;
	} else if (strcmp(msName, "-mcc") == 0) {
		for (int aa = 3; argv[aa]; aa++) {
			const int m = wtFind(wt, argv[aa]);
			if (m != ERR)
				wt->inset[m] = YES;
		}

		// Go through the sites and variants and print out the phi's for the set
		struct cm cm[1];
		for (int ss = 0; ss < wt->nSites; ss++) {
			for (int vv = '0'; vv <= wt->maxrdgs[ss]; vv++) {
				double pop = wtBinClass(wt, ss, vv, cm);
				//printf("%4u %c phi: %7.04f %7.04f %7.04f %7g ", ss, vv, cm->MCC, cm->TPR, cm->TNR, cm->DOR);
				printf("%4u %c phi: %7.04f ", ss, vv, cm->MCC);
				for (int ll = 0; ll < wt->nLeafs; ll++) {
					if (wt->inset[ll] && wt->rdgs[ll][ss] == vv)
						printf(" %s", wt->names[ll]);
				}
				printf(" [!");
				for (int ll = 0; ll < wt->nLeafs; ll++) {
					if (wt->inset[ll] && wt->rdgs[ll][ss] != vv)
						printf(" %s", wt->names[ll]);
				}
				printf(" !]");
				for (int ll = 0; ll < wt->nLeafs; ll++) {
					if (!wt->inset[ll] && wt->rdgs[ll][ss] == vv)
						printf(" %s", wt->names[ll]);
				}
				printf("\n");
			}
		}
		return OK;
	} else if (strcmp(msName, "-O") == 0) {
		if (argc < 4) {
			fprintf(stderr, "Usage: %s <mss>.tx -O <ms-name1> <ms-name2>\n", argv[0]);
			return ERR;
		}
		return agrPrint(wt, _O, argv[3], argv[4]);
	} else if (strcmp(msName, "-A") == 0) {
		if (argc < 4) {
			fprintf(stderr, "Usage: %s <mss>.tx -A <ms-name1> <ms-name2>\n", argv[0]);
			return ERR;
		}
		return agrPrint(wt, _A, argv[3], argv[4]);
	} else if (strcmp(msName, "-B") == 0) {
		if (argc < 4) {
			fprintf(stderr, "Usage: %s <mss>.tx -B <ms-name1> <ms-name2>\n", argv[0]);
			return ERR;
		}
		return agrPrint(wt, _B, argv[3], argv[4]);
	} else if (strcmp(msName, "-AB") == 0) {
		if (argc < 4) {
			fprintf(stderr, "Usage: %s <mss>.tx -AB <ms-name1> <ms-name2>\n", argv[0]);
			return ERR;
		}
		return agrPrint(wt, _AB, argv[3], argv[4]);
	} else {
		fprintf(stderr, "%s not found in %s\n", msName, mss_tx);
		return ERR;
	}

	// Now print the stats for a given ms
	struct st results[1];
	for (ll = 0; ll < wt->nLeafs; ll++) {
		double rate = wtSim(wt, ms, ll, results);
		double rn   = wtRN(wt, ms, ll);
		printf("%-10s %-10s  |O: %0.03f %5u %5u  |A: %0.03f %5u %5u  |AB: %0.03f %5u %5u  |B: %0.03f %5u %5u  || %g\n",
			msName, wt->names[ll], 
			RATE(results->agrO,  results->totO),  results->agrO,  results->totO,
			RATE(results->agrA,  results->totA),  results->agrA,  results->totA,
			RATE(results->agrAB, results->totAB), results->agrAB, results->totAB,
			RATE(results->agrB,  results->totB),  results->agrB,  results->totB,
			rn);
	}
	return OK;
}
