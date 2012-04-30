/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file common.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <macros.h>
#include <build_version.h>
#include <confirm.h>
#include <base_units.h>

#include <wx/process.h>

/**
 * Global variables definitions.
 *
 * TODO: All if these variables should be moved into the class were they
 *       are defined and used.  Most of them probably belong in the
 *       application class.
 */

wxString       g_ProductName    = wxT( "KiCad E.D.A.  " );
bool           g_ShowPageLimits = true;
wxString       g_UserLibDirBuffer;

wxString       g_Prj_Default_Config_FullFilename;
wxString       g_Prj_Config_LocalFilename;

EDA_UNITS_T    g_UserUnit;

int            g_GhostColor;


#if defined(KICAD_GOST)
static bool s_gost = true;
#else
static bool s_gost = false;
#endif

bool IsGOST()
{
    return s_gost;
}


/**
 * The predefined colors used in KiCad.
 * Please: if you change a value, remember these values are carefully chosen
 * to have good results in Pcbnew, that uses the ORed value of basic colors
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


/**
 * Function to use local notation or C standard notation for floating point numbers
 * some countries use 1,5 and others (and C) 1.5
 * so we switch from local to C and C to local when reading or writing files
 * And other problem is a bug when cross compiling under linux:
 * a printf print 1,5 and the read functions expects 1.5
 * (depending on version print = 1.5 and read = 1,5
 * Very annoying and we detect this and use a stupid but necessary workarount
*/
bool g_DisableFloatingPointLocalNotation = false;


int LOCALE_IO::C_count;


void SetLocaleTo_C_standard()
{
    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C
}

void SetLocaleTo_Default()
{
    if( !g_DisableFloatingPointLocalNotation )
        setlocale( LC_NUMERIC, "" );      // revert to the current locale
}


bool EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString )
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


wxString ReturnUnitSymbol( EDA_UNITS_T aUnit, const wxString& formatString )
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


wxString GetUnitsLabel( EDA_UNITS_T aUnit )
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


wxString GetAbbreviatedUnitsLabel( EDA_UNITS_T aUnit )
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


void AddUnitSymbol( wxStaticText& Stext, EDA_UNITS_T aUnit )
{
    wxString msg = Stext.GetLabel();

    msg += ReturnUnitSymbol( aUnit );

    Stext.SetLabel( msg );
}


wxArrayString* wxStringSplit( wxString aString, wxChar aSplitter )
{
    wxArrayString* list = new wxArrayString();

    while( 1 )
    {
        int index = aString.Find( aSplitter );

        if( index == wxNOT_FOUND )
            break;

        wxString tmp;
        tmp = aString.Mid( 0, index );
        aString = aString.Mid( index + 1, aString.size() - index );
        list->Add( tmp );
    }

    if( !aString.IsEmpty() )
    {
        list->Add( aString );
    }

    return list;
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


unsigned long GetNewTimeStamp()
{
    static unsigned long oldTimeStamp;
    unsigned long newTimeStamp;

    newTimeStamp = time( NULL );

    if( newTimeStamp <= oldTimeStamp )
        newTimeStamp = oldTimeStamp + 1;

    oldTimeStamp = newTimeStamp;

    return newTimeStamp;
}


double RoundTo0( double x, double precision )
{
    assert( precision != 0 );

    long long ix = KiROUND( x * precision );

    if ( x < 0.0 )
        NEGATE( ix );

    int remainder = ix % 10;   // remainder is in precision mm

    if ( remainder <= 2 )
        ix -= remainder;       // truncate to the near number
    else if (remainder >= 8 )
        ix += 10 - remainder;  // round to near number

    if ( x < 0 )
        NEGATE( ix );

    return (double) ix / precision;
}
