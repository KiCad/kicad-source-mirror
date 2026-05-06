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
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <view/view.h>


DIALOG_CREATE_NET_CHAIN::DIALOG_CREATE_NET_CHAIN( SCH_EDIT_FRAME* aParent, const FOCUS_HINT& aHint ) :
        DIALOG_CREATE_NET_CHAIN_BASE( aParent ),
        m_frame( aParent ),
        m_hint( aHint )
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
    if( !m_hint.fromRef.IsEmpty() )
        m_fromComponent->SetValue( m_hint.fromRef );

    if( !m_hint.toRef.IsEmpty() )
        m_toComponent->SetValue( m_hint.toRef );

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

    applyFocusHint();

    finishDialogSettings();
}


DIALOG_CREATE_NET_CHAIN::~DIALOG_CREATE_NET_CHAIN()
{
    // Clear highlighting on whichever sheet we last touched.  The user may have navigated
    // away via row selection, and we don't want stale brightening to outlive the dialog.
    SCH_SCREEN* screen = !m_lastHighlightedSheet.empty() ? m_lastHighlightedSheet.LastScreen()
                                                         : ( m_frame ? m_frame->GetCurrentSheet().LastScreen()
                                                                     : nullptr );
    highlightChainNets( {}, screen );
}


bool DIALOG_CREATE_NET_CHAIN::TransferDataToWindow()
{
    return true;
}


bool DIALOG_CREATE_NET_CHAIN::TransferDataFromWindow()
{
    if( !validateAndCreate() )
        return false;

    // Drop livePtr-bearing rows BEFORE the OnModify notify, then reload AFTER.  OnModify()
    // does not recalculate today, but if a future hook attaches a recalc to it, the cleared
    // window keeps livePtrs from dangling across the destruction of m_potentialNetChains.
    m_rows.clear();
    m_filteredIndices.clear();
    m_nameInput->Clear();
    m_frame->OnModify();
    loadPotentials();
    rebuildGrid();

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
        std::set<SCH_SYMBOL*> symbols;

        SCH_ITEM* fromItem = m_frame->Schematic().ResolveItem( prow.forceFromUuid, nullptr, true );
        SCH_ITEM* toItem   = m_frame->Schematic().ResolveItem( prow.forceToUuid, nullptr, true );

        if( fromItem && fromItem->Type() == SCH_SYMBOL_T )
            symbols.insert( static_cast<SCH_SYMBOL*>( fromItem ) );

        if( toItem && toItem->Type() == SCH_SYMBOL_T )
            symbols.insert( static_cast<SCH_SYMBOL*>( toItem ) );

        SCH_NETCHAIN* committed = graph->CreateManualNetChain( name, symbols, prow.memberNets,
                                                               prow.forceFromPinUuid,
                                                               prow.forceToPinUuid,
                                                               prow.forceFromRef, prow.forceFromPinNum,
                                                               prow.forceToRef, prow.forceToPinNum );

        if( !committed )
        {
            wxMessageBox( _( "Failed to create the manual net chain." ), _( "Create Net Chain" ),
                          wxOK | wxICON_ERROR, this );
            return false;
        }
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

        navigateAndHighlightChain( m_rows[dataIdx] );
    }
    else
    {
        SCH_SCREEN* screen = !m_lastHighlightedSheet.empty() ? m_lastHighlightedSheet.LastScreen()
                                                              : m_frame->GetCurrentSheet().LastScreen();
        highlightChainNets( {}, screen );
    }

    aEvent.Skip();
}


void DIALOG_CREATE_NET_CHAIN::OnRefreshClicked( wxCommandEvent& aEvent )
{
    if( !m_frame->Schematic().ConnectionGraph() )
        return;

    // Clear any focus-hint filter so Refresh truly restores the full list.  Otherwise the
    // empty-state message ("Use Refresh to restore the full list.") would refresh and
    // immediately re-apply the same no-match filter.
    m_filterInput->Clear();

    populateComponentCombos();
    recalculateAndReload( true );
}


