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
#define MIN( x, y )       ( (x) > (y) ? (y) : (x) )
#endif
#ifndef MAX
#define MAX( x, y )       ( (x) > (y) ? (x) : (y) )
#endif

#ifndef ABS
#define ABS( y )          ( (y) >= 0 ? (y) : ( -(y) ) )
#endif

#define DEG2RAD( Deg )    ( (Deg) * M_PI / 180.0 )
#define RAD2DEG( Rad )    ( (Rad) * 180.0 / M_PI )

/* Normalize angle to be in the -360.0 .. 360.0 range or 0 .. 360.0: */
#define NORMALIZE_ANGLE( Angle )  { while( Angle < 0 ) \
                                        Angle += 3600;\
                                    while( Angle > 3600 ) \
                                        Angle -= 3600; }

/* Normalize angle to be in the 0.0 .. 360.0 range: */
#define NORMALIZE_ANGLE_POS( Angle )  { while( Angle < 0 ) \
                                            Angle += 3600;while( Angle >= 3600 ) \
                                            Angle -= 3600; }
#define NEGATE_AND_NORMALIZE_ANGLE_POS( Angle ) \
    { Angle = -Angle; while( Angle < 0 ) \
          Angle += 3600;while( Angle >= 3600 ) \
          Angle -= 3600; }

/* Normalize angle to be in the -90.0 .. 90.0 range */
#define NORMALIZE_ANGLE_90( Angle )   { while( Angle < -900 ) \
                                            Angle += 1800;\
                                        while( Angle > 900 ) \
                                            Angle -= 1800; }


/****************************************/
/* inline functions to exchange 2 items */
/****************************************/

#define EXCHG( a, b ) { typeof(a) __temp__ = (a); (a) = (b); (b) = __temp__; }


class Hierarchical_PIN_Sheet_Struct;
static inline void EXCHG( const Hierarchical_PIN_Sheet_Struct* a, const Hierarchical_PIN_Sheet_Struct* b )
{
    const Hierarchical_PIN_Sheet_Struct* temp = a;
    a = b;
    b = temp;
};

class SCH_CMP_FIELD;
static inline void EXCHG( const SCH_CMP_FIELD* a, const SCH_CMP_FIELD* b )
{
    const SCH_CMP_FIELD* temp = a;
    a = b;
    b = temp;
};

class BOARD_ITEM;
static inline void EXCHG( const BOARD_ITEM* a, const BOARD_ITEM* b )
{
    const BOARD_ITEM* temp = a;
    a = b;
    b = temp;
};

class D_PAD;
static inline void EXCHG( const D_PAD* a, const D_PAD* b )
{
    const D_PAD* temp = a;
    a = b;
    b = temp;
};

#endif

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
	extern wxFont * g_ItalicFont;
    wxMenuItem* l_item;

    l_item = new wxMenuItem( menu, id, text );
    l_item->SetSubMenu( submenu );
    l_item->SetBitmap( icon );
    l_item->SetFont( *g_ItalicFont );
    menu->Append( l_item );
};

static inline void ADD_MENUITEM_WITH_HELP_AND_SUBMENU( wxMenu* menu,
                                                       wxMenu* submenu,
                                                       int             id,
                                                       const wxString& text,
                                                       const wxString& help,
                                                       const wxBitmap&       icon )
{
	extern wxFont * g_ItalicFont;
    wxMenuItem* l_item;

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
                                                       const wxBitmap&       icon )
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
