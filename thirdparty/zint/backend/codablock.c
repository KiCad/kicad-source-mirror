/* codablock.c - Handles Codablock-F and Codablock-E */

/*
    libzint - the open source barcode library
    Copyright (C) 2016 Harald Oehlmann

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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "gs1.h"

#define uchar unsigned char

/* FTab C128 flags - may be added */
#define CodeA 1
#define CodeB 2
#define CodeC 4
#define CEnd 8
#define CShift 16
#define CFill 32
#define CodeFNC1 64
#define ZTNum (CodeA+CodeB+CodeC)
#define ZTFNC1 (CodeA+CodeB+CodeC+CodeFNC1)

/* ASCII-Extension for Codablock-F */
#define aFNC1 (uchar)(128)
#define aFNC2 (uchar)(129)
#define aFNC3 (uchar)(130)
#define aFNC4 (uchar)(131)
#define aCodeA (uchar)(132)
#define aCodeB (uchar)(133)
#define aCodeC (uchar)(134)
#define aShift (uchar)(135)

static const char *C128Table[107] = {
    /* Code 128 character encodation - Table 1 */
    "212222", "222122", "222221", "121223", "121322", "131222", "122213",
    "122312", "132212", "221213", "221312", "231212", "112232", "122132", "122231", "113222",
    "123122", "123221", "223211", "221132", "221231", "213212", "223112", "312131", "311222",
    "321122", "321221", "312212", "322112", "322211", "212123", "212321", "232121", "111323",
    "131123", "131321", "112313", "132113", "132311", "211313", "231113", "231311", "112133",
    "112331", "132131", "113123", "113321", "133121", "313121", "211331", "231131", "213113",
    "213311", "213131", "311123", "311321", "331121", "312113", "312311", "332111", "314111",
    "221411", "431111", "111224", "111422", "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211", "241211", "221114", "413111", "241112",
    "134111", "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112",
    "421211", "212141", "214121", "412121", "111143", "111341", "131141", "114113", "114311",
    "411113", "411311", "113141", "114131", "311141", "411131", "211412", "211214", "211232",
    "2331112"
};

/* Code F Analysing-Chart */
typedef struct sCharacterSetTable
{
    int CharacterSet;       /* Still possible character sets for actual*/
    int AFollowing;     /* Still following Characters in Charset A */
    int BFollowing;     /* Still following Characters in Charset B */
    int CFollowing;     /* Still following Characters in Charset C */
} CharacterSetTable;

/* Find the possible Code-128 Character sets for a character
 * The result is an or of CodeA,CodeB,CodeC,CodeFNC1 in dependency of the
 * possible Code 128 character sets.
 */
int GetPossibleCharacterSet(unsigned char C)
{
    if (C<='\x19')      /* Dec:31 */
        return CodeA;
    if (C>='0' && C<='9')
        return ZTNum;   /* ZTNum=CodeA+CodeB+CodeC */
    if (C==aFNC1)
        return ZTFNC1;  /* ZTFNC1=CodeA+CodeB+CodeC+CodeFNC1 */
    if (C>='\x60' && C<='\x7f')      /* 60 to 127 */
        return CodeB;
    return CodeA+CodeB;
}

/* Create a Table with the following information for each Data character:
 *  int CharacterSet    is an or of CodeA,CodeB,CodeC,CodeFNC1, in
 *          dependency which character set is applicable.
 *          (Result of GetPossibleCharacterSet)
 *  int AFollowing,BFollowing   The number of source characters you still may encode
 *          in this character set.
 *  int CFollowing  The number of characters encodable in CodeC if we
 *          start here.
 */
