#include "drw_textcodec.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "../drw_base.h"
#include "drw_cptables.h"
#include "drw_cptable932.h"
#include "drw_cptable936.h"
#include "drw_cptable949.h"
#include "drw_cptable950.h"

DRW_TextCodec::DRW_TextCodec()
{
    version = DRW::AC1021;
    conv = new DRW_Converter( NULL, 0 );
}


DRW_TextCodec::~DRW_TextCodec()
{
    delete conv;
}


void DRW_TextCodec::setVersion( std::string* v )
{
    std::string versionStr = *v;

    if( versionStr == "AC1009" || versionStr == "AC1006" )
    {
        version = DRW::AC1009;
        cp = "ANSI_1252";
        setCodePage( &cp );
    }
    else if( versionStr == "AC1012" || versionStr == "AC1014"
             || versionStr == "AC1015" || versionStr == "AC1018" )
    {
        version = DRW::AC1015;

        if( cp.empty() )    // codepage not set, initialize
        {
            cp = "ANSI_1252";
            setCodePage( &cp );
        }
    }
    else
    {
        version = DRW::AC1021;
        cp = "ANSI_1252";
    }
}


void DRW_TextCodec::setCodePage( std::string* c )
{
    cp = correctCodePage( *c );
    delete conv;

    if( version == DRW::AC1009 || version == DRW::AC1015 )
    {
        if( cp == "ANSI_874" )
            conv = new DRW_ConvTable( DRW_Table874, CPLENGHTCOMMON );
        else if( cp == "ANSI_932" )
            conv = new DRW_Conv932Table( DRW_Table932, DRW_LeadTable932,
                    DRW_DoubleTable932, CPLENGHT932 );
        else if( cp == "ANSI_936" )
            conv = new DRW_ConvDBCSTable( DRW_Table936, DRW_LeadTable936,
                    DRW_DoubleTable936, CPLENGHT936 );
        else if( cp == "ANSI_949" )
            conv = new DRW_ConvDBCSTable( DRW_Table949, DRW_LeadTable949,
                    DRW_DoubleTable949, CPLENGHT949 );
        else if( cp == "ANSI_950" )
            conv = new DRW_ConvDBCSTable( DRW_Table950, DRW_LeadTable950,
                    DRW_DoubleTable950, CPLENGHT950 );
        else if( cp == "ANSI_1250" )
            conv = new DRW_ConvTable( DRW_Table1250, CPLENGHTCOMMON );
        else if( cp == "ANSI_1251" )
            conv = new DRW_ConvTable( DRW_Table1251, CPLENGHTCOMMON );
        else if( cp == "ANSI_1253" )
            conv = new DRW_ConvTable( DRW_Table1253, CPLENGHTCOMMON );
        else if( cp == "ANSI_1254" )
            conv = new DRW_ConvTable( DRW_Table1254, CPLENGHTCOMMON );
        else if( cp == "ANSI_1255" )
            conv = new DRW_ConvTable( DRW_Table1255, CPLENGHTCOMMON );
        else if( cp == "ANSI_1256" )
            conv = new DRW_ConvTable( DRW_Table1256, CPLENGHTCOMMON );
        else if( cp == "ANSI_1257" )
            conv = new DRW_ConvTable( DRW_Table1257, CPLENGHTCOMMON );
        else if( cp == "ANSI_1258" )
            conv = new DRW_ConvTable( DRW_Table1258, CPLENGHTCOMMON );
        else if( cp == "UTF-8" )    // DXF older than 2007 are write in win codepages
        {
            cp = "ANSI_1252";
            conv = new DRW_Converter( NULL, 0 );
        }
        else
            conv = new DRW_ConvTable( DRW_Table1252, CPLENGHTCOMMON );
    }
    else
    {
        conv = new DRW_Converter( NULL, 0 );
    }
}


std::string DRW_TextCodec::toUtf8( std::string s )
{
    return conv->toUtf8( &s );
}


