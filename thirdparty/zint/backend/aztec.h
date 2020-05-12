/* aztec.c - Handles Aztec Mesa 2D Symbols */

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

#define UPPER	1
#define LOWER	2
#define MIXED	4
#define PUNC	8
#define DIGIT	16
#define BINARY	32

static const unsigned short int CompactAztecMap[] = {
    /* 27 x 27 data grid */
    609, 608, 411, 413, 415, 417, 419, 421, 423, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445, 447, 449, 451, 453, 455, 457, 459,
    607, 606, 410, 412, 414, 416, 418, 420, 422, 424, 426, 428, 430, 432, 434, 436, 438, 440, 442, 444, 446, 448, 450, 452, 454, 456, 458,
    605, 604, 409, 408, 243, 245, 247, 249, 251, 253, 255, 257, 259, 261, 263, 265, 267, 269, 271, 273, 275, 277, 279, 281, 283, 460, 461,
    603, 602, 407, 406, 242, 244, 246, 248, 250, 252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276, 278, 280, 282, 462, 463,
    601, 600, 405, 404, 241, 240, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 284, 285, 464, 465,
    599, 598, 403, 402, 239, 238, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 286, 287, 466, 467,
    597, 596, 401, 400, 237, 236, 105, 104, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 140, 141, 288, 289, 468, 469,
    595, 594, 399, 398, 235, 234, 103, 102, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 142, 143, 290, 291, 470, 471,
    593, 592, 397, 396, 233, 232, 101, 100, 1, 1, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 0, 1, 28, 29, 144, 145, 292, 293, 472, 473,
    591, 590, 395, 394, 231, 230, 99, 98, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 30, 31, 146, 147, 294, 295, 474, 475,
    589, 588, 393, 392, 229, 228, 97, 96, 2027, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2007, 32, 33, 148, 149, 296, 297, 476, 477,
    587, 586, 391, 390, 227, 226, 95, 94, 2026, 1, 0, 1, 1, 1, 1, 1, 0, 1, 2008, 34, 35, 150, 151, 298, 299, 478, 479,
    585, 584, 389, 388, 225, 224, 93, 92, 2025, 1, 0, 1, 0, 0, 0, 1, 0, 1, 2009, 36, 37, 152, 153, 300, 301, 480, 481,
    583, 582, 387, 386, 223, 222, 91, 90, 2024, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2010, 38, 39, 154, 155, 302, 303, 482, 483,
    581, 580, 385, 384, 221, 220, 89, 88, 2023, 1, 0, 1, 0, 0, 0, 1, 0, 1, 2011, 40, 41, 156, 157, 304, 305, 484, 485,
    579, 578, 383, 382, 219, 218, 87, 86, 2022, 1, 0, 1, 1, 1, 1, 1, 0, 1, 2012, 42, 43, 158, 159, 306, 307, 486, 487,
    577, 576, 381, 380, 217, 216, 85, 84, 2021, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2013, 44, 45, 160, 161, 308, 309, 488, 489,
    575, 574, 379, 378, 215, 214, 83, 82, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 46, 47, 162, 163, 310, 311, 490, 491,
    573, 572, 377, 376, 213, 212, 81, 80, 0, 0, 2020, 2019, 2018, 2017, 2016, 2015, 2014, 0, 0, 48, 49, 164, 165, 312, 313, 492, 493,
    571, 570, 375, 374, 211, 210, 78, 76, 74, 72, 70, 68, 66, 64, 62, 60, 58, 56, 54, 50, 51, 166, 167, 314, 315, 494, 495,
    569, 568, 373, 372, 209, 208, 79, 77, 75, 73, 71, 69, 67, 65, 63, 61, 59, 57, 55, 52, 53, 168, 169, 316, 317, 496, 497,
    567, 566, 371, 370, 206, 204, 202, 200, 198, 196, 194, 192, 190, 188, 186, 184, 182, 180, 178, 176, 174, 170, 171, 318, 319, 498, 499,
    565, 564, 369, 368, 207, 205, 203, 201, 199, 197, 195, 193, 191, 189, 187, 185, 183, 181, 179, 177, 175, 172, 173, 320, 321, 500, 501,
    563, 562, 366, 364, 362, 360, 358, 356, 354, 352, 350, 348, 346, 344, 342, 340, 338, 336, 334, 332, 330, 328, 326, 322, 323, 502, 503,
    561, 560, 367, 365, 363, 361, 359, 357, 355, 353, 351, 349, 347, 345, 343, 341, 339, 337, 335, 333, 331, 329, 327, 324, 325, 504, 505,
    558, 556, 554, 552, 550, 548, 546, 544, 542, 540, 538, 536, 534, 532, 530, 528, 526, 524, 522, 520, 518, 516, 514, 512, 510, 506, 507,
    559, 557, 555, 553, 551, 549, 547, 545, 543, 541, 539, 537, 535, 533, 531, 529, 527, 525, 523, 521, 519, 517, 515, 513, 511, 508, 509
};