static void CreateCharacterSetTable(CharacterSetTable T[], unsigned char *data,const size_t dataLength)
{
    int charCur;
    int runChar;

    /* Treat the Data backwards */
    charCur=dataLength-1;
    T[charCur].CharacterSet=GetPossibleCharacterSet(data[charCur]);
    T[charCur].AFollowing=((T[charCur].CharacterSet & CodeA)==0)?0:1;
    T[charCur].BFollowing=((T[charCur].CharacterSet & CodeB)==0)?0:1;
    T[charCur].CFollowing=0;

    for (charCur--;charCur>=0;charCur--)
    {
        T[charCur].CharacterSet=GetPossibleCharacterSet(data[charCur]);
        T[charCur].AFollowing=
            ((T[charCur].CharacterSet & CodeA)==0)?0:T[charCur+1].AFollowing+1;
        T[charCur].BFollowing=
            ((T[charCur].CharacterSet & CodeB)==0)?0:T[charCur+1].BFollowing+1;
        T[charCur].CFollowing=0;

    }
    /* Find the CodeC-chains */
    for (charCur=0;charCur<dataLength;charCur++)
    {
        T[charCur].CFollowing=0;
        if ((T[charCur].CharacterSet & CodeC)!=0)
        {
            /* CodeC possible */
            runChar=charCur;
            do{
                /* Wether this is FNC1 wether next is */
                /* numeric */
                if (T[runChar].CharacterSet==ZTFNC1)
                    /* FNC1 */
                    ++(T[charCur].CFollowing);
                else
                {
                    ++runChar;
                    if (runChar>=dataLength)
                        break;
                    /* Only a Number may follow */
                    if (T[runChar].CharacterSet==ZTNum)
                        T[charCur].CFollowing+=2;
                    else
                        break;
                }
                ++runChar;
            } while (runChar<dataLength);
        }
    }
}

/* Find the amount of numerical characters in pairs which will fit in
 * one bundle into the line (up to here). This is calculated online because
 * it depends on the space in the line.
 */
int RemainingDigits(CharacterSetTable *T, int charCur,int emptyColumns)
{
    int digitCount;     /* Numerical digits fitting in the line */
    int runChar;
    runChar=charCur;
    digitCount=0;
    while(emptyColumns>0 && runChar<charCur+T[charCur].CFollowing)
    {
        if (T[runChar].CharacterSet!=ZTFNC1)
        {
            /* NOT FNC1 */
            digitCount+=2;
            runChar++;
        }
        runChar++;
        emptyColumns--;
    }
    return digitCount;
}

/* Find the Character distribution at a given column count.
 * If to many rows (>44) are requested the columns is extended.
 * A oneLigner may be choosen if shorter.
 * Parameters :
 *  T       Pointer on the Characters which fit in the row
 *          If a different count is calculated it is corrected
 *          in the callers workspace.
 *  pFillings   Output of filling characters
 *  pSet        Output of the character sets used, allocated by me.
 *  Data        The Data string to encode, exceptionnally not an out
 *  Return value    Resulting row count
 */

