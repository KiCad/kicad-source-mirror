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

#include <algorithm>
#include <map>
#include <set>

#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/wupdlock.h>

#include <bitmaps.h>
#include <confirm.h>
#include <dialog_shim.h>
#include <grid_tricks.h>
#include <kiface_ids.h>
#include <kiplatform/ui.h>
#include <kiway.h>
#include <kiway_holder.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <string_utils.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>


// Menu ids carved out of the GRID_TRICKS client range so they coexist with the base cut/copy/paste.
enum
{
    PIN_MAP_MENU_RESET_COLUMN = GRIDTRICKS_FIRST_CLIENT_ID,
    PIN_MAP_MENU_RENAME,
    PIN_MAP_MENU_COPY_FROM_FIRST
};


/// Pin-map grid context menu: adds reset-to-1:1 and copy-from-column to GRID_TRICKS.
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

        if( col >= PANEL_SYMBOL_PIN_MAP::FIXED_COLS )
        {
            menu.Append( PIN_MAP_MENU_RENAME, _( "Rename Map..." ) );
            menu.Append( PIN_MAP_MENU_RESET_COLUMN,
                         wxString::Format( _( "Reset '%s' to 1:1" ), m_panel->GetColumnMapName( col ) ) );

            wxMenu* copyMenu = new wxMenu;
            int     copyId = PIN_MAP_MENU_COPY_FROM_FIRST;

            for( int other = PANEL_SYMBOL_PIN_MAP::FIXED_COLS; other < m_grid->GetNumberCols(); ++other )
            {
                if( other == col )
                    continue;

                copyMenu->Append( copyId, m_panel->GetColumnMapName( other ) );
                m_copyFromCols[copyId] = other;
                ++copyId;
            }

            if( copyMenu->GetMenuItemCount() > 0 )
                menu.Append( wxID_ANY, _( "Copy From..." ), copyMenu );
            else
                delete copyMenu;

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
        else if( id == PIN_MAP_MENU_RENAME )
        {
            m_panel->RenameColumn( m_menuCol );
        }
        else if( m_copyFromCols.count( id ) )
        {
            m_panel->CopyColumn( m_copyFromCols[id], m_menuCol );
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

    m_addMapButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeMapButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_gridTricks = std::make_unique<PIN_MAP_GRID_TRICKS>( m_grid, this );
    m_grid->PushEventHandler( m_gridTricks.get() );

    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &PANEL_SYMBOL_PIN_MAP::onCellChanged, this );
    m_grid->Bind( wxEVT_GRID_LABEL_LEFT_DCLICK, &PANEL_SYMBOL_PIN_MAP::onLabelDClick, this );
}


PANEL_SYMBOL_PIN_MAP::~PANEL_SYMBOL_PIN_MAP()
{
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

    const std::vector<PIN_MAP>& maps = m_pinMaps.GetAll();
    const int                   wantCols = FIXED_COLS + (int) maps.size();

    if( m_grid->GetNumberCols() > wantCols )
        m_grid->DeleteCols( wantCols, m_grid->GetNumberCols() - wantCols );
    else if( m_grid->GetNumberCols() < wantCols )
        m_grid->AppendCols( wantCols - m_grid->GetNumberCols() );

    for( size_t ii = 0; ii < maps.size(); ++ii )
        m_grid->SetColLabelValue( FIXED_COLS + (int) ii, maps[ii].GetName() );

    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    m_grid->AppendRows( (int) m_pinNumbers.size() + 1 );

    m_grid->SetCellSize( 0, 0, 1, FIXED_COLS );
    m_grid->SetCellValue( 0, 0, _( "Footprint" ) );
    m_grid->SetReadOnly( 0, 0 );

    DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) );

    for( size_t col = 0; col < maps.size(); ++col )
    {
        wxString footprint;

        for( const ASSOCIATED_FOOTPRINT& assoc : m_associations )
        {
            if( assoc.m_MapName == maps[col].GetName() )
            {
                footprint = assoc.m_FootprintLibId.GetUniStringLibId();
                break;
            }
        }

        m_grid->SetCellValue( 0, FIXED_COLS + (int) col, footprint );

        if( dlg )
            m_grid->SetCellEditor( 0, FIXED_COLS + (int) col, new GRID_CELL_FPID_EDITOR( dlg, wxEmptyString ) );
        else
            m_grid->SetReadOnly( 0, FIXED_COLS + (int) col );
    }

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
        const int      gridRow = (int) row + 1;
        const wxString unit =
                pinUnits[row] > 0 ? wxString::Format( wxT( "%d" ), pinUnits[row] ) : wxString( wxT( "-" ) );

        m_grid->SetCellValue( gridRow, 0, unit );
        m_grid->SetCellValue( gridRow, 1, m_pinNumbers[row] );
        m_grid->SetCellValue( gridRow, 2, pinNames[row] );

        m_grid->SetReadOnly( gridRow, 0 );
        m_grid->SetReadOnly( gridRow, 1 );
        m_grid->SetReadOnly( gridRow, 2 );

        for( size_t col = 0; col < maps.size(); ++col )
        {
            wxString pad = m_pinNumbers[row];

            if( maps[col].HasEntry( m_pinNumbers[row] ) )
                pad = maps[col].GetPadNumber( m_pinNumbers[row] );

            m_grid->SetCellValue( gridRow, FIXED_COLS + (int) col, pad );
        }
    }

    validateAllCells();
    adjustGridColumns();
    m_removeMapButton->Enable( !maps.empty() );
}


