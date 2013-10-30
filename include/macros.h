/**
 * @file macros.h
 * @brief This file contains miscellaneous helper definitions and functions.
 */

#ifndef MACROS_H
#define MACROS_H

#include <wx/wx.h>

/**
 * Macro TO_UTF8
 * converts a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 * NOTE: Trying to convert it to a function is tricky because of the
 * type of the parameter!
 */
#define TO_UTF8( wxstring )  ( (const char*) (wxstring).utf8_str() )

/**
 * function FROM_UTF8
 * converts a UTF8 encoded C string to a wxString for all wxWidgets build modes.
 */
static inline wxString FROM_UTF8( const char* cstring )
{
    wxString line = wxString::FromUTF8( cstring );

    if( line.IsEmpty() )  // happens when cstring is not a valid UTF8 sequence
        line = wxConvCurrent->cMB2WC( cstring );    // try to use locale conversion

    return line;
}

/**
 * Function GetChars
 * returns a wxChar* to the actual character data within a wxString, and is
 * helpful for passing strings to wxString::Printf(wxT("%s"), GetChars(wxString) )
 * <p>
 * wxChar is defined to be
 * <ul>
 * <li> standard C style char when wxUSE_UNICODE==0 </li>
 * <li> wchar_t when wxUSE_UNICODE==1 (the default). </li>
 * </ul>
 * i.e. it depends on how the wxWidgets library was compiled.
 * ( wxUSE_UNICODE is defined in wxWidgets, inside setup.h.
 * for version >= 2.9 wxUSE_UNICODE is always defined to 1 )
 * There was a period
 * during the development of wxWidgets 2.9 when GetData() was missing, so this
 * function was used to provide insulation from that design change.  It may
 * no longer be needed, and is harmless.  GetData() seems to be an acceptable
 * alternative in all cases now.
 */
static inline const wxChar* GetChars( const wxString& s )
{
#if wxCHECK_VERSION( 2, 9, 0 )
    return (const wxChar*) s.c_str();
#else
    return s.GetData();
#endif
}

// This really needs a function? well, it is used *a lot* of times
template <class T> inline void NEGATE( T &x ) { x = -x; }

/// # of elements in an array
#define DIM( x )    unsigned( sizeof(x) / sizeof( (x)[0] ) )    // not size_t

/// Exchange two values
// std::swap works only with arguments of the same type (which is saner);
// here the compiler will figure out what to do (I hope to get rid of
// this soon or late)
template <class T, class T2> inline void EXCHG( T& a, T2& b )
{
    T temp = a;
    a = b;
    b = temp;
}

#endif /* ifdef MACRO_H */