static int Columns2Rows(CharacterSetTable *T, unsigned char *data, const size_t dataLength,
        int * pRows, int * pUseColumns, int * pSet, int * pFillings)
{
    int useColumns;     /* Usable Characters per line */
    int fillings;       /* Number of filling characters */
    int rowsCur;
    int runChar;
    int emptyColumns;   /* Number of codes still empty in line. */
    int emptyColumns2;  /* Alternative emptyColumns to compare */
    int CPaires;        /* Number of digit pairs which may fit in the line */
    int characterSetCur;        /* Current Character Set */

    useColumns=*pUseColumns;
    if (useColumns<3)
        useColumns=3;

    /* >>> Loop until rowsCur<44 */
    do {
        int charCur=0;
        int fOneLiner=1;        /* First try one-Liner */
        memset(pSet,0,dataLength*sizeof(int));
        rowsCur=0;

        /* >>> Line and OneLiner-try Loop */
        do{
            /* >> Start Character */
            emptyColumns=useColumns;    /* Remained place in Line */
            if (fOneLiner)
                emptyColumns+=2;

            /* >>Choose in Set A or B */
            /* (C is changed as an option later on) */

            pSet[charCur]=characterSetCur=
                (T[charCur].AFollowing > T[charCur].BFollowing)
                ? CodeA : CodeB;

            /* >> Test on Numeric Mode C */
            CPaires=RemainingDigits(T,charCur, emptyColumns);
            if (CPaires>=4)
            {
                /* 4 Digits in Numeric compression ->OK */
                /* > May an odd start find more ? */
                /* Skip leading <FNC1>'s */
                /* Typical structure : <FNC1><FNC1>12... */
                /* Test if numeric after one isn't better.*/
                runChar=charCur;
                emptyColumns2=emptyColumns;
                while (T[runChar].CharacterSet==ZTFNC1)
                {
                    ++runChar;
                    --emptyColumns2;
                }
                if (CPaires>=RemainingDigits(T,runChar+1,emptyColumns2-1))
                {
                    /* Start odd is not better */
                    /* We start in C */
                    pSet[charCur]=characterSetCur=CodeC;
                    /* Inkrement charCur */
                    if (T[charCur].CharacterSet!=ZTFNC1)
                        ++charCur;      /* 2 Num.Digits */
                }
            }
            ++charCur;
            --emptyColumns;

            /* >> Following characters */
            while(emptyColumns>0 && charCur<dataLength)
            {
                switch(characterSetCur){
                case CodeA:
                case CodeB:
                    /* >> Check switching to CodeC */
                    /* Switch if :
                     *  - Character not FNC1
                     *  - 4 real Digits will fit in line
                     *  - an odd Start will not be better
                     */
                    if (T[charCur].CharacterSet==ZTNum
                        && (CPaires=RemainingDigits(T,charCur, emptyColumns-1))>=4
                        && CPaires > RemainingDigits(T,charCur+1,emptyColumns-2))
                    {
                        /* > Change to C */
                        pSet[charCur]=characterSetCur=CodeC;
                        charCur+=2; /* 2 Digit */
                        emptyColumns-=2; /* <SwitchC>12 */
                    } else if (characterSetCur==CodeA)
                    {
                        if(T[charCur].AFollowing==0)
                        {
                            /* Must change to B */
                            if (emptyColumns==1)
                            {
                                /* Can't switch: */
                                pSet[charCur-1]|=CEnd+CFill;
                                emptyColumns=0;
                            }else{
                                /* <Shift> or <switchB>? */
                                if (T[charCur].BFollowing==1)
                                {
                                    pSet[charCur]|=CShift;
                                } else {
                                    pSet[charCur]|=CodeB;
                                    characterSetCur = CodeB;
                                }
                                emptyColumns-=2;
                                ++charCur;
                            }
                        }else{
                            --emptyColumns;
                            ++charCur;
                        }
                    } else { /* Last possibility : CodeB */
                        if(T[charCur].BFollowing==0)
                        {
                            /* Must change to A */
                            if (emptyColumns==1)
                            {
                                /* Can't switch: */
                                pSet[charCur-1]|=CEnd+CFill;
                                emptyColumns=0;
                            } else {
                                /* <Shift> or <switchA>? */
                                if (T[charCur].AFollowing==1)
                                {
                                    pSet[charCur]|=CShift;
                                } else {
                                    pSet[charCur]|=CodeA;
                                    characterSetCur = CodeA;
                                }
                                emptyColumns-=2;
                                ++charCur;
                            }
                        }else{
                            --emptyColumns;
                            ++charCur;
                        }
                    }
                    break;
                case CodeC:
                    if(T[charCur].CFollowing>0)
                    {
                        charCur+=(T[charCur].CharacterSet==ZTFNC1)?1:2;
                        emptyColumns--;
                    }else{
                        /* Must change to A or B */
                        if (emptyColumns==1)
                        {
                            /* Can't switch: */
                            pSet[charCur-1]|=CEnd+CFill;
                            emptyColumns=0;
                        }else{
                            /*<SwitchA> or <switchA>?*/
                            characterSetCur=pSet[charCur]=
                                (T[charCur].AFollowing > T[charCur].BFollowing)
                                ?CodeA:CodeB;
                            emptyColumns-=2;
                            ++charCur;
                        }
                    }
                    break;
                } /* switch */
            } /* while */

            /* > End of Codeline */
            pSet[charCur-1]|=CEnd;
            ++rowsCur;
            if ( fOneLiner)
            {
                if (charCur<dataLength)
                {
                    /* One line not sufficiant */
                    fOneLiner=0;
                    /* Reset and Start again */
                    charCur=0;
                    rowsCur=0;
                    memset(pSet,0,dataLength*sizeof(int));
                }else{
                    /* Calculate real Length of OneLiner */
                    /* This is -2 BASED !!! */
                    useColumns-=emptyColumns;
                }
            }
        } while (charCur<dataLength); /* <= Data.Len-1 */

        /* Place check characters C1,C2 */
        if (fOneLiner)
            fillings=0;
        else{
            switch (emptyColumns) {
            case 1:
                pSet[charCur-1]|=CFill;
                /* Glide in following block without break */
            case 0:
                ++rowsCur;
                fillings=useColumns-2+emptyColumns;
                break;
            case 2:
                fillings=0;
                break;
            default:
                pSet[charCur-1]|=CFill;
                fillings=emptyColumns-2;
            }
        }

        if (rowsCur>44) {
            ++useColumns;
            if (useColumns > 62) {
                return ZINT_ERROR_TOO_LONG;
            }
        }
    } while(rowsCur>44);
    #ifdef _DEBUG
        printf("  -> out: rowsCur <%i>, useColumns <%i>, fillings <%i>\n",rowsCur,useColumns,fillings);
    #endif
    *pUseColumns=useColumns;
    *pRows=rowsCur;
    *pFillings=fillings;
    return 0;
}
/* Find columns if row count is given.
 */
