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
#include <magic_enum.hpp>


// Standard page sizes in mils, all constants
// see:  https://lists.launchpad.net/kicad-developers/msg07389.html
// also see: wx/defs.h

// local readability macro for millimeter wxSize
#define MMsize( x, y ) VECTOR2D( EDA_UNIT_UTILS::Mm2mils( x ), EDA_UNIT_UTILS::Mm2mils( y ) )

// List of page formats.
// they are prefixed by "_HKI" (already in use for hotkeys) instead of "_",
// because we need both the translated and the not translated version.
// when displayed in dialog we should explicitly call wxGetTranslation()
#define _HKI( x ) wxT( x )

std::vector<PAGE_INFO> PAGE_INFO::standardPageSizes = {
    // All MUST be defined as landscape.
    PAGE_INFO( MMsize( 210, 148 ), PAGE_SIZE_TYPE::A5, wxPAPER_A5, _HKI( "A5 148 x 210mm" ) ),
    PAGE_INFO( MMsize( 297, 210 ), PAGE_SIZE_TYPE::A4, wxPAPER_A4, _HKI( "A4 210 x 297mm" ) ),
    PAGE_INFO( MMsize( 420, 297 ), PAGE_SIZE_TYPE::A3, wxPAPER_A3, _HKI( "A3 297 x 420mm" ) ),
    PAGE_INFO( MMsize( 594, 420 ), PAGE_SIZE_TYPE::A2, wxPAPER_A2, _HKI( "A2 420 x 594mm" ) ),
    PAGE_INFO( MMsize( 841, 594 ), PAGE_SIZE_TYPE::A1, wxPAPER_A1, _HKI( "A1 594 x 841mm" ) ),
    PAGE_INFO( MMsize( 1189, 841 ), PAGE_SIZE_TYPE::A0, wxPAPER_A0, _HKI( "A0 841 x 1189mm" ) ),
    PAGE_INFO( VECTOR2D( 11000, 8500 ), PAGE_SIZE_TYPE::A, wxPAPER_LETTER, _HKI( "A 8.5 x 11in" ) ),
    PAGE_INFO( VECTOR2D( 17000, 11000 ), PAGE_SIZE_TYPE::B, wxPAPER_TABLOID, _HKI( "B 11 x 17in" ) ),
    PAGE_INFO( VECTOR2D( 22000, 17000 ), PAGE_SIZE_TYPE::C, wxPAPER_CSHEET, _HKI( "C 17 x 22in" ) ),
    PAGE_INFO( VECTOR2D( 34000, 22000 ), PAGE_SIZE_TYPE::D, wxPAPER_DSHEET, _HKI( "D 22 x 34in" ) ),
    PAGE_INFO( VECTOR2D( 44000, 34000 ), PAGE_SIZE_TYPE::E, wxPAPER_ESHEET, _HKI( "E 34 x 44in" ) ),

    // US paper sizes
    PAGE_INFO( VECTOR2D( 32000, 32000 ), PAGE_SIZE_TYPE::GERBER, wxPAPER_NONE ),
    PAGE_INFO( VECTOR2D( 17000, 11000 ), PAGE_SIZE_TYPE::User, wxPAPER_NONE, _HKI( "User (Custom)" ) ),

    PAGE_INFO( VECTOR2D( 11000, 8500 ), PAGE_SIZE_TYPE::USLetter, wxPAPER_LETTER, _HKI("US Letter 8.5 x 11in") ),
    PAGE_INFO( VECTOR2D( 14000, 8500 ), PAGE_SIZE_TYPE::USLegal, wxPAPER_LEGAL, _HKI("US Legal 8.5 x 14in") ),
    PAGE_INFO( VECTOR2D( 17000, 11000 ), PAGE_SIZE_TYPE::USLedger, wxPAPER_TABLOID, _HKI("US Ledger 11 x 17in") )
};

// Custom paper size for next instantiation of type "User"
double PAGE_INFO::s_user_width  = 17000;
double PAGE_INFO::s_user_height = 11000;


