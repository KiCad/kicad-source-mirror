/**************************************/
/* Useful macros and inline functions */
/**************************************/

#ifndef MACROS_H
#define MACROS_H

#if wxUSE_UNICODE
#define CONV_TO_UTF8( wxstring )     ( (const char*) wxConvCurrent->cWX2MB( wxstring ) )
#define CONV_FROM_UTF8( utf8string ) ( wxConvCurrent->cMB2WC( utf8string ) )
#else
#define CONV_TO_UTF8( wxstring )     ( (const char*) ( wxstring.c_str() ) )
#define CONV_FROM_UTF8( utf8string ) (utf8string)
#endif


#ifndef MIN
#define MIN( x, y ) ( (x) > (y) ? (y) : (x) )
#endif
#ifndef MAX
#define MAX( x, y ) ( (x) > (y) ? (x) : (y) )
#endif

#ifndef ABS
#define ABS( y ) ( (y) >= 0 ? (y) : ( -(y) ) )
#endif

/// # of elements in an arrray
#define DIM(x)           (sizeof(x)/sizeof((x)[0]))


#define DEG2RAD( Deg ) ( (Deg) * M_PI / 180.0 )
#define RAD2DEG( Rad ) ( (Rad) * 180.0 / M_PI )

/* Normalize angle to be in the -360.0 .. 360.0 range or 0 .. 360.0: */
#define NORMALIZE_ANGLE( Angle ) { while( Angle < 0 ) \
                                       Angle += 3600;\
                                   while( Angle > 3600 ) \
                                       Angle -= 3600; }

/* Normalize angle to be in the 0.0 .. 360.0 range: */
#define NORMALIZE_ANGLE_POS( Angle ) { while( Angle < 0 ) \
                                           Angle += 3600;while( Angle >= 3600 ) \
                                           Angle -= 3600; }
#define NEGATE_AND_NORMALIZE_ANGLE_POS( Angle ) \
    { Angle = -Angle; while( Angle < 0 ) \
          Angle += 3600;while( Angle >= 3600 ) \
          Angle -= 3600; }

/* Normalize angle to be in the -90.0 .. 90.0 range */
#define NORMALIZE_ANGLE_90( Angle ) { while( Angle < -900 ) \
                                          Angle += 1800;\
                                      while( Angle > 900 ) \
                                          Angle -= 1800; }


/*****************************/
/* macro to exchange 2 items */
/*****************************/

/* this macro uses the typeof keyword
 * for compilers that do not know typeof (MSVC )
 * the boost libs have a workaround for the typeof problem
 */
#ifdef __MSVC__     // MSCV does not know typeof. Others def can be added here
#include "boost/typeof/typeof.hpp"

// we have to register the types used with the typeof keyword with boost
BOOST_TYPEOF_REGISTER_TYPE( wxPoint );
BOOST_TYPEOF_REGISTER_TYPE( wxSize );
BOOST_TYPEOF_REGISTER_TYPE( wxString );
class DrawSheetLabelStruct;
BOOST_TYPEOF_REGISTER_TYPE( DrawSheetLabelStruct* );
class EDA_BaseStruct;
BOOST_TYPEOF_REGISTER_TYPE( EDA_BaseStruct* );
class D_PAD;
BOOST_TYPEOF_REGISTER_TYPE( D_PAD* );
BOOST_TYPEOF_REGISTER_TYPE( const D_PAD* );
class BOARD_ITEM;
BOOST_TYPEOF_REGISTER_TYPE( BOARD_ ITEM* );

#define typeof (expr)BOOST_TYPEOF( expr )
#endif  // #ifdef __MSVC__

// here is the macro:
#define EXCHG( a, b ) { typeof(a)__temp__ = (a); (a) = (b); (b) = __temp__; }


/*****************************************************/
/* inline functions to insert menuitems with a icon: */
/*****************************************************/
static inline void ADD_MENUITEM( wxMenu* menu, int id,
                                 const wxString& text,
                                 const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text );
    l_item->SetBitmap( icon );
    menu->Append( l_item );
};

static inline void ADD_MENUITEM_WITH_HELP( wxMenu* menu, int id,
                                           const wxString& text,
                                           const wxString& help,
                                           const wxBitmap& icon )
{
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text, help );
    l_item->SetBitmap( icon );
    menu->Append( l_item );
};

#ifdef __WINDOWS__
static inline void ADD_MENUITEM_WITH_SUBMENU( wxMenu* menu, wxMenu* submenu,
                                              int id, const wxString& text,
                                              const wxBitmap& icon )
{
    extern wxFont* g_ItalicFont;
    wxMenuItem*    l_item;

    l_item = new wxMenuItem( menu, id, text );
    l_item->SetSubMenu( submenu );
    l_item->SetBitmap( icon );
    l_item->SetFont( *g_ItalicFont );
    menu->Append( l_item );
};

static inline void ADD_MENUITEM_WITH_HELP_AND_SUBMENU( wxMenu*         menu,
                                                       wxMenu*         submenu,
                                                       int             id,
                                                       const wxString& text,
                                                       const wxString& help,
                                                       const wxBitmap& icon )
{
    extern wxFont* g_ItalicFont;
    wxMenuItem*    l_item;

    l_item = new wxMenuItem( menu, id, text, help );
    l_item->SetSubMenu( submenu );
    l_item->SetBitmap( icon );
    l_item->SetFont( *g_ItalicFont );
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

#endif

#ifdef __WINDOWS__
#define SETBITMAPS( icon ) item->SetBitmaps( apply_xpm, (icon) )
#else
#define SETBITMAPS( icon )
#endif

#endif /* ifdef MACRO_H */