void DIALOG_CREATE_NET_CHAIN::recalculateAndReload( bool aRunRecalculate )
{
    // Drop all references into m_potentialNetChains BEFORE any recalc clears that vector.
    // This is the load-bearing step: while m_rows is non-empty, livePtr values may be live
    // pointers into the pool, and Recalculate() destroys those entries.
    m_rows.clear();
    m_filteredIndices.clear();

    if( aRunRecalculate )
    {
        if( CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph() )
            graph->Recalculate( m_frame->Schematic().BuildSheetListSortedByPageNumbers(), true );
    }

    loadPotentials();
    rebuildGrid();
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
                wxString::Format( _( "No net chain path found between %s and %s through "
                                     "passthrough components.\n\n"
                                     "Would you like to force-create a manual chain link "
                                     "between these components?" ),
                                  fromRef, toRef ),
                _( "Find Path" ), wxYES_NO | wxICON_QUESTION, this );

        if( answer == wxYES )
        {
            std::set<wxString> fromNets, toNets;
            SCH_PIN*           fromTerminalPin = nullptr;
            SCH_PIN*           toTerminalPin = nullptr;

            for( SCH_PIN* pin : fromPins )
            {
                if( pin->Connection() && !pin->Connection()->Name().IsEmpty() )
                {
                    fromNets.insert( pin->Connection()->Name() );

                    if( !fromTerminalPin )
                        fromTerminalPin = pin;
                }
            }

            for( SCH_PIN* pin : toPins )
            {
                if( pin->Connection() && !pin->Connection()->Name().IsEmpty() )
                {
                    toNets.insert( pin->Connection()->Name() );

                    if( !toTerminalPin )
                        toTerminalPin = pin;
                }
            }

            if( !fromTerminalPin || !toTerminalPin )
            {
                wxMessageBox( _( "Selected components have no connected pins to anchor a manual chain." ),
                              _( "Find Path" ), wxOK | wxICON_ERROR, this );
                return;
            }

            // Replace grid with just this one force-created entry. Clear m_filteredIndices
            // before mutating m_rows so rebuildGrid()'s "sync edited names back" pass
            // cannot index stale grid rows into freshly-replaced m_rows entries.
            m_filteredIndices.clear();
            m_rows.clear();

            POTENTIAL_ROW row;
            row.livePtr = nullptr;
            row.isManual = true;
            row.forceFromUuid = fromSym->m_Uuid;
            row.forceToUuid = toSym->m_Uuid;
            row.forceFromPinUuid = fromTerminalPin->m_Uuid;
            row.forceToPinUuid = toTerminalPin->m_Uuid;
            row.forceFromRef = fromRef;
            row.forceToRef = toRef;
            row.forceFromPinNum = fromTerminalPin->GetNumber();
            row.forceToPinNum = toTerminalPin->GetNumber();
            row.suggestedName = fromRef + wxT( "_" ) + toRef;
            row.terminals = fromRef + wxT( " \u2192 " ) + toRef;
            row.memberNets.insert( fromNets.begin(), fromNets.end() );
            row.memberNets.insert( toNets.begin(), toNets.end() );

            m_rows.push_back( std::move( row ) );
            rebuildGrid();

            autoSelectFirstRow();
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
        wxMessageBox( wxString::Format( _( "All %zu net chain paths between %s and %s are "
                                           "already committed as net chains." ),
                                        foundChains.size(), fromRef, toRef ),
                      _( "Find Path" ), wxOK | wxICON_INFORMATION, this );
        return;
    }

    // Replace grid contents with only the found paths. Clear m_filteredIndices first so
    // rebuildGrid()'s "sync edited names back" pass cannot index stale grid rows into
    // freshly-replaced m_rows entries.
    m_filteredIndices.clear();
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

    autoSelectFirstRow();

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

        // Surface the terminal refs in the row so a ref-keyed filter (e.g. right-click on a
        // single symbol → focus on chains touching that symbol) can match them.
        wxString terminalA = chain->GetTerminalRef( 0 );
        wxString terminalB = chain->GetTerminalRef( 1 );

        if( !chain->GetTerminalPinNum( 0 ).IsEmpty() )
            terminalA += wxT( ":" ) + chain->GetTerminalPinNum( 0 );

        if( !chain->GetTerminalPinNum( 1 ).IsEmpty() )
            terminalB += wxT( ":" ) + chain->GetTerminalPinNum( 1 );

        if( !terminalA.IsEmpty() || !terminalB.IsEmpty() )
            row.terminals = terminalA + wxT( " → " ) + terminalB;

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

    // Invariant: callers that replace m_rows must clear m_filteredIndices first. Otherwise
    // this sync-edits pass would copy grid cell values into the wrong (or out-of-bounds)
    // m_rows entries. The dataIdx bounds check below is belt-and-suspenders.
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

        // Match against terminal refs/pins (e.g. "J1:1 → J2:1") so a focus hint based on a
        // selected symbol or pin reference catches chains anchored at that ref.
        if( !row.terminals.IsEmpty() && row.terminals.Lower().Contains( filter ) )
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

    // m_nameInput is intentionally not cleared here. OnFilterChanged calls rebuildGrid, so
    // clearing would erase a name the user is typing while narrowing the filter list. The
    // explicit clear in TransferDataFromWindow handles the post-create reset, and the row
    // selection handler overwrites the field with each row's suggested name on click.

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

    // The grid cursor can survive a DeleteRows/AppendRows rebuild and point at an index
    // beyond the new row count.  Clamp before returning so callers don't dereference a
    // stale data slot.
    int cursor = m_chainsGrid->GetGridCursorRow();

    if( cursor < 0 || cursor >= m_chainsGrid->GetNumberRows() )
        return -1;

    return cursor;
}


