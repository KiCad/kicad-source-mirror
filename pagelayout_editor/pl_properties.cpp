/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pl_properties.h"

#include <font/font.h>
#include <font/fontconfig.h>
#include <font/kicad_font_name.h>
#include <i18n_utility.h>
#include <pgm_base.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <properties/property_validators.h>

#include <cmath>
#include <climits>

// EDA_TEXT owns the alignment conversions.  Declare its specializations here instead of
// instantiating wxAny's generic enum storage a second time in this module.
DECLARE_ENUM_TO_WXANY( GR_TEXT_H_ALIGN_T )
DECLARE_ENUM_TO_WXANY( GR_TEXT_V_ALIGN_T )
ENUM_TO_WXANY( CORNER_ANCHOR )
ENUM_TO_WXANY( PAGE_OPTION )


double PL_ITEM_PROPERTIES::GetRotation() const
{
    if( m_item->GetType() == DS_DATA_ITEM::DS_TEXT )
        return text()->m_Orient;

    return static_cast<DS_DATA_ITEM_POLYGONS*>( m_item )->m_Orient.AsDegrees();
}


void PL_ITEM_PROPERTIES::SetRotation( double aValue )
{
    if( m_item->GetType() == DS_DATA_ITEM::DS_TEXT )
        text()->m_Orient = aValue;
    else
        static_cast<DS_DATA_ITEM_POLYGONS*>( m_item )->m_Orient = EDA_ANGLE( aValue, DEGREES_T );
}


wxString PL_ITEM_PROPERTIES::GetFont() const
{
    return text()->m_Font ? text()->m_Font->GetName() : wxString( KICAD_FONT_NAME );
}


void PL_ITEM_PROPERTIES::SetFont( wxString aValue )
{
    text()->m_Font = aValue == KICAD_FONT_NAME ? nullptr : KIFONT::FONT::GetFont( aValue, GetBold(), GetItalic() );
}


void PL_ITEM_PROPERTIES::SetBold( bool aValue )
{
    text()->m_Bold = aValue;
    SetFont( GetFont() );
}


void PL_ITEM_PROPERTIES::SetItalic( bool aValue )
{
    text()->m_Italic = aValue;
    SetFont( GetFont() );
}


// wxPropertyGrid supplies integer values as longs; accept conversion before checking the range.
static PROPERTY_VALIDATOR_FN rangeValidator( int aMin, int aMax, bool aDistance = true )
{
    return
            [=]( const wxAny&& aValue, EDA_ITEM* ) -> VALIDATOR_RESULT
            {
                int value;

                if( !aValue.GetAs( &value ) )
                    return std::make_unique<VALIDATION_ERROR_MSG>( _( "Enter an integer value." ) );

                EDA_DATA_TYPE type = aDistance ? EDA_DATA_TYPE::DISTANCE : EDA_DATA_TYPE::UNITLESS;

                if( value < aMin )
                    return std::make_unique<VALIDATION_ERROR_TOO_SMALL<int>>( value, aMin, type );

                if( value > aMax )
                    return std::make_unique<VALIDATION_ERROR_TOO_LARGE<int>>( value, aMax, type );

                return std::nullopt;
            };
}


static struct PL_PROPERTIES_DESC
{
    PL_PROPERTIES_DESC()
    {
        PROPERTY_MANAGER& mgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PL_ITEM_PROPERTIES );
        REGISTER_TYPE( PL_PAGE_PROPERTIES );

        ENUM_MAP<CORNER_ANCHOR>& anchors = ENUM_MAP<CORNER_ANCHOR>::Instance();
        anchors.Map( LT_CORNER, _HKI( "Top left" ) )
                .Map( RT_CORNER, _HKI( "Top right" ) )
                .Map( LB_CORNER, _HKI( "Bottom left" ) )
                .Map( RB_CORNER, _HKI( "Bottom right" ) );

        ENUM_MAP<PAGE_OPTION>& pages = ENUM_MAP<PAGE_OPTION>::Instance();
        pages.Map( ALL_PAGES, _HKI( "All pages" ) )
                .Map( FIRST_PAGE_ONLY, _HKI( "First page only" ) )
                .Map( SUBSEQUENT_PAGES, _HKI( "Subsequent pages" ) );

        // EDA_TEXT also registers these enums; initialization order is unspecified.
        auto& hAlign = ENUM_MAP<GR_TEXT_H_ALIGN_T>::Instance();

