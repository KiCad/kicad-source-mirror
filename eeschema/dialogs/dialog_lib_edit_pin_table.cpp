/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_lib_edit_pin_table.h"
#include "grid_tricks.h"
#include "lib_pin.h"
#include "pin_numbers.h"
#include "pgm_base.h"
#include <base_units.h>
#include <bitmaps.h>
#include <confirm.h>
#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <kiplatform/ui.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <widgets/wx_grid.h>
#include <widgets/bitmap_button.h>
#include <widgets/std_bitmap_button.h>
#include <settings/settings_manager.h>
#include <wx/tokenzr.h>
#include <string_utils.h>

#define UNITS_ALL _( "ALL" )
#define DEMORGAN_ALL _( "ALL" )
#define DEMORGAN_STD _( "Standard" )
#define DEMORGAN_ALT _( "Alternate" )


void getSelectedArea( WX_GRID* aGrid, int* aRowStart, int* aRowCount )
{
    wxGridCellCoordsArray topLeft  = aGrid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray botRight = aGrid->GetSelectionBlockBottomRight();

    wxArrayInt  cols = aGrid->GetSelectedCols();
    wxArrayInt  rows = aGrid->GetSelectedRows();

    if( topLeft.Count() && botRight.Count() )
    {
        *aRowStart = topLeft[0].GetRow();
        *aRowCount = botRight[0].GetRow() - *aRowStart + 1;
    }
    else if( cols.Count() )
    {
        *aRowStart = 0;
        *aRowCount = aGrid->GetNumberRows();
    }
    else if( rows.Count() )
    {
        *aRowStart = rows[0];
        *aRowCount = rows.Count();
    }
    else
    {
        *aRowStart = aGrid->GetGridCursorRow();
        *aRowCount = *aRowStart >= 0 ? 1 : 0;
    }
}