std::string DRW_TextCodec::fromUtf8( std::string s )
{
    return conv->fromUtf8( &s );
}


std::string DRW_Converter::toUtf8( std::string* s )
{
    std::string result;
    int j = 0;
    unsigned int i = 0;

    for( i = 0; i < s->length(); i++ )
    {
        unsigned char c = s->at( i );

        if( c < 0x80 )    // ascii check for /U+????
        {
            if( c == '\\' && i + 6 < s->length() && s->at( i + 1 ) == 'U' && s->at( i + 2 ) == '+' )
            {
                result  += s->substr( j, i - j );
                result  += encodeText( s->substr( i, 7 ) );
                i   += 6;
                j   = i + 1;
            }
        }
        else if( c < 0xE0 )    // 2 bits
        {
            i++;
        }
        else if( c < 0xF0 )    // 3 bits
        {
            i += 2;
        }
        else if( c < 0xF8 )    // 4 bits
        {
            i += 3;
        }
    }

    result += s->substr( j );

    return result;
}


std::string DRW_ConvTable::fromUtf8( std::string* s )
{
    std::string result;
    bool    notFound;
    int     code;

    int j = 0;

    for( unsigned int i = 0; i < s->length(); i++ )
    {
        unsigned char c = s->at( i );

        if( c > 0x7F )    // need to decode
        {
            result += s->substr( j, i - j );
            std::string part1 = s->substr( i, 4 );
            int l;
            code = decodeNum( part1, &l );
            j   = i + l;
            i   = j - 1;
            notFound = true;

            for( int k = 0; k<cpLenght; k++ )
            {
                if( table[k] == code )
                {
                    result += CPOFFSET + k;    // translate from table
                    notFound = false;
                    break;
                }
            }

            if( notFound )
                result += decodeText( code );
        }
    }

    result += s->substr( j );

    return result;
}


std::string DRW_ConvTable::toUtf8( std::string* s )
{
    std::string res;
    std::string::iterator it;

    for( it = s->begin(); it < s->end(); ++it )
    {
        unsigned char c = *it;

        if( c < 0x80 )
        {
            // check for \U+ encoded text
            if( c == '\\' )
            {
                if( it + 6 < s->end() && *(it + 1) == 'U' && *(it + 2) == '+' )
                {
                    res += encodeText( std::string( it, it + 7 ) );
                    it  += 6;
                }
                else
                {
                    res += c;    // no \U+ encoded text write
                }
            }
            else
                res += c;                           // c!='\' ascii char write
        }
        else                                        // end c < 0x80
        {
            res += encodeNum( table[c - 0x80] );    // translate from table
        }
    }                                               // end for

    return res;
}


std::string DRW_Converter::encodeText( const std::string& stmp )
{
    int code;

#if defined(__APPLE__)
    int Succeeded = sscanf( &( stmp.substr( 3, 4 )[0]), "%x", &code );

    if( !Succeeded || Succeeded == EOF )
        code = 0;

#else
    std::istringstream sd( stmp.substr( 3, 4 ) );
    sd >> std::hex >> code;
#endif
    return encodeNum( code );
}


std::string DRW_Converter::decodeText( int c )
{
    std::string res = "\\U+";
    std::string num;

#if defined(__APPLE__)
    std::string str( 16, '\0' );
    snprintf( &(str[0]), 16, "%04X", c );
    num = str;
#else
    std::stringstream ss;
    ss << std::uppercase << std::setfill( '0' ) << std::setw( 4 ) << std::hex << c;
    ss >> num;
#endif
    res += num;
    return res;
}


