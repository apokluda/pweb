/* vector.h
 *
 * Functions for working with vectors over a finite field of order
 * of a prime p.
 *
 * By Sebastian Raaphorst, 2002
 * ID#: 1256241
 *
 * $Author: vorpal $
 * $Date: 2002/12/09 04:06:59 $
 */

#ifndef VECTOR_H
#define VECTOR_H

int *vector_add(int, int, int*, int*, int*);
int *vector_subtract(int, int, int*, int*, int*);
int *vector_multiply(int, int, int*, int*, int*);
int *vector_not(int, int, int*, int*);
int *vector_copy(int, int*, int*);
int *vector_clear(int, int*);

int vector_dotproduct(int, int, int*, int*);

int *vector_add(int p, int k, int *v1, int *v2, int *v3)
{
	int i;

	for (i = 0; i < k; ++i)
	{
		v3[i] = (v1[i] + v2[i]) % p;
		for (; v3[i] < 0; v3[i] += p)
			;
	}

	return v3;
}

int *vector_subtract(int p, int k, int *v1, int *v2, int *v3)
{
	int i;

	for (i = 0; i < k; ++i)
	{
		v3[i] = (v1[i] - v2[i]) % p;
		for (; v3[i] < 0; v3[i] += p)
			;
	}

	return v3;
}

int *vector_multiply(int p, int k, int *v1, int *v2, int *v3)
{
	int i;

	for (i = 0; i < k; ++i)
	{
		v3[i] = (v1[i] * v2[i]) % p;
		for (; v3[i] < 0; v3[i] += p)
			;
	}

	return v3;
}

int *vector_not(int p, int k, int *v1, int *v2)
{
	int i;

	for (i = 0; i < k; ++i)
		v2[i] = p - 1 - v1[i];
	return v2;
}

int *vector_copy(int k, int *v1, int *v2)
{
	int i;

	for (i = 0; i < k; ++i)
		v2[i] = v1[i];
	return v2;
}

int *vector_clear(int k, int *v)
{
	int i;

	for (i = 0; i < k; ++i)
		v[i] = 0;
	return v;
}

int vector_dotproduct(int p, int k, int *v1, int *v2)
{
	int dp;
	int i;

	for (i = 0, dp = 0; i < k; ++i)
		dp += (v1[i] * v2[i]);
	dp %= p;
	for (; dp < 0; dp += p)
		;
	return dp;
}
#endif

/*
 * $Log: vector.h,v $
 * Revision 1.2  2002/12/09 04:06:59  vorpal
 * Added changes to allow for decoding.
 * Still have to write rmdecode.c and test.
 *
 * Revision 1.1  2002/11/14 20:28:06  vorpal
 * Adding new files to project.
 *
 */