class PIN_TABLE_DATA_MODEL : public wxGridTableBase
{
public:
    PIN_TABLE_DATA_MODEL( SYMBOL_EDIT_FRAME* aFrame, DIALOG_LIB_EDIT_PIN_TABLE* aPinTable,
                          LIB_SYMBOL* aSymbol ) :
            m_frame( aFrame ),
            m_unitFilter( -1 ),
            m_edited( false ),
            m_pinTable( aPinTable ),
            m_symbol( aSymbol )
    {
        m_eval = std::make_unique<NUMERIC_EVALUATOR>( m_frame->GetUserUnits() );

        m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PIN_TABLE_DATA_MODEL::onUnitsChanged, this );
    }

    ~PIN_TABLE_DATA_MODEL()
    {
        m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PIN_TABLE_DATA_MODEL::onUnitsChanged, this );
    }

    void onUnitsChanged( wxCommandEvent& aEvent )
    {
        if( GetView() )
            GetView()->ForceRefresh();

        aEvent.Skip();
    }

    void SetUnitFilter( int aFilter ) { m_unitFilter = aFilter; }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case COL_PIN_COUNT:    return _( "Count" );
        case COL_NUMBER:       return _( "Number" );
        case COL_NAME:         return _( "Name" );
        case COL_TYPE:         return _( "Electrical Type" );
        case COL_SHAPE:        return _( "Graphic Style" );
        case COL_ORIENTATION:  return _( "Orientation" );
        case COL_NUMBER_SIZE:  return _( "Number Text Size" );
        case COL_NAME_SIZE:    return _( "Name Text Size" );
        case COL_LENGTH:       return _( "Length" );
        case COL_POSX:         return _( "X Position" );
        case COL_POSY:         return _( "Y Position" );
        case COL_VISIBLE:      return _( "Visible" );
        case COL_UNIT:         return _( "Unit" );
        case COL_DEMORGAN:     return _( "De Morgan" );
        default:               wxFAIL; return wxEmptyString;
        }
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        wxGrid*  grid = GetView();

        if( grid->GetGridCursorRow() == aRow && grid->GetGridCursorCol() == aCol
                && grid->IsCellEditControlShown() )
        {
            auto it = m_evalOriginal.find( { m_rows[ aRow ], aCol } );

            if( it != m_evalOriginal.end() )
                return it->second;
        }

        return GetValue( m_rows[ aRow ], aCol, m_frame );
    }

    static wxString GetValue( const LIB_PINS& pins, int aCol, EDA_DRAW_FRAME* aParentFrame )
    {
        wxString fieldValue;

        if( pins.empty() )
            return fieldValue;

        for( LIB_PIN* pin : pins )
        {
            wxString val;

            switch( aCol )
            {
            case COL_PIN_COUNT:
                val << pins.size();
                break;

            case COL_NUMBER:
                val = pin->GetNumber();
                break;

            case COL_NAME:
                val = pin->GetName();
                break;

            case COL_TYPE:
                val = PinTypeNames()[static_cast<int>( pin->GetType() )];
                break;

            case COL_SHAPE:
                val = PinShapeNames()[static_cast<int>( pin->GetShape() )];
                break;

            case COL_ORIENTATION:
                if( PinOrientationIndex( pin->GetOrientation() ) >= 0 )
                    val = PinOrientationNames()[ PinOrientationIndex( pin->GetOrientation() ) ];

                break;

            case COL_NUMBER_SIZE:
                val = aParentFrame->StringFromValue( pin->GetNumberTextSize(), true );
                break;

            case COL_NAME_SIZE:
                val = aParentFrame->StringFromValue( pin->GetNameTextSize(), true );
                break;

            case COL_LENGTH:
                val = aParentFrame->StringFromValue( pin->GetLength(), true );
                break;

            case COL_POSX:
                val = aParentFrame->StringFromValue( pin->GetPosition().x, true );
                break;

            case COL_POSY:
                val = aParentFrame->StringFromValue( -pin->GetPosition().y, true );
                break;

            case COL_VISIBLE:
                val = StringFromBool( pin->IsVisible() );
                break;

            case COL_UNIT:
                if( pin->GetUnit() )
                    val = LIB_SYMBOL::SubReference( pin->GetUnit(), false );
                else
                    val = UNITS_ALL;
                break;

            case COL_DEMORGAN:
                switch( pin->GetConvert() )
                {
                case LIB_ITEM::LIB_CONVERT::BASE:
                    val = DEMORGAN_STD;
                    break;
                case LIB_ITEM::LIB_CONVERT::DEMORGAN:
                    val = DEMORGAN_ALT;
                    break;
                default:
                    val = DEMORGAN_ALL;
                    break;
                }
                break;

            default:
                wxFAIL;
                break;
            }

            if( aCol == COL_NUMBER )
            {
                if( fieldValue.length() )
                    fieldValue += wxT( ", " );
                fieldValue += val;
            }
            else
            {
                if( !fieldValue.Length() )
                    fieldValue = val;
                else if( val != fieldValue )
                    fieldValue = INDETERMINATE_STATE;
            }
        }

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aValue == INDETERMINATE_STATE )
            return;

        wxString value = aValue;

        switch( aCol )
        {
        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

            if( m_eval->Process( value ) )
            {
                m_evalOriginal[ { m_rows[ aRow ], aCol } ] = value;
                value = m_eval->Result();
            }

            break;

        default:
            break;
        }

        LIB_PINS pins = m_rows[ aRow ];

        // If the NUMBER column is edited and the pins are grouped, renumber, and add or
        // remove pins based on the comma separated list of pins.
        if( aCol == COL_NUMBER && m_pinTable->IsDisplayGrouped() )
        {
            wxStringTokenizer tokenizer( value, "," );
            size_t            i = 0;

            while( tokenizer.HasMoreTokens() )
            {
                wxString pinName = tokenizer.GetNextToken();

                // Trim whitespace from both ends of the string
                pinName.Trim( true ).Trim( false );

                if( i < pins.size() )
                {
                    // Renumber the existing pins
                    pins.at( i )->SetNumber( pinName );
                }
                else
                {
                    // Create new pins
                    LIB_PIN* newPin = new LIB_PIN( this->m_symbol );
                    LIB_PIN* last = pins.back();

                    newPin->SetNumber( pinName );
                    newPin->SetName( last->GetName() );
                    newPin->SetOrientation( last->GetOrientation() );
                    newPin->SetType( last->GetType() );
                    newPin->SetShape( last->GetShape() );
                    newPin->SetUnit( last->GetUnit() );

                    VECTOR2I pos = last->GetPosition();

                    auto* cfg = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

                    if( last->GetOrientation() == PIN_ORIENTATION::PIN_LEFT
                        || last->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
                    {
                        pos.y -= schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                    }
                    else
                    {
                        pos.x += schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
                    }

                    newPin->SetPosition( pos );

                    pins.push_back( newPin );
                    m_pinTable->AddPin( newPin );
                }

                i++;
            }

            while( pins.size() > i )
            {
                m_pinTable->RemovePin( pins.back() );
                pins.pop_back();
            }

            m_rows[aRow] = pins;
            m_edited = true;

            return;
        }

        for( LIB_PIN* pin : pins )
        {
            switch( aCol )
            {
            case COL_NUMBER:
                if( !m_pinTable->IsDisplayGrouped() )
                    pin->SetNumber( value );

                break;

            case COL_NAME:
                pin->SetName( value );
                break;

            case COL_TYPE:
                if( PinTypeNames().Index( value ) != wxNOT_FOUND )
                    pin->SetType( (ELECTRICAL_PINTYPE) PinTypeNames().Index( value ) );

                break;

            case COL_SHAPE:
                if( PinShapeNames().Index( value ) != wxNOT_FOUND )
                    pin->SetShape( (GRAPHIC_PINSHAPE) PinShapeNames().Index( value ) );

                break;

            case COL_ORIENTATION:
                if( PinOrientationNames().Index( value ) != wxNOT_FOUND )
                    pin->SetOrientation( PinOrientationCode( PinOrientationNames().Index( value ) ) );
                break;

            case COL_NUMBER_SIZE:
                pin->SetNumberTextSize( m_frame->ValueFromString( value ) );
                break;

            case COL_NAME_SIZE:
                pin->SetNameTextSize( m_frame->ValueFromString( value ) );
                break;

            case COL_LENGTH:
                pin->ChangeLength( m_frame->ValueFromString( value ) );
                break;

            case COL_POSX:
                pin->SetPosition( VECTOR2I( m_frame->ValueFromString( value ),
                                            pin->GetPosition().y ) );
                break;

            case COL_POSY:
                pin->SetPosition( VECTOR2I( pin->GetPosition().x,
                                            -m_frame->ValueFromString( value ) ) );
                break;

            case COL_VISIBLE:
                pin->SetVisible(BoolFromString( value ));
                break;

            case COL_UNIT:
                if( value == UNITS_ALL )
                {
                    pin->SetUnit( 0 );
                }
                else
                {
                    for( int i = 1; i <= m_symbol->GetUnitCount(); i++ )
                    {
                        if( value == LIB_SYMBOL::SubReference( i, false ) )
                        {
                            pin->SetUnit( i );
                            break;
                        }
                    }
                }
                break;

            case COL_DEMORGAN:
                if( value == DEMORGAN_STD )
                    pin->SetConvert( 1 );
                else if( value == DEMORGAN_ALT )
                    pin->SetConvert( 2 );
                else
                    pin->SetConvert( 0 );
                break;

            default:
                wxFAIL;
                break;
            }
        }

        m_edited = true;
    }

    static int findRow( const std::vector<LIB_PINS>& aRowSet, const wxString& aName )
    {
        for( size_t i = 0; i < aRowSet.size(); ++i )
        {
            if( aRowSet[ i ][ 0 ] && aRowSet[ i ][ 0 ]->GetName() == aName )
                return i;
        }

        return -1;
    }

    static bool compare( const LIB_PINS& lhs, const LIB_PINS& rhs, int sortCol, bool ascending,
                         EDA_DRAW_FRAME* parentFrame )
    {
        wxString lhStr = GetValue( lhs, sortCol, parentFrame );
        wxString rhStr = GetValue( rhs, sortCol, parentFrame );

        if( lhStr == rhStr )
        {
            // Secondary sort key is always COL_NUMBER
            sortCol = COL_NUMBER;
            lhStr = GetValue( lhs, sortCol, parentFrame );
            rhStr = GetValue( rhs, sortCol, parentFrame );
        }

        bool res;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto cmp = [ ascending ]( const auto a, const auto b )
                   {
                       if( ascending )
                           return a < b;
                       else
                           return b < a;
                   };

        switch( sortCol )
        {
        case COL_NUMBER:
        case COL_NAME:
            res = cmp( PIN_NUMBERS::Compare( lhStr, rhStr ), 0 );
            break;
        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
            res = cmp( parentFrame->ValueFromString( lhStr ),
                       parentFrame->ValueFromString( rhStr ) );
            break;
        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            res = cmp( parentFrame->ValueFromString( lhStr ),
                       parentFrame->ValueFromString( rhStr ) );
            break;
        case COL_VISIBLE:
        case COL_DEMORGAN:
        default:
            res = cmp( StrNumCmp( lhStr, rhStr ), 0 );
            break;
        }

        return res;
    }

    void RebuildRows( const LIB_PINS& aPins, bool groupByName, bool groupBySelection )
    {
        WX_GRID* grid = dynamic_cast<WX_GRID*>( GetView() );
        std::vector<LIB_PIN*> clear_flags;

        clear_flags.reserve( aPins.size() );

        if( grid )
        {
            if( groupBySelection )
            {
                for( LIB_PIN* pin : aPins )
                    pin->ClearTempFlags();

                int firstSelectedRow;
                int selectedRowCount;

                getSelectedArea( grid, &firstSelectedRow, &selectedRowCount );

                for( int ii = 0; ii < selectedRowCount; ++ii )
                {
                    for( LIB_PIN* pin : m_rows[ firstSelectedRow + ii ] )
                    {
                        pin->SetFlags( CANDIDATE );
                        clear_flags.push_back( pin );
                    }
                }
            }

            // Commit any pending in-place edits before the row gets moved out from under
            // the editor.
            grid->CommitPendingChanges( true );

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        if( groupBySelection )
            m_rows.emplace_back( LIB_PINS() );

        for( LIB_PIN* pin : aPins )
        {
            if( m_unitFilter == -1 || pin->GetUnit() == 0 || pin->GetUnit() == m_unitFilter )
            {
                int rowIndex = -1;

                if( groupByName )
                    rowIndex = findRow( m_rows, pin->GetName() );
                else if( groupBySelection && ( pin->GetFlags() & CANDIDATE ) )
                    rowIndex = 0;

                if( rowIndex < 0 )
                {
                    m_rows.emplace_back( LIB_PINS() );
                    rowIndex = m_rows.size() - 1;
                }

                m_rows[ rowIndex ].push_back( pin );
            }
        }

        int sortCol = 0;
        bool ascending = true;

        if( GetView() && GetView()->GetSortingColumn() != wxNOT_FOUND )
        {
            sortCol = GetView()->GetSortingColumn();
            ascending = GetView()->IsSortOrderAscending();
        }

        for( LIB_PINS& row : m_rows )
            SortPins( row );

        if( !groupBySelection )
            SortRows( sortCol, ascending );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
            GetView()->ProcessTableMessage( msg );

            if( groupBySelection )
                GetView()->SelectRow( 0 );
        }

        for( LIB_PIN* pin : clear_flags )
            pin->ClearFlags( CANDIDATE );
    }

    void SortRows( int aSortCol, bool ascending )
    {
        std::sort( m_rows.begin(), m_rows.end(),
                   [ aSortCol, ascending, this ]( const LIB_PINS& lhs, const LIB_PINS& rhs ) -> bool
                   {
                       return compare( lhs, rhs, aSortCol, ascending, m_frame );
                   } );
    }

    void SortPins( LIB_PINS& aRow )
    {
        std::sort( aRow.begin(), aRow.end(),
                   []( LIB_PIN* lhs, LIB_PIN* rhs ) -> bool
                   {
                       return PIN_NUMBERS::Compare( lhs->GetNumber(), rhs->GetNumber() ) < 0;
                   } );
    }

    void AppendRow( LIB_PIN* aPin )
    {
        LIB_PINS row;
        row.push_back( aPin );
        m_rows.push_back( row );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            GetView()->ProcessTableMessage( msg );
        }
    }

    LIB_PINS RemoveRow( int aRow )
    {
        LIB_PINS removedRow = m_rows[ aRow ];

        m_rows.erase( m_rows.begin() + aRow );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
            GetView()->ProcessTableMessage( msg );
        }

        return removedRow;
    }

    LIB_PINS GetRowPins( int aRow )
    {
        return m_rows[ aRow ];
    }

    bool IsEdited()
    {
        return m_edited;
    }

