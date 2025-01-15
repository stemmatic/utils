/*
	scc.h - My basic defines and stuff
*/

// Some types in all modules
typedef unsigned short Cursor;
typedef unsigned int Index;
typedef unsigned char Flag;
typedef unsigned char Byte;

// Some standard CCI/ICLisms:
#define NO  0
#define YES 1
#define EOS '\0'
#define OK 0
#define ERR (-1)

#define dimof(a) ((sizeof a)/(sizeof a[0]))

#define ASET(a,c,n) memset((a), (c), (n)*sizeof *(a))
#define ACPY(a,b,n) memcpy((a), (b), (n)*sizeof *(a))
#define ACMP(a,b,n) memcmp((a), (b), (n)*sizeof *(a))
#define ZERO(a,n)   memset((a), 0x0, (n)*sizeof *(a))
#define AMOV(a,b,n) memmove((a), (b), (n)*sizeof *(a))

// Control flow hacks

#define block switch (0) default:

#define ASSERT(msg, cond) if (!(cond)) { fprintf(stderr, "%s: %s\n", msg, #cond); exit(ERR); } else 

// Memory management
#define newmem(p, n) if (!((p) = malloc((n) * sizeof (p)[0]))) { \
	fprintf(stderr, "No memory for %s by %lu in %s:%d (aborting).\n", \
		#p, (unsigned long) n, __FILE__, __LINE__); \
	abort(); \
} else

#define NEWMEM(p, n) if (!p && !((p) = malloc((n) * sizeof (p)[0]))) { \
	fprintf(stderr, "No memory for %s by %lu in %s:%d (aborting).\n", \
		#p, (unsigned long) n, __FILE__, __LINE__); \
	abort(); \
} else

#define NEWMAT(a, x, y) do { \
	int _ii; \
	newmem(a, (x)); \
	newmem(a[0], (x)*(y)); \
	for (_ii = 1; _ii < (x); _ii++) \
		a[_ii] = a[0] + _ii*(y); \
	} while (0)

#define FREMAT(a) (void)(free(a[0]),free(a))
