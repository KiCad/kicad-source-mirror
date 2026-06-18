/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "panel_symbol_pin_map.h"

#include <map>
#include <set>

#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/wupdlock.h>

#include <grid_tricks.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <string_utils.h>
#include <widgets/wx_grid.h>


/// Pad-cell colours for live validation.  Malformed bracketed syntax is flagged red; a syntactically
/// valid pad that cannot be confirmed against a footprint (footprints are not loadable in eeschema)
/// is left at the default colour rather than guessed.
static const wxColour PIN_MAP_CELL_DEFAULT = wxColour( 255, 255, 255 );
static const wxColour PIN_MAP_CELL_INVALID = wxColour( 255, 190, 190 );


// Menu ids carved out of the GRID_TRICKS client range so they coexist with the base cut/copy/paste.
enum
{
    PIN_MAP_MENU_RESET_COLUMN = GRIDTRICKS_FIRST_CLIENT_ID,
    PIN_MAP_MENU_COPY_FROM_FIRST,
    // Leave a wide gap so the copy-source range (one id per footprint column) cannot collide with
    // the bind range (one id per named map).
    PIN_MAP_MENU_BIND_FIRST = PIN_MAP_MENU_COPY_FROM_FIRST + 1000
};


/**
 * Grid context-menu helper for the pin-map grid (issue #2282).
 *
 * Inherits cut/copy/paste/select-all from GRID_TRICKS and adds the pin-map-specific column actions:
 * reset a footprint column to 1:1, copy another column's pads into it, and bind it to an existing
 * named map.
 */
class PIN_MAP_GRID_TRICKS : public GRID_TRICKS
{
public:
    PIN_MAP_GRID_TRICKS( WX_GRID* aGrid, PANEL_SYMBOL_PIN_MAP* aPanel ) :
            GRID_TRICKS( aGrid ),
            m_panel( aPanel )
    {
    }

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        const int col = aEvent.GetCol();

        m_menuCol = col;
        m_copyFromCols.clear();
        m_bindNames.clear();

        if( col >= PANEL_SYMBOL_PIN_MAP::FIXED_COLS )
        {
            menu.Append( PIN_MAP_MENU_RESET_COLUMN,
                         wxString::Format( _( "Reset '%s' to 1:1" ), m_panel->GetColumnFootprintLabel( col ) ) );

            wxMenu* copyMenu = new wxMenu;
            int     copyId = PIN_MAP_MENU_COPY_FROM_FIRST;

            for( int other = PANEL_SYMBOL_PIN_MAP::FIXED_COLS; other < m_grid->GetNumberCols(); ++other )
            {
                if( other == col )
                    continue;

                copyMenu->Append( copyId, m_panel->GetColumnFootprintLabel( other ) );
                m_copyFromCols[copyId] = other;
                ++copyId;
            }

            if( copyMenu->GetMenuItemCount() > 0 )
                menu.Append( wxID_ANY, _( "Copy From..." ), copyMenu );
            else
                delete copyMenu;

            wxMenu* bindMenu = new wxMenu;
            int     bindId = PIN_MAP_MENU_BIND_FIRST;

            for( const wxString& name : m_panel->GetMapNames() )
            {
                bindMenu->Append( bindId, name );
                m_bindNames[bindId] = name;
                ++bindId;
            }

            if( bindMenu->GetMenuItemCount() > 0 )
                menu.Append( wxID_ANY, _( "Bind to Existing Map..." ), bindMenu );
            else
                delete bindMenu;

            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& aEvent ) override
    {
        const int id = aEvent.GetId();

        if( id == PIN_MAP_MENU_RESET_COLUMN )
        {
            m_panel->ResetColumnToIdentity( m_menuCol );
        }
        else if( m_copyFromCols.count( id ) )
        {
            m_panel->CopyColumn( m_copyFromCols[id], m_menuCol );
        }
        else if( m_bindNames.count( id ) )
        {
            m_panel->BindColumnToMap( m_menuCol, m_bindNames[id] );
        }
        else
        {
            GRID_TRICKS::doPopupSelection( aEvent );
        }
    }

private:
    PANEL_SYMBOL_PIN_MAP*   m_panel;
    int                     m_menuCol = -1;
    std::map<int, int>      m_copyFromCols;
    std::map<int, wxString> m_bindNames;
};


