/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <page_info.h>
#include <macros.h>
#include <eda_units.h>
#include <richio.h>         // for OUTPUTFORMATTER and IO_ERROR
#include <string_utils.h>


// late arriving wxPAPER_A0, wxPAPER_A1
#if wxABI_VERSION >= 20999
 #define PAPER_A0   wxPAPER_A0
 #define PAPER_A1   wxPAPER_A1
#else
 #define PAPER_A0   wxPAPER_A2
 #define PAPER_A1   wxPAPER_A2
#endif


// Standard paper sizes nicknames.
const wxChar PAGE_INFO::A5[] = wxT( "A5" );
const wxChar PAGE_INFO::A4[] = wxT( "A4" );
const wxChar PAGE_INFO::A3[] = wxT( "A3" );
const wxChar PAGE_INFO::A2[] = wxT( "A2" );
const wxChar PAGE_INFO::A1[] = wxT( "A1" );
const wxChar PAGE_INFO::A0[] = wxT( "A0" );
const wxChar PAGE_INFO::A[]  = wxT( "A" );
const wxChar PAGE_INFO::B[]  = wxT( "B" );
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
#define MMsize( x, y ) VECTOR2D( EDA_UNIT_UTILS::Mm2mils( x ), EDA_UNIT_UTILS::Mm2mils( y ) )

// All MUST be defined as landscape.
const PAGE_INFO  PAGE_INFO::pageA5(     MMsize( 210,   148 ),   wxT( "A5" ),    wxPAPER_A5 );
const PAGE_INFO  PAGE_INFO::pageA4(     MMsize( 297,   210 ),   wxT( "A4" ),    wxPAPER_A4 );
const PAGE_INFO  PAGE_INFO::pageA3(     MMsize( 420,   297 ),   wxT( "A3" ),    wxPAPER_A3 );
const PAGE_INFO  PAGE_INFO::pageA2(     MMsize( 594,   420 ),   wxT( "A2" ),    wxPAPER_A2 );
const PAGE_INFO  PAGE_INFO::pageA1(     MMsize( 841,   594 ),   wxT( "A1" ),    PAPER_A1 );
const PAGE_INFO  PAGE_INFO::pageA0(     MMsize( 1189,  841 ),   wxT( "A0" ),    PAPER_A0 );

const PAGE_INFO  PAGE_INFO::pageA( VECTOR2D( 11000, 8500 ), wxT( "A" ), wxPAPER_LETTER );
const PAGE_INFO  PAGE_INFO::pageB( VECTOR2D( 17000, 11000 ), wxT( "B" ), wxPAPER_TABLOID );
const PAGE_INFO  PAGE_INFO::pageC( VECTOR2D( 22000, 17000 ), wxT( "C" ), wxPAPER_CSHEET );
const PAGE_INFO  PAGE_INFO::pageD( VECTOR2D( 34000, 22000 ), wxT( "D" ), wxPAPER_DSHEET );
const PAGE_INFO  PAGE_INFO::pageE( VECTOR2D( 44000, 34000 ), wxT( "E" ), wxPAPER_ESHEET );

const PAGE_INFO PAGE_INFO::pageGERBER( VECTOR2D( 32000, 32000 ), wxT( "GERBER" ), wxPAPER_NONE );
const PAGE_INFO PAGE_INFO::pageUser( VECTOR2D( 17000, 11000 ), Custom, wxPAPER_NONE );

// US paper sizes
const PAGE_INFO  PAGE_INFO::pageUSLetter( VECTOR2D( 11000, 8500  ),  wxT( "USLetter" ),
                                          wxPAPER_LETTER );
const PAGE_INFO PAGE_INFO::pageUSLegal( VECTOR2D( 14000, 8500 ), wxT( "USLegal" ), wxPAPER_LEGAL );
const PAGE_INFO  PAGE_INFO::pageUSLedger( VECTOR2D( 17000, 11000 ), wxT( "USLedger" ),
                                          wxPAPER_TABLOID );