std::string DRW_Converter::encodeNum( int c )
{
    unsigned char ret[5];

    if( c < 128 )    // 0-7F US-ASCII 7 bits
    {
        ret[0]  = c;
        ret[1]  = 0;
    }
    else if( c < 0x800 )    // 80-07FF 2 bytes
    {
        ret[0]  = 0xC0 | (c >> 6);
        ret[1]  = 0x80 | (c & 0x3f);
        ret[2]  = 0;
    }
    else if( c< 0x10000 )    // 800-FFFF 3 bytes
    {
        ret[0]  = 0xe0 | (c >> 12);
        ret[1]  = 0x80 | ( (c >> 6) & 0x3f );
        ret[2]  = 0x80 | (c & 0x3f);
        ret[3]  = 0;
    }
    else     // 10000-10FFFF 4 bytes
    {
        ret[0]  = 0xf0 | (c >> 18);
        ret[1]  = 0x80 | ( (c >> 12) & 0x3f );
        ret[2]  = 0x80 | ( (c >> 6) & 0x3f );
        ret[3]  = 0x80 | (c & 0x3f);
        ret[4]  = 0;
    }

    return std::string( (char*) ret );
}


/** 's' is a string with at least 4 bytes lenght
** returned 'b' is byte lenght of encoded char: 2,3 or 4
**/
int DRW_Converter::decodeNum( std::string s, int* b )
{
    int code = 0;
    unsigned char c = s.at( 0 );

    if( (c & 0xE0)  == 0xC0 )    // 2 bytes
    {
        code    = ( c & 0x1F) << 6;
        code    = (s.at( 1 ) & 0x3F) | code;
        *b = 2;
    }
    else if( (c & 0xF0)  == 0xE0 )    // 3 bytes
    {
        code    = ( c & 0x0F) << 12;
        code    = ( (s.at( 1 ) & 0x3F) << 6 ) | code;
        code    = (s.at( 2 ) & 0x3F) | code;
        *b = 3;
    }
    else if( (c & 0xF8)  == 0xF0 )    // 4 bytes
    {
        code    = ( c & 0x07) << 18;
        code    = ( (s.at( 1 ) & 0x3F) << 12 ) | code;
        code    = ( (s.at( 2 ) & 0x3F) << 6 ) | code;
        code    = (s.at( 3 ) & 0x3F) | code;
        *b = 4;
    }

    return code;
}


std::string DRW_ConvDBCSTable::fromUtf8( std::string* s )
{
    std::string result;
    bool    notFound;
    int     code;

    int j = 0;

    for( unsigned int i = 0; i < s->length(); i++ )
    {
        unsigned char c = s->at( i );

        if( c > 0x7F )    // need to decode
        {
            result += s->substr( j, i - j );
            std::string part1 = s->substr( i, 4 );
            int l;
            code = decodeNum( part1, &l );
            j   = i + l;
            i   = j - 1;
            notFound = true;

            for( int k = 0; k<cpLenght; k++ )
            {
                if( doubleTable[k][1] == code )
                {
                    int data = doubleTable[k][0];
                    char d[3];
                    d[0]    = data >> 8;
                    d[1]    = data & 0xFF;
                    d[2]    = '\0';
                    result += d;     // translate from table
                    notFound = false;
                    break;
                }
            }

            if( notFound )
                result += decodeText( code );
        }    // direct conversion
    }

    result += s->substr( j );

    return result;
}


std::string DRW_ConvDBCSTable::toUtf8( std::string* s )
{
    std::string res;
    std::string::iterator it;

    for( it = s->begin(); it < s->end(); ++it )
    {
        bool notFound = true;
        unsigned char c = *it;

        if( c < 0x80 )
        {
            notFound = false;

            // check for \U+ encoded text
            if( c == '\\' )
            {
                if( it + 6 < s->end() && *(it + 1) == 'U' && *(it + 2) == '+' )
                {
                    res += encodeText( std::string( it, it + 7 ) );
                    it  += 6;
                }
                else
                {
                    res += c;    // no \U+ encoded text write
                }
            }
            else
                res += c;       // c!='\' ascii char write
        }
        else if( c == 0x80 )    // 1 byte table
        {
            notFound = false;
            res += encodeNum( 0x20AC ); // euro sign
        }
        else                            // 2 bytes
        {
            ++it;
            int code    = (c << 8) | (unsigned char) (*it);
            int sta     = leadTable[c - 0x81];
            int end     = leadTable[c - 0x80];

            for( int k = sta; k<end; k++ )
            {
                if( doubleTable[k][0] == code )
                {
                    res += encodeNum( doubleTable[k][1] );    // translate from table
                    notFound = false;
                    break;
                }
            }
        }

        // not found
        if( notFound )
            res += encodeNum( NOTFOUND936 );
    }    // end for

    return res;
}


