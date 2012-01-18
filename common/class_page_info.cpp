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


const wxString PAGE_INFO::Custom( wxT( "User" ) );

// Standard page sizes in mils, all constants

// A4 see:  https://lists.launchpad.net/kicad-developers/msg07389.html
#if defined(KICAD_GOST)
const PAGE_INFO  PAGE_INFO::pageA4(     wxSize(  8268, 11693 ),  wxT( "A4" ) );
#else
const PAGE_INFO  PAGE_INFO::pageA4(     wxSize( 11693,  8268 ),  wxT( "A4" ) );
#endif

const PAGE_INFO  PAGE_INFO::pageA3(     wxSize( 16535, 11700 ),  wxT( "A3" ) );
const PAGE_INFO  PAGE_INFO::pageA2(     wxSize( 23400, 16535 ),  wxT( "A2" ) );
const PAGE_INFO  PAGE_INFO::pageA1(     wxSize( 33070, 23400 ),  wxT( "A1" ) );
const PAGE_INFO  PAGE_INFO::pageA0(     wxSize( 46800, 33070 ),  wxT( "A0" ) );
const PAGE_INFO  PAGE_INFO::pageA(      wxSize( 11000,  8500 ),  wxT( "A" ) );
const PAGE_INFO  PAGE_INFO::pageB(      wxSize( 17000, 11000 ),  wxT( "B" ) );
const PAGE_INFO  PAGE_INFO::pageC(      wxSize( 22000, 17000 ),  wxT( "C" ) );
const PAGE_INFO  PAGE_INFO::pageD(      wxSize( 34000, 22000 ),  wxT( "D" ) );
const PAGE_INFO  PAGE_INFO::pageE(      wxSize( 44000, 34000 ),  wxT( "E" ) );
const PAGE_INFO  PAGE_INFO::pageGERBER( wxSize( 32000, 32000 ),  wxT( "GERBER" ) );
const PAGE_INFO  PAGE_INFO::pageUser(   wxSize( 17000, 11000 ),  Custom );

// US paper sizes
const PAGE_INFO  PAGE_INFO::pageUSLetter( wxSize( 11000, 8500  ),  wxT( "USLetter" ) );
const PAGE_INFO  PAGE_INFO::pageUSLegal(  wxSize( 14000, 8500  ),  wxT( "USLegal" ) );
const PAGE_INFO  PAGE_INFO::pageUSLedger( wxSize( 17000, 11000 ),  wxT( "USLedger" ) );

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



PAGE_INFO::PAGE_INFO( const wxSize& aSizeMils, const wxString& aType ) :
    m_type( aType ),
    m_size( aSizeMils )
{

#if defined(KICAD_GOST)
/*
#define GOST_LEFTMARGIN   800    // 20mm
#define GOST_RIGHTMARGIN  200    // 5mm
#define GOST_TOPMARGIN    200    // 5mm
#define GOST_BOTTOMMARGIN 200    // 5mm
*/

    m_left_margin   = 800;      // 20mm
    m_right_margin  = 200;      // 5mm
    m_top_margin    = 200;      // 5mm
    m_bottom_margin = 200;      // 5mm
#else
    m_left_margin   =
    m_right_margin  =
    m_top_margin    =
    m_bottom_margin = 400;
#endif

    updatePortrait();
}


PAGE_INFO::PAGE_INFO( const wxString& aType )
{
    SetType( aType );
}


bool PAGE_INFO::SetType( const wxString& aType )
{
    bool rc = true;

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
    if( aWidthInMils < 4000 )       // 4" is about a baseball card
        aWidthInMils = 4000;
    else if( aWidthInMils > 44000 ) //44" is plotter size
        aWidthInMils = 44000;
    return aWidthInMils;
}


static int clampHeight( int aHeightInMils )
{
    if( aHeightInMils < 4000 )
        aHeightInMils = 4000;
    else if( aHeightInMils > 44000 )
        aHeightInMils = 44000;
    return aHeightInMils;
}


void PAGE_INFO::SetUserWidthMils( int aWidthInMils )
{
    s_user_width = clampWidth( aWidthInMils );
}


void PAGE_INFO::SetUserHeightMils( int aHeightInMils )
{
    s_user_height = clampHeight( aHeightInMils );
}


void PAGE_INFO::SetWidthMils(  int aWidthInMils )
{
    m_size.x = clampWidth( aWidthInMils );
    updatePortrait();
}


void PAGE_INFO::SetHeightMils( int aHeightInMils )
{
    m_size.y = clampHeight( aHeightInMils );
    updatePortrait();
}

