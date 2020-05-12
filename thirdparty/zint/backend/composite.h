/* composite.c - Tables for UCC.EAN Composite Symbols */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2017 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

#define NUMERIC		110
#define ALPHA		97
#define ISOIEC		105
#define	INVALID_CHAR	100
#define	ANY_ENC		120
#define ALPHA_OR_ISO	121

/* CC-A component coefficients from ISO/IEC 24728:2006 Annex F */
static const unsigned short int ccaCoeffs[30] = {
    /* k = 4 */
    522, 568, 723, 809,

    /* k = 5 */
    427, 919, 460, 155, 566,

    /* k = 6 */
    861, 285, 19, 803, 17, 766,

    /* k = 7 */
    76, 925, 537, 597, 784, 691, 437,

    /* k = 8 */
    237, 308, 436, 284, 646, 653, 428, 379
};

/* rows, error codewords, k-offset of valid CC-A sizes from ISO/IEC 24723:2006 Table 9 */
static const char ccaVariants[51] = {
    5, 6, 7, 8, 9, 10, 12, 4, 5, 6, 7, 8, 3, 4, 5, 6, 7,
    4, 4, 5, 5, 6, 6, 7, 4, 5, 6, 7, 7, 4, 5, 6, 7, 8,
    0, 0, 4, 4, 9, 9, 15, 0, 4, 9, 15, 15, 0, 4, 9, 15, 22
};

/* following is Left RAP, Centre RAP, Right RAP and Start Cluster from ISO/IEC 24723:2006 tables 10 and 11 */
static const char aRAPTable[68] = {
    39, 1, 32, 8, 14, 43, 20, 11, 1, 5, 15, 21, 40, 43, 46, 34, 29,
    0, 0, 0, 0, 0, 0, 0, 43, 33, 37, 47, 1, 20, 23, 26, 14, 9,
    19, 33, 12, 40, 46, 23, 52, 23, 13, 17, 27, 33, 52, 3, 6, 46, 41,
    6, 0, 3, 3, 3, 0, 3, 3, 0, 3, 6, 6, 0, 0, 0, 0, 3
};

/* Row Address Patterns are as defined in pdf417.h */