PANEL_SYMBOL_PIN_MAP::PANEL_SYMBOL_PIN_MAP( wxWindow* aParent ) :
        PANEL_SYMBOL_PIN_MAP_BASE( aParent ),
        m_symbol( nullptr )
{
    m_grid->SetUseNativeColLabels();
    m_grid->ClearGrid();

    if( m_grid->GetNumberCols() > 0 )
        m_grid->DeleteCols( 0, m_grid->GetNumberCols() );

    m_grid->AppendCols( FIXED_COLS );
    m_grid->SetColLabelValue( 0, _( "Unit" ) );
    m_grid->SetColLabelValue( 1, _( "Symbol Pin" ) );
    m_grid->SetColLabelValue( 2, _( "Name" ) );

    m_gridTricks = std::make_unique<PIN_MAP_GRID_TRICKS>( m_grid, this );
    m_grid->PushEventHandler( m_gridTricks.get() );

    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &PANEL_SYMBOL_PIN_MAP::onCellChanged, this );
}


PANEL_SYMBOL_PIN_MAP::~PANEL_SYMBOL_PIN_MAP()
{
    // GRID_TRICKS was pushed as an event handler; pop it before it is destroyed.
    m_grid->PopEventHandler();
}


void PANEL_SYMBOL_PIN_MAP::SetSymbol( LIB_SYMBOL* aSymbol )
{
    m_symbol = aSymbol;

    m_pinNumbers.clear();
    m_pinMaps = PIN_MAP_SET();
    m_associations.clear();

    if( !m_symbol )
        return;

    std::set<wxString> seen;

    for( const LIB_SYMBOL::LOGICAL_PIN& logical : m_symbol->GetLogicalPins( 0, 0 ) )
    {
        if( seen.insert( logical.number ).second )
            m_pinNumbers.push_back( logical.number );
    }

    m_pinMaps = m_symbol->GetPinMaps();
    m_associations = m_symbol->GetAssociatedFootprints();
}


bool PANEL_SYMBOL_PIN_MAP::TransferDataToWindow()
{
    rebuildGrid();
    return true;
}


bool PANEL_SYMBOL_PIN_MAP::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( m_symbol )
        ApplyToSymbol( m_symbol );

    return true;
}


bool PANEL_SYMBOL_PIN_MAP::CommitPendingChanges()
{
    return m_grid->CommitPendingChanges();
}


void PANEL_SYMBOL_PIN_MAP::ApplyToSymbol( LIB_SYMBOL* aSymbol )
{
    if( !aSymbol )
        return;

    harvestGrid();

    aSymbol->SetPinMaps( m_pinMaps );
    aSymbol->SetAssociatedFootprints( m_associations );
}


void PANEL_SYMBOL_PIN_MAP::rebuildGrid()
{
    wxWindowUpdateLocker lock( m_grid );

    const int wantCols = FIXED_COLS + (int) m_associations.size();

    if( m_grid->GetNumberCols() > wantCols )
        m_grid->DeleteCols( wantCols, m_grid->GetNumberCols() - wantCols );
    else if( m_grid->GetNumberCols() < wantCols )
        m_grid->AppendCols( wantCols - m_grid->GetNumberCols() );

    for( size_t ii = 0; ii < m_associations.size(); ++ii )
    {
        const wxString fpName = m_associations[ii].m_FootprintLibId.GetUniStringLibId();
        m_grid->SetColLabelValue( FIXED_COLS + (int) ii, wxString::Format( _( "Pad on %s" ), fpName ) );
    }

    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    if( !m_pinNumbers.empty() )
        m_grid->AppendRows( (int) m_pinNumbers.size() );

    std::vector<wxString> pinNames( m_pinNumbers.size() );
    std::vector<int>      pinUnits( m_pinNumbers.size(), 0 );

    if( m_symbol )
    {
        for( const LIB_SYMBOL::LOGICAL_PIN& logical : m_symbol->GetLogicalPins( 0, 0 ) )
        {
            auto it = std::find( m_pinNumbers.begin(), m_pinNumbers.end(), logical.number );

            if( it == m_pinNumbers.end() )
                continue;

            const size_t row = std::distance( m_pinNumbers.begin(), it );

            if( pinNames[row].IsEmpty() && logical.pin )
            {
                pinNames[row] = logical.pin->GetName();
                pinUnits[row] = logical.pin->GetUnit();
            }
        }
    }

    for( size_t row = 0; row < m_pinNumbers.size(); ++row )
    {
        const wxString unit =
                pinUnits[row] > 0 ? wxString::Format( wxT( "%d" ), pinUnits[row] ) : wxString( wxT( "-" ) );

        m_grid->SetCellValue( (int) row, 0, unit );
        m_grid->SetCellValue( (int) row, 1, m_pinNumbers[row] );
        m_grid->SetCellValue( (int) row, 2, pinNames[row] );

        m_grid->SetReadOnly( (int) row, 0 );
        m_grid->SetReadOnly( (int) row, 1 );
        m_grid->SetReadOnly( (int) row, 2 );

        for( size_t col = 0; col < m_associations.size(); ++col )
        {
            wxString pad = m_pinNumbers[row];

            if( const PIN_MAP* map = m_pinMaps.FindByName( m_associations[col].m_MapName ) )
            {
                if( map->HasEntry( m_pinNumbers[row] ) )
                    pad = map->GetPadNumber( m_pinNumbers[row] );
            }

            m_grid->SetCellValue( (int) row, FIXED_COLS + (int) col, pad );
        }
    }

    validateAllCells();
    adjustGridColumns();
    m_removeFootprintButton->Enable( !m_associations.empty() );
}