std::string DRW_Conv932Table::fromUtf8( std::string* s )
{
    std::string result;
    bool    notFound;
    int     code;

    int j = 0;

    for( unsigned int i = 0; i < s->length(); i++ )
    {
        unsigned char c = s->at( i );

        if( c > 0x7F )    // need to decode
        {
            result += s->substr( j, i - j );
            std::string part1 = s->substr( i, 4 );
            int l;
            code = decodeNum( part1, &l );
            j   = i + l;
            i   = j - 1;
            notFound = true;

            // 1 byte table
            if( code > 0xff60 && code < 0xFFA0 )
            {
                result += code - CPOFFSET932;    // translate from table
                notFound = false;
            }

            if( notFound && ( code<0xF8 || (code>0x390 && code<0x542)
                              || (code>0x200F && code<0x9FA1) || code>0xF928 ) )
            {
                for( int k = 0; k<cpLenght; k++ )
                {
                    if( doubleTable[k][1] == code )
                    {
                        int data = doubleTable[k][0];
                        char d[3];
                        d[0]    = data >> 8;
                        d[1]    = data & 0xFF;
                        d[2]    = '\0';
                        result += d;    // translate from table
                        notFound = false;
                        break;
                    }
                }
            }

            if( notFound )
                result += decodeText( code );
        }    // direct conversion
    }

    result += s->substr( j );

    return result;
}


std::string DRW_Conv932Table::toUtf8( std::string* s )
{
    std::string res;
    std::string::iterator it;

    for( it = s->begin(); it < s->end(); ++it )
    {
        bool notFound = true;
        unsigned char c = *it;

        if( c < 0x80 )
        {
            notFound = false;

            // check for \U+ encoded text
            if( c == '\\' )
            {
                if( it + 6 < s->end() && *(it + 1) == 'U' && *(it + 2) == '+' )
                {
                    res += encodeText( std::string( it, it + 7 ) );
                    it  += 6;
                }
                else
                {
                    res += c;    // no \U+ encoded text write
                }
            }
            else
                res += c;               // c!='\' ascii char write
        }
        else if( c > 0xA0 && c < 0xE0 ) // 1 byte table
        {
            notFound = false;
            res += encodeNum( c + CPOFFSET932 );    // translate from table
        }
        else                                        // 2 bytes
        {
            ++it;
            int code = (c << 8) | (unsigned char) (*it);
            int sta;
            int end = 0;

            if( c > 0x80 && c < 0xA0 )
            {
                sta = DRW_LeadTable932[c - 0x81];
                end = DRW_LeadTable932[c - 0x80];
            }
            else if( c > 0xDF && c < 0xFD )
            {
                sta = DRW_LeadTable932[c - 0xC1];
                end = DRW_LeadTable932[c - 0xC0];
            }

            if( end > 0 )
            {
                for( int k = sta; k<end; k++ )
                {
                    if( DRW_DoubleTable932[k][0] == code )
                    {
                        res += encodeNum( DRW_DoubleTable932[k][1] );    // translate from table
                        notFound = false;
                        break;
                    }
                }
            }
        }

        // not found
        if( notFound )
            res += encodeNum( NOTFOUND932 );
    }    // end for

    return res;
}