        if( hAlign.Choices().GetCount() == 0 )
        {
            hAlign.Map( GR_TEXT_H_ALIGN_LEFT, _HKI( "Left" ) )
                    .Map( GR_TEXT_H_ALIGN_CENTER, _HKI( "Center" ) )
                    .Map( GR_TEXT_H_ALIGN_RIGHT, _HKI( "Right" ) );
        }

        auto& vAlign = ENUM_MAP<GR_TEXT_V_ALIGN_T>::Instance();

        if( vAlign.Choices().GetCount() == 0 )
        {
            vAlign.Map( GR_TEXT_V_ALIGN_TOP, _HKI( "Top" ) )
                    .Map( GR_TEXT_V_ALIGN_CENTER, _HKI( "Center" ) )
                    .Map( GR_TEXT_V_ALIGN_BOTTOM, _HKI( "Bottom" ) );
        }

        auto isText =
                []( INSPECTABLE* aItem )
                {
                    return static_cast<PL_ITEM_PROPERTIES*>( aItem )->GetItem()->GetType() == DS_DATA_ITEM::DS_TEXT;
                };

        auto hasEnd =
                []( INSPECTABLE* aItem )
                {
                    auto type = static_cast<PL_ITEM_PROPERTIES*>( aItem )->GetItem()->GetType();
                    return type == DS_DATA_ITEM::DS_SEGMENT || type == DS_DATA_ITEM::DS_RECT;
                };

        auto hasRotation =
                []( INSPECTABLE* aItem )
                {
                    auto type = static_cast<PL_ITEM_PROPERTIES*>( aItem )->GetItem()->GetType();
                    return type == DS_DATA_ITEM::DS_TEXT || type == DS_DATA_ITEM::DS_POLYPOLYGON;
                };