static int Rows2Columns(CharacterSetTable *T, unsigned char *data, const size_t dataLength,
        int * pRows, int * pUseColumns, int * pSet, int * pFillings)
{
    int rowsCur;
    int rowsRequested;  /* Number of requested rows */
    int backupRows = 0;
    int fillings;
    int backupFillings = 0;
    int useColumns;
    int testColumns;    /* To enter into Width2Rows */
    int backupColumns = 0;
    int fBackupOk = 0;      /* The memorysed set is o.k. */
    int testListSize = 0;
    int pTestList[62];
#ifndef _MSC_VER
    int *pBackupSet[dataLength];
#else
    int *pBackupSet = (int *)_alloca(dataLength*sizeof(int));
#endif

    rowsRequested=*pRows;

    #ifdef _DEBUG
        fprintf(stderr,"Optimizer : Searching <%i> rows\n",rowsRequested);
    #endif

    if (rowsRequested==1)
        /* OneLiners are self-calibrating */
        testColumns=32767;
    else {
        /* First guess */
        testColumns=dataLength/rowsRequested;
        if (testColumns > 62)
            testColumns = 62;
        else if (testColumns < 1)
            testColumns = 1;
    }

    for (;;) {
        int errorCur;
        pTestList[testListSize] = testColumns;
        testListSize++;
        useColumns=testColumns; /* Make a copy because it may be modified */
        errorCur = Columns2Rows(T, data, dataLength, &rowsCur, &useColumns, pSet, &fillings);
        if (errorCur != 0)
            return errorCur;
        if (rowsCur<=rowsRequested) {
            /* Less or exactly line number found */
            /* check if column count below already tested or Count = 1*/
            int fInTestList = (rowsCur == 1 || testColumns == 1);
            int posCur;
            for (posCur = 0; posCur < testListSize && ! fInTestList; posCur++) {
                if ( pTestList[posCur] == testColumns-1 )
                    fInTestList = 1;
            }
            if (fInTestList) {
                /* >> Smaller Width already tested
                 * if rowsCur=rowsRequested->Exit
                 * if rowsCur<rowsRequested and fillings>0
                 * -> New search for rowsRequested:=rowsCur
                 */
                if ( rowsCur == rowsRequested || fillings == 0 || testColumns == 1 ) {
                    /* Exit with actual */
                    *pFillings=fillings;
                    *pRows=rowsCur;
                    *pUseColumns = useColumns;
                    return 0;
                }
                /* Search again for smaller Line number */
                rowsRequested=rowsCur;
                pTestList[0] = testColumns;
                testListSize = 1;
            }
            /* > Test more rows (shorter CDB) */
            fBackupOk=(rowsCur==rowsRequested);
            memcpy(pBackupSet,pSet,dataLength*sizeof(int));
            backupFillings=fillings;
            backupColumns=useColumns;
            backupRows=rowsCur;
            --testColumns;
        } else {
            /* > To many rows */
            int fInTestList = fBackupOk;
            int posCur;
            for (posCur = 0; posCur < testListSize && ! fInTestList; posCur++) {
                if ( pTestList[posCur] == testColumns+1 )
                    fInTestList = 1;
            }
            if (fInTestList) {
                /* The next less-rows (larger) code was
                 * already tested. So give the larger
                 * back.
                 */
                memcpy(pSet,pBackupSet,dataLength*sizeof(int));
                *pFillings=backupFillings;
                *pRows=backupRows;
                *pUseColumns=backupColumns;
                return 0;
            }
            /* > Test less rows (longer code) */
            backupRows=rowsCur;
            memcpy(pBackupSet,pSet,dataLength*sizeof(int));
            backupFillings=fillings;
            backupColumns=useColumns;
            fBackupOk=0;
            ++testColumns;
        }
    }
}