void PANEL_SYMBOL_PIN_MAP::validateAllCells()
{
    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        for( int col = FIXED_COLS; col < m_grid->GetNumberCols(); ++col )
            ValidateCell( row, col );
    }
}


void PANEL_SYMBOL_PIN_MAP::ValidateCell( int aRow, int aCol )
{
    if( aRow < 0 || aRow >= m_grid->GetNumberRows() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    const wxString pad = m_grid->GetCellValue( aRow, aCol );

    // A blank pad means the pin resolves 1:1 by identity, which is always valid.  Footprint pad
    // existence cannot be checked here because eeschema cannot load FOOTPRINT objects, so a
    // syntactically valid pad is left at the default colour rather than guessed.  Malformed
    // bracketed stacked syntax is the one thing we can flag, via ExpandStackedPinNotation.
    bool valid = true;

    if( !pad.IsEmpty() )
        ExpandStackedPinNotation( pad, &valid );

    m_grid->SetCellBackgroundColour( aRow, aCol, valid ? PIN_MAP_CELL_DEFAULT : PIN_MAP_CELL_INVALID );
}


void PANEL_SYMBOL_PIN_MAP::onCellChanged( wxGridEvent& aEvent )
{
    const int row = aEvent.GetRow();
    const int col = aEvent.GetCol();

    // Keep columns bound to the same named map in sync, so an edit on one shared column is not lost
    // when harvestGrid() merges them (issue #2282).  SetCellValue does not re-fire this event.
    if( col >= FIXED_COLS )
    {
        const size_t index = (size_t) ( col - FIXED_COLS );

        if( index < m_associations.size() && !m_associations[index].m_MapName.IsEmpty() )
        {
            const wxString mapName = m_associations[index].m_MapName;
            const wxString value = m_grid->GetCellValue( row, col );

            for( size_t other = 0; other < m_associations.size(); ++other )
            {
                if( (int) other + FIXED_COLS == col )
                    continue;

                if( m_associations[other].m_MapName == mapName )
                {
                    m_grid->SetCellValue( row, FIXED_COLS + (int) other, value );
                    ValidateCell( row, FIXED_COLS + (int) other );
                }
            }
        }
    }

    ValidateCell( row, col );
    m_grid->ForceRefresh();
    aEvent.Skip();
}


wxString PANEL_SYMBOL_PIN_MAP::GetColumnFootprintLabel( int aCol ) const
{
    const size_t index = (size_t) ( aCol - FIXED_COLS );

    if( index >= m_associations.size() )
        return wxEmptyString;

    return m_associations[index].m_FootprintLibId.GetUniStringLibId();
}


std::vector<wxString> PANEL_SYMBOL_PIN_MAP::GetMapNames() const
{
    std::vector<wxString> names;

    for( const PIN_MAP& map : m_pinMaps.GetAll() )
        names.push_back( map.GetName() );

    return names;
}


void PANEL_SYMBOL_PIN_MAP::ResetColumnToIdentity( int aCol )
{
    if( !m_grid->CommitPendingChanges() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        m_grid->SetCellValue( row, aCol, m_grid->GetCellValue( row, 1 ) );
        ValidateCell( row, aCol );
    }

    m_grid->ForceRefresh();
}


void PANEL_SYMBOL_PIN_MAP::CopyColumn( int aSrcCol, int aDstCol )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    if( aSrcCol < FIXED_COLS || aSrcCol >= m_grid->GetNumberCols() )
        return;

    if( aDstCol < FIXED_COLS || aDstCol >= m_grid->GetNumberCols() )
        return;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        m_grid->SetCellValue( row, aDstCol, m_grid->GetCellValue( row, aSrcCol ) );
        ValidateCell( row, aDstCol );
    }

    m_grid->ForceRefresh();
}