static const char AztecSymbolChar[128] = {
    /* From Table 2 */
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 15, 16, 17, 18, 19, 1, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 0, 18, 0, 20, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 21, 22,
    23, 24, 25, 26, 20, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 27, 21, 28, 22, 23, 24, 2, 3, 4,
    5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 29, 25, 30, 26, 27
};

static const char AztecModes[129] = "BMMMMMMMMMMMMXBBBBBBBBBBBBBMMMMMXPPPPPPPPPPPXPXPDDDDDDDDDDPPPPPPMUUUUUUUUUUUUUUUUUUUUUUUUUUPMPMMMLLLLLLLLLLLLLLLLLLLLLLLLLLPMPMM";

static const unsigned short int AztecSizes[32] = {
    /* Codewords per symbol */
    21, 48, 60, 88, 120, 156, 196, 240, 230, 272, 316, 364, 416, 470, 528, 588, 652, 720, 790,
    864, 940, 1020, 920, 992, 1066, 1144, 1224, 1306, 1392, 1480, 1570, 1664
};

static const int AztecCompactSizes[4] = {
    17, 40, 51, 76
};

static const unsigned short int Aztec10DataSizes[32] = {
    /* Data bits per symbol maximum with 10% error correction */
    96, 246, 408, 616, 840, 1104, 1392, 1704, 2040, 2420, 2820, 3250, 3720, 4200, 4730,
    5270, 5840, 6450, 7080, 7750, 8430, 9150, 9900, 10680, 11484, 12324, 13188, 14076,
    15000, 15948, 16920, 17940
};

static const unsigned short int Aztec23DataSizes[32] = {
    /* Data bits per symbol maximum with 23% error correction */
    84, 204, 352, 520, 720, 944, 1184, 1456, 1750, 2070, 2410, 2780, 3180, 3590, 4040,
    4500, 5000, 5520, 6060, 6630, 7210, 7830, 8472, 9132, 9816, 10536, 11280, 12036,
    12828, 13644, 14472, 15348
};

static const unsigned short int Aztec36DataSizes[32] = {
    /* Data bits per symbol maximum with 36% error correction */
    66, 168, 288, 432, 592, 776, 984, 1208, 1450, 1720, 2000, 2300, 2640, 2980, 3350,
    3740, 4150, 4580, 5030, 5500, 5990, 6500, 7032, 7584, 8160, 8760, 9372, 9996, 10656,
    11340, 12024, 12744
};

static const unsigned short int Aztec50DataSizes[32] = {
    /* Data bits per symbol maximum with 50% error correction */
    48, 126, 216, 328, 456, 600, 760, 936, 1120, 1330, 1550, 1790, 2050, 2320, 2610,
    2910, 3230, 3570, 3920, 4290, 4670, 5070, 5484, 5916, 6360, 6828, 7308, 7800, 8316,
    8844, 9384, 9948
};

static const unsigned short int AztecCompact10DataSizes [4] = {
    78, 198, 336, 520
};

static const unsigned short int AztecCompact23DataSizes [4] = {
    66, 168, 288, 440
};

static const unsigned short int AztecCompact36DataSizes [4] = {
    48, 138, 232, 360
};

static const unsigned short int AztecCompact50DataSizes [4] = {
    36, 102, 176, 280
};

static const char AztecOffset[32] = {
    66, 64, 62, 60, 57, 55, 53, 51, 49, 47, 45, 42, 40, 38, 36, 34, 32, 30, 28, 25, 23, 21,
    19, 17, 15, 13, 10, 8, 6, 4, 2, 0
};

static const char AztecCompactOffset[4] = {
    6, 4, 2, 0
};