std::string DRW_TextCodec::correctCodePage( const std::string& s )
{
    // stringstream cause crash in OS/X, bug#3597944
    std::string c = s;

    transform( c.begin(), c.end(), c.begin(), toupper );

    // Latin/Thai
    if( c=="ANSI_874" || c=="c874" || c=="ISO8859-11" || c=="TIS-620" )
    {
        return "ANSI_874";
        // Central Europe and Eastern Europe
    }
    else if( c=="ANSI_1250" || c=="c1250" || c=="ISO8859-2" )
    {
        return "ANSI_1250";
        // Cyrillic script
    }
    else if( c=="ANSI_1251" || c=="c1251" || c=="ISO8859-5" || c=="KOI8-R"
             || c=="KOI8-U" || c=="IBM 866" )
    {
        return "ANSI_1251";
        // Western Europe
    }
    else if( c=="ANSI_1252" || c=="c1252" || c=="LATIN1" || c=="ISO-8859-1"
             || c=="c819" || c=="CSISO" || c=="IBM819" || c=="ISO_8859-1" || c=="APPLE ROMAN"
             || c=="ISO8859-1" || c=="ISO8859-15" || c=="ISO-IR-100" || c=="L1" || c==
             "IBM 850" )
    {
        return "ANSI_1252";
        // Greek
    }
    else if( c=="ANSI_1253" || c=="c1253" || c=="iso8859-7" )
    {
        return "ANSI_1253";
        // Turkish
    }
    else if( c=="ANSI_1254" || c=="c1254" || c=="iso8859-9" || c=="iso8859-3" )
    {
        return "ANSI_1254";
        // Hebrew
    }
    else if( c=="ANSI_1255" || c=="c1255" || c=="iso8859-8" )
    {
        return "ANSI_1255";
        // Arabic
    }
    else if( c=="ANSI_1256" || c=="c1256" || c=="ISO8859-6" )
    {
        return "ANSI_1256";
        // Baltic
    }
    else if( c=="ANSI_1257" || c=="c1257" || c=="ISO8859-4" || c=="ISO8859-10" || c==
             "ISO8859-13" )
    {
        return "ANSI_1257";
        // Vietnamese
    }
    else if( c=="ANSI_1258" || c=="c1258" )
    {
        return "ANSI_1258";

        // Japanese
    }
    else if( c=="ANSI_932" || c=="SHIFT-JIS" || c=="SHIFT_JIS" || c=="CSSHIFTJIS"
             || c=="CSWINDOWS31J" || c=="MS_KANJI" || c=="X-MS-c932" || c=="X-SJIS"
             || c=="EUCJP" || c=="EUC-JP" || c=="CSEUcKDFMTJAPANESE" || c=="X-EUC"
             || c=="X-EUC-JP" || c=="JIS7" )
    {
        return "ANSI_932";
        // Chinese PRC GBK (XGB) simplified
    }
    else if( c=="ANSI_936" || c=="GBK" || c=="GB2312" || c=="CHINESE" || c=="CN-GB"
             || c=="CSGB2312" || c=="CSGB231280" || c=="CSISO58BG231280"
             || c=="GB_2312-80" || c=="GB231280" || c=="GB2312-80"
             || c=="ISO-IR-58" || c=="GB18030" )
    {
        return "ANSI_936";
        // Korean
    }
    else if( c=="ANSI_949" || c=="EUCKR" )
    {
        return "ANSI_949";
        // Chinese Big5 (Taiwan, Hong Kong SAR)
    }
    else if( c=="ANSI_950" || c=="BIG5" || c=="CN-BIG5" || c=="CSBIG5"
             || c=="X-X-BIG5" || c=="BIG5-HKSCS" )
    {
        return "ANSI_950";

// celtic
/*    } else if (c=="ISO8859-14") {
 *      return "ISO8859-14";
 *   } else if (c=="TSCII") {
 *       return "TSCII"; //tamil
 *   } else if (c=="UTF16") {
 *       return "UTF16"; */
    }
    else if( c=="UTF-8" || c=="UTF8" || c=="UTF88-BIT" )
    {
        return "UTF-8";
    }

    return "ANSI_1252";
}