BOX2I DIALOG_CREATE_NET_CHAIN::highlightChainNets( const std::set<wxString>& aNets, SCH_SCREEN* aScreen )
{
    BOX2I highlightedBBox;

    if( !m_frame || !aScreen )
        return highlightedBBox;

    // Only the current sheet's view can be live-updated; brightening flags on items belonging
    // to other screens still apply visually next time that sheet is shown.
    bool onCurrentSheet = aScreen == m_frame->GetCurrentSheet().LastScreen();
    KIGFX::VIEW*           view = onCurrentSheet ? m_frame->GetCanvas()->GetView() : nullptr;
    std::vector<EDA_ITEM*> itemsToRedraw;

    auto recordHighlight = [&]( SCH_ITEM* aItem )
    {
        if( highlightedBBox.GetWidth() == 0 && highlightedBBox.GetHeight() == 0 )
            highlightedBBox = aItem->GetBoundingBox();
        else
            highlightedBBox.Merge( aItem->GetBoundingBox() );
    };

    for( SCH_ITEM* item : aScreen->Items() )
    {
        if( !item || !item->IsConnectable() )
            continue;

        SCH_ITEM* redrawItem = nullptr;

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            bool        anyPinHighlighted = false;

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                SCH_CONNECTION* pinConn = pin->Connection();

                if( pinConn && !aNets.empty() )
                {
                    if( !pin->IsBrightened() && aNets.count( pinConn->Name() ) )
                    {
                        pin->SetBrightened();
                        redrawItem = symbol;
                        anyPinHighlighted = true;
                    }
                    else if( pin->IsBrightened() && !aNets.count( pinConn->Name() ) )
                    {
                        pin->ClearBrightened();
                        redrawItem = symbol;
                    }
                    else if( pin->IsBrightened() )
                    {
                        anyPinHighlighted = true;
                    }
                }
                else if( pin->IsBrightened() )
                {
                    pin->ClearBrightened();
                    redrawItem = symbol;
                }
            }

            if( anyPinHighlighted )
                recordHighlight( symbol );
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
                    recordHighlight( item );
                }
                else if( item->IsBrightened() && !aNets.count( itemConn->Name() ) )
                {
                    item->ClearBrightened();
                    redrawItem = item;
                }
                else if( item->IsBrightened() )
                {
                    recordHighlight( item );
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

    if( !itemsToRedraw.empty() && view )
    {
        for( EDA_ITEM* redrawItem : itemsToRedraw )
            view->Update( static_cast<KIGFX::VIEW_ITEM*>( redrawItem ), KIGFX::REPAINT );

        m_frame->GetCanvas()->Refresh();
    }

    return highlightedBBox;
}


const SCH_SHEET_PATH& DIALOG_CREATE_NET_CHAIN::findSheetForRow( POTENTIAL_ROW& aRow )
{
    if( aRow.cachedSheetResolved )
        return aRow.cachedSheet;

    aRow.cachedSheetResolved = true;
    aRow.cachedSheet = m_frame->GetCurrentSheet(); // safe default

    // Prefer the deterministic terminal ref recorded with the chain.  std::set<SCH_SYMBOL*>
    // ordering depends on pointer values and is unstable across runs, so we never key off
    // GetSymbols().begin() here.
    wxString targetRef;

    if( aRow.livePtr )
    {
        targetRef = aRow.livePtr->GetTerminalRef( 0 );

        if( targetRef.IsEmpty() )
            targetRef = aRow.livePtr->GetTerminalRef( 1 );
    }
    else
    {
        targetRef = aRow.forceFromRef;

        if( targetRef.IsEmpty() )
            targetRef = aRow.forceToRef;
    }

    if( !targetRef.IsEmpty() )
    {
        SCH_REFERENCE_LIST refs;
        m_frame->Schematic().Hierarchy().GetSymbols( refs, SYMBOL_FILTER_ALL );

        for( const SCH_REFERENCE& ref : refs )
        {
            if( ref.GetRef() == targetRef && ref.GetSymbol() )
            {
                aRow.cachedSheet = ref.GetSheetPath();
                return aRow.cachedSheet;
            }
        }
    }

    // Fallback: scan every sheet for an item whose connection matches a member net.
    for( const SCH_SHEET_PATH& path : m_frame->Schematic().Hierarchy() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( !item || !item->IsConnectable() )
                continue;

            if( SCH_CONNECTION* conn = item->Connection() )
            {
                if( aRow.memberNets.count( conn->Name() ) )
                {
                    aRow.cachedSheet = path;
                    return aRow.cachedSheet;
                }
            }
        }
    }

    return aRow.cachedSheet;
}