// Custom paper size for next instantiation of type "User"
double PAGE_INFO::s_user_width  = 17000;
double PAGE_INFO::s_user_height = 11000;


inline void PAGE_INFO::updatePortrait()
{
    // update m_portrait based on orientation of m_size.x and m_size.y
    m_portrait = ( m_size.y > m_size.x );
}


PAGE_INFO::PAGE_INFO( const VECTOR2D& aSizeMils, const wxString& aType, wxPaperSize aPaperId ) :
    m_type( aType ), m_size( aSizeMils ), m_paper_id( aPaperId )
{
    updatePortrait();

    // This constructor is protected, and only used by const PAGE_INFO's known
    // only to class implementation, so no further changes to "this" object are
    // expected.
}


PAGE_INFO::PAGE_INFO( const wxString& aType, bool aIsPortrait )
{
    SetType( aType, aIsPortrait );
}


bool PAGE_INFO::SetType( const wxString& aType, bool aIsPortrait )
{
    bool rc = true;

    // all are landscape initially
    if( aType == pageA5.GetType() )
        *this = pageA5;
    else if( aType == pageA4.GetType() )
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

    if( aIsPortrait )
    {
        // all private PAGE_INFOs are landscape, must swap x and y
        std::swap( m_size.y, m_size.x );
        updatePortrait();
    }

    return rc;
}


bool PAGE_INFO::IsCustom() const
{
    return m_type == Custom;
}


void PAGE_INFO::SetPortrait( bool aIsPortrait )
{
    if( m_portrait != aIsPortrait )
    {
        // swap x and y in m_size
        std::swap( m_size.y, m_size.x );

        m_portrait = aIsPortrait;

        // margins are not touched, do that if you want
    }
}


static double clampWidth( double aWidthInMils )
{
/*  was giving EESCHEMA single component SVG plotter grief
    However a minimal test is made to avoid values that crashes KiCad
    if( aWidthInMils < 4000 )       // 4" is about a baseball card
        aWidthInMils = 4000;
    else if( aWidthInMils > 44000 ) //44" is plotter size
        aWidthInMils = 44000;
*/
    if( aWidthInMils < 10 )
        aWidthInMils = 10;

    return aWidthInMils;
}


static double clampHeight( double aHeightInMils )
{
/*  was giving EESCHEMA single component SVG plotter grief
    clamping is best done at the UI, i.e. dialog, levels
    However a minimal test is made to avoid values that crashes KiCad
    if( aHeightInMils < 4000 )
        aHeightInMils = 4000;
    else if( aHeightInMils > 44000 )
        aHeightInMils = 44000;
*/
    if( aHeightInMils < 10.0 )
        aHeightInMils = 10.0;

    return aHeightInMils;
}


void PAGE_INFO::SetCustomWidthMils( double aWidthInMils )
{
    s_user_width = clampWidth( aWidthInMils );
}


void PAGE_INFO::SetCustomHeightMils( double aHeightInMils )
{
    s_user_height = clampHeight( aHeightInMils );
}


void PAGE_INFO::SetWidthMils( double aWidthInMils )
{
    if( m_size.x != aWidthInMils )
    {
        m_size.x = clampWidth( aWidthInMils );

        m_type = Custom;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::SetHeightMils( double aHeightInMils )
{
    if( m_size.y != aHeightInMils )
    {
        m_size.y = clampHeight( aHeightInMils );

        m_type = Custom;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::Format( OUTPUTFORMATTER* aFormatter ) const
{
    aFormatter->Print( "(paper %s", aFormatter->Quotew( GetType() ).c_str() );

    // The page dimensions are only required for user defined page sizes.
    // Internally, the page size is in mils
    if( GetType() == PAGE_INFO::Custom )
    {
        aFormatter->Print( " %s %s",
                           FormatDouble2Str( GetWidthMils() * 25.4 / 1000.0 ).c_str(),
                           FormatDouble2Str( GetHeightMils() * 25.4 / 1000.0 ).c_str() );
    }

    if( !IsCustom() && IsPortrait() )
        aFormatter->Print( " portrait" );

    aFormatter->Print( ")" );
}