inline void PAGE_INFO::updatePortrait()
{
    // update m_portrait based on orientation of m_size.x and m_size.y
    m_portrait = ( m_size.y > m_size.x );
}


PAGE_INFO::PAGE_INFO( const VECTOR2D& aSizeMils, const PAGE_SIZE_TYPE& aType, wxPaperSize aPaperId,
                      const wxString& aDescription ) :
        m_type( aType ),
        m_size( aSizeMils ),
        m_paper_id( aPaperId ),
        m_description( aDescription )
{
    updatePortrait();

    // This constructor is protected, and only used by const PAGE_INFO's known
    // only to class implementation, so no further changes to "this" object are
    // expected.
}


PAGE_INFO::PAGE_INFO( PAGE_SIZE_TYPE aType, bool aIsPortrait ) :
        m_type( PAGE_SIZE_TYPE::A4 ),
        m_size( s_user_width, s_user_height ),
        m_portrait( false ),
        m_paper_id( wxPAPER_NONE )
{
    SetType( aType, aIsPortrait );
}


bool PAGE_INFO::SetType( const wxString& aPageSize, bool aIsPortrait )
{
    auto type = magic_enum::enum_cast<PAGE_SIZE_TYPE>( aPageSize.ToStdString(), magic_enum::case_insensitive );

    if( !type.has_value() )
        return false;

    return SetType( type.value(), aIsPortrait );
}


bool PAGE_INFO::SetType( PAGE_SIZE_TYPE aType, bool aIsPortrait )
{
    bool rc = true;

    auto result = std::find_if( standardPageSizes.begin(), standardPageSizes.end(),
                  [aType]( const PAGE_INFO& p )
                  {
                      return p.m_type == aType;
                  } );

    if( result != standardPageSizes.end() )
        *this = *result;
    else
        rc = false;

    if( aType == PAGE_SIZE_TYPE::User )
    {
        m_type = PAGE_SIZE_TYPE::User;
        m_paper_id = wxPAPER_NONE;
        m_size.x = s_user_width;
        m_size.y = s_user_height;

        updatePortrait();
    }

    if( aIsPortrait )
    {
        // all private PAGE_INFOs are landscape, must swap x and y
        std::swap( m_size.y, m_size.x );
        updatePortrait();
    }

    return rc;
}


wxString PAGE_INFO::GetTypeAsString() const
{
    std::string typeStr( magic_enum::enum_name( m_type ) );
    return wxString( typeStr );
}


bool PAGE_INFO::IsCustom() const
{
    return m_type == PAGE_SIZE_TYPE::User;
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
    if( aWidthInMils < 10 )
        aWidthInMils = 10;

    return aWidthInMils;
}


static double clampHeight( double aHeightInMils )
{
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

        m_type = PAGE_SIZE_TYPE::User;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::SetHeightMils( double aHeightInMils )
{
    if( m_size.y != aHeightInMils )
    {
        m_size.y = clampHeight( aHeightInMils );

        m_type = PAGE_SIZE_TYPE::User;
        m_paper_id = wxPAPER_NONE;

        updatePortrait();
    }
}


void PAGE_INFO::Format( OUTPUTFORMATTER* aFormatter ) const
{
    std::string typeStr( magic_enum::enum_name( GetType() ) );
    aFormatter->Print( "(paper %s", aFormatter->Quotew( typeStr ).c_str() );

    // The page dimensions are only required for user defined page sizes.
    // Internally, the page size is in mils
    if( GetType() == PAGE_SIZE_TYPE::User )
    {
        aFormatter->Print( " %s %s",
                           FormatDouble2Str( GetWidthMils() * 25.4 / 1000.0 ).c_str(),
                           FormatDouble2Str( GetHeightMils() * 25.4 / 1000.0 ).c_str() );
    }

    if( !IsCustom() && IsPortrait() )
        aFormatter->Print( " portrait" );

    aFormatter->Print( ")" );
}


const std::vector<PAGE_INFO>& PAGE_INFO::GetPageFormatsList()
{
    return PAGE_INFO::standardPageSizes;
}