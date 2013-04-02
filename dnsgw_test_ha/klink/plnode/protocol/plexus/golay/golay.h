#ifndef GOLEY_H_
#define GOLEY_H_

/*
 * golay.h
 *
 * functions for (23,12) golay encoding/decoding
 */

long arr2int(int *, int);
/*
 * Convert a binary vector of Hamming weight r, and nonzero positions in
 * array a[1]...a[r], to a long integer \sum_{i=1}^r 2^{a[i]-1}.
 */

void nextcomb(int,int,int*);
/*
 * Calculate next r-combination of an n-set.
 */

long get_syndrome(long);
/*
 * Compute the syndrome corresponding to the given pattern, i.e., the
 * remainder after dividing the pattern (when considering it as the vector
 * representation of a polynomial) by the generator polynomial, GENPOL.
 * In the program this pattern has several meanings: (1) pattern = infomation
 * bits, when constructing the encoding table; (2) pattern = error pattern,
 * when constructing the decoding table; and (3) pattern = received vector, to
 * obtain its syndrome in decoding.
 */

void gen_enc_table(void);
   /*
    * ---------------------------------------------------------------------
    *                  Generate ENCODING TABLE
    *
    * An entry to the table is an information vector, a 32-bit integer,
    * whose 12 least significant positions are the information bits. The
    * resulting value is a codeword in the (23,12,7) Golay code: A 32-bit
    * integer whose 23 least significant bits are coded bits: Of these, the
    * 12 most significant bits are information bits and the 11 least
    * significant bits are redundant bits (systematic encoding).
    * --------------------------------------------------------------------- 
    */

void gen_dec_table(void);
   /*
    * ---------------------------------------------------------------------
    *                  Generate DECODING TABLE
    *
    * An entry to the decoding table is a syndrome and the resulting value
    * is the most likely error pattern. First an error pattern is generated.
    * Then its syndrome is calculated and used as a pointer to the table
    * where the error pattern value is stored.
    * --------------------------------------------------------------------- 
    *            
    */

long encode_golay(long data);

/*
 * encodes data and returns the codeword.  
 * data is assumed to contain 12 bits in the least significant bit positions
 * codeword contains the 23 bits in the LSB positions
 * 
 */

long decode_golay(long codeword);

/*
 * decodes codeword and returns the dataword contained within.  
 * 
 * note: no detection is done here!
 * codeword is assumed to contain the 23 bits in the LSB positions
 * the returned bits contain the bits in the LSB positions
 */

#endif