        auto isBitmap =
                []( INSPECTABLE* aItem )
                {
                    return static_cast<PL_ITEM_PROPERTIES*>( aItem )->GetItem()->GetType() == DS_DATA_ITEM::DS_BITMAP;
                };

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, wxString>( _HKI( "Comment" ), &PL_ITEM_PROPERTIES::SetComment,
                                                                     &PL_ITEM_PROPERTIES::GetComment ) );

        mgr.AddProperty( new PROPERTY_ENUM<PL_ITEM_PROPERTIES, PAGE_OPTION>(
                _HKI( "Page Option" ), &PL_ITEM_PROPERTIES::SetPageOption, &PL_ITEM_PROPERTIES::GetPageOption ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Position X" ), &PL_ITEM_PROPERTIES::SetStartX,
                                                                &PL_ITEM_PROPERTIES::GetStartX, PT_SIZE ),
                         _HKI( "Position" ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Position Y" ), &PL_ITEM_PROPERTIES::SetStartY,
                                                                &PL_ITEM_PROPERTIES::GetStartY, PT_SIZE ),
                         _HKI( "Position" ) );

        mgr.AddProperty( new PROPERTY_ENUM<PL_ITEM_PROPERTIES, CORNER_ANCHOR>( _HKI( "Anchor" ),
                                                                               &PL_ITEM_PROPERTIES::SetStartAnchor,
                                                                               &PL_ITEM_PROPERTIES::GetStartAnchor ),
                         _HKI( "Position" ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "End X" ), &PL_ITEM_PROPERTIES::SetEndX,
                                                                &PL_ITEM_PROPERTIES::GetEndX, PT_SIZE ),
                         _HKI( "End Position" ) )
                .SetAvailableFunc( hasEnd );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "End Y" ), &PL_ITEM_PROPERTIES::SetEndY,
                                                                &PL_ITEM_PROPERTIES::GetEndY, PT_SIZE ),
                         _HKI( "End Position" ) )
                .SetAvailableFunc( hasEnd );

        mgr.AddProperty( new PROPERTY_ENUM<PL_ITEM_PROPERTIES, CORNER_ANCHOR>( _HKI( "End Anchor" ),
                                                                               &PL_ITEM_PROPERTIES::SetEndAnchor,
                                                                               &PL_ITEM_PROPERTIES::GetEndAnchor ),
                         _HKI( "End Position" ) )
                .SetAvailableFunc( hasEnd );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Line Width" ), &PL_ITEM_PROPERTIES::SetLineWidth,
                                                                &PL_ITEM_PROPERTIES::GetLineWidth, PT_SIZE ) )
                .SetAvailableFunc(
                        [isBitmap]( INSPECTABLE* aItem )
                        {
                            return !isBitmap( aItem );
                        } )
                .SetValidator( rangeValidator( 0, drawSheetIUScale.mmToIU( 10 ), true ) )
                .SetHelpString( _HKI( "Set to 0 to use the drawing sheet default." ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, double>( _HKI( "Rotation" ), &PL_ITEM_PROPERTIES::SetRotation,
                                                                   &PL_ITEM_PROPERTIES::GetRotation, PT_DEGREE ) )
                .SetAvailableFunc( hasRotation )
                .SetValidator(
                        []( const wxAny&& aValue, EDA_ITEM* ) -> VALIDATOR_RESULT
                        {
                            double value;

                            if( !aValue.GetAs( &value ) || !std::isfinite( value ) )
                                return std::make_unique<VALIDATION_ERROR_MSG>( _( "Enter a finite angle." ) );

                            return std::nullopt;
                        } );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Image PPI" ), &PL_ITEM_PROPERTIES::SetPPI,
                                                                &PL_ITEM_PROPERTIES::GetPPI ) )
                .SetAvailableFunc( isBitmap )
                .SetValidator( rangeValidator( 1, INT_MAX, false ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, wxString>( _HKI( "Text" ), &PL_ITEM_PROPERTIES::SetText,
                                                                     &PL_ITEM_PROPERTIES::GetText ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetHelpString(
                        _HKI( "Text supports ${keyword} variables. Click the button to edit multiple lines." ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, wxString>( _HKI( "Font" ), &PL_ITEM_PROPERTIES::SetFont,
                                                                     &PL_ITEM_PROPERTIES::GetFont ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetChoicesFunc(
                        []( INSPECTABLE* )
                        {
                            wxPGChoices              fonts;
                            std::vector<std::string> names;
                            Fontconfig()->ListFonts( names, std::string( Pgm().GetLanguageTag().utf8_str() ) );
                            fonts.Add( KICAD_FONT_NAME );

                            for( const std::string& name : names )
                                fonts.Add( wxString( name ) );

                            return fonts;
                        } );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Text Width" ), &PL_ITEM_PROPERTIES::SetTextWidth,
                                                                &PL_ITEM_PROPERTIES::GetTextWidth, PT_SIZE ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetValidator( rangeValidator( 0, drawSheetIUScale.mmToIU( 100 ), true ) )
                .SetHelpString( _HKI( "Set to 0 to use the drawing sheet default." ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Text Height" ),
                                                                &PL_ITEM_PROPERTIES::SetTextHeight,
                                                                &PL_ITEM_PROPERTIES::GetTextHeight, PT_SIZE ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetValidator( rangeValidator( 0, drawSheetIUScale.mmToIU( 100 ), true ) )
                .SetHelpString( _HKI( "Set to 0 to use the drawing sheet default." ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, bool>( _HKI( "Bold" ), &PL_ITEM_PROPERTIES::SetBold,
                                                                 &PL_ITEM_PROPERTIES::GetBold ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, bool>( _HKI( "Italic" ), &PL_ITEM_PROPERTIES::SetItalic,
                                                                 &PL_ITEM_PROPERTIES::GetItalic ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY_ENUM<PL_ITEM_PROPERTIES, GR_TEXT_H_ALIGN_T>( _HKI( "Horizontal Alignment" ),
                                                                                   &PL_ITEM_PROPERTIES::SetHAlign,
                                                                                   &PL_ITEM_PROPERTIES::GetHAlign ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY_ENUM<PL_ITEM_PROPERTIES, GR_TEXT_V_ALIGN_T>( _HKI( "Vertical Alignment" ),
                                                                                   &PL_ITEM_PROPERTIES::SetVAlign,
                                                                                   &PL_ITEM_PROPERTIES::GetVAlign ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, KIGFX::COLOR4D>( _HKI( "Text Color" ),
                                                                           &PL_ITEM_PROPERTIES::SetTextColor,
                                                                           &PL_ITEM_PROPERTIES::GetTextColor ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Maximum Text Width" ),
                                                                &PL_ITEM_PROPERTIES::SetConstraintWidth,
                                                                &PL_ITEM_PROPERTIES::GetConstraintWidth, PT_SIZE ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetValidator( rangeValidator( 0, INT_MAX, true ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Maximum Text Height" ),
                                                                &PL_ITEM_PROPERTIES::SetConstraintHeight,
                                                                &PL_ITEM_PROPERTIES::GetConstraintHeight, PT_SIZE ),
                         _HKI( "Text Properties" ) )
                .SetAvailableFunc( isText )
                .SetValidator( rangeValidator( 0, INT_MAX, true ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Repeat Count" ),
                                                                &PL_ITEM_PROPERTIES::SetRepeatCount,
                                                                &PL_ITEM_PROPERTIES::GetRepeatCount ),
                         _HKI( "Repeat" ) )
                .SetValidator( rangeValidator( 1, DS_MAX_REPEAT_COUNT, false ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Step X" ), &PL_ITEM_PROPERTIES::SetStepX,
                                                                &PL_ITEM_PROPERTIES::GetStepX, PT_SIZE ),
                         _HKI( "Repeat" ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Step Y" ), &PL_ITEM_PROPERTIES::SetStepY,
                                                                &PL_ITEM_PROPERTIES::GetStepY, PT_SIZE ),
                         _HKI( "Repeat" ) );

        mgr.AddProperty( new PROPERTY<PL_ITEM_PROPERTIES, int>( _HKI( "Label Increment" ),
                                                                &PL_ITEM_PROPERTIES::SetLabelIncrement,
                                                                &PL_ITEM_PROPERTIES::GetLabelIncrement ),
                         _HKI( "Repeat" ) )
                .SetAvailableFunc( isText );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Default Text Width" ),
                                                                &PL_PAGE_PROPERTIES::SetDefaultTextWidth,
                                                                &PL_PAGE_PROPERTIES::GetDefaultTextWidth, PT_SIZE ),
                         _HKI( "Defaults" ) )
                .SetValidator(
                        rangeValidator( drawSheetIUScale.mmToIU( 0.01 ), drawSheetIUScale.mmToIU( 100 ), true ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Default Text Height" ),
                                                                &PL_PAGE_PROPERTIES::SetDefaultTextHeight,
                                                                &PL_PAGE_PROPERTIES::GetDefaultTextHeight, PT_SIZE ),
                         _HKI( "Defaults" ) )
                .SetValidator(
                        rangeValidator( drawSheetIUScale.mmToIU( 0.01 ), drawSheetIUScale.mmToIU( 100 ), true ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Default Line Width" ),
                                                                &PL_PAGE_PROPERTIES::SetDefaultLineWidth,
                                                                &PL_PAGE_PROPERTIES::GetDefaultLineWidth, PT_SIZE ),
                         _HKI( "Defaults" ) )
                .SetValidator( rangeValidator( 0, drawSheetIUScale.mmToIU( 10 ), true ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Default Text Thickness" ),
                                                                &PL_PAGE_PROPERTIES::SetDefaultTextThickness,
                                                                &PL_PAGE_PROPERTIES::GetDefaultTextThickness, PT_SIZE ),
                         _HKI( "Defaults" ) )
                .SetValidator( rangeValidator( 0, drawSheetIUScale.mmToIU( 5 ), true ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Left Margin" ),
                                                                &PL_PAGE_PROPERTIES::SetLeftMargin,
                                                                &PL_PAGE_PROPERTIES::GetLeftMargin, PT_SIZE ),
                         _HKI( "Margins" ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Right Margin" ),
                                                                &PL_PAGE_PROPERTIES::SetRightMargin,
                                                                &PL_PAGE_PROPERTIES::GetRightMargin, PT_SIZE ),
                         _HKI( "Margins" ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Top Margin" ), &PL_PAGE_PROPERTIES::SetTopMargin,
                                                                &PL_PAGE_PROPERTIES::GetTopMargin, PT_SIZE ),
                         _HKI( "Margins" ) );

        mgr.AddProperty( new PROPERTY<PL_PAGE_PROPERTIES, int>( _HKI( "Bottom Margin" ),
                                                                &PL_PAGE_PROPERTIES::SetBottomMargin,
                                                                &PL_PAGE_PROPERTIES::GetBottomMargin, PT_SIZE ),
                         _HKI( "Margins" ) );
    }
} s_propertiesDesc;
