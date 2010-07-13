/**************/
/* common.cpp */
/**************/

#include "fctsys.h"
#include "gr_basic.h"
#include "trigo.h"
#include "wxstruct.h"
#include "base_struct.h"
#include "common.h"
#include "macros.h"
#include "build_version.h"
#include "confirm.h"
#include <wx/process.h>

/**
 * Global variables definitions.
 *
 * TODO: All if these variables should be moved into the class were they
 *       are defined and used.  Most of them probably belong in the
 *       application class.
 */

/* Standard page sizes in 1/1000 inch */
#if defined(KICAD_GOST)
Ki_PageDescr  g_Sheet_A4( wxSize( 8283, 11700 ), wxPoint( 0, 0 ), wxT( "A4" ) );
#else
Ki_PageDescr  g_Sheet_A4( wxSize( 11700, 8267 ), wxPoint( 0, 0 ), wxT( "A4" ) );
#endif
Ki_PageDescr  g_Sheet_A3( wxSize( 16535, 11700 ), wxPoint( 0, 0 ), wxT( "A3" ) );
Ki_PageDescr  g_Sheet_A2( wxSize( 23400, 16535 ), wxPoint( 0, 0 ), wxT( "A2" ) );
Ki_PageDescr  g_Sheet_A1( wxSize( 33070, 23400 ), wxPoint( 0, 0 ), wxT( "A1" ) );
Ki_PageDescr  g_Sheet_A0( wxSize( 46800, 33070 ), wxPoint( 0, 0 ), wxT( "A0" ) );
Ki_PageDescr  g_Sheet_A( wxSize( 11000, 8500 ), wxPoint( 0, 0 ), wxT( "A" ) );
Ki_PageDescr  g_Sheet_B( wxSize( 17000, 11000 ), wxPoint( 0, 0 ), wxT( "B" ) );
Ki_PageDescr  g_Sheet_C( wxSize( 22000, 17000 ), wxPoint( 0, 0 ), wxT( "C" ) );
Ki_PageDescr  g_Sheet_D( wxSize( 34000, 22000 ), wxPoint( 0, 0 ), wxT( "D" ) );
Ki_PageDescr  g_Sheet_E( wxSize( 44000, 34000 ), wxPoint( 0, 0 ), wxT( "E" ) );
Ki_PageDescr  g_Sheet_GERBER( wxSize( 32000, 32000 ), wxPoint( 0, 0 ),
                              wxT( "GERBER" ) );
Ki_PageDescr  g_Sheet_user( wxSize( 17000, 11000 ), wxPoint( 0, 0 ),
                            wxT( "User" ) );

Ki_PageDescr* g_SheetSizeList[NB_ITEMS + 1] =
{
    &g_Sheet_A4,   &g_Sheet_A3, &g_Sheet_A2, &g_Sheet_A1, &g_Sheet_A0,
    &g_Sheet_A,    &g_Sheet_B,  &g_Sheet_C,  &g_Sheet_D,  &g_Sheet_E,
    &g_Sheet_user, NULL
};


/* File extension definitions.  Please do not changes these.  If a different
 * file extension is needed, create a new definition in the application.
 * Please note, just because they are defined as const doesn't guarantee
 * that they cannot be changed. */
const wxString ProjectFileExtension( wxT( "pro" ) );
const wxString SchematicFileExtension( wxT( "sch" ) );
const wxString NetlistFileExtension( wxT( "net" ) );
const wxString GerberFileExtension( wxT( "pho" ) );
const wxString PcbFileExtension( wxT( "brd" ) );
const wxString PdfFileExtension( wxT( "pdf" ) );