private:
    static wxString StringFromBool( bool aValue )
    {
        if( aValue )
            return wxT( "1" );
        else
            return wxT( "0" );
    }

    static bool BoolFromString( wxString aValue )
    {
        if( aValue == wxS( "1" ) )
        {
            return true;
        }
        else if( aValue == wxS( "0" ) )
        {
            return false;
        }
        else
        {
            wxFAIL_MSG( wxString::Format( "string '%s' can't be converted to boolean correctly, "
                                          "it will have been perceived as FALSE",
                                          aValue ) );
            return false;
        }
    }

private:
    SYMBOL_EDIT_FRAME*    m_frame;

    // Because the rows of the grid can either be a single pin or a group of pins, the
    // data model is a 2D vector.  If we're in the single pin case, each row's LIB_PINS
    // contains only a single pin.
    std::vector<LIB_PINS> m_rows;
    int                   m_unitFilter;     // 0 to show pins for all units

    bool                  m_edited;

    DIALOG_LIB_EDIT_PIN_TABLE* m_pinTable;
    LIB_SYMBOL*                m_symbol;    // Parent symbol that the pins belong to.

    std::unique_ptr<NUMERIC_EVALUATOR>             m_eval;
    std::map< std::pair<LIB_PINS, int>, wxString > m_evalOriginal;
};


