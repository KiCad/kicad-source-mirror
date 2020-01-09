/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "lib_pin.h"
#include "pin_number.h"
#include "grid_tricks.h"
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/wx_grid.h>
#include <queue>
#include <base_units.h>
#include <bitmaps.h>
#include <wx/bmpcbox.h>
#include <kiface_i.h>
#include <kicad_string.h>
#include <confirm.h>
#include <lib_edit_frame.h>

#define PinTableShownColumnsKey    wxT( "PinTableShownColumns" )


static std::vector<BITMAP_DEF> g_typeIcons;
static wxArrayString           g_typeNames;

static std::vector<BITMAP_DEF> g_shapeIcons;
static wxArrayString           g_shapeNames;

static std::vector<BITMAP_DEF> g_orientationIcons;
static wxArrayString           g_orientationNames;


class PIN_TABLE_DATA_MODEL : public wxGridTableBase
{

private:
    // Because the rows of the grid can either be a single pin or a group of pins, the
    // data model is a 2D vector.  If we're in the single pin case, each row's LIB_PINS
    // contains only a single pin.
    std::vector<LIB_PINS> m_rows;

    EDA_UNITS m_userUnits;
    bool      m_edited;

public:
    PIN_TABLE_DATA_MODEL( EDA_UNITS aUserUnits ) : m_userUnits( aUserUnits ), m_edited( false )
    {
    }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
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
        default:               wxFAIL; return wxEmptyString;
        }
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        return GetValue( m_rows[ aRow ], aCol, m_userUnits );
    }

    static wxString GetValue( const LIB_PINS& pins, int aCol, EDA_UNITS aUserUnits )
    {
        wxString fieldValue;

        if( pins.empty())
            return fieldValue;

        for( LIB_PIN* pin : pins )
        {
            wxString val;

            switch( aCol )
            {
            case COL_NUMBER:
                val = pin->GetNumber();
                break;
            case COL_NAME:
                val = pin->GetName();
                break;
            case COL_TYPE:
                val = g_typeNames[ static_cast<int>( pin->GetType() ) ];
                break;
            case COL_SHAPE:
                val = g_shapeNames[ static_cast<int>( pin->GetShape() ) ];
                break;
            case COL_ORIENTATION:
                if( LIB_PIN::GetOrientationIndex( pin->GetOrientation() ) >= 0 )
                    val = g_orientationNames[ LIB_PIN::GetOrientationIndex( pin->GetOrientation() ) ];
                break;
            case COL_NUMBER_SIZE:
                val = StringFromValue( aUserUnits, pin->GetNumberTextSize(), true, true );
                break;
            case COL_NAME_SIZE:
                val = StringFromValue( aUserUnits, pin->GetNameTextSize(), true, true );
                break;
            case COL_LENGTH:
                val = StringFromValue( aUserUnits, pin->GetLength(), true );
                break;
            case COL_POSX:
                val = StringFromValue( aUserUnits, pin->GetPosition().x, true );
                break;
            case COL_POSY:
                val = StringFromValue( aUserUnits, pin->GetPosition().y, true );
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
                    fieldValue = INDETERMINATE;
            }
        }

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aValue == INDETERMINATE )
            return;

        LIB_PINS pins = m_rows[ aRow ];

        for( LIB_PIN* pin : pins )
        {
            switch( aCol )
            {
            case COL_NUMBER:
                pin->SetNumber( aValue );
                break;
            case COL_NAME:
                pin->SetName( aValue );
                break;
            case COL_TYPE:
                if( g_typeNames.Index( aValue ) != wxNOT_FOUND )
                    pin->SetType( (ELECTRICAL_PINTYPE) g_typeNames.Index( aValue ), false );

                break;
            case COL_SHAPE:
                if( g_shapeNames.Index( aValue ) != wxNOT_FOUND )
                    pin->SetShape( (GRAPHIC_PINSHAPE) g_shapeNames.Index( aValue ) );

                break;
            case COL_ORIENTATION:
                if( g_orientationNames.Index( aValue ) != wxNOT_FOUND )
                    pin->SetOrientation( LIB_PIN::GetOrientationCode(
                                              g_orientationNames.Index( aValue ) ), false );
                break;
            case COL_NUMBER_SIZE:
                pin->SetNumberTextSize( ValueFromString( m_userUnits, aValue, true ) );
                break;
            case COL_NAME_SIZE:
                pin->SetNameTextSize( ValueFromString( m_userUnits, aValue, true ) );
                break;
            case COL_LENGTH:
                pin->SetLength( ValueFromString( m_userUnits, aValue ) );
                break;
            case COL_POSX:
                pin->SetPinPosition( wxPoint( ValueFromString( m_userUnits, aValue ),
                                              pin->GetPosition().y ) );
                break;
            case COL_POSY:
                pin->SetPinPosition( wxPoint( pin->GetPosition().x,
                                              ValueFromString( m_userUnits, aValue ) ) );
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

    static bool compare(
            const LIB_PINS& lhs, const LIB_PINS& rhs, int sortCol, bool ascending, EDA_UNITS units )
    {
        wxString lhStr = GetValue( lhs, sortCol, units );
        wxString rhStr = GetValue( rhs, sortCol, units );

        if( lhStr == rhStr )
        {
            // Secondary sort key is always COL_NUMBER
            sortCol = COL_NUMBER;
            lhStr = GetValue( lhs, sortCol, units );
            rhStr = GetValue( rhs, sortCol, units );
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
            res = cmp( PinNumbers::Compare( lhStr, rhStr ), 0 );
            break;
        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
            res = cmp( ValueFromString( units, lhStr, true ),
                    ValueFromString( units, rhStr, true ) );
            break;
        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            res = cmp( ValueFromString( units, lhStr ), ValueFromString( units, rhStr ) );
            break;
        default:
            res = cmp( StrNumCmp( lhStr, rhStr ), 0 );
            break;
        }

        return res;
    }

    void RebuildRows( LIB_PINS& aPins, bool groupByName )
    {
        if ( GetView() )
        {
            // Commit any pending in-place edits before the row gets moved out from under
            // the editor.
            if( auto grid = dynamic_cast<WX_GRID*>( GetView() ) )
                grid->CommitPendingChanges( true );

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        for( LIB_PIN* pin : aPins )
        {
            int      rowIndex = -1;

            if( groupByName )
                rowIndex = findRow( m_rows, pin->GetName() );

            if( rowIndex < 0 )
            {
                m_rows.emplace_back( LIB_PINS() );
                rowIndex = m_rows.size() - 1;
            }

            m_rows[ rowIndex ].push_back( pin );
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

        SortRows( sortCol, ascending );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }
    }

    void SortRows( int aSortCol, bool ascending )
    {
        std::sort( m_rows.begin(), m_rows.end(),
                   [ aSortCol, ascending, this ]( const LIB_PINS& lhs, const LIB_PINS& rhs ) -> bool
                   {
                       return compare( lhs, rhs, aSortCol, ascending, m_userUnits );
                   } );
    }

    void SortPins( LIB_PINS& aRow )
    {
        std::sort( aRow.begin(), aRow.end(),
                   []( LIB_PIN* lhs, LIB_PIN* rhs ) -> bool
                   {
                       return PinNumbers::Compare( lhs->GetNumber(), rhs->GetNumber() ) < 0;
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

    bool IsEdited()
    {
        return m_edited;
    }
};


DIALOG_LIB_EDIT_PIN_TABLE::DIALOG_LIB_EDIT_PIN_TABLE( LIB_EDIT_FRAME* parent, LIB_PART* aPart ) :
    DIALOG_LIB_EDIT_PIN_TABLE_BASE( parent ),
    m_editFrame( parent ),
    m_part( aPart )
{
    m_config = Kiface().KifaceSettings();

    if( g_typeNames.empty())
    {
        for( unsigned i = 0; i < PINTYPE_COUNT; ++i )
            g_typeIcons.push_back( GetBitmap( static_cast<ELECTRICAL_PINTYPE>( i ) ) );
        for( unsigned i = 0; i < PINTYPE_COUNT; ++i )
            g_typeNames.push_back( GetText( static_cast<ELECTRICAL_PINTYPE>( i ) ) );
        g_typeNames.push_back( INDETERMINATE );

        for( unsigned i = 0; i < static_cast<int>(GRAPHIC_PINSHAPE::NUM_OPTIONS ); ++i )
            g_shapeIcons.push_back( GetBitmap( static_cast<GRAPHIC_PINSHAPE>( i ) ) );
        for( unsigned i = 0; i < static_cast<int>(GRAPHIC_PINSHAPE::NUM_OPTIONS); ++i )
            g_shapeNames.push_back( GetText( static_cast<GRAPHIC_PINSHAPE>( i ) ) );
        g_shapeNames.push_back( INDETERMINATE );

        for( unsigned i = 0; i < LIB_PIN::GetOrientationNames().size(); ++i )
            g_orientationIcons.push_back( LIB_PIN::GetOrientationSymbols()[ i ] );
        g_orientationNames = LIB_PIN::GetOrientationNames();
        g_orientationNames.push_back( INDETERMINATE );
    }

    m_dataModel = new PIN_TABLE_DATA_MODEL( GetUserUnits() );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_dataModel );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    // Show/hide columns according to the user's preference
    m_config->Read( PinTableShownColumnsKey, &m_columnsShown, wxT( "0 1 2 3 4 8 9" ) );
    m_grid->ShowHideColumns( m_columnsShown );

    // Set special attributes
    wxGridCellAttr* attr;

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_typeIcons, g_typeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_typeIcons, g_typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_shapeIcons, g_shapeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_shapeIcons, g_shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( g_orientationIcons, g_orientationNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( g_orientationIcons, g_orientationNames ) );
    m_grid->SetColAttr( COL_ORIENTATION, attr );

    /* Right-aligned position values look much better, but only MSW and GTK2+
     * currently support righ-aligned textEditCtrls, so the text jumps on all
     * the other platforms when you edit it.
    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSX, attr );

    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSY, attr );
    */

    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_deleteButton->SetBitmap( KiBitmap( trash_xpm ) );
    m_refreshButton->SetBitmap( KiBitmap( refresh_xpm ) );

    GetSizer()->SetSizeHints(this);
    Centre();

    m_ButtonsOK->SetDefault();
    m_initialized = true;
    m_modified = false;
    m_width = 0;

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );
}


DIALOG_LIB_EDIT_PIN_TABLE::~DIALOG_LIB_EDIT_PIN_TABLE()
{
    m_config->Write( PinTableShownColumnsKey, m_grid->GetShownColumns() );

    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT, wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_dataModel );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // This is our copy of the pins.  If they were transfered to the part on an OK, then
    // m_pins will already be empty.
    for( auto pin : m_pins )
        delete pin;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataToWindow()
{
    // Make a copy of the pins for editing
    for( LIB_PIN* pin = m_part->GetNextPin( nullptr ); pin; pin = m_part->GetNextPin( pin ) )
        m_pins.push_back( new LIB_PIN( *pin ) );

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue() );

    updateSummary();

    return true;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Delete the part's pins
    while( LIB_PIN* pin = m_part->GetNextPin( nullptr ) )
        m_part->RemoveDrawItem( pin );

    // Transfer our pins to the part
    for( LIB_PIN* pin : m_pins )
    {
        pin->SetParent( m_part );
        m_part->AddDrawItem( pin );
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

    LIB_PIN* newPin = new LIB_PIN( nullptr );

    if( m_pins.size() > 0 )
    {
        LIB_PIN* last = m_pins.back();

        newPin->SetOrientation( last->GetOrientation() );
        newPin->SetType( last->GetType() );
        newPin->SetShape( last->GetShape() );

        wxPoint pos = last->GetPosition();

        if( last->GetOrientation() == PIN_LEFT || last->GetOrientation() == PIN_RIGHT )
            pos.y -= m_editFrame->GetRepeatPinStep();
        else
            pos.x += m_editFrame->GetRepeatPinStep();

        newPin->SetPosition( pos );
    }

    m_pins.push_back( newPin );

    m_dataModel->AppendRow( m_pins[ m_pins.size() - 1 ] );

    m_grid->MakeCellVisible( m_grid->GetNumberRows() - 1, 0 );
    m_grid->SetGridCursor( m_grid->GetNumberRows() - 1, 0 );

    m_grid->EnableCellEditControl( true );
    m_grid->ShowCellEditControl();

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnDeleteRow( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    if( m_pins.size() == 0 )   // empty table
        return;

    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    LIB_PINS removedRow = m_dataModel->RemoveRow( curRow );

    for( auto pin : removedRow )
        m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );

    curRow = std::max( 0, curRow - 1 );
    m_grid->MakeCellVisible( curRow, m_grid->GetGridCursorCol() );
    m_grid->SetGridCursor( curRow, m_grid->GetGridCursorCol() );

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellEdited( wxGridEvent& event )
{
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnRebuildRows( wxCommandEvent&  )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue() );

    adjustGridColumns( m_grid->GetRect().GetWidth() );
}


void DIALOG_LIB_EDIT_PIN_TABLE::adjustGridColumns( int aWidth )
{
    m_width = aWidth;

    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

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
        aWidth -= m_grid->GetColSize( i );

    if( aWidth > 0 )
    {
        m_grid->SetColSize( COL_NUMBER, m_grid->GetColSize( COL_NUMBER ) + aWidth / 2 );
        m_grid->SetColSize( COL_NAME, m_grid->GetColSize( COL_NAME ) + aWidth / 2 );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnSize( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_initialized && m_width != new_size )
    {
        adjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString columnsShown = m_grid->GetShownColumns();

    if( columnsShown != m_columnsShown )
    {
        m_columnsShown = columnsShown;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns( m_grid->GetRect().GetWidth() );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() )
    {
        if( !HandleUnsavedChanges( this, wxEmptyString,
                                   [&]()->bool { return TransferDataFromWindow(); } ) )
        {
            event.Veto();
            return;
        }
    }

    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );
    else if( IsModal() )
        EndModal( wxID_CANCEL );
    else
        event.Skip();
}


void DIALOG_LIB_EDIT_PIN_TABLE::updateSummary()
{
    PinNumbers pinNumbers;

    for( LIB_PIN* pin : m_pins )
    {
        if( pin->GetNumber().Length() )
            pinNumbers.insert( pin->GetNumber() );
    }

    m_summary->SetLabel( pinNumbers.GetSummary() );
}