/* Print a character in character set A
 */
void A2C128_A(uchar **ppOutPos,uchar c)
{
    uchar * pOutPos = *ppOutPos;
    switch(c){
    case aCodeB: *pOutPos=100; break;
    case aFNC4: *pOutPos=101; break;
    case aFNC1: *pOutPos=102; break;
    case aFNC2: *pOutPos=97; break;
    case aFNC3: *pOutPos=96; break;
    case aCodeC: *pOutPos=99; break;
    case aShift: *pOutPos=98; break;
    default:
        /* +++ HaO 13.11.98 c>' ' && c < '\x1F' corrected */
        if(c>=' ' && c<='_')
            *pOutPos=(uchar)(c-' ');
        else
            *pOutPos=(uchar)(c+64);
        break;
    }
    (*ppOutPos)++;
}
/* Output c in Set B
 */
void A2C128_B(uchar **ppOutPos,uchar c)
{
    uchar * pOutPos = *ppOutPos;
    switch(c){
    case aFNC1: *pOutPos=102; break;
    case aFNC2: *pOutPos=97; break;
    case aFNC3: *pOutPos=96; break;
    case aFNC4: *pOutPos=100; break;
    case aCodeA: *pOutPos=101; break;
    case aCodeC: *pOutPos=99; break;
    case aShift: *pOutPos=98; break;
    default: *pOutPos=(uchar)(c-' '); break;
    }
    ++(*ppOutPos);
}
/* Output c1, c2 in Set C
 */
void A2C128_C(uchar **ppOutPos,uchar c1,uchar c2)
{
    uchar * pOutPos = *ppOutPos;
    switch(c1){
    case aFNC1: *pOutPos=102; break;
    case aCodeB: *pOutPos=100; break;
    case aCodeA: *pOutPos=101; break;
    default: *pOutPos=(char)(10 * (c1- '0') + (c2 - '0'));break;
    }
    (*ppOutPos)++;
}
/* Output a character in Characterset
 */
void ASCIIZ128(uchar **ppOutPos, int CharacterSet,uchar c1, uchar c2)
{
    if (CharacterSet==CodeA)
        A2C128_A(ppOutPos,c1);
    else if(CharacterSet==CodeB)
        A2C128_B(ppOutPos,c1);
    else
        A2C128_C(ppOutPos,c1,c2);
}
/* XLate Table A of Codablock-F Specification and call output
 */
void SumASCII(uchar **ppOutPos, int Sum, int CharacterSet)
{
    switch (CharacterSet){
    case CodeA:
        A2C128_A(ppOutPos, (uchar)Sum);
        break;
    case CodeB:
        if (Sum<=31)
            A2C128_B(ppOutPos, (uchar)(Sum+96));
        else if(Sum<=47)
            A2C128_B(ppOutPos, (uchar)Sum);
        else
            A2C128_B(ppOutPos, (uchar)(Sum+10));
        break;
    case CodeC:
        A2C128_C(ppOutPos
            ,(char)(Sum/10+'0') ,(uchar)(Sum%10+'0'));
        break;
    }
}

/* Main function called by zint framework
 */
