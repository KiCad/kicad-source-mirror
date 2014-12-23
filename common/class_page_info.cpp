/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <common.h>
#include <class_page_info.h>
#include <macros.h>


// late arriving wxPAPER_A0, wxPAPER_A1
#if wxABI_VERSION >= 20999
 #define PAPER_A0   wxPAPER_A0
 #define PAPER_A1   wxPAPER_A1
#else
 #define PAPER_A0   wxPAPER_A2
 #define PAPER_A1   wxPAPER_A2
#endif


// Standard paper sizes nicknames.
const wxChar PAGE_INFO::A4[] = wxT( "A4" );
const wxChar PAGE_INFO::A3[] = wxT( "A3" );
const wxChar PAGE_INFO::A2[] = wxT( "A2" );
const wxChar PAGE_INFO::A1[] = wxT( "A1" );
const wxChar PAGE_INFO::A0[] = wxT( "A0" );
const wxChar PAGE_INFO::A[]  = wxT( "A" );
const wxChar PAGE_INFO::B[]  = wxT( "B" ) ;
const wxChar PAGE_INFO::C[]  = wxT( "C" );
const wxChar PAGE_INFO::D[]  = wxT( "D" );
const wxChar PAGE_INFO::E[]  = wxT( "E" );

const wxChar PAGE_INFO::GERBER[]   = wxT( "GERBER" );
const wxChar PAGE_INFO::USLetter[] = wxT( "USLetter" );
const wxChar PAGE_INFO::USLegal[]  = wxT( "USLegal" );
const wxChar PAGE_INFO::USLedger[] = wxT( "USLedger" );
const wxChar PAGE_INFO::Custom[]   = wxT( "User" );


// Standard page sizes in mils, all constants
// see:  https://lists.launchpad.net/kicad-developers/msg07389.html
// also see: wx/defs.h

// local readability macro for millimeter wxSize
#define MMsize( x, y )  wxSize( Mm2mils( x ), Mm2mils( y ) )

// All MUST be defined as landscape.
const PAGE_INFO  PAGE_INFO::pageA4(     MMsize( 297,   210 ),   wxT( "A4" ),    wxPAPER_A4 );
const PAGE_INFO  PAGE_INFO::pageA3(     MMsize( 420,   297 ),   wxT( "A3" ),    wxPAPER_A3 );
const PAGE_INFO  PAGE_INFO::pageA2(     MMsize( 594,   420 ),   wxT( "A2" ),    wxPAPER_A2 );
const PAGE_INFO  PAGE_INFO::pageA1(     MMsize( 841,   594 ),   wxT( "A1" ),    PAPER_A1 );
const PAGE_INFO  PAGE_INFO::pageA0(     MMsize( 1189,  841 ),   wxT( "A0" ),    PAPER_A0 );

const PAGE_INFO  PAGE_INFO::pageA(      wxSize( 11000,  8500 ), wxT( "A" ), wxPAPER_LETTER );
const PAGE_INFO  PAGE_INFO::pageB(      wxSize( 17000, 11000 ), wxT( "B" ), wxPAPER_TABLOID );
const PAGE_INFO  PAGE_INFO::pageC(      wxSize( 22000, 17000 ), wxT( "C" ), wxPAPER_CSHEET );
const PAGE_INFO  PAGE_INFO::pageD(      wxSize( 34000, 22000 ), wxT( "D" ), wxPAPER_DSHEET );
const PAGE_INFO  PAGE_INFO::pageE(      wxSize( 44000, 34000 ), wxT( "E" ), wxPAPER_ESHEET );

const PAGE_INFO  PAGE_INFO::pageGERBER( wxSize( 32000, 32000 ), wxT( "GERBER" ), wxPAPER_NONE  );
const PAGE_INFO  PAGE_INFO::pageUser(   wxSize( 17000, 11000 ), Custom,         wxPAPER_NONE );

// US paper sizes
const PAGE_INFO  PAGE_INFO::pageUSLetter( wxSize( 11000, 8500  ),  wxT( "USLetter" ), wxPAPER_LETTER );
const PAGE_INFO  PAGE_INFO::pageUSLegal(  wxSize( 14000, 8500  ),  wxT( "USLegal" ),  wxPAPER_LEGAL );
const PAGE_INFO  PAGE_INFO::pageUSLedger( wxSize( 17000, 11000 ),  wxT( "USLedger" ), wxPAPER_TABLOID );

// Custom paper size for next instantiation of type "User"
int PAGE_INFO::s_user_width  = 17000;
int PAGE_INFO::s_user_height = 11000;

/*
wxArrayString PAGE_INFO::GetStandardSizes()
{
    wxArrayString ret;

    static const PAGE_INFO* stdPageSizes[] = {
        &pageA4,
        &pageA3,
        &pageA2,
        &pageA1,
        &pageA0,
        &pageA,
        &pageB,
        &pageC,
        &pageD,
        &pageE,
        // &pageGERBER,     // standard?
        &pageUSLetter,
        &pageUSLegal,
        &pageUSLedger,
        &pageUser,
    };

    for( unsigned i=0;  i < DIM( stdPageSizes );  ++i )
        ret.Add( stdPageSizes[i]->GetType() );

    return ret;
}
*/