/* Proper wxFileDialog wild card definitions. */
const wxString ProjectFileWildcard( _( "Kicad project files (*.pro)|*.pro" ) );
const wxString SchematicFileWildcard( _( "Kicad schematic files (*.sch)|*.sch" ) );
const wxString NetlistFileWildcard( _( "Kicad netlist files (*.net)|*.net" ) );
const wxString GerberFileWildcard( _( "Gerber files (*.pho)|*.pho" ) );
const wxString PcbFileWildcard( _( "Kicad printed circuit board files (*.brd)|*.brd" ) );
const wxString PdfFileWildcard( _( "Portable document format files (*.pdf)|*.pdf" ) );
const wxString AllFilesWildcard( _( "All files (*)|*" ) );


wxString       g_ProductName    = wxT( "KiCad E.D.A.  " );
bool           g_ShowPageLimits = true;
wxString       g_UserLibDirBuffer;
int            g_DebugLevel;
int            g_MouseOldButtons;
int            g_KeyPressed;

wxString       g_Prj_Default_Config_FullFilename;
wxString       g_Prj_Config_LocalFilename;

/* Current user unit of measure */
UserUnitType   g_UserUnit;

/* Draw color for moving objects: */
int            g_GhostColor;

/* predefined colors used in kicad.
 * Please: if you change a value, remember these values are carefully chosen
 * to have good results in pcbnew, that uses the ORed value of basic colors
 * when displaying superimposed objects
 * This list must have exactly NBCOLOR items
 */
StructColors ColorRefs[NBCOLOR] =
{
    { 0,    0,   0,   BLACK,        wxT( "BLACK" ),        DARKDARKGRAY          },
    { 192,  0,   0,   BLUE,         wxT( "BLUE" ),         LIGHTBLUE             },
    { 0,    160, 0,   GREEN,        wxT( "GREEN" ),        LIGHTGREEN            },
    { 160,  160, 0,   CYAN,         wxT( "CYAN" ),         LIGHTCYAN             },
    { 0,    0,   160, RED,          wxT( "RED" ),          LIGHTRED              },
    { 160,  0,   160, MAGENTA,      wxT( "MAGENTA" ),      LIGHTMAGENTA          },
    { 0,    128, 128, BROWN,        wxT( "BROWN" ),        YELLOW                },
    { 192,  192, 192, LIGHTGRAY,    wxT( "GRAY" ),         WHITE                 },
    { 128,  128, 128, DARKGRAY,     wxT( "DARKGRAY" ),     LIGHTGRAY             },
    { 255,  0,   0,   LIGHTBLUE,    wxT( "LIGHTBLUE" ),    LIGHTBLUE             },
    { 0,    255, 0,   LIGHTGREEN,   wxT( "LIGHTGREEN" ),   LIGHTGREEN            },
    { 255,  255, 0,   LIGHTCYAN,    wxT( "LIGHTCYAN" ),    LIGHTCYAN             },
    { 0,    0,   255, LIGHTRED,     wxT( "LIGHTRED" ),     LIGHTRED              },
    { 255,  0,   255, LIGHTMAGENTA, wxT( "LIGHTMAGENTA" ), LIGHTMAGENTA          },
    { 0,    255, 255, YELLOW,       wxT( "YELLOW" ),       YELLOW                },
    { 255,  255, 255, WHITE,        wxT( "WHITE" ),        WHITE                 },
    {  64,  64,  64,  DARKDARKGRAY, wxT( "DARKDARKGRAY" ), DARKGRAY              },
    {  64,  0,   0,   DARKBLUE,     wxT( "DARKBLUE" ),     BLUE                  },
    {    0, 64,  0,   DARKGREEN,    wxT( "DARKGREEN" ),    GREEN                 },
    {  64,  64,  0,   DARKCYAN,     wxT( "DARKCYAN" ),     CYAN                  },
    {    0, 0,   80,  DARKRED,      wxT( "DARKRED" ),      RED                   },
    {  64,  0,   64,  DARKMAGENTA,  wxT( "DARKMAGENTA" ),  MAGENTA               },
    {    0, 64,  64,  DARKBROWN,    wxT( "DARKBROWN" ),    BROWN                 },
    {  128, 255, 255, LIGHTYELLOW,  wxT( "LIGHTYELLOW" ),  LIGHTYELLOW           }
};