int codablock(struct zint_symbol *symbol,const unsigned char source[], const size_t length) {
    size_t charCur,dataLength;
    int Error;
    int rows, columns, useColumns;
    int fillings;
    int Sum1,Sum2;
    uchar * pOutPos;
    int rowCur;
    int characterSetCur;
    int emptyColumns;
        char dest[1000];
        int r, c;
#ifdef _MSC_VER
    CharacterSetTable *T;
    unsigned char *data;
    int *pSet;
    uchar * pOutput;
#endif

    /* Parameter check */
    /* option1: rows 0: automatic, 1..44 */
    rows = symbol->option_1;
    if (rows > 44) {
        strcpy(symbol->errtxt, "410: Row parameter not in 0..44");
        return ZINT_ERROR_INVALID_OPTION;
    }
    /* option_2: (usable data) columns: 0: automatic, 6..66 */
    columns = symbol->option_2;
    if ( ! (columns <= 0 || (columns >= 6 && columns <=66)) ) {
        strcpy(symbol->errtxt, "411: Columns parameter not in 0,6..66");
        return ZINT_ERROR_INVALID_OPTION;
    }
    /* GS1 not implemented */
    if  (symbol->input_mode == GS1_MODE) {
        strcpy(symbol->errtxt, "412: GS1 mode not supported");
        return ZINT_ERROR_INVALID_OPTION;
    }
#ifndef _MSC_VER
    unsigned char data[length*2+1];
#else
    data = (unsigned char *) _alloca(length * 2+1);
#endif

    dataLength = 0;
    if (symbol->output_options & READER_INIT) {
        data[dataLength] = aFNC3;
        dataLength++;
    }
    /* Replace all Codes>127 with <fnc4>Code-128 */
    for (charCur=0;charCur<length;charCur++) {
        if (source[charCur]>127)
        {
            data[dataLength] = aFNC4;
            dataLength++;
            data[dataLength] = (unsigned char)(source[charCur]&127);
        } else
            data[dataLength] = source[charCur];
        dataLength++;
    }

    /* Build character set table */
#ifndef _MSC_VER
    CharacterSetTable T[dataLength];
    int pSet[dataLength];
#else
    T=(CharacterSetTable *)_alloca(dataLength*sizeof(CharacterSetTable));
    pSet = (int *)_alloca(dataLength*sizeof(int));
#endif
    CreateCharacterSetTable(T,data,dataLength);

    /* Find final row and column count */
    /* nor row nor column count given */
    if ( rows <= 0 && columns <= 5 ) {
        /* Use Code128 until reasonable size */
        if (dataLength < 9) {
            rows = 1;
        } else {
            /* use 1/1 aspect/ratio Codablock */
            columns = ((int)floor(sqrt(1.0*dataLength))+5);
            if (columns > 64)
                columns = 64;
                #ifdef _DEBUG
                printf("Auto column count for %zu characters:%d\n",dataLength,columns);
                #endif
        }
    }
    /* There are 5 Codewords for Organisation Start(2),row(1),CheckSum,Stop */
    useColumns = columns - 5;
    if ( rows > 0 ) {
        /* row count given */
        Error=Rows2Columns(T,data,dataLength,&rows,&useColumns,pSet,&fillings);
    } else {
        /* column count given */
        Error=Columns2Rows(T,data,dataLength,&rows,&useColumns,pSet,&fillings);
    }
    if (Error != 0) {
        strcpy(symbol->errtxt, "413: Data string to long");
        return Error;
    }
    /* Checksum */
    Sum1=Sum2=0;
    if (rows>1)
    {
        size_t charCur;
        for (charCur=0 ; charCur<dataLength ; charCur++) {
            Sum1=(Sum1 + (charCur%86+1)*data[charCur])%86;
            Sum2=(Sum2 + (charCur%86)*data[charCur])%86;
        }
    }

    #ifdef _DEBUG
    {   /* start a new level of local variables */
        int DPos;
        printf("\nData:");
        for (DPos=0 ; DPos< dataLength ; DPos++)
            fputc(data[DPos],stdout);

        printf("\n Set:");
        for (DPos=0 ; DPos< dataLength ; DPos++) {
            switch (pSet[DPos]&(CodeA+CodeB+CodeC)) {
            case CodeA: fputc('A',stdout); break;
            case CodeB: fputc('B',stdout); break;
            case CodeC: fputc('C',stdout); break;
            default: fputc('.',stdout); break;
            }
        }
        printf("\nFNC1:");
        for (DPos=0 ; DPos< dataLength ; DPos++)
            fputc((pSet[DPos]&CodeFNC1)==0?'.':'X',stdout);
        printf("\n END:");
        for (DPos=0 ; DPos< dataLength ; DPos++)
            fputc((pSet[DPos]&CEnd)==0?'.':'X',stdout);
        printf("\nShif:");
        for (DPos=0 ; DPos< dataLength ; DPos++)
            fputc((pSet[DPos]&CShift)==0?'.':'X',stdout);
        printf("\nFILL:");
        for (DPos=0 ; DPos< dataLength ; DPos++)
            fputc((pSet[DPos]&CFill)==0?'.':'X',stdout);
        fputc('\n',stdout);
    }
    #endif

    columns = useColumns + 5;

    /* >>> Build C128 code numbers */
    /* The C128 column count contains Start (2CW), Row ID, Checksum, Stop */
#ifndef _MSC_VER
    uchar pOutput[columns * rows];
#else
    pOutput = (unsigned char *)_alloca(columns * rows * sizeof(char));
#endif
    pOutPos = pOutput;
    charCur=0;
    /* >> Loop over rows */
    for (rowCur=0 ; rowCur<rows ; rowCur++) {
        if (charCur>=dataLength)
        {
            /* >> Empty line with StartCCodeBCodeC */
            characterSetCur=CodeC;
            /* CDB Start C*/
            *pOutPos='\x67';
            pOutPos++;
            *pOutPos='\x63';
            pOutPos++;
            SumASCII(&pOutPos,rowCur+42,CodeC);
            emptyColumns=useColumns-2;
            while (emptyColumns>0)
            {
                if(characterSetCur==CodeC)
                {
                    A2C128_C(&pOutPos,aCodeB,'\0');
                    characterSetCur=CodeB;
                }else{
                    A2C128_B(&pOutPos,aCodeC);
                    characterSetCur=CodeC;
                }
                --emptyColumns;
            }
        }else{
            /* >> Normal Line */
            /* > Startcode */
            switch (pSet[charCur] & (CodeA+CodeB+CodeC)){
            case CodeA:
                *pOutPos = '\x67';
                pOutPos++;
                if (rows>1) {
                    *pOutPos = '\x62';
                    pOutPos++;
                }
                characterSetCur=CodeA;
                break;
            case CodeB:
                if (rows==1) {
                    *pOutPos = '\x68';
                    pOutPos++;
                } else {
                    *pOutPos = '\x67';
                    pOutPos++;
                    *pOutPos = '\x64';
                    pOutPos++;
                }
                characterSetCur=CodeB;
                break;
            case CodeC:
            default:
                if (rows==1) {
                    *pOutPos = '\x69';
                    pOutPos++;
                } else {
                    *pOutPos = '\x67';
                    pOutPos++;
                    *pOutPos = '\x63';
                    pOutPos++;
                }
                characterSetCur=CodeC;
                break;
            }
            if (rows>1)
            {
                /* > Set F1 */
                /* In first line : # of rows */
                /* In Case of CodeA we shifted to CodeB */
                SumASCII(&pOutPos
                    ,(rowCur==0)?rows-2:rowCur+42
                    ,(characterSetCur==CodeA)?CodeB:characterSetCur
                );
            }
            /* >>> Data */
            emptyColumns=useColumns;
            /* +++ One liner don't have start/stop code */
            if (rows == 1)
                emptyColumns +=2;
            /* >> Character loop */
            while (emptyColumns>0)
            {
                /* ? Change character set */
                /* not at first possition (It was then the start set) */
                /* +++ special case for one-ligner */
                if (emptyColumns<useColumns || (rows == 1 && charCur!=0) )
                {
                    if ((pSet[charCur]&CodeA)!=0)
                    {
                        /* Change to A */
                        ASCIIZ128(&pOutPos,characterSetCur,aCodeA,'\0');
                        --emptyColumns;
                        characterSetCur=CodeA;
                    } else if ((pSet[charCur]&CodeB)!=0)
                    {
                        /* Change to B */
                        ASCIIZ128(&pOutPos,characterSetCur,aCodeB,'\0');
                        --emptyColumns;
                        characterSetCur=CodeB;
                    } else if ((pSet[charCur]&CodeC)!=0)
                    {
                        /* Change to C */
                        ASCIIZ128(&pOutPos,characterSetCur,aCodeC,'\0');
                        --emptyColumns;
                        characterSetCur=CodeC;
                    }
                }
                if ((pSet[charCur]&CShift)!=0)
                {
                    /* >> Shift it and put out the shifted character */
                    ASCIIZ128(&pOutPos,characterSetCur,aShift,'\0');
                    emptyColumns-=2;
                    characterSetCur=(characterSetCur==CodeB)?CodeA:CodeB;
                    ASCIIZ128(&pOutPos,characterSetCur,data[charCur],'\0');
                    characterSetCur=(characterSetCur==CodeB)?CodeA:CodeB;
                }else{
                    /* Normal Character */
                    if (characterSetCur==CodeC)
                    {
                        if (data[charCur]==aFNC1)
                            A2C128_C(&pOutPos,aFNC1,'\0');
                        else
                        {
                            A2C128_C(&pOutPos,data[charCur],data[charCur+1]);
                            ++charCur;
                            /* We need this here to get the good index */
                            /* for the termination flags in Set. */
                        }
                    }else
                        ASCIIZ128(&pOutPos,characterSetCur,data[charCur],'\0');
                    --emptyColumns;
                }
                /* >> End Criteria */
                if ((pSet[charCur] & CFill)!=0)
                {
                    /* Fill Line but leave space for checks in last line */
                    if(rowCur==rows-1 && emptyColumns>=2)
                        emptyColumns-=2;
                    while(emptyColumns>0)
                    {
                        switch(characterSetCur){
                        case CodeC:
                            A2C128_C(&pOutPos,aCodeB,'\0');
                            characterSetCur=CodeB;
                            break;
                        case CodeB:
                            A2C128_B(&pOutPos,aCodeC);
                            characterSetCur=CodeC;
                            break;
                        case CodeA:
                            A2C128_A(&pOutPos,aCodeC);
                            characterSetCur=CodeC;
                            break;
                        }
                        --emptyColumns;
                    }
                }
                if ((pSet[charCur] & CEnd)!=0)
                    emptyColumns=0;
                ++charCur;
            } /* Loop over characters */
        } /* if filling-Line / normal */

        /* Add checksum in last line */
        if(rows>1 && rowCur==rows-1)
        {
            SumASCII(&pOutPos,Sum1,characterSetCur);
            SumASCII(&pOutPos,Sum2,characterSetCur);
        }
        /* Add Code 128 checksum */
        {
            int Sum=0;
            int Pos=0;
            for ( ; Pos < useColumns+3 ; Pos++)
            {
                Sum = (Sum +
                    ((Pos==0?1:Pos) * pOutput[columns*rowCur+Pos]) % 103
                    ) % 103;
            }
            *pOutPos=(uchar)Sum;
            pOutPos++;
        }
        /* Add end character */
        *pOutPos=106;
        pOutPos++;
    } /* End Lineloop */

    #ifdef _DEBUG
        /* Dump the output to the screen
         */
        printf("\nCode 128 Code Numbers:\n");
        {   /* start a new level of local variables */
            int DPos, DPos2;
            for (DPos=0 ; DPos< rows ; DPos++)
            {
                for (DPos2=0 ; DPos2 < columns ; DPos2++)
                {
                    printf("%3d ",(int)(pOutput[DPos*columns+DPos2]));
                }
                printf("\n");
            }
        }
        printf("rows=%i columns=%i fillings=%i\n", rows, columns, fillings);
    #endif

    /* Paint the C128 patterns */
    for (r = 0; r < rows; r++) {
        strcpy(dest, "");
        for(c = 0; c < columns; c++) {
            strcat(dest, C128Table[pOutput[r * columns + c]]);
        }
        expand(symbol, dest);
        symbol->row_height[r] = 10;
    }

    if (!(symbol->output_options & BARCODE_BIND)) {
        symbol->output_options += BARCODE_BIND;
    }

    if (symbol->border_width < 2) {
        symbol->border_width = 2;
    }
    return 0;
}