inline void PAGE_INFO::updatePortrait()
{
    // update m_portrait based on orientation of m_size.x and m_size.y
    m_portrait = ( m_size.y > m_size.x );
}


PAGE_INFO::PAGE_INFO( const wxSize& aSizeMils, const wxString& aType, wxPaperSize aPaperId ) :
    m_type( aType ), m_size( aSizeMils ), m_paper_id( aPaperId )
{
    updatePortrait();

    // This constructor is protected, and only used by const PAGE_INFO's known
    // only to class implementation, so no further changes to "this" object are
    // expected.
}


PAGE_INFO::PAGE_INFO( const wxString& aType, bool IsPortrait )
{
    SetType( aType, IsPortrait );
}


bool PAGE_INFO::SetType( const wxString& aType, bool IsPortrait )
{
    bool rc = true;

    // all are landscape initially
    if( aType == pageA4.GetType() )
        *this = pageA4;
    else if( aType == pageA3.GetType() )
        *this = pageA3;
    else if( aType == pageA2.GetType() )
        *this = pageA2;
    else if( aType == pageA1.GetType() )
        *this = pageA1;
    else if( aType == pageA0.GetType() )
        *this = pageA0;
    else if( aType == pageA.GetType() )
        *this = pageA;
    else if( aType == pageB.GetType() )
        *this = pageB;
    else if( aType == pageC.GetType() )
        *this = pageC;
    else if( aType == pageD.GetType() )
        *this = pageD;
    else if( aType == pageE.GetType() )
        *this = pageE;
    else if( aType == pageGERBER.GetType() )
        *this = pageGERBER;
    else if( aType == pageUSLetter.GetType() )
        *this = pageUSLetter;
    else if( aType == pageUSLegal.GetType() )
        *this = pageUSLegal;
    else if( aType == pageUSLedger.GetType() )
        *this = pageUSLedger;
    else if( aType == pageUser.GetType() )
    {
        // pageUser is const, and may not and does not hold the custom size,
        // so customize *this later
        *this  = pageUser;

        // customize:
        m_size.x = s_user_width;
        m_size.y = s_user_height;

        updatePortrait();
    }
    else
        rc = false;

    if( IsPortrait )
    {
        // all private PAGE_INFOs are landscape, must swap x and y
        m_size = wxSize( m_size.y, m_size.x );
        updatePortrait();
    }

    return rc;
}


bool PAGE_INFO::IsCustom() const
{
    return m_type == Custom;
}


void PAGE_INFO::SetPortrait( bool isPortrait )
{
    if( m_portrait != isPortrait )
    {
        // swap x and y in m_size
        m_size = wxSize( m_size.y, m_size.x );

        m_portrait = isPortrait;

        // margins are not touched, do that if you want
    }
}


static int clampWidth( int aWidthInMils )
{
/*  was giving EESCHEMA single component SVG plotter grief
    However a minimal test is made to avoid values that crashes Kicad
    if( aWidthInMils < 4000 )       // 4" is about a baseball card
        aWidthInMils = 4000;
    else if( aWidthInMils > 44000 ) //44" is plotter size
        aWidthInMils = 44000;
*/
    if( aWidthInMils < 10 )
        aWidthInMils = 10;
    return aWidthInMils;
}


static int clampHeight( int aHeightInMils )
{
/*  was giving EESCHEMA single component SVG plotter grief
    clamping is best done at the UI, i.e. dialog, levels
    However a minimal test is made to avoid values that crashes Kicad
    if( aHeightInMils < 4000 )
        aHeightInMils = 4000;
    else if( aHeightInMils > 44000 )
        aHeightInMils = 44000;
*/
    if( aHeightInMils < 10 )
        aHeightInMils = 10;
    return aHeightInMils;
}


void PAGE_INFO::SetCustomWidthMils( int aWidthInMils )
{
    s_user_width = clampWidth( aWidthInMils );
}


void PAGE_INFO::SetCustomHeightMils( int aHeightInMils )
{
    s_user_height = clampHeight( aHeightInMils );
}


void PAGE_INFO::SetWidthMils(  int aWidthInMils )
{
    if( m_size.x != aWidthInMils )
    {
        m_size.x = clampWidth( aWidthInMils );

        m_type = Custom;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::SetHeightMils( int aHeightInMils )
{
    if( m_size.y != aHeightInMils )
    {
        m_size.y = clampHeight( aHeightInMils );

        m_type = Custom;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(page %s", aFormatter->Quotew( GetType() ).c_str() );

    // The page dimensions are only required for user defined page sizes.
    // Internally, the page size is in mils
    if( GetType() == PAGE_INFO::Custom )
        aFormatter->Print( 0, " %g %g",
                           GetWidthMils() * 25.4 / 1000.0,
                           GetHeightMils() * 25.4 / 1000.0 );

    if( !IsCustom() && IsPortrait() )
        aFormatter->Print( 0, " portrait" );

    aFormatter->Print( 0, ")\n" );
}
