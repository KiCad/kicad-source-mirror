/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <widgets/msgpanel.h>
#include <string_utils.h>
#include <schematic.h>
#include <sch_table.h>
#include <sch_tablecell.h>


SCH_TABLECELL::SCH_TABLECELL( int aLineWidth, FILL_T aFillType ) :
        SCH_TEXTBOX( aLineWidth, aFillType, wxEmptyString, SCH_TABLECELL_T ),
        m_colSpan( 1 ),
        m_rowSpan( 1 )
{
}


void SCH_TABLECELL::SwapData( SCH_ITEM* aItem )
{
    SCH_TEXTBOX::SwapData( aItem );

    SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( aItem );

    std::swap( m_colSpan, cell->m_colSpan );
    std::swap( m_rowSpan, cell->m_rowSpan );
}


wxString SCH_TABLECELL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Table Cell %s" ), GetAddr() );
}


int SCH_TABLECELL::GetRow() const
{
    const SCH_TABLE* table = static_cast<const SCH_TABLE*>( GetParent() );

    for( int row = 0; row < table->GetRowCount(); ++row )
    {
        for( int col = 0; col < table->GetColCount(); ++col )
        {
            if( table->GetCell( row, col ) == this )
                return row;
        }
    }

    return -1;
}


int SCH_TABLECELL::GetColumn() const
{
    const SCH_TABLE* table = static_cast<const SCH_TABLE*>( GetParent() );

    for( int row = 0; row < table->GetRowCount(); ++row )
    {
        for( int col = 0; col < table->GetColCount(); ++col )
        {
            if( table->GetCell( row, col ) == this )
                return col;
        }
    }

    return -1;
}


wxString SCH_TABLECELL::GetAddr() const
{
    return wxString::Format( wxT( "%c%d" ),
                             'A' + GetColumn() % 26,
                             GetRow() + 1 );
}


void SCH_TABLECELL::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                           const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    if( m_colSpan >= 1 && m_rowSpan >= 1 )
        SCH_TEXTBOX::Print( aSettings, aUnit, aBodyStyle, aOffset, aForceNoFill, aDimmed );
}


void SCH_TABLECELL::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                          int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( m_colSpan >= 1 && m_rowSpan >= 1 )
        SCH_TEXTBOX::Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );
}


void SCH_TABLECELL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Table Cell" ), GetAddr() );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Cell Width" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );
    aList.emplace_back( _( "Cell Height" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
    aList.emplace_back( _( "Style" ), textStyle[style] );

    aList.emplace_back( _( "Text Size" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
}


double SCH_TABLECELL::Similarity( const SCH_ITEM& aOtherItem ) const
{
    if( aOtherItem.Type() != Type() )
        return 0.0;

    const SCH_TABLECELL& other = static_cast<const SCH_TABLECELL&>( aOtherItem );

    double similarity = 1.0;

    if( m_colSpan != other.m_colSpan )
        similarity *= 0.9;

    if( m_rowSpan != other.m_rowSpan )
        similarity *= 0.9;

    similarity *= SCH_TEXTBOX::Similarity( other );

    return similarity;
}


bool SCH_TABLECELL::operator==( const SCH_ITEM& aOtherItem ) const
{
    if( aOtherItem.Type() != Type() )
        return false;

    const SCH_TABLECELL& other = static_cast<const SCH_TABLECELL&>( aOtherItem );

    return *this == other;
}


bool SCH_TABLECELL::operator==( const SCH_TABLECELL& aOtherItem ) const
{
    return m_colSpan == aOtherItem.m_colSpan && m_rowSpan == aOtherItem.m_rowSpan
           && SCH_TEXTBOX::operator==( aOtherItem );
}

static struct SCH_TABLECELL_DESC
{
    SCH_TABLECELL_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_TABLECELL );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_TABLECELL, SCH_TEXTBOX> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TABLECELL, SCH_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TABLECELL, EDA_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_TABLECELL, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( SCH_TEXTBOX ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( SCH_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start X" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start Y" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "End X" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "End Y" ) );

        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Style" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Color" ) );

        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
    }
} _SCH_TABLECELL_DESC;