/** Function to use local notation or C standard notation for floating point numbers
 * some countries use 1,5 and others (and C) 1.5
 * so we switch from local to C and C to local when reading or writing files
 * And other problem is a bug when cross compiling under linux:
 * a printf print 1,5 and the read functions expects 1.5
 * (depending on version print = 1.5 and read = 1,5
 * Very annoying and we detect this and use a stupid but necessary workarount
*/
bool g_DisableFloatingPointLocalNotation = false;


/** function SetLocaleTo_C_standard
 * because kicad is internationalized, switch internalization to "C" standard
 * i.e. uses the . (dot) as separator in print/read float numbers
 * (some countries (France, Germany ..) use , (comma) as separator)
 * This function must be called before read or write ascii files using float
 * numbers in data the SetLocaleTo_C_standard function must be called after
 * reading or writing the file
 *
 * This is wrapper to the C setlocale( LC_NUMERIC, "C" ) function,
 * but could make more easier an optional use of locale in kicad
 */
void SetLocaleTo_C_standard( void )
{
    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C
}


/** function SetLocaleTo_Default
 * because kicad is internationalized, switch internalization to default
 * to use the default separator in print/read float numbers
 * (. (dot) but some countries (France, Germany ..) use , (comma) as separator)
 * This function must be called after a call to SetLocaleTo_C_standard
 *
 * This is wrapper to the C setlocale( LC_NUMERIC, "" ) function,
 * but could make more easier an optional use of locale in kicad
 */
void SetLocaleTo_Default( void )
{
    if( ! g_DisableFloatingPointLocalNotation )
        setlocale( LC_NUMERIC, "" );      // revert to the current locale
}


bool EnsureTextCtrlWidth( wxTextCtrl*     aCtrl,
                          const wxString* aString )
{
    wxWindow* window = aCtrl->GetParent();

    if( !window )
        window = aCtrl;

    wxString ctrlText;

    if( !aString )
    {
        ctrlText = aCtrl->GetValue();
        aString  = &ctrlText;
    }

    wxCoord width;
    wxCoord height;

    {
        wxClientDC dc( window );
        dc.SetFont( aCtrl->GetFont() );
        dc.GetTextExtent( *aString, &width, &height );
    }

    wxSize size = aCtrl->GetSize();
    if( size.GetWidth() < width + 10 )
    {
        size.SetWidth( width + 10 );
        aCtrl->SetSizeHints( size );
        return true;
    }
    return false;
}


Ki_PageDescr::Ki_PageDescr( const wxSize&   size,
                            const wxPoint&  offset,
                            const wxString& name )
{
    // All sizes are in 1/1000 inch
    m_Size   = size;
    m_Offset = offset;
    m_Name   = name;

    // Adjust the default value for margins to 400 mils (0,4 inch or 10 mm)
#if defined(KICAD_GOST)
    m_LeftMargin   = GOST_LEFTMARGIN;
    m_RightMargin  = GOST_RIGHTMARGIN;
    m_TopMargin    = GOST_TOPMARGIN;
    m_BottomMargin = GOST_BOTTOMMARGIN;
#else
    m_LeftMargin = m_RightMargin = m_TopMargin = m_BottomMargin = 400;
#endif
}


wxString ReturnUnitSymbol( UserUnitType aUnit, const wxString& formatString )
{
    wxString tmp;
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        tmp = _( "\"" );
        break;

    case MILLIMETRES:
        tmp = _( "mm" );
        break;

    case UNSCALED_UNITS:
        break;
    }

    if( formatString.IsEmpty() )
        return tmp;

    label.Printf( formatString, GetChars( tmp ) );

    return label;
}


wxString GetUnitsLabel( UserUnitType aUnit )
{
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        label = _( "inches" );
        break;

    case MILLIMETRES:
        label = _( "millimeters" );
        break;

    case UNSCALED_UNITS:
        label = _( "units" );
        break;
    }

    return label;
}


