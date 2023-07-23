#pragma once

#include <stdint.h>
#include <string>

template <typename T>
static bool GetNextCodePointFromUTF16z(const T* u16, size_t* pos, uint32_t* cp)
{
    *cp = static_cast<uint32_t>(u16[*pos]);
    if (*cp == 0)
        return false;

    (*pos)++;
    if ((*cp & 0xFC00) == 0xD800)
    {
        uint16_t cp2 = static_cast<uint16_t>(u16[*pos]);
        if ((cp2 & 0xFC00) == 0xDC00)
        {
            (*pos)++;
            *cp = (*cp << 10) + cp2 - 0x35FDC00;
        }
    }
    return true;
}

template <typename T>
static bool GetNextCodePointFromUTF16(const T* u16, size_t len, size_t* pos, uint32_t* cp)
{
    if (len == 0)
        return GetNextCodePointFromUTF16z(u16, pos, cp);

    if (*pos >= len)
        return false;

    *cp = static_cast<uint32_t>(u16[*pos]);
    (*pos)++;
    if ((*cp & 0xFC00) == 0xD800)
    {
        if (*pos < len)
        {
            uint16_t cp2 = static_cast<uint16_t>(u16[*pos]);
            if ((cp2 & 0xFC00) == 0xDC00)
            {
                (*pos)++;
                *cp = (*cp << 10) + cp2 - 0x35FDC00;
            }
        }
    }
    return true;
}

static int CodePointToUTF8(uint32_t cp, uint32_t* c1, uint32_t* c2, uint32_t* c3, uint32_t* c4)
{
    if (cp < 0x80)
    {
        *c1 = cp;
        return 1;
    }
    else if (cp <= 0x7FF)
    {
        *c1 = (cp >> 6) + 0xC0;
        *c2 = (cp & 0x3F) + 0x80;
        return 2;
    }
    else if (cp <= 0xFFFF)
    {
        *c1 = (cp >> 12) + 0xE0;
        *c2 = ((cp >> 6) & 0x3F) + 0x80;
        *c3 = (cp & 0x3F) + 0x80;
        return 3;
    }
    else if (cp <= 0x10FFFF)
    {
        *c1 = (cp >> 18) + 0xF0;
        *c2 = ((cp >> 12) & 0x3F) + 0x80;
        *c3 = ((cp >> 6) & 0x3F) + 0x80;
        *c4 = (cp & 0x3F) + 0x80;
        return 4;
    }
    return 0;   
}

template <typename T>
std::string UTF16ToUTF8(const T* u16, size_t len = 0)
{
    std::string u8;
    uint32_t cp;
    size_t pos = 0;
    while (GetNextCodePointFromUTF16(u16, len, &pos, &cp))
    {
        uint32_t c[4];
        int count = CodePointToUTF8(cp, c, c+1, c+2, c+3);
        for (int i = 0; i < count; i++)
        {
            u8 += static_cast<char>(c[i]);
        }
    }
    return u8;
}

template <typename T>
std::wstring UTF16ToWstring(const T* u16, size_t len = 0)
{
    std::wstring ret;
    if( sizeof( wchar_t ) == 2 )
    {
        while( *u16 )
            ret += *u16++;
    }
    else
    {
        uint32_t cp;
        size_t   pos = 0;
        while( GetNextCodePointFromUTF16( u16, len, &pos, &cp ) )
        {
            if( !cp )
                break;

            ret += cp;
        }
    }
    return ret;
}

template <typename T>
std::string WstringToUTF8(const T* wstr)
{
    if( sizeof( wchar_t ) == 2 )
    {
        return UTF16ToUTF8( wstr );
    }
    else
    {
        std::string u8;
        uint32_t    cp;
        while( ( cp = *wstr++ ) != 0 )
        {
            uint32_t c[4];
            int      count = CodePointToUTF8( cp, c, c + 1, c + 2, c + 3 );
            for( int i = 0; i < count; i++ )
            {
                u8 += static_cast<char>( c[i] );
            }
        }
        return u8;
    }
}
