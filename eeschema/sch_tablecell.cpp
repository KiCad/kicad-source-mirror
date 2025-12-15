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
#include <sch_edit_frame.h>
#include <widgets/msgpanel.h>
#include <string_utils.h>
#include <sch_table.h>
#include <sch_tablecell.h>


SCH_TABLECELL::SCH_TABLECELL( int aLineWidth, FILL_T aFillType ) :
        SCH_TEXTBOX( LAYER_NOTES, aLineWidth, aFillType, wxEmptyString, SCH_TABLECELL_T ),
        m_colSpan( 1 ),
        m_rowSpan( 1 )
{
}


void SCH_TABLECELL::swapData( SCH_ITEM* aItem )
{
    SCH_TEXTBOX::swapData( aItem );

    SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( aItem );

    std::swap( m_colSpan, cell->m_colSpan );
    std::swap( m_rowSpan, cell->m_rowSpan );
}


wxString SCH_TABLECELL::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
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
    return wxString::Format( wxT( "%c%d" ), 'A' + GetColumn() % 26, GetRow() );
}


/**
 * Parse a cell address string like "A0" or "B12" into 0-based row and column indices.
 * Supports single-letter columns (A-Z) and multi-letter columns (AA-ZZ).
 * Both row and column numbers in the address are 0-based (A0 = first row, first column).
 *
 * @param aAddr The address string to parse (e.g., "A0", "B5", "AA10")
 * @param aRow Output parameter for the 0-based row index
 * @param aCol Output parameter for the 0-based column index
 * @return true if parsing succeeded, false if format is invalid
 */
static bool parseCellAddress( const wxString& aAddr, int& aRow, int& aCol )
{
    if( aAddr.IsEmpty() )
        return false;

    // Extract column part (letters) and row part (numbers)
    size_t   i = 0;
    wxString colPart;

    // Read column letters (A-Z, AA-ZZ, etc.)
    while( i < aAddr.Length() && wxIsalpha( aAddr[i] ) )
    {
        colPart += wxToupper( aAddr[i] );
        i++;
    }

    // Must have at least one letter
    if( colPart.IsEmpty() )
        return false;

    // Convert column letters to 0-based index (A=0, B=1, ..., Z=25, AA=26, AB=27, etc.)
    aCol = 0;
    for( size_t j = 0; j < colPart.Length(); j++ )
    {
        aCol = aCol * 26 + ( colPart[j] - 'A' + 1 );
    }
    aCol -= 1; // Convert to 0-based

    // Read row number (0-based)
    wxString rowPart = aAddr.Mid( i );
    if( rowPart.IsEmpty() )
        return false;

    // Convert row string to number (already 0-based in address string)
    long rowNum;
    if( !rowPart.ToLong( &rowNum ) || rowNum < 0 )
        return false;

    aRow = rowNum; // Already 0-based

    return true;
}