wxString GetAbbreviatedUnitsLabel( UserUnitType aUnit )
{
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        label = _( "in" );
        break;

    case MILLIMETRES:
        label = _( "mm" );
        break;

    case UNSCALED_UNITS:
        break;
    }

    return label;
}


/*
 * Add string "  (mm):" or " ("):" to the static text Stext.
 *  Used in dialog boxes for entering values depending on selected units
 */
void AddUnitSymbol( wxStaticText& Stext, UserUnitType aUnit )
{
    wxString msg = Stext.GetLabel();

    msg += ReturnUnitSymbol( aUnit );

    Stext.SetLabel( msg );
}


/*
 * Convert the number Value in a string according to the internal units
 *  and the selected unit (g_UserUnit) and put it in the wxTextCtrl TextCtrl
 */
void PutValueInLocalUnits( wxTextCtrl& TextCtr, int Value, int Internal_Unit )
{
    wxString msg = ReturnStringFromValue( g_UserUnit, Value, Internal_Unit );

    TextCtr.SetValue( msg );
}


/*
 * Convert the Value in the wxTextCtrl TextCtrl in an integer,
 *  according to the internal units and the selected unit (g_UserUnit)
 */
int ReturnValueFromTextCtrl( const wxTextCtrl& TextCtr, int Internal_Unit )
{
    int      value;
    wxString msg = TextCtr.GetValue();

    value = ReturnValueFromString( g_UserUnit, msg, Internal_Unit );

    return value;
}


/** Function ReturnStringFromValue
 * Return the string from Value, according to units (inch, mm ...) for display,
 * and the initial unit for value
 * @param aUnit = display units (INCHES, MILLIMETRE ..)
 * @param aValue = value in Internal_Unit
 * @param aInternal_Unit = units per inch for Value
 * @param aAdd_unit_symbol = true to add symbol unit to the string value
 * @return a wxString what contains value and optionally the symbol unit
 *         (like 2.000 mm)
 */
wxString ReturnStringFromValue( UserUnitType aUnit, int aValue, int aInternal_Unit,
                                bool aAdd_unit_symbol )
{
    wxString StringValue;
    double   value_to_print;

    value_to_print = To_User_Unit( aUnit, aValue, aInternal_Unit );
    /* Yet another 'if pcbnew' :( */
    StringValue.Printf( ( aInternal_Unit > 1000 ) ? wxT( "%.4f" ) :
                        wxT( "%.3f" ),
                        value_to_print );

    if( aAdd_unit_symbol )
        switch( aUnit )
        {
        case INCHES:
            StringValue += _( " \"" );
            break;

        case MILLIMETRES:
            StringValue += _( " mm" );
            break;

        case UNSCALED_UNITS:
            break;
        }

    return StringValue;
}


/*
 * Return the string from Value, according to units (inch, mm ...) for display,
 *  and the initial unit for value
 *  Unit = display units (INCH, MM ..)
 *  Value = text
 *  Internal_Unit = units per inch for computed value
 */
int ReturnValueFromString( UserUnitType aUnit, const wxString& TextValue,
                           int Internal_Unit )
{
    int    Value;
    double dtmp = 0;

    /* Acquire the 'right' decimal point separator */
    const struct lconv* lc = localeconv();
    wxChar decimal_point = lc->decimal_point[0];
    wxString            buf( TextValue.Strip( wxString::both ) );

    /* Convert the period in decimal point */
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );

    /* Find the end of the numeric part */
    unsigned brk_point = 0;
    while( brk_point < buf.Len() )
    {
        wxChar ch = buf[brk_point];
        if( !( (ch >= '0' && ch <='9') || (ch == decimal_point)
             || (ch == '-') || (ch == '+') ) )
        {
            break;
        }
        ++brk_point;
    }

    /* Extract the numeric part */
    buf.Left( brk_point ).ToDouble( &dtmp );

    /* Check the optional unit designator (2 ch significant) */
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );
    if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
    {
        aUnit = INCHES;
    }
    else if( unit == wxT( "mm" ) )
    {
        aUnit = MILLIMETRES;
    }
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) ) /* Mils or thous */
    {
        aUnit = INCHES;
        dtmp /= 1000;
    }
    Value = From_User_Unit( aUnit, dtmp, Internal_Unit );

    return Value;
}