void DIALOG_CREATE_NET_CHAIN::navigateAndHighlightChain( POTENTIAL_ROW& aRow )
{
    if( !m_frame )
        return;

    SCH_SHEET_PATH targetPath = findSheetForRow( aRow );

    // Clear leftover brightening on the previously-touched sheet first when it differs from
    // the new target.  Otherwise navigating from row A (sheet 1) to row B (sheet 2) leaves
    // sheet 1's items lit when the user pages back.  Skip when the path is unchanged so we
    // don't double-walk the same screen.
    if( !m_lastHighlightedSheet.empty() && m_lastHighlightedSheet != targetPath )
        highlightChainNets( {}, m_lastHighlightedSheet.LastScreen() );

    if( targetPath != m_frame->GetCurrentSheet() )
    {
        // SCH_ACTIONS::changeSheet wraps cancelInteractive, selectionClear, zoom-state save,
        // history push, and DisplayCurrentSheet — same path eeschema cross-probing uses.
        m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &targetPath );
    }

    SCH_SCREEN* screen = m_frame->GetCurrentSheet().LastScreen();
    BOX2I       bbox = highlightChainNets( aRow.memberNets, screen );

    if( bbox.GetWidth() > 0 || bbox.GetHeight() > 0 )
    {
        if( SCH_SELECTION_TOOL* selTool = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>() )
            selTool->ZoomFitCrossProbeBBox( bbox );
    }

    m_lastHighlightedSheet = m_frame->GetCurrentSheet();
}


void DIALOG_CREATE_NET_CHAIN::autoSelectFirstRow()
{
    if( m_filteredIndices.empty() )
        return;

    int dataIdx = m_filteredIndices[0];
    m_chainsGrid->SelectRow( 0 );
    m_chainsGrid->SetGridCursor( 0, 0 );
    updateMemberDetail( dataIdx );
    m_nameInput->SetValue( m_rows[dataIdx].suggestedName );
    navigateAndHighlightChain( m_rows[dataIdx] );
}


void DIALOG_CREATE_NET_CHAIN::applyFocusHint()
{
    // Both refs set: existing two-component find-path flow.
    if( !m_hint.fromRef.IsEmpty() && !m_hint.toRef.IsEmpty() )
    {
        wxCommandEvent dummy;
        OnFindPathClicked( dummy );
        return;
    }

    wxString filterValue;
    wxString hintLabel;

    if( !m_hint.netName.IsEmpty() )
    {
        filterValue = m_hint.netName;
        hintLabel = wxString::Format( _( "net '%s'" ), m_hint.netName );
    }
    else if( !m_hint.fromRef.IsEmpty() )
    {
        filterValue = m_hint.fromRef;
        hintLabel = wxString::Format( _( "component %s" ), m_hint.fromRef );
    }
    else if( !m_hint.toRef.IsEmpty() )
    {
        filterValue = m_hint.toRef;
        hintLabel = wxString::Format( _( "component %s" ), m_hint.toRef );
    }
    else
    {
        return; // No hint supplied — leave the dialog showing the full list.
    }

    m_filterInput->ChangeValue( filterValue );

    wxCommandEvent dummy;
    OnFilterChanged( dummy );

    if( m_filteredIndices.size() == 1 )
    {
        autoSelectFirstRow();
    }
    else if( m_filteredIndices.empty() )
    {
        m_headerLabel->SetLabel( wxString::Format(
                _( "No uncommitted chains match the selected %s. Use Refresh to restore the full list." ),
                hintLabel ) );
    }
}