wxString SCH_TABLECELL::GetShownText( const RENDER_SETTINGS* aSettings, const SCH_SHEET_PATH* aPath,
                                      bool aAllowExtraText, int aDepth ) const
{
    // Local depth counter for ResolveTextVars iteration tracking (separate from cross-cell aDepth)
    int depth = 0;

    SCH_SHEET* sheet = nullptr;

    if( aPath )
        sheet = aPath->Last();

    // Text variable resolver supporting:
    // - ${ROW}, ${COL}, ${ADDR} - cell position variables
    // - @{expression} - math expression evaluation
    // - ${CELL("A1")} or ${CELL(row,col)} - reference to another cell's evaluated text
    // - \${...} and \@{...} - escape sequences for literal display
    std::function<bool( wxString* )> tableCellResolver = [&]( wxString* token ) -> bool
    {
        if( token->IsSameAs( wxT( "ROW" ) ) )
        {
            *token = wxString::Format( wxT( "%d" ), GetRow() ); // 0-based
            return true;
        }
        else if( token->IsSameAs( wxT( "COL" ) ) )
        {
            *token = wxString::Format( wxT( "%d" ), GetColumn() ); // 0-based
            return true;
        }
        else if( token->IsSameAs( wxT( "ADDR" ) ) )
        {
            *token = GetAddr();
            return true;
        }
        else if( token->StartsWith( wxT( "CELL(" ) ) && token->EndsWith( wxT( ")" ) ) )
        {
            // Handle CELL("A0") or CELL(0, 1) syntax
            wxString args = token->Mid( 5, token->Length() - 6 ); // Extract arguments
            args.Trim( true ).Trim( false );                      // Remove whitespace

            SCH_TABLE* table = static_cast<SCH_TABLE*>( GetParent() );
            if( !table )
            {
                *token = wxT( "<Unresolved: CELL() requires table context>" );
                return true;
            }

            int targetRow = -1;
            int targetCol = -1;

            // Check if it's cell("A1") format (string argument with quotes)
            if( args.StartsWith( wxT( "\"" ) ) && args.EndsWith( wxT( "\"" ) ) )
            {
                wxString addr = args.Mid( 1, args.Length() - 2 ); // Remove quotes
                if( !parseCellAddress( addr, targetRow, targetCol ) )
                {
                    *token = wxString::Format( wxT( "<Unresolved: Invalid cell address: %s>" ), addr );
                    return true;
                }
            }
            // Check if it's cell(row, col) format (two numeric arguments)
            else if( args.Find( ',' ) != wxNOT_FOUND )
            {
                wxString rowStr = args.BeforeFirst( ',' ).Trim( true ).Trim( false );
                wxString colStr = args.AfterFirst( ',' ).Trim( true ).Trim( false );

                long rowNum, colNum;
                if( !rowStr.ToLong( &rowNum ) || !colStr.ToLong( &colNum ) )
                {
                    *token = wxString::Format( wxT( "<Unresolved: Invalid cell coordinates: %s>" ), args );
                    return true;
                }

                // Arguments are already 0-based
                targetRow = rowNum;
                targetCol = colNum;
            }
            else
            {
                *token = wxString::Format( wxT( "<Unresolved: Invalid CELL() syntax: %s>" ), args );
                return true;
            }

            // Check bounds
            if( targetRow < 0 || targetRow >= table->GetRowCount() || targetCol < 0
                || targetCol >= table->GetColCount() )
            {
                wxString cellAddr;
                if( targetRow >= 0 && targetCol >= 0 )
                {
                    char colLetter = 'A' + ( targetCol % 26 );
                    cellAddr = wxString::Format( wxT( "%c%d" ), colLetter, targetRow );
                }
                else
                {
                    cellAddr = args;
                }
                *token = wxString::Format( wxT( "<Unresolved: Cell %s not found>" ), cellAddr );
                return true;
            }

            // Get the target cell and return its evaluated text
            SCH_TABLECELL* targetCell = table->GetCell( targetRow, targetCol );
            if( targetCell )
            {
                // Check for excessive recursion depth (circular references)
                const int maxDepth = ADVANCED_CFG::GetCfg().m_ResolveTextRecursionDepth;
                if( aDepth >= maxDepth )
                {
                    *token = wxT( "<Circular reference>" );
                    return true;
                }

                *token = targetCell->GetShownText( aSettings, aPath, aAllowExtraText, aDepth + 1 );
                return true;
            }
            else
            {
                *token = wxT( "<Unresolved: Cell not found>" );
                return true;
            }
        }

        // Fall back to sheet variables
        if( sheet )
        {
            if( sheet->ResolveTextVar( aPath, token, depth + 1 ) )
                return true;
        }

        return false;
    };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, depth );

    if( HasTextVars() )
        text = ResolveTextVars( text, &tableCellResolver, depth );

    VECTOR2I size = GetEnd() - GetStart();
    int      colWidth;

    if( GetTextAngle().IsVertical() )
        colWidth = abs( size.y ) - ( GetMarginTop() + GetMarginBottom() );
    else
        colWidth = abs( size.x ) - ( GetMarginLeft() + GetMarginRight() );

    GetDrawFont( aSettings )
            ->LinebreakText( text, colWidth, GetTextSize(), GetEffectiveTextPenWidth(), IsBold(), IsItalic() );

    // Convert escape markers back to literal ${} and @{} for final display
    // Only do this at the top level (aDepth == 0) to avoid premature unescaping in nested CELL() calls
    if( aDepth == 0 )
    {
        text.Replace( wxT( "<<<ESC_DOLLAR:" ), wxT( "${" ) );
        text.Replace( wxT( "<<<ESC_AT:" ), wxT( "@{" ) );
    }

    return text;
}