DIALOG_LIB_EDIT_PIN_TABLE::DIALOG_LIB_EDIT_PIN_TABLE( SYMBOL_EDIT_FRAME* parent,
                                                      LIB_SYMBOL* aSymbol ) :
        DIALOG_LIB_EDIT_PIN_TABLE_BASE( parent ),
        m_editFrame( parent ),
        m_symbol( aSymbol )
{
    m_dataModel = new PIN_TABLE_DATA_MODEL( m_editFrame, this, this->m_symbol );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_dataModel );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid, [this]( wxCommandEvent& aEvent )
                                                       {
                                                           OnAddRow( aEvent );
                                                       } ) );

    // Show/hide columns according to the user's preference
    if( SYMBOL_EDITOR_SETTINGS* cfg = parent->GetSettings() )
    {
        m_grid->ShowHideColumns( cfg->m_PinTableVisibleColumns );
        m_columnsShown = m_grid->GetShownColumns();
    }

    // Set special attributes
    wxGridCellAttr* attr;

    attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_grid->SetColAttr( COL_PIN_COUNT, attr );

    attr = new wxGridCellAttr;
    wxArrayString typeNames = PinTypeNames();
    typeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), typeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinTypeIcons(), typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString shapeNames = PinShapeNames();
    shapeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(), shapeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinShapeIcons(), shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString orientationNames = PinOrientationNames();
    orientationNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinOrientationIcons(),
                                                         orientationNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinOrientationIcons(), orientationNames ) );
    m_grid->SetColAttr( COL_ORIENTATION, attr );

    attr = new wxGridCellAttr;
    wxArrayString unitNames;
    unitNames.push_back( UNITS_ALL );

    for( int i = 1; i <= aSymbol->GetUnitCount(); i++ )
        unitNames.push_back( LIB_SYMBOL::SubReference( i, false ) );

    attr->SetEditor( new GRID_CELL_COMBOBOX( unitNames ) );
    m_grid->SetColAttr( COL_UNIT, attr );

    attr = new wxGridCellAttr;
    wxArrayString demorganNames;
    demorganNames.push_back( DEMORGAN_ALL );
    demorganNames.push_back( DEMORGAN_STD );
    demorganNames.push_back( DEMORGAN_ALT );
    attr->SetEditor( new GRID_CELL_COMBOBOX( demorganNames ) );
    m_grid->SetColAttr( COL_DEMORGAN, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetEditor( new wxGridCellBoolEditor() );
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_grid->SetColAttr( COL_VISIBLE, attr );

    /* Right-aligned position values look much better, but only MSW and GTK2+
     * currently support right-aligned textEditCtrls, so the text jumps on all
     * the other platforms when you edit it.
    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSX, attr );

    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSY, attr );
    */

    m_addButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_deleteButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_refreshButton->SetBitmap( KiBitmap( BITMAPS::small_refresh ) );

    m_divider1->SetIsSeparator();
    m_divider2->SetIsSeparator();

    GetSizer()->SetSizeHints(this);
    Centre();

    if( aSymbol->IsMulti() )
    {
        m_unitFilter->Append( UNITS_ALL );

        for( int ii = 0; ii < aSymbol->GetUnitCount(); ++ii )
            m_unitFilter->Append( aSymbol->GetUnitReference( ii + 1 ) );

        m_unitFilter->SetSelection( -1 );
    }
    else
    {
        m_cbFilterByUnit->Show( false );
        m_unitFilter->Show( false );
    }

    SetupStandardButtons();

    if( !parent->IsSymbolEditable() || parent->IsSymbolAlias() )
    {
        m_ButtonsCancel->SetDefault();
        m_ButtonsOK->SetLabel( _( "Read Only" ) );
        m_ButtonsOK->Enable( false );
    }

    m_initialized = true;
    m_modified = false;

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT,
                     wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );
}