/**
 * Function wxStringSplit
 * Split a String to a String List when founding 'splitter'
 * @return the list
 * @param txt : wxString : a String text
 * @param splitter : wxChar : the 'split' character
 */
wxArrayString* wxStringSplit( wxString txt, wxChar splitter )
{
    wxArrayString* list = new wxArrayString();

    while( 1 )
    {
        int index = txt.Find( splitter );
        if( index == wxNOT_FOUND )
            break;

        wxString tmp;
        tmp = txt.Mid( 0, index );
        txt = txt.Mid( index + 1, txt.size() - index );
        list->Add( tmp );
    }

    if( !txt.IsEmpty() )
    {
        list->Add( txt );
    }

    return list;
}


/**
 * Function To_User_Unit
 * Convert in inch or mm the variable "val" (double)given in internal units
 * @return the converted value, in double
 * @param aUnit : user measure unit
 * @param val : double : the given value
 * @param internal_unit_value = internal units per inch
 */
double To_User_Unit( UserUnitType aUnit, double val, int internal_unit_value )
{
    switch( aUnit )
    {
    case MILLIMETRES:
        return val * 25.4 / internal_unit_value;

    case INCHES:
        return val / internal_unit_value;

    default:
        return val;
    }
}


/*
 * Return in internal units the value "val" given in inch or mm
 */
int From_User_Unit( UserUnitType aUnit, double val, int internal_unit_value )
{
    double value;

    switch( aUnit )
    {
    case MILLIMETRES:
        value = val * internal_unit_value / 25.4;
        break;

    case INCHES:
        value = val * internal_unit_value;
        break;

    default:
    case UNSCALED_UNITS:
        value = val;
    }

    return wxRound( value );
}


/*
 * Return the string date "day month year" like "23 jun 2005"
 */
wxString GenDate()
{
    static const wxString mois[12] =
    {
        wxT( "jan" ), wxT( "feb" ), wxT( "mar" ), wxT( "apr" ), wxT( "may" ), wxT( "jun" ),
        wxT( "jul" ), wxT( "aug" ), wxT( "sep" ), wxT( "oct" ), wxT( "nov" ), wxT( "dec" )
    };

    time_t     buftime;
    struct tm* Date;
    wxString   string_date;

    time( &buftime );
    Date = gmtime( &buftime );
    string_date.Printf( wxT( "%d %s %d" ), Date->tm_mday,
                        GetChars( mois[Date->tm_mon] ),
                        Date->tm_year + 1900 );
    return string_date;
}


/*
 * My memory allocation
 */
void* MyMalloc( size_t nb_octets )
{
    void* pt_mem;

    if( nb_octets == 0 )
    {
        DisplayError( NULL, wxT( "Allocate 0 bytes !!" ) );
        return NULL;
    }
    pt_mem = malloc( nb_octets );
    if( pt_mem == NULL )
    {
        wxString msg;
        msg.Printf( wxT( "Out of memory: allocation %d bytes" ), nb_octets );
        DisplayError( NULL, msg );
    }
    return pt_mem;
}


/**
 * Function ProcessExecute
 * runs a child process.
 * @param aCommandLine The process and any arguments to it all in a single string.
 * @param aFlags The same args as allowed for wxExecute()
 * @return bool - true if success, else false
 */
