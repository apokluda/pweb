/* ksubset.h
 *
 * Used to manipulate k-subsets of an n-set in lexicographical order.
 * Note that the sets considered are {1, 2, ..., n}.
 *
 * By Sebastian Raaphorst, 2002
 * ID#: 1256241
 *
 * Note: These algorithms were inspired by the book, "Combinatorial
 * Algorithms: Generation, Enumeration, and Search", by Donald L.
 * Kreher and Douglas R. Stinson.
 */

#ifndef KSUBSET_H
#define KSUBSET_H

#include <cstdlib>
#include "common.h"
#include "combination.h"

struct _set
{
	int n;
	int **nCr;
};
typedef struct _set *Set;

Set ksubset_init(int);
void ksubset_free(Set);

int ksubset_lex_succ(Set, int, int*, int*);
long ksubset_lex_rank(Set, int, int*);
void ksubset_lex_unrank(Set, int, long, int*);

Set ksubset_init(int n)
{
	Set s;

	if (!(s = (Set) malloc(sizeof(struct _set))))
		return FALSE;

	s->n = n;
	if (!(s->nCr = combination_init(n)))
	{
		free(s);
		return FALSE;
	}

	return s;
}

void ksubset_free(Set s)
{
	combination_free(s->n, s->nCr);
	free(s);
}

int ksubset_lex_succ(Set s, int k, int *orig, int *succ)
{
	int i, j;

	for (i = 0; i < k; ++i)
		succ[i] = orig[i];

	for (i = k - 1; i >= 0 && orig[i] == s->n - k + i + 1; --i)
		;

	if (i < 0)
		return FALSE;

	for (j = i; j < k; ++j)
		succ[j] = orig[i] + 1 + j - i;

	return TRUE;
}

long ksubset_lex_rank(Set s, int k, int *subset)
{
	int i, j;
	long r;

	for (i = 0, r = 0; i < k; ++i)
		if ((i ? subset[i - 1] : 0) + 1 <= subset[i] - 1)
			for (j = (i ? subset[i - 1] : 0) + 1; j < subset[i]; ++j)
				r += s->nCr[s->n - j][k - i - 1];

	return r;
}

void ksubset_lex_unrank(Set s, int k, long r, int *subset)
{
	int i;
	int x;

	for (x = 1, i = 0; i < k; ++i)
	{
		while (s->nCr[s->n - x][k - i - 1] <= r)
		{
			r -= s->nCr[s->n - x][k - i - 1];
			++x;
		}

		subset[i] = x;
		++x;
	}
}
#endif

/*
 * $Log: ksubset.h,v $
 * Revision 1.1  2002/11/14 19:11:00  vorpal
 * Initial checkin.
 *
 */