int SCH_TABLECELL::GetColumnWidth() const
{
    return static_cast<SCH_TABLE*>( GetParent() )->GetColWidth( GetColumn() );
}


void SCH_TABLECELL::SetColumnWidth( int aWidth )
{
    SCH_TABLE* table = static_cast<SCH_TABLE*>( GetParent() );

    table->SetColWidth( GetColumn(), aWidth );
    table->Normalize();
}


int SCH_TABLECELL::GetRowHeight() const
{
    return static_cast<SCH_TABLE*>( GetParent() )->GetRowHeight( GetRow() );
}


void SCH_TABLECELL::SetRowHeight( int aHeight )
{
    SCH_TABLE* table = static_cast<SCH_TABLE*>( GetParent() );

    table->SetRowHeight( GetRow(), aHeight );
    table->Normalize();
}


void SCH_TABLECELL::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts, int aUnit,
                          int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    const int cell_body_style = -1; // flag to disable box ouline plotting

    if( m_colSpan >= 1 && m_rowSpan >= 1 )
        SCH_TEXTBOX::Plot( aPlotter, aBackground, aPlotOpts, aUnit, cell_body_style, aOffset, aDimmed );
}


void SCH_TABLECELL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Table Cell" ), GetAddr() );

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Cell Width" ), aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );
    aList.emplace_back( _( "Cell Height" ), aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );

    wxString textStyle[] = { _( "Normal" ), _( "Italic" ), _( "Bold" ), _( "Bold Italic" ) };
    int      style = IsBold() && IsItalic() ? 3 : IsBold() ? 2 : IsItalic() ? 1 : 0;
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
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Fill" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Fill Color" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Style" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Color" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Width" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Height" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Thickness" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Mirrored" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Visible" ) );
        propMgr.Mask( TYPE_HASH( SCH_TABLECELL ), TYPE_HASH( EDA_TEXT ), _HKI( "Hyperlink" ) );

        const wxString tableProps = _( "Table" );

        propMgr.AddProperty( new PROPERTY<SCH_TABLECELL, int>( _HKI( "Column Width" ), &SCH_TABLECELL::SetColumnWidth,
                                                               &SCH_TABLECELL::GetColumnWidth,
                                                               PROPERTY_DISPLAY::PT_SIZE ),
                             tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLECELL, int>( _HKI( "Row Height" ), &SCH_TABLECELL::SetRowHeight,
                                                               &SCH_TABLECELL::GetRowHeight,
                                                               PROPERTY_DISPLAY::PT_SIZE ),
                             tableProps );

        const wxString cellProps = _( "Cell Properties" );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, bool>( _HKI( "Background Fill" ), &EDA_SHAPE::SetFilled,
                                                            &EDA_SHAPE::IsSolidFill ),
                             cellProps );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, COLOR4D>( _HKI( "Background Fill Color" ),
                                                               &EDA_SHAPE::SetFillColor, &EDA_SHAPE::GetFillColor ),
                             cellProps )
                .SetIsHiddenFromRulesEditor();
    }
} _SCH_TABLECELL_DESC;