void PANEL_SYMBOL_PIN_MAP::validateAllCells()
{
    for( int row = 1; row < m_grid->GetNumberRows(); ++row )
    {
        for( int col = FIXED_COLS; col < m_grid->GetNumberCols(); ++col )
            ValidateCell( row, col );
    }
}


void PANEL_SYMBOL_PIN_MAP::ValidateCell( int aRow, int aCol )
{
    if( aRow < 1 || aRow >= m_grid->GetNumberRows() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    const wxString pad = m_grid->GetCellValue( aRow, aCol );

    // A blank pad means the pin resolves 1:1 by identity, which is always valid.  Malformed bracketed
    // stacked syntax is flagged red.  A syntactically valid pad absent from the map's footprints is
    // flagged yellow.
    bool                  valid = true;
    std::vector<wxString> expanded;

    if( !pad.IsEmpty() )
        expanded = ExpandStackedPinNotation( pad, &valid );

    const wxColour defaultColour = m_grid->GetDefaultCellBackgroundColour();
    wxColour       background = defaultColour;

    if( !valid )
    {
        background.Set( 255, 190, 190 );
    }
    else if( !expanded.empty() )
    {
        const size_t                index = (size_t) ( aCol - FIXED_COLS );
        const std::vector<PIN_MAP>& maps = m_pinMaps.GetAll();

        if( index < maps.size() )
        {
            std::set<wxString> pads;

            for( const ASSOCIATED_FOOTPRINT& assoc : m_associations )
            {
                if( assoc.m_MapName != maps[index].GetName() )
                    continue;

                const std::set<wxString>& fpPads = padNumbersFor( assoc.m_FootprintLibId.GetUniStringLibId() );
                pads.insert( fpPads.begin(), fpPads.end() );
            }

            if( !pads.empty()
                && std::any_of( expanded.begin(), expanded.end(),
                                [&]( const wxString& p )
                                {
                                    return !pads.count( p );
                                } ) )
            {
                background.Set( 250, 230, 150 );
            }
        }
    }

    if( background != defaultColour && KIPLATFORM::UI::IsDarkTheme() )
        background = background.ChangeLightness( 50 );

    m_grid->SetCellBackgroundColour( aRow, aCol, background );
}


void PANEL_SYMBOL_PIN_MAP::onCellChanged( wxGridEvent& aEvent )
{
    if( aEvent.GetRow() == 0 && aEvent.GetCol() >= FIXED_COLS )
    {
        applyColumnFootprint( aEvent.GetCol(), m_grid->GetCellValue( 0, aEvent.GetCol() ) );
        aEvent.Skip();
        return;
    }

    ValidateCell( aEvent.GetRow(), aEvent.GetCol() );
    m_grid->ForceRefresh();
    aEvent.Skip();
}


void PANEL_SYMBOL_PIN_MAP::onLabelDClick( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() >= FIXED_COLS )
        RenameColumn( aEvent.GetCol() );
    else
        aEvent.Skip();
}


