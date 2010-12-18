/**************************************/
/* Useful macros and inline functions */
/**************************************/

#ifndef MACROS_H
#define MACROS_H

#include <wx/wx.h>


#if wxUSE_UNICODE
#define CONV_TO_UTF8( wxstring )     ( (const char*) wxConvCurrent->cWX2MB( wxstring ) )
#define CONV_FROM_UTF8( utf8string ) ( wxConvCurrent->cMB2WC( utf8string ) )
#else
#define CONV_TO_UTF8( wxstring )     ( (const char*) ( (wxstring).c_str() ) )
#define CONV_FROM_UTF8( utf8string ) (utf8string)
#endif


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
 * i.e. it depends on how the wxWidgets library was compiled.  There was a period
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


#ifndef MIN
#define MIN( x, y ) ( (x) > (y) ? (y) : (x) )
#endif
#ifndef MAX
#define MAX( x, y ) ( (x) > (y) ? (x) : (y) )
#endif

#ifndef ABS
#define ABS( y ) ( (y) >= 0 ? (y) : ( -(y) ) )
#endif

#define NEGATE( x ) (x = -x)

/// # of elements in an arrray
#define DIM( x )    unsigned( sizeof(x) / sizeof( (x)[0] ) )    // not size_t


#define DEG2RAD( Deg ) ( (Deg) * M_PI / 180.0 )
#define RAD2DEG( Rad ) ( (Rad) * 180.0 / M_PI )

/* Normalize angle to be in the -360.0 .. 360.0 range or 0 .. 360.0: */
#define NORMALIZE_ANGLE( Angle ) { while( Angle < 0 ) \
                                       Angle += 3600;\
                                   while( Angle > 3600 ) \
                                       Angle -= 3600;}

/* Normalize angle to be in the 0.0 .. 360.0 range: */
#define NORMALIZE_ANGLE_POS( Angle ) { while( Angle < 0 ) \
                                           Angle += 3600;while( Angle >= 3600 ) \
                                           Angle -= 3600;}
#define NEGATE_AND_NORMALIZE_ANGLE_POS( Angle ) \
    { Angle = -Angle; while( Angle < 0 ) \
          Angle += 3600;while( Angle >= 3600 ) \
          Angle -= 3600;}

/* Normalize angle to be in the -90.0 .. 90.0 range */
#define NORMALIZE_ANGLE_90( Angle ) { while( Angle < -900 ) \
                                          Angle += 1800;\
                                      while( Angle > 900 ) \
                                          Angle -= 1800;}

/* Normalize angle to be in the -180.0 .. 180.0 range */
#define NORMALIZE_ANGLE_180( Angle ) { while( Angle <= -1800 ) \
                                           Angle += 3600;\
                                       while( Angle > 1800 ) \
                                           Angle -= 3600;}

/*****************************/
/* macro to exchange 2 items */
/*****************************/

/*
 * The EXCHG macro uses BOOST_TYPEOF for compilers that do not have native
 * typeof support (MSVC).  Please do not attempt to qualify these macros
 * within #ifdef compiler definitions pragmas.  BOOST_TYPEOF is smart enough
 * to check for native typeof support and use it instead of it's own
 * implementation.  These macros effectively compile to nothing on platforms
 * with native typeof support.
 */

#include "boost/typeof/typeof.hpp"

// we have to register the types used with the typeof keyword with boost
BOOST_TYPEOF_REGISTER_TYPE( wxPoint )
BOOST_TYPEOF_REGISTER_TYPE( wxSize )
BOOST_TYPEOF_REGISTER_TYPE( wxString )
class DrawSheetLabelStruct;
BOOST_TYPEOF_REGISTER_TYPE( DrawSheetLabelStruct* )
class EDA_ITEM;
BOOST_TYPEOF_REGISTER_TYPE( EDA_ITEM* )
class D_PAD;
BOOST_TYPEOF_REGISTER_TYPE( D_PAD* )
BOOST_TYPEOF_REGISTER_TYPE( const D_PAD* )
class BOARD_ITEM;
BOOST_TYPEOF_REGISTER_TYPE( BOARD_ITEM* )

#define EXCHG( a, b ) { BOOST_TYPEOF( a ) __temp__ = (a);      \
                        (a) = (b);                           \
                        (b) = __temp__; }


/*****************************************************/
/* inline functions to insert menuitems with a icon: */
/*****************************************************/
static inline void ADD_MENUITEM( wxMenu* menu, int id,
                                 const wxString& text,
                                 const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text );

#if !defined( __WXMAC__ )
    l_item->SetBitmap( icon );
#endif /* !defined( __WXMAC__ ) */

    menu->Append( l_item );
}

static inline void ADD_MENUITEM_WITH_HELP( wxMenu* menu, int id,
                                           const wxString& text,
                                           const wxString& help,
                                           const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text, help );

#if !defined( __WXMAC__ )
    l_item->SetBitmap( icon );
#endif /* !defined( __WXMAC__ ) */

    menu->Append( l_item );
}

#ifdef __WINDOWS__
static inline void ADD_MENUITEM_WITH_SUBMENU( wxMenu* menu, wxMenu* submenu,
                                              int id, const wxString& text,
                                              const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text );
    l_item->SetSubMenu( submenu );
    l_item->SetBitmap( icon );
    menu->Append( l_item );
};

static inline void ADD_MENUITEM_WITH_HELP_AND_SUBMENU( wxMenu*         menu,
                                                       wxMenu*         submenu,
                                                       int             id,
                                                       const wxString& text,
                                                       const wxString& help,
                                                       const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text, help );
    l_item->SetSubMenu( submenu );
    l_item->SetBitmap( icon );
    menu->Append( l_item );
};

#else
static inline void ADD_MENUITEM_WITH_SUBMENU( wxMenu* menu, wxMenu* submenu,
                                              int id,
                                              const wxString& text,
                                              const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text );
    l_item->SetSubMenu( submenu );

#if !defined( __WXMAC__ )
    l_item->SetBitmap( icon );
#endif /* !defined( __WXMAC__ ) */

    menu->Append( l_item );
}

static inline void ADD_MENUITEM_WITH_HELP_AND_SUBMENU( wxMenu*         menu,
                                                       wxMenu*         submenu,
                                                       int             id,
                                                       const wxString& text,
                                                       const wxString& help,
                                                       const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text, help );
    l_item->SetSubMenu( submenu );

#if !defined( __WXMAC__ )
    l_item->SetBitmap( icon );
#endif /* !defined( __WXMAC__ ) */

    menu->Append( l_item );
}

#endif

#ifdef __WINDOWS__
#  define SETBITMAPS( icon ) item->SetBitmaps( apply_xpm, (icon) )
#else
#  define SETBITMAPS( icon )
#endif

#endif /* ifdef MACRO_H */
