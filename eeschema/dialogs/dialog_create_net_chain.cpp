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

#include "dialog_create_net_chain.h"

#include <algorithm>
#include <map>

#include <wx/msgdlg.h>

#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <connection_graph.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_edit_frame.h>
#include <sch_field.h>
#include <sch_netchain.h>
#include <sch_pin.h>
#include <sch_reference_list.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <view/view.h>


DIALOG_CREATE_NET_CHAIN::DIALOG_CREATE_NET_CHAIN( SCH_EDIT_FRAME* aParent, const wxString& aFromRef,
                                                  const wxString& aToRef ) :
        DIALOG_CREATE_NET_CHAIN_BASE( aParent ),
        m_frame( aParent )
{
    m_refreshButton->SetBitmap( KiBitmapBundle( BITMAPS::refresh ) );
    m_findPathButton->SetBitmap( KiBitmapBundle( BITMAPS::find ) );

    // Override hardcoded wxFormBuilder font with system default bold
    m_manualLabel->SetFont( GetFont().Bold() );

    m_chainsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_chainsGrid->EnableEditing( true ); // Override wxFB default; per-cell ReadOnly controls editability

    // Set placeholder hint on filter (wxFormBuilder only sets tooltip, not hint)
    m_filterInput->SetHint( _( "Filter chains by name or net..." ) );

    populateComponentCombos();

    // Pre-seed From/To from caller (e.g. current selection)
    if( !aFromRef.IsEmpty() )
        m_fromComponent->SetValue( aFromRef );

    if( !aToRef.IsEmpty() )
        m_toComponent->SetValue( aToRef );

    SetupStandardButtons();

    // Rename buttons after SetupStandardButtons() which resets labels
    m_sdbSizerOK->SetLabel( _( "Create" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    m_chainsGrid->Bind( wxEVT_GRID_CELL_CHANGED,
            [this]( wxGridEvent& evt )
            {
                if( evt.GetCol() == 0 )
                    m_nameInput->SetValue( m_chainsGrid->GetCellValue( evt.GetRow(), 0 ) );
            } );

    loadPotentials();
    rebuildGrid();

    // If both endpoints were pre-seeded, auto-trigger Find Path
    if( !aFromRef.IsEmpty() && !aToRef.IsEmpty() )
    {
        wxCommandEvent dummy;
        OnFindPathClicked( dummy );
    }

    finishDialogSettings();
}


DIALOG_CREATE_NET_CHAIN::~DIALOG_CREATE_NET_CHAIN()
{
    // Clear all highlighting when dialog closes
    highlightChainNets( {} );
}


bool DIALOG_CREATE_NET_CHAIN::TransferDataToWindow()
{
    return true;
}


bool DIALOG_CREATE_NET_CHAIN::TransferDataFromWindow()
{
    if( !validateAndCreate() )
        return false;

    // Refresh grid, then notify (OnModify may trigger recalculation).
    loadPotentials();
    m_filteredIndices.clear();
    rebuildGrid();
    m_nameInput->Clear();
    m_frame->OnModify();

    m_headerLabel->SetLabel(
            wxString::Format( _( "Chain created (%d total). Select another or close." ), m_createdCount ) );

    // Return false to keep the dialog open for multi-create
    return false;
}


bool DIALOG_CREATE_NET_CHAIN::validateAndCreate()
{
    int gridRow = selectedRow();

    if( gridRow < 0 || gridRow >= static_cast<int>( m_filteredIndices.size() ) )
    {
        wxMessageBox( _( "Please select a chain from the list." ), _( "Create Net Chain" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    int dataIdx = m_filteredIndices[gridRow];

    // Sync the edited name from the grid cell before using it
    wxString editedName = m_chainsGrid->GetCellValue( gridRow, 0 );

    if( !editedName.IsEmpty() )
        m_rows[dataIdx].suggestedName = editedName;

    // Use the Chain Name input field (or fall back to the grid cell name)
    wxString name = m_nameInput->GetValue().Trim().Trim( false );

    if( name.IsEmpty() )
        name = m_rows[dataIdx].suggestedName;

    if( name.IsEmpty() )
    {
        wxMessageBox( _( "Please enter a name for the net chain." ), _( "Create Net Chain" ), wxOK | wxICON_ERROR,
                      this );
        return false;
    }

    if( !SCH_NETCHAIN::IsValidName( name ) )
    {
        wxMessageBox( _( "Chain name cannot contain spaces, quotes, or parentheses." ),
                      _( "Create Net Chain" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
    {
        wxMessageBox( _( "Connection graph not available." ), _( "Create Net Chain" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    if( graph->GetNetChainByName( name ) )
    {
        wxMessageBox( wxString::Format( _( "A net chain named '%s' already exists." ), name ), _( "Create Net Chain" ),
                      wxOK | wxICON_ERROR, this );
        return false;
    }

    const POTENTIAL_ROW& prow = m_rows[dataIdx];

    if( prow.livePtr )
    {
        SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( prow.livePtr, name );

        if( !committed )
        {
            wxMessageBox( _( "Failed to create the net chain." ), _( "Create Net Chain" ), wxOK | wxICON_ERROR, this );
            return false;
        }
    }
    else
    {
        graph->AddNetChain( prow.forceFromUuid, prow.forceToUuid, name );
    }

    m_createdCount++;
    return true;
}


void DIALOG_CREATE_NET_CHAIN::OnChainSelected( wxGridEvent& aEvent )
{
    if( m_rebuilding )
    {
        aEvent.Skip();
        return;
    }

    int gridRow = aEvent.GetRow();

    // Map grid row to m_rows index through filter
    int dataIdx = ( gridRow >= 0 && gridRow < static_cast<int>( m_filteredIndices.size() ) )
                          ? m_filteredIndices[gridRow]
                          : -1;

    updateMemberDetail( dataIdx );

    if( dataIdx >= 0 )
    {
        // Sync the edited name from the grid cell (user may have edited column 0)
        wxString editedName = m_chainsGrid->GetCellValue( gridRow, 0 );

        if( !editedName.IsEmpty() )
            m_rows[dataIdx].suggestedName = editedName;

        m_nameInput->SetValue( m_rows[dataIdx].suggestedName );

        highlightChainNets( m_rows[dataIdx].memberNets );
    }
    else
    {
        highlightChainNets( {} );
    }

    aEvent.Skip();
}


void DIALOG_CREATE_NET_CHAIN::OnRefreshClicked( wxCommandEvent& aEvent )
{
    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( graph )
    {
        m_rows.clear();
        graph->Recalculate( m_frame->Schematic().BuildSheetListSortedByPageNumbers(), true );
        populateComponentCombos();
        loadPotentials();
        rebuildGrid();
    }
}


void DIALOG_CREATE_NET_CHAIN::OnFindPathClicked( wxCommandEvent& aEvent )
{
    wxString fromRef = m_fromComponent->GetValue().Trim().Trim( false );
    wxString toRef = m_toComponent->GetValue().Trim().Trim( false );

    if( fromRef.IsEmpty() || toRef.IsEmpty() )
    {
        wxMessageBox( _( "Please select both a From and To component." ), _( "Find Path" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    if( fromRef == toRef )
    {
        wxMessageBox( _( "From and To must be different components." ), _( "Find Path" ),
                      wxOK | wxICON_ERROR, this );
        return;
    }

    // Find the symbols by reference
    SCH_REFERENCE_LIST refs;
    m_frame->Schematic().Hierarchy().GetSymbols( refs, SYMBOL_FILTER_ALL );

    SCH_SYMBOL*    fromSym = nullptr;
    SCH_SYMBOL*    toSym = nullptr;
    SCH_SHEET_PATH fromSheet, toSheet;

    for( const SCH_REFERENCE& ref : refs )
    {
        if( ref.GetRef() == fromRef && ref.GetSymbol() )
        {
            fromSym = ref.GetSymbol();
            fromSheet = ref.GetSheetPath();
        }

        if( ref.GetRef() == toRef && ref.GetSymbol() )
        {
            toSym = ref.GetSymbol();
            toSheet = ref.GetSheetPath();
        }
    }

    if( !fromSym || !toSym )
    {
        wxMessageBox( wxString::Format( _( "Could not find component '%s' or '%s'." ),
                                        fromRef, toRef ),
                      _( "Find Path" ), wxOK | wxICON_ERROR, this );
        return;
    }

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return;

    // Find ALL unique chains between all pin pairs of the two components
    struct FOUND_CHAIN
    {
        SCH_NETCHAIN*      chain;
        wxString           fromPin;
        wxString           toPin;
        std::set<wxString> nets;
    };

    std::vector<FOUND_CHAIN> foundChains;
    std::set<SCH_NETCHAIN*>  seenChains;

    std::vector<SCH_PIN*> fromPins = fromSym->GetPins( &fromSheet );
    std::vector<SCH_PIN*> toPins = toSym->GetPins( &toSheet );

    for( SCH_PIN* pinA : fromPins )
    {
        for( SCH_PIN* pinB : toPins )
        {
            SCH_NETCHAIN* chain = graph->FindPotentialNetChainBetweenPins( pinA, pinB );

            if( chain && !seenChains.count( chain ) )
            {
                seenChains.insert( chain );

                FOUND_CHAIN fc;
                fc.chain = chain;
                fc.fromPin = fromRef + wxT( ":" ) + pinA->GetNumber();
                fc.toPin = toRef + wxT( ":" ) + pinB->GetNumber();
                fc.nets = chain->GetNets();
                foundChains.push_back( fc );
            }
        }
    }

    if( foundChains.empty() )
    {
        // No potential chain found — offer force-create
        int answer = wxMessageBox(
                wxString::Format( _( "No signal path found between %s and %s through "
                                     "passthrough components.\n\n"
                                     "Would you like to force-create a manual chain link "
                                     "between these components?" ),
                                  fromRef, toRef ),
                _( "Find Path" ), wxYES_NO | wxICON_QUESTION, this );

        if( answer == wxYES )
        {
            std::set<wxString> fromNets, toNets;

            for( SCH_PIN* pin : fromPins )
            {
                if( pin->Connection() && !pin->Connection()->Name().IsEmpty() )
                    fromNets.insert( pin->Connection()->Name() );
            }

            for( SCH_PIN* pin : toPins )
            {
                if( pin->Connection() && !pin->Connection()->Name().IsEmpty() )
                    toNets.insert( pin->Connection()->Name() );
            }

            // Replace grid with just this one force-created entry
            m_rows.clear();

            POTENTIAL_ROW row;
            row.livePtr = nullptr;
            row.isManual = true;
            row.forceFromUuid = fromSym->m_Uuid;
            row.forceToUuid = toSym->m_Uuid;
            row.suggestedName = fromRef + wxT( "_" ) + toRef;
            row.terminals = fromRef + wxT( " \u2192 " ) + toRef;
            row.memberNets.insert( fromNets.begin(), fromNets.end() );
            row.memberNets.insert( toNets.begin(), toNets.end() );

            m_rows.push_back( std::move( row ) );
            rebuildGrid();

            m_chainsGrid->SelectRow( 0 );
            updateMemberDetail( 0 );
            m_nameInput->SetValue( m_rows[0].suggestedName );
            m_headerLabel->SetLabel( wxString::Format(
                    _( "Showing manual chain between %s and %s. Use Refresh to restore all." ),
                    fromRef, toRef ) );
        }

        return;
    }

    // Filter out chains that are already committed
    std::set<wxString> committedNames;

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetCommittedNetChains() )
    {
        if( chain )
            committedNames.insert( chain->GetName() );
    }

    std::vector<FOUND_CHAIN> uncommitted;

    for( const FOUND_CHAIN& fc : foundChains )
    {
        bool isCommitted = false;

        for( const wxString& net : fc.nets )
        {
            if( SCH_NETCHAIN* existing = graph->GetNetChainForNet( net ) )
            {
                if( committedNames.count( existing->GetName() ) )
                {
                    isCommitted = true;
                    break;
                }
            }
        }

        if( !isCommitted )
            uncommitted.push_back( fc );
    }

    if( uncommitted.empty() )
    {
        wxMessageBox( wxString::Format( _( "All %zu signal paths between %s and %s are "
                                           "already committed as net chains." ),
                                        foundChains.size(), fromRef, toRef ),
                      _( "Find Path" ), wxOK | wxICON_INFORMATION, this );
        return;
    }

    // Replace grid contents with only the found paths
    m_rows.clear();

    for( const FOUND_CHAIN& fc : uncommitted )
    {
        POTENTIAL_ROW row;
        row.livePtr = fc.chain;
        row.memberNets = fc.nets;
        row.isManual = true;
        row.terminals = fc.fromPin + wxT( " \u2192 " ) + fc.toPin;
        row.suggestedName = fromRef + wxT( "_" ) + toRef;

        if( uncommitted.size() > 1 )
        {
            wxString pinNum = fc.fromPin.AfterLast( ':' );
            row.suggestedName = fromRef + wxT( "_" ) + toRef + wxT( "_" ) + pinNum;
        }

        m_rows.push_back( std::move( row ) );
    }

    rebuildGrid();

    m_chainsGrid->SelectRow( 0 );
    updateMemberDetail( 0 );
    m_nameInput->SetValue( m_rows[0].suggestedName );

    m_headerLabel->SetLabel( wxString::Format(
            _( "Showing %zu path(s) between %s and %s. Use Refresh to restore all." ),
            uncommitted.size(), fromRef, toRef ) );
}


void DIALOG_CREATE_NET_CHAIN::populateComponentCombos()
{
    m_fromComponent->Clear();
    m_toComponent->Clear();

    SCH_REFERENCE_LIST refs;
    m_frame->Schematic().Hierarchy().GetSymbols( refs, SYMBOL_FILTER_ALL );

    std::set<wxString> refNames;

    for( const SCH_REFERENCE& ref : refs )
    {
        if( ref.GetSymbol() )
            refNames.insert( ref.GetRef() );
    }

    for( const wxString& name : refNames )
    {
        m_fromComponent->Append( name );
        m_toComponent->Append( name );
    }
}


void DIALOG_CREATE_NET_CHAIN::loadPotentials()
{
    m_rows.clear();

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return;

    std::set<wxString> committedNames;

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetCommittedNetChains() )
    {
        if( chain )
            committedNames.insert( chain->GetName() );
    }

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetPotentialNetChains() )
    {
        if( !chain )
            continue;

        // Skip potentials that already match a committed chain by net membership
        bool alreadyCommitted = false;

        for( const wxString& net : chain->GetNets() )
        {
            if( SCH_NETCHAIN* existing = graph->GetNetChainForNet( net ) )
            {
                if( committedNames.count( existing->GetName() ) )
                {
                    alreadyCommitted = true;
                    break;
                }
            }
        }

        if( alreadyCommitted )
            continue;

        POTENTIAL_ROW row;
        row.livePtr = chain.get();
        row.memberNets = chain->GetNets();

        // Suggest name from the longest member net name
        for( const wxString& net : row.memberNets )
        {
            if( net.length() > row.suggestedName.length() )
                row.suggestedName = net;
        }

        m_rows.push_back( std::move( row ) );
    }
}


void DIALOG_CREATE_NET_CHAIN::OnFilterChanged( wxCommandEvent& aEvent )
{
    // Sync any edited names back before rebuilding
    for( size_t gi = 0; gi < m_filteredIndices.size(); ++gi )
    {
        int dataIdx = m_filteredIndices[gi];

        if( dataIdx >= 0 && dataIdx < static_cast<int>( m_rows.size() ) )
        {
            wxString edited = m_chainsGrid->GetCellValue( static_cast<int>( gi ), 0 );

            if( !edited.IsEmpty() )
                m_rows[dataIdx].suggestedName = edited;
        }
    }

    rebuildGrid();
}


void DIALOG_CREATE_NET_CHAIN::rebuildGrid()
{
    m_rebuilding = true;

    // Sync any edited names back from the grid before clearing it
    for( size_t gi = 0; gi < m_filteredIndices.size(); ++gi )
    {
        int gridRow = static_cast<int>( gi );
        int dataIdx = m_filteredIndices[gi];

        if( gridRow < m_chainsGrid->GetNumberRows()
                && dataIdx >= 0 && dataIdx < static_cast<int>( m_rows.size() ) )
        {
            wxString edited = m_chainsGrid->GetCellValue( gridRow, 0 );

            if( !edited.IsEmpty() )
                m_rows[dataIdx].suggestedName = edited;
        }
    }

    if( m_chainsGrid->GetNumberRows() )
        m_chainsGrid->DeleteRows( 0, m_chainsGrid->GetNumberRows() );

    // Build filtered index list
    m_filteredIndices.clear();
    wxString filter = m_filterInput->GetValue().Lower().Trim().Trim( false );

    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        if( filter.IsEmpty() )
        {
            m_filteredIndices.push_back( static_cast<int>( i ) );
            continue;
        }

        const POTENTIAL_ROW& row = m_rows[i];

        // Match against suggested name
        if( row.suggestedName.Lower().Contains( filter ) )
        {
            m_filteredIndices.push_back( static_cast<int>( i ) );
            continue;
        }

        // Match against member net names
        bool netMatch = false;

        for( const wxString& net : row.memberNets )
        {
            if( net.Lower().Contains( filter ) )
            {
                netMatch = true;
                break;
            }
        }

        if( netMatch )
            m_filteredIndices.push_back( static_cast<int>( i ) );
    }

    m_chainsGrid->AppendRows( static_cast<int>( m_filteredIndices.size() ) );

    for( size_t gi = 0; gi < m_filteredIndices.size(); ++gi )
    {
        const POTENTIAL_ROW& row = m_rows[m_filteredIndices[gi]];
        int                  r = static_cast<int>( gi );

        m_chainsGrid->SetCellValue( r, 0, row.suggestedName );
        m_chainsGrid->SetCellValue( r, 1, wxString::Format( wxT( "%zu" ), row.memberNets.size() ) );

        // Column 0 (Suggested Name) is editable, columns 1 and 2 are read-only
        m_chainsGrid->SetReadOnly( r, 1, true );
        m_chainsGrid->SetReadOnly( r, 2, true );

        wxString preview;

        if( !row.terminals.IsEmpty() )
            preview = wxT( "[" ) + row.terminals + wxT( "] " );

        for( const wxString& net : row.memberNets )
        {
            if( !preview.IsEmpty() && preview.Last() != ' ' )
                preview += wxT( ", " );

            preview += net;
        }

        m_chainsGrid->SetCellValue( r, 2, preview );
    }

    m_membersListBox->Clear();
    m_membersLabel->SetLabel( _( "Member Nets" ) );
    m_nameInput->Clear();

    m_rebuilding = false;
}


void DIALOG_CREATE_NET_CHAIN::updateMemberDetail( int aRow )
{
    m_membersListBox->Clear();

    if( aRow < 0 || aRow >= static_cast<int>( m_rows.size() ) )
    {
        m_membersLabel->SetLabel( _( "Member Nets" ) );
        return;
    }

    const POTENTIAL_ROW& row = m_rows[aRow];

    wxString label;

    if( !row.terminals.IsEmpty() )
    {
        label = wxString::Format( _( "Member Nets \u2014 %s (%zu) [%s]" ), row.suggestedName, row.memberNets.size(),
                                  row.terminals );
    }
    else
    {
        label = wxString::Format( _( "Member Nets \u2014 %s (%zu)" ), row.suggestedName, row.memberNets.size() );
    }

    m_membersLabel->SetLabel( label );

    wxArrayString sorted;
    sorted.reserve( row.memberNets.size() );

    for( const wxString& net : row.memberNets )
        sorted.Add( net );

    sorted.Sort();
    m_membersListBox->Append( sorted );
}


int DIALOG_CREATE_NET_CHAIN::selectedRow() const
{
    wxArrayInt rows = m_chainsGrid->GetSelectedRows();

    if( !rows.empty() )
        return rows.front();

    return m_chainsGrid->GetGridCursorRow();
}


void DIALOG_CREATE_NET_CHAIN::highlightChainNets( const std::set<wxString>& aNets )
{
    if( !m_frame )
        return;

    SCH_SCREEN* screen = m_frame->GetCurrentSheet().LastScreen();

    if( !screen )
        return;

    KIGFX::VIEW*           view = m_frame->GetCanvas()->GetView();
    std::vector<EDA_ITEM*> itemsToRedraw;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( !item || !item->IsConnectable() )
            continue;

        SCH_ITEM* redrawItem = nullptr;

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                SCH_CONNECTION* pinConn = pin->Connection();

                if( pinConn && !aNets.empty() )
                {
                    if( !pin->IsBrightened() && aNets.count( pinConn->Name() ) )
                    {
                        pin->SetBrightened();
                        redrawItem = symbol;
                    }
                    else if( pin->IsBrightened() && !aNets.count( pinConn->Name() ) )
                    {
                        pin->ClearBrightened();
                        redrawItem = symbol;
                    }
                }
                else if( pin->IsBrightened() )
                {
                    pin->ClearBrightened();
                    redrawItem = symbol;
                }
            }
        }
        else
        {
            SCH_CONNECTION* itemConn = item->Connection();

            if( itemConn && !aNets.empty() )
            {
                if( !item->IsBrightened() && aNets.count( itemConn->Name() ) )
                {
                    item->SetBrightened();
                    redrawItem = item;
                }
                else if( item->IsBrightened() && !aNets.count( itemConn->Name() ) )
                {
                    item->ClearBrightened();
                    redrawItem = item;
                }
            }
            else if( item->IsBrightened() )
            {
                item->ClearBrightened();
                redrawItem = item;
            }
        }

        if( redrawItem )
            itemsToRedraw.push_back( redrawItem );
    }

    if( !itemsToRedraw.empty() )
    {
        for( EDA_ITEM* redrawItem : itemsToRedraw )
            view->Update( static_cast<KIGFX::VIEW_ITEM*>( redrawItem ), KIGFX::REPAINT );

        // Zoom to fit the highlighted items if we're highlighting (not clearing)
        if( !aNets.empty() )
        {
            BOX2I bbox;
            bool  first = true;

            for( EDA_ITEM* item : itemsToRedraw )
            {
                if( item->IsBrightened() )
                {
                    if( first )
                    {
                        bbox = item->GetBoundingBox();
                        first = false;
                    }
                    else
                    {
                        bbox.Merge( item->GetBoundingBox() );
                    }
                }
            }

            if( !first )
            {
                // Add some margin around the highlighted area
                bbox.Inflate( bbox.GetWidth() / 5, bbox.GetHeight() / 5 );

                BOX2D viewport( bbox.GetOrigin(), bbox.GetSize() );
                view->SetViewport( viewport );
            }
        }

        m_frame->GetCanvas()->Refresh();
    }
}