void PANEL_SYMBOL_PIN_MAP::BindColumnToMap( int aCol, const wxString& aMapName )
{
    if( !m_grid->CommitPendingChanges() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    const size_t index = (size_t) ( aCol - FIXED_COLS );

    if( index >= m_associations.size() )
        return;

    // Harvest first so an in-progress edit on the rebound column is not lost, then point the
    // association at the chosen map and rebuild so the column shows that map's pads.
    harvestGrid();

    const PIN_MAP* target = m_pinMaps.FindByName( aMapName );

    if( !target )
        return;

    const wxString oldMapName = m_associations[index].m_MapName;
    m_associations[index].m_MapName = aMapName;

    // Drop the previous map if it was unique to this column so the set does not accumulate orphans.
    if( !oldMapName.IsEmpty() && oldMapName != aMapName )
    {
        bool stillUsed = false;

        for( const ASSOCIATED_FOOTPRINT& assoc : m_associations )
        {
            if( assoc.m_MapName == oldMapName )
            {
                stillUsed = true;
                break;
            }
        }

        if( !stillUsed )
            m_pinMaps.Remove( oldMapName );
    }

    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::adjustGridColumns()
{
    if( m_grid->GetNumberCols() == 0 )
        return;

    for( int col = 0; col < FIXED_COLS; ++col )
        m_grid->AutoSizeColumn( col, false );

    int fixedWidth = 0;

    for( int col = 0; col < FIXED_COLS; ++col )
        fixedWidth += m_grid->GetColSize( col );

    const int padCols = m_grid->GetNumberCols() - FIXED_COLS;

    if( padCols <= 0 )
        return;

    const int remaining = std::max( m_grid->GetClientSize().GetWidth() - fixedWidth, padCols * 80 );

    for( int col = FIXED_COLS; col < m_grid->GetNumberCols(); ++col )
        m_grid->SetColSize( col, remaining / padCols );
}


void PANEL_SYMBOL_PIN_MAP::harvestGrid()
{
    // Rebuild the set from the grid.  Several columns can share one named map (a single pinout
    // used by more than one footprint); merge those columns into the same PIN_MAP so an edit made
    // through one shared column is not overwritten by another column that left a cell blank.
    m_pinMaps = PIN_MAP_SET();

    for( size_t col = 0; col < m_associations.size(); ++col )
    {
        const wxString mapName = m_associations[col].m_MapName;

        if( mapName.IsEmpty() )
            continue;

        PIN_MAP* existing = m_pinMaps.FindByName( mapName );
        PIN_MAP  map = existing ? *existing : PIN_MAP( mapName );

        for( size_t row = 0; row < m_pinNumbers.size(); ++row )
        {
            const wxString pad = m_grid->GetCellValue( (int) row, FIXED_COLS + (int) col );

            if( !pad.IsEmpty() )
                map.SetEntry( m_pinNumbers[row], pad );
        }

        m_pinMaps.AddOrReplace( std::move( map ) );
    }
}


wxString PANEL_SYMBOL_PIN_MAP::makeUniqueMapName() const
{
    int suffix = 1;

    while( true )
    {
        wxString candidate = wxString::Format( wxT( "map%d" ), suffix++ );

        if( !m_pinMaps.FindByName( candidate ) )
            return candidate;
    }
}


void PANEL_SYMBOL_PIN_MAP::OnAddFootprint( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    harvestGrid();

    wxTextEntryDialog dlg( this, _( "Footprint library identifier (e.g. Library:Footprint):" ), _( "Add Footprint" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    const wxString text = dlg.GetValue().Trim().Trim( false );

    if( text.IsEmpty() )
        return;

    LIB_ID fpId;

    if( fpId.Parse( text ) >= 0 )
    {
        wxMessageBox( _( "Invalid footprint identifier." ), _( "Add Footprint" ), wxOK | wxICON_ERROR, this );
        return;
    }

    ASSOCIATED_FOOTPRINT assoc;
    assoc.m_FootprintLibId = fpId;
    assoc.m_MapName = makeUniqueMapName();

    PIN_MAP map( assoc.m_MapName );

    for( const wxString& pin : m_pinNumbers )
        map.SetEntry( pin, pin );

    m_pinMaps.AddOrReplace( std::move( map ) );
    m_associations.push_back( std::move( assoc ) );

    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::OnRemoveFootprint( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    harvestGrid();

    int col = m_grid->GetGridCursorCol();

    if( col < FIXED_COLS )
    {
        if( m_associations.empty() )
            return;

        col = m_grid->GetNumberCols() - 1;
    }

    const size_t index = (size_t) ( col - FIXED_COLS );

    if( index >= m_associations.size() )
        return;

    const wxString mapName = m_associations[index].m_MapName;
    m_associations.erase( m_associations.begin() + index );

    bool stillUsed = false;

    for( const ASSOCIATED_FOOTPRINT& assoc : m_associations )
    {
        if( assoc.m_MapName == mapName )
        {
            stillUsed = true;
            break;
        }
    }

    if( !stillUsed )
        m_pinMaps.Remove( mapName );

    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::OnSizeGrid( wxSizeEvent& aEvent )
{
    adjustGridColumns();
    aEvent.Skip();
}
