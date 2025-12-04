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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <advanced_config.h>
#include <common.h>
#include <pcb_edit_frame.h>
#include <font/font.h>
#include <widgets/msgpanel.h>
#include <string_utils.h>
#include <board.h>
#include <pcb_table.h>
#include <footprint.h>


PCB_TABLECELL::PCB_TABLECELL( BOARD_ITEM* aParent ) :
        PCB_TEXTBOX( aParent, PCB_TABLECELL_T ),
        m_colSpan( 1 ),
        m_rowSpan( 1 )
{
    if( BOARD* board = GetBoard() )
        SetMirrored( board->IsBackLayer( aParent->GetLayer() ) );
    else
        SetMirrored( IsBackLayer( aParent->GetLayer() ) );

    SetRectangleHeight( std::numeric_limits<int>::max() / 2 );
    SetRectangleWidth( std::numeric_limits<int>::max() / 2 );
}


void PCB_TABLECELL::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == PCB_TABLECELL_T );

    std::swap( *( (PCB_TABLECELL*) this ), *( (PCB_TABLECELL*) aImage ) );
}


wxString PCB_TABLECELL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Table cell %s" ), GetAddr() );
}


int PCB_TABLECELL::GetRow() const
{
    const PCB_TABLE* table = static_cast<const PCB_TABLE*>( GetParent() );

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


int PCB_TABLECELL::GetColumn() const
{
    const PCB_TABLE* table = static_cast<const PCB_TABLE*>( GetParent() );

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


wxString PCB_TABLECELL::GetAddr() const
{
    return wxString::Format( wxT( "%c%d" ), 'A' + GetColumn() % 26, GetRow() + 1 );
}


wxString PCB_TABLECELL::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = GetParentFootprint();
    const BOARD*     board = GetBoard();

    std::function<bool( wxString* )> tableCellResolver = [&]( wxString* token ) -> bool
    {
        if( token->IsSameAs( wxT( "ROW" ) ) )
        {
            *token = wxString::Format( wxT( "%d" ), GetRow() + 1 ); // 1-based
            return true;
        }
        else if( token->IsSameAs( wxT( "COL" ) ) )
        {
            *token = wxString::Format( wxT( "%d" ), GetColumn() + 1 ); // 1-based
            return true;
        }
        else if( token->IsSameAs( wxT( "ADDR" ) ) )
        {
            *token = GetAddr();
            return true;
        }
        else if( token->IsSameAs( wxT( "LAYER" ) ) )
        {
            *token = GetLayerName();
            return true;
        }

        if( parentFootprint && parentFootprint->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        if( board->ResolveTextVar( token, aDepth + 1 ) )
            return true;

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
        text = ResolveTextVars( text, &tableCellResolver, aDepth );

    KIFONT::FONT*         font = GetDrawFont( nullptr );
    EDA_ANGLE             drawAngle = GetDrawRotation();
    std::vector<VECTOR2I> corners = GetCornersInSequence( drawAngle );
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    if( GetTextAngle().IsHorizontal() )
        colWidth -= ( GetMarginLeft() + GetMarginRight() );
    else
        colWidth -= ( GetMarginTop() + GetMarginBottom() );

    font->LinebreakText( text, colWidth, GetTextSize(), GetEffectiveTextPenWidth(), IsBold(), IsItalic() );

    // Convert escape markers back to literal ${} and @{} for final display
    text.Replace( wxT( "<<<ESC_DOLLAR:" ), wxT( "${" ) );
    text.Replace( wxT( "<<<ESC_AT:" ), wxT( "@{" ) );

    return text;
}


int PCB_TABLECELL::GetColumnWidth() const
{
    return static_cast<PCB_TABLE*>( GetParent() )->GetColWidth( GetColumn() );
}


void PCB_TABLECELL::SetColumnWidth( int aWidth )
{
    PCB_TABLE* table = static_cast<PCB_TABLE*>( GetParent() );

    table->SetColWidth( GetColumn(), aWidth );
    table->Normalize();
}


int PCB_TABLECELL::GetRowHeight() const
{
    return static_cast<PCB_TABLE*>( GetParent() )->GetRowHeight( GetRow() );
}


void PCB_TABLECELL::SetRowHeight( int aHeight )
{
    PCB_TABLE* table = static_cast<PCB_TABLE*>( GetParent() );

    table->SetRowHeight( GetRow(), aHeight );
    table->Normalize();
}


void PCB_TABLECELL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Table Cell" ), GetAddr() );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Cell Width" ), aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );
    aList.emplace_back( _( "Cell Height" ), aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    if( GetTextThickness() )
        aList.emplace_back( _( "Text Thickness" ), aFrame->MessageTextFromValue( GetEffectiveTextPenWidth() ) );
    else
        aList.emplace_back( _( "Text Thickness" ), _( "Auto" ) );

    aList.emplace_back( _( "Text Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );
}


double PCB_TABLECELL::Similarity( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return 0.0;

    const PCB_TABLECELL& other = static_cast<const PCB_TABLECELL&>( aBoardItem );

    double similarity = 1.0;

    if( m_colSpan != other.m_colSpan )
        similarity *= 0.9;

    if( m_rowSpan != other.m_rowSpan )
        similarity *= 0.9;

    similarity *= PCB_TEXTBOX::Similarity( other );

    return similarity;
}

bool PCB_TABLECELL::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_TABLECELL& other = static_cast<const PCB_TABLECELL&>( aBoardItem );

    return *this == other;
}

bool PCB_TABLECELL::operator==( const PCB_TABLECELL& aOther ) const
{
    return m_colSpan == aOther.m_colSpan && m_rowSpan == aOther.m_rowSpan && PCB_TEXTBOX::operator==( aOther );
}


static struct PCB_TABLECELL_DESC
{
    PCB_TABLECELL_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TABLECELL );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, BOARD_CONNECTED_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, PCB_TEXTBOX> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, PCB_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, EDA_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLECELL, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_TEXTBOX ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_SHAPE ), _HKI( "Layer" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_SHAPE ), _HKI( "Soldermask" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_SHAPE ), _HKI( "Soldermask Margin Override" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );

        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_TEXTBOX ), _HKI( "Border" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_TEXTBOX ), _HKI( "Border Style" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( PCB_TEXTBOX ), _HKI( "Border Width" ) );

        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "End X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "End Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Width" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Style" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Color" ) );

        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );
        propMgr.Mask( TYPE_HASH( PCB_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Color" ) );

        const wxString tableProps = _( "Table" );

        propMgr.AddProperty( new PROPERTY<PCB_TABLECELL, int>( _HKI( "Column Width" ), &PCB_TABLECELL::SetColumnWidth,
                                                               &PCB_TABLECELL::GetColumnWidth,
                                                               PROPERTY_DISPLAY::PT_SIZE ),
                             tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLECELL, int>( _HKI( "Row Height" ), &PCB_TABLECELL::SetRowHeight,
                                                               &PCB_TABLECELL::GetRowHeight,
                                                               PROPERTY_DISPLAY::PT_SIZE ),
                             tableProps );
    }
} _PCB_TABLECELL_DESC;