wxString PANEL_SYMBOL_PIN_MAP::GetColumnMapName( int aCol ) const
{
    const size_t                index = (size_t) ( aCol - FIXED_COLS );
    const std::vector<PIN_MAP>& maps = m_pinMaps.GetAll();

    if( index >= maps.size() )
        return wxEmptyString;

    return maps[index].GetName();
}


void PANEL_SYMBOL_PIN_MAP::ResetColumnToIdentity( int aCol )
{
    if( !m_grid->CommitPendingChanges() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    for( int row = 1; row < m_grid->GetNumberRows(); ++row )
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

    for( int row = 1; row < m_grid->GetNumberRows(); ++row )
    {
        m_grid->SetCellValue( row, aDstCol, m_grid->GetCellValue( row, aSrcCol ) );
        ValidateCell( row, aDstCol );
    }

    m_grid->ForceRefresh();
}


void PANEL_SYMBOL_PIN_MAP::RenameColumn( int aCol )
{
    if( !m_grid->CommitPendingChanges() || aCol < FIXED_COLS || aCol >= m_grid->GetNumberCols() )
        return;

    const wxString oldName = GetColumnMapName( aCol );

    if( oldName.IsEmpty() )
        return;

    wxTextEntryDialog dlg( this, _( "Map name:" ), _( "Rename Map" ), oldName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    const wxString newName = dlg.GetValue().Trim().Trim( false );

    if( newName.IsEmpty() || newName == oldName )
        return;

    if( m_pinMaps.FindByName( newName ) )
    {
        wxMessageBox( wxString::Format( _( "A pin map named '%s' already exists." ), newName ), _( "Rename Map" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    harvestGrid();

    if( PIN_MAP* map = m_pinMaps.FindByName( oldName ) )
        map->SetName( newName );

    for( ASSOCIATED_FOOTPRINT& assoc : m_associations )
    {
        if( assoc.m_MapName == oldName )
            assoc.m_MapName = newName;
    }

    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::applyColumnFootprint( int aCol, const wxString& aFootprintId )
{
    const wxString mapName = GetColumnMapName( aCol );

    if( mapName.IsEmpty() )
        return;

    auto currentFootprint = [&]() -> wxString
    {
        for( const ASSOCIATED_FOOTPRINT& a : m_associations )
        {
            if( a.m_MapName == mapName )
                return a.m_FootprintLibId.GetUniStringLibId();
        }

        return wxEmptyString;
    };

    wxString fpid = aFootprintId;
    fpid.Trim().Trim( false );

    LIB_ID fpId;

    if( !fpid.IsEmpty() && fpId.Parse( fpid ) >= 0 )
    {
        wxMessageBox( _( "Invalid footprint identifier." ), _( "Assign Footprint" ), wxOK | wxICON_ERROR, this );
        m_grid->SetCellValue( 0, aCol, currentFootprint() );
        return;
    }

    for( const ASSOCIATED_FOOTPRINT& a : m_associations )
    {
        if( a.m_MapName != mapName && a.m_FootprintLibId == fpId && !fpid.IsEmpty() )
        {
            wxMessageBox( _( "That footprint is already assigned to another pin map." ), _( "Assign Footprint" ),
                          wxOK | wxICON_INFORMATION, this );
            m_grid->SetCellValue( 0, aCol, currentFootprint() );
            return;
        }
    }

    m_associations.erase( std::remove_if( m_associations.begin(), m_associations.end(),
                                          [&]( const ASSOCIATED_FOOTPRINT& a )
                                          {
                                              return a.m_MapName == mapName;
                                          } ),
                          m_associations.end() );

    if( !fpid.IsEmpty() )
    {
        ASSOCIATED_FOOTPRINT assoc;
        assoc.m_FootprintLibId = fpId;
        assoc.m_MapName = mapName;
        m_associations.push_back( std::move( assoc ) );

        m_grid->SetCellValue( 0, aCol, fpId.GetUniStringLibId() );
    }
    else
    {
        m_grid->SetCellValue( 0, aCol, wxEmptyString );
    }

    for( int row = 1; row < m_grid->GetNumberRows(); ++row )
        ValidateCell( row, aCol );

    m_grid->ForceRefresh();
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
    // Preserve maps no footprint references so a free-standing map survives a round-trip.
    // Associations are kept as-is and edited separately.
    std::vector<wxString> names;

    for( const PIN_MAP& map : m_pinMaps.GetAll() )
        names.push_back( map.GetName() );

    PIN_MAP_SET rebuilt;

    for( size_t col = 0; col < names.size(); ++col )
    {
        PIN_MAP map( names[col] );

        for( size_t row = 0; row < m_pinNumbers.size(); ++row )
        {
            const wxString pad = m_grid->GetCellValue( (int) row + 1, FIXED_COLS + (int) col );

            if( !pad.IsEmpty() && pad != m_pinNumbers[row] )
                map.SetEntry( m_pinNumbers[row], pad );
        }

        rebuilt.AddOrReplace( std::move( map ) );
    }

    m_pinMaps = std::move( rebuilt );
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


const std::set<wxString>& PANEL_SYMBOL_PIN_MAP::padNumbersFor( const wxString& aFootprintId )
{
    auto it = m_footprintPads.find( aFootprintId );

    if( it != m_footprintPads.end() )
        return it->second;

    std::set<wxString>& pads = m_footprintPads[aFootprintId];

    if( aFootprintId.IsEmpty() )
        return pads;

    if( KIWAY_HOLDER* holder = dynamic_cast<KIWAY_HOLDER*>( wxGetTopLevelParent( this ) ) )
    {
        if( KIFACE* cvpcb = holder->Kiway().KiFACE( KIWAY::FACE_CVPCB ) )
        {
            typedef void ( *PAD_NUMBERS_FN_PTR )( const wxString&, PROJECT*, std::set<wxString>& );

            if( auto fetch = (PAD_NUMBERS_FN_PTR) cvpcb->IfaceOrAddress( KIFACE_FOOTPRINT_PAD_NUMBERS ) )
                fetch( aFootprintId, &holder->Prj(), pads );
        }
    }

    return pads;
}


void PANEL_SYMBOL_PIN_MAP::OnAddMap( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    harvestGrid();
    m_pinMaps.AddOrReplace( PIN_MAP( makeUniqueMapName() ) );
    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::OnRemoveMap( wxCommandEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    const std::vector<PIN_MAP>& maps = m_pinMaps.GetAll();

    if( maps.empty() )
        return;

    int col = m_grid->GetGridCursorCol();

    if( col < FIXED_COLS )
    {
        if( maps.size() > 1 )
        {
            wxMessageBox( _( "Click a pin map column to choose which map to remove." ), _( "Remove Pin Map" ),
                          wxOK | wxICON_INFORMATION, this );
            return;
        }

        col = FIXED_COLS;
    }

    const size_t index = (size_t) ( col - FIXED_COLS );

    if( index >= maps.size() )
        return;

    const wxString mapName = maps[index].GetName();

    bool hasAssociation = std::any_of( m_associations.begin(), m_associations.end(),
                                       [&]( const ASSOCIATED_FOOTPRINT& a )
                                       {
                                           return a.m_MapName == mapName;
                                       } );

    if( hasAssociation
        && !IsOK( this, wxString::Format( _( "Remove pin map '%s' and its footprint association?" ), mapName ) ) )
    {
        return;
    }

    harvestGrid();

    m_pinMaps.Remove( mapName );

    m_associations.erase( std::remove_if( m_associations.begin(), m_associations.end(),
                                          [&]( const ASSOCIATED_FOOTPRINT& a )
                                          {
                                              return a.m_MapName == mapName;
                                          } ),
                          m_associations.end() );

    rebuildGrid();
}


void PANEL_SYMBOL_PIN_MAP::OnSizeGrid( wxSizeEvent& aEvent )
{
    adjustGridColumns();
    aEvent.Skip();
}