bool ProcessExecute( const wxString& aCommandLine, int aFlags )
{
#ifdef __WINDOWS__
    int        pid = wxExecute( aCommandLine );
    return pid ? true : false;
#else
    wxProcess* process = wxProcess::Open( aCommandLine, aFlags );
    return (process != NULL) ? true : false;
#endif
}


/*
 * My memory allocation, memory space is cleared
 */
void* MyZMalloc( size_t nb_octets )
{
    void* pt_mem = MyMalloc( nb_octets );

    if( pt_mem )
        memset( pt_mem, 0, nb_octets );
    return pt_mem;
}


void MyFree( void* pt_mem )
{
    if( pt_mem )
        free( pt_mem );
}

enum textbox {
    ID_TEXTBOX_LIST = 8010
};


BEGIN_EVENT_TABLE( WinEDA_TextFrame, wxDialog )
    EVT_LISTBOX_DCLICK( ID_TEXTBOX_LIST, WinEDA_TextFrame::D_ClickOnList )
    EVT_LISTBOX( ID_TEXTBOX_LIST, WinEDA_TextFrame::D_ClickOnList )
    EVT_CLOSE( WinEDA_TextFrame::OnClose )
END_EVENT_TABLE()


WinEDA_TextFrame::WinEDA_TextFrame( wxWindow*       parent,
                                    const wxString& title ) :
    wxDialog( parent,
              -1, title,
              wxPoint( -1, -1 ),
              wxSize( 250, 350 ),
              wxDEFAULT_DIALOG_STYLE |
              wxFRAME_FLOAT_ON_PARENT |
              MAYBE_RESIZE_BORDER )
{
    wxSize size;

    m_Parent = parent;

    CentreOnParent();

    size   = GetClientSize();
    m_List = new wxListBox( this,
                            ID_TEXTBOX_LIST,
                            wxPoint( 0, 0 ),
                            size,
                            0, NULL,
                            wxLB_ALWAYS_SB | wxLB_SINGLE );

    SetReturnCode( -1 );
}


void WinEDA_TextFrame::Append( const wxString& text )
{
    m_List->Append( text );
}


void WinEDA_TextFrame::D_ClickOnList( wxCommandEvent& event )
{
    int ii = m_List->GetSelection();

    EndModal( ii );
}


void WinEDA_TextFrame::OnClose( wxCloseEvent& event )
{
    EndModal( -1 );
}


void Affiche_1_Parametre( WinEDA_DrawFrame* frame, int pos_X,
                          const wxString& texte_H, const wxString& texte_L,
                          int color )
{
    frame->MsgPanel->Affiche_1_Parametre( pos_X, texte_H, texte_L, color );
}


int GetTimeStamp()
{
    static int OldTimeStamp, NewTimeStamp;

    NewTimeStamp = time( NULL );
    if( NewTimeStamp <= OldTimeStamp )
        NewTimeStamp = OldTimeStamp + 1;
    OldTimeStamp = NewTimeStamp;
    return NewTimeStamp;
}


/* Returns to display the value of a parameter, by type of units selected
 * Input: value in mils, buffer text
 * Returns to buffer: text: value expressed in inches or millimeters
 * Followed by " or mm
 */
const wxString& valeur_param( int valeur, wxString& buf_texte )
{
    switch( g_UserUnit )
    {
    case MILLIMETRES:
        buf_texte.Printf( wxT( "%3.3f mm" ), valeur * 0.00254 );
        break;

    case INCHES:
        buf_texte.Printf( wxT( "%2.4f \"" ), valeur * 0.0001 );
        break;

    case UNSCALED_UNITS:
        buf_texte.Printf( wxT( "%d" ), valeur );
        break;
    }

    return buf_texte;
}


/*
 *
 */
wxString& operator <<( wxString& aString, const wxPoint& aPos )
{
    wxString temp;

    aString << wxT( "@ (" ) << valeur_param( aPos.x, temp );
    aString << wxT( "," ) << valeur_param( aPos.y, temp );
    aString << wxT( ")" );

    return aString;
}