DIALOG_LIB_EDIT_PIN_TABLE::~DIALOG_LIB_EDIT_PIN_TABLE()
{
    if( SYMBOL_EDITOR_SETTINGS* cfg = m_editFrame->GetSettings() )
        cfg->m_PinTableVisibleColumns = m_grid->GetShownColumnsAsString();

    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_dataModel );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // This is our copy of the pins.  If they were transferred to the part on an OK, then
    // m_pins will already be empty.
    for( LIB_PIN* pin : m_pins )
        delete pin;

    WINDOW_THAWER thawer( m_editFrame );

    m_editFrame->FocusOnItem( nullptr );
    m_editFrame->GetCanvas()->Refresh();
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataToWindow()
{
    // Make a copy of the pins for editing
    std::vector<LIB_PIN*> pins = m_symbol->GetAllLibPins();

    for( LIB_PIN* pin : pins )
        m_pins.push_back( new LIB_PIN( *pin ) );

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue(), false );

    if( m_symbol->IsMulti() )
        m_grid->ShowCol( COL_UNIT );
    else
        m_grid->HideCol( COL_UNIT );

    if( m_editFrame->GetShowDeMorgan() )
        m_grid->ShowCol( COL_DEMORGAN );
    else
        m_grid->HideCol( COL_DEMORGAN );

    updateSummary();

    return true;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Delete the part's pins
    std::vector<LIB_PIN*> pins = m_symbol->GetAllLibPins();

    for( LIB_PIN* pin : pins )
        m_symbol->RemoveDrawItem( pin );

    // Transfer our pins to the part
    for( LIB_PIN* pin : m_pins )
    {
        m_symbol->AddDrawItem( pin );
    }

    m_pins.clear();

    return true;
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the
    // event, and if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    else
        // different column; start with ascending
        ascending = true;

    m_dataModel->SortRows( sortCol, ascending );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnAddRow( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    LIB_PIN* newPin = new LIB_PIN( this->m_symbol );

    // Copy the settings of the last pin onto the new pin.
    if( m_pins.size() > 0 )
    {
        LIB_PIN* last = m_pins.back();

        newPin->SetOrientation( last->GetOrientation() );
        newPin->SetType( last->GetType() );
        newPin->SetShape( last->GetShape() );
        newPin->SetUnit( last->GetUnit() );

        VECTOR2I pos = last->GetPosition();

        SYMBOL_EDITOR_SETTINGS* cfg = m_editFrame->GetSettings();

        if( last->GetOrientation() == PIN_ORIENTATION::PIN_LEFT
            || last->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
        {
            pos.y -= schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
        }
        else
        {
            pos.x += schIUScale.MilsToIU( cfg->m_Repeat.pin_step );
        }

        newPin->SetPosition( pos );
    }

    m_pins.push_back( newPin );

    m_dataModel->AppendRow( m_pins[ m_pins.size() - 1 ] );

    m_grid->MakeCellVisible( m_grid->GetNumberRows() - 1, 1 );
    m_grid->SetGridCursor( m_grid->GetNumberRows() - 1, 1 );

    m_grid->EnableCellEditControl( true );
    m_grid->ShowCellEditControl();

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::AddPin( LIB_PIN* pin )
{
    m_pins.push_back( pin );
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnDeleteRow( wxCommandEvent& event )
{
    // TODO: handle delete of multiple rows....

    if( !m_grid->CommitPendingChanges() )
        return;

    if( m_pins.size() == 0 )   // empty table
        return;

    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    LIB_PINS removedRow = m_dataModel->RemoveRow( curRow );

    for( LIB_PIN* pin : removedRow )
        m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );

    curRow = std::min( curRow, m_grid->GetNumberRows() - 1 );
    m_grid->GoToCell( curRow, m_grid->GetGridCursorCol() );
    m_grid->SetGridCursor( curRow, m_grid->GetGridCursorCol() );
    m_grid->SelectRow( curRow );

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::RemovePin( LIB_PIN* pin )
{
    m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellEdited( wxGridEvent& event )
{
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellSelected( wxGridEvent& event )
{
    LIB_PIN* pin = nullptr;

    if( event.GetRow() >= 0 && event.GetRow() < m_dataModel->GetNumberRows() )
    {
        const LIB_PINS& pins = m_dataModel->GetRowPins( event.GetRow() );

        if( pins.size() == 1 && m_editFrame->GetCurSymbol() )
        {
            for( LIB_PIN* candidate : m_editFrame->GetCurSymbol()->GetAllLibPins() )
            {
                if( candidate->GetNumber() == pins.at( 0 )->GetNumber() )
                {
                    pin = candidate;
                    break;
                }
            }
        }
    }

    WINDOW_THAWER thawer( m_editFrame );

    m_editFrame->FocusOnItem( pin );
    m_editFrame->GetCanvas()->Refresh();
}


bool DIALOG_LIB_EDIT_PIN_TABLE::IsDisplayGrouped()
{
    return m_cbGroup->GetValue();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnGroupSelected( wxCommandEvent& event )
{
    m_cbGroup->SetValue( false );

    m_dataModel->RebuildRows( m_pins, false, true );

    m_grid->ShowCol( COL_PIN_COUNT );
    m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    adjustGridColumns();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnRebuildRows( wxCommandEvent&  )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue(), false );

    if( m_cbGroup->GetValue() )
    {
        m_grid->ShowCol( COL_PIN_COUNT );
        m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    }

    adjustGridColumns();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnFilterCheckBox( wxCommandEvent& event )
{
    if( event.IsChecked() )
    {
        m_dataModel->SetUnitFilter( m_unitFilter->GetSelection() );
    }
    else
    {
        m_dataModel->SetUnitFilter( -1 );
        m_unitFilter->SetSelection( -1 );
    }

    OnRebuildRows( event );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnFilterChoice( wxCommandEvent& event )
{
    m_cbFilterByUnit->SetValue( true );
    m_dataModel->SetUnitFilter( m_unitFilter->GetSelection() );

    OnRebuildRows( event );
}


void DIALOG_LIB_EDIT_PIN_TABLE::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_grid ).x;

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    // The Number and Name columns must be at least wide enough to hold their contents, but
    // no less wide than their original widths.

    m_grid->AutoSizeColumn( COL_NUMBER );

    if( m_grid->GetColSize( COL_NUMBER ) < m_originalColWidths[ COL_NUMBER ] )
        m_grid->SetColSize( COL_NUMBER, m_originalColWidths[ COL_NUMBER ] );

    m_grid->AutoSizeColumn( COL_NAME );

    if( m_grid->GetColSize( COL_NAME ) < m_originalColWidths[ COL_NAME ] )
        m_grid->SetColSize( COL_NAME, m_originalColWidths[ COL_NAME ] );

    // If the grid is still wider than the columns, then stretch the Number and Name columns
    // to fit.

    for( int i = 0; i < COL_COUNT; ++i )
        width -= m_grid->GetColSize( i );

    if( width > 0 )
    {
        m_grid->SetColSize( COL_NUMBER, m_grid->GetColSize( COL_NUMBER ) + width / 2 );
        m_grid->SetColSize( COL_NAME, m_grid->GetColSize( COL_NAME ) + width / 2 );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnSize( wxSizeEvent& event )
{
    wxSize new_size = event.GetSize();

    if( m_initialized && m_size != new_size )
    {
        m_size = new_size;

        adjustGridColumns();
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    std::bitset<64> columnsShown = m_grid->GetShownColumns();

    if( columnsShown != m_columnsShown )
    {
        m_columnsShown = columnsShown;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns();
    }

    int firstSelectedRow;
    int selectedRowCount;

    getSelectedArea( m_grid, &firstSelectedRow, &selectedRowCount );

    if( ( selectedRowCount > 1 ) != m_groupSelected->IsEnabled() )
        m_groupSelected->Enable( selectedRowCount > 1 );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    int retval = wxID_CANCEL;

    if( m_dataModel->IsEdited() )
    {
        if( HandleUnsavedChanges( this, _( "Save changes?" ),
                                  [&]() -> bool
                                  {
                                      if( TransferDataFromWindow() )
                                      {
                                          retval = wxID_OK;
                                          return true;
                                      }

                                      return false;
                                  } ) )
        {
            if( IsQuasiModal() )
                EndQuasiModal( retval );
            else
                EndDialog( retval );

            return;
        }
        else
        {
            event.Veto();
            return;
        }
    }

    // No change in dialog: we can close it
    if( IsQuasiModal() )
        EndQuasiModal( retval );
    else
        EndDialog( retval );

    return;
}


void DIALOG_LIB_EDIT_PIN_TABLE::updateSummary()
{
    PIN_NUMBERS pinNumbers;

    for( LIB_PIN* pin : m_pins )
    {
        if( pin->GetNumber().Length() )
            pinNumbers.insert( pin->GetNumber() );
    }

    m_pin_numbers_summary->SetLabel( pinNumbers.GetSummary() );
    m_pin_count->SetLabel( wxString::Format( wxT( "%u" ), (unsigned) m_pins.size() ) );
    m_duplicate_pins->SetLabel( pinNumbers.GetDuplicates() );

    Layout();
}
