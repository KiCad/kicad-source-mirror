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

#include "panel_setup_net_chains.h"

#include <algorithm>
#include <set>
#include <vector>

#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <connection_graph.h>
#include <sch_edit_frame.h>
#include <sch_netchain.h>
#include <schematic.h>

#include <netclass.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>

#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <bitmaps.h>
#include <widgets/paged_dialog.h>


static const wxString c_statusCommitted = _( "Committed" );
static const wxString c_statusPotential = _( "Potential" );


PANEL_SETUP_NET_CHAINS::PANEL_SETUP_NET_CHAINS( wxWindow* aParent, SCH_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_NET_CHAINS_BASE( aParent ),
        m_frame( aFrame )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( PAGED_DIALOG::GetDialog( this ) ) );
    attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( PAGED_DIALOG::GetDialog( this ),
                                                   m_chainsGrid ) );
    m_chainsGrid->SetColAttr( COL_COLOUR, attr );

    wxGridCellAttr* roAttr = new wxGridCellAttr;
    roAttr->SetReadOnly( true );
    m_chainsGrid->SetColAttr( COL_MEMBERS, roAttr );

    wxGridCellAttr* roAttr2 = new wxGridCellAttr;
    roAttr2->SetReadOnly( true );
    m_classesGrid->SetColAttr( CLASS_COL_MEMBERS, roAttr2 );

    m_deleteChainButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_addClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_renameClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_deleteClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_chainsGrid->Bind( wxEVT_GRID_CELL_CHANGED,
            [this]( wxGridEvent& evt )
            {
                if( evt.GetCol() == COL_NAME )
                {
                    wxString val = m_chainsGrid->GetCellValue( evt.GetRow(), COL_NAME );

                    if( !SCH_NETCHAIN::IsValidName( val ) )
                    {
                        wxMessageBox( wxString::Format(
                                              _( "Name '%s' contains invalid characters." ), val ),
                                      _( "Net Chains" ), wxOK | wxICON_ERROR, this );
                        m_chainsGrid->SetCellValue( evt.GetRow(), COL_NAME, evt.GetString() );
                        return;
                    }

                    int gridRow = evt.GetRow();

                    if( gridRow >= 0 && gridRow < static_cast<int>( m_gridToChainIdx.size() ) )
                    {
                        int dataIdx = m_gridToChainIdx[gridRow];

                        if( dataIdx >= 0 && dataIdx < static_cast<int>( m_chainRows.size() ) )
                            m_chainRows[dataIdx].newName = val;
                    }
                }
            } );
}


PANEL_SETUP_NET_CHAINS::~PANEL_SETUP_NET_CHAINS()
{
}


void PANEL_SETUP_NET_CHAINS::loadFromModel()
{
    m_chainRows.clear();
    m_classRows.clear();

    if( !m_frame )
        return;

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return;

    std::shared_ptr<NET_SETTINGS> ns = m_frame->Prj().GetProjectFile().NetSettings();
    std::map<wxString, wxString>  chainToClass;

    if( ns )
        chainToClass = ns->GetNetChainClasses();

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetCommittedNetChains() )
    {
        if( !chain )
            continue;

        CHAIN_ROW row;
        row.origName    = chain->GetName();
        row.newName     = row.origName;
        row.newColor    = chain->GetColor();
        row.newNetClass = chain->GetNetClass();
        row.livePtr     = chain.get();
        row.memberNets  = chain->GetNets();

        auto it = chainToClass.find( row.origName );

        if( it != chainToClass.end() )
            row.newChainClass = it->second;

        m_chainRows.push_back( std::move( row ) );
    }

    // Distinct class names from the chain->class map.
    std::set<wxString> distinctClasses;

    for( const auto& [chainName, className] : chainToClass )
    {
        if( !className.IsEmpty() )
            distinctClasses.insert( className );
    }

    for( const wxString& cn : distinctClasses )
    {
        CLASS_ROW row;
        row.origName = cn;
        row.newName  = cn;
        m_classRows.push_back( std::move( row ) );
    }
}


void PANEL_SETUP_NET_CHAINS::refreshChainClassDropdownChoices()
{
    wxArrayString choices;
    choices.Add( wxEmptyString ); // allow clearing the assignment

    for( const CLASS_ROW& cr : m_classRows )
    {
        if( !cr.deletePending && !cr.newName.IsEmpty() )
            choices.Add( cr.newName );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( choices, true /* allowOthers */ ) );
    m_chainsGrid->SetColAttr( COL_CHAIN_CLASS, attr );
}


void PANEL_SETUP_NET_CHAINS::refreshNetClassDropdownChoices()
{
    wxArrayString choices;
    choices.Add( wxEmptyString ); // empty == no override

    if( m_frame )
    {
        std::shared_ptr<NET_SETTINGS> ns = m_frame->Prj().GetProjectFile().NetSettings();

        if( ns )
        {
            for( const auto& [name, nc] : ns->GetNetclasses() )
                choices.Add( name );
        }
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( choices, true /* allowOthers */ ) );
    m_chainsGrid->SetColAttr( COL_NET_CLASS, attr );
}


void PANEL_SETUP_NET_CHAINS::rebuildChainsGrid()
{
    if( m_chainsGrid->GetNumberRows() )
        m_chainsGrid->DeleteRows( 0, m_chainsGrid->GetNumberRows() );

    refreshNetClassDropdownChoices();
    refreshChainClassDropdownChoices();

    int activeCount = 0;

    for( const CHAIN_ROW& row : m_chainRows )
    {
        if( !row.deletePending )
            ++activeCount;
    }

    m_chainsHeader->SetLabel( wxString::Format( _( "%d net chain(s)" ), activeCount ) );

    m_gridToChainIdx.clear();

    for( size_t i = 0; i < m_chainRows.size(); ++i )
    {
        if( !m_chainRows[i].deletePending )
            m_gridToChainIdx.push_back( static_cast<int>( i ) );
    }

    m_chainsGrid->AppendRows( static_cast<int>( m_gridToChainIdx.size() ) );

    for( int r = 0; r < static_cast<int>( m_gridToChainIdx.size() ); ++r )
    {
        const CHAIN_ROW& row = m_chainRows[m_gridToChainIdx[r]];

        m_chainsGrid->SetCellValue( r, COL_NAME, row.newName );
        m_chainsGrid->SetCellValue( r, COL_MEMBERS,
                                    wxString::Format( _( "%zu nets" ), row.memberNets.size() ) );
        m_chainsGrid->SetCellValue( r, COL_CHAIN_CLASS, row.newChainClass );
        m_chainsGrid->SetCellValue( r, COL_NET_CLASS, row.newNetClass );

        if( row.newColor != KIGFX::COLOR4D::UNSPECIFIED )
            m_chainsGrid->SetCellValue( r, COL_COLOUR, row.newColor.ToCSSString() );
    }

    int sel = selectedChainRow();
    int dataIdx = ( sel >= 0 && sel < static_cast<int>( m_gridToChainIdx.size() ) )
                          ? m_gridToChainIdx[sel] : -1;
    updateMembersDetail( dataIdx );
}


void PANEL_SETUP_NET_CHAINS::rebuildClassesGrid()
{
    if( m_classesGrid->GetNumberRows() )
        m_classesGrid->DeleteRows( 0, m_classesGrid->GetNumberRows() );

    m_classesGrid->AppendRows( static_cast<int>( m_classRows.size() ) );

    // Member-count map: count chains that reference each class in the (live
    // edit-buffered) chain rows.
    std::map<wxString, int> memberCount;

    for( const CHAIN_ROW& row : m_chainRows )
    {
        if( row.deletePending || row.newChainClass.IsEmpty() )
            continue;

        ++memberCount[row.newChainClass];
    }

    for( size_t i = 0; i < m_classRows.size(); ++i )
    {
        const CLASS_ROW& row = m_classRows[i];
        int              r   = static_cast<int>( i );

        m_classesGrid->SetCellValue( r, CLASS_COL_NAME, row.newName );
        m_classesGrid->SetCellValue( r, CLASS_COL_MEMBERS,
                                     wxString::Format( wxT( "%d" ),
                                                       memberCount[row.newName] ) );

        if( row.deletePending )
        {
            for( int c = 0; c < m_classesGrid->GetNumberCols(); ++c )
                m_classesGrid->SetReadOnly( r, c, true );
        }
    }
}


bool PANEL_SETUP_NET_CHAINS::TransferDataToWindow()
{
    loadFromModel();
    rebuildChainsGrid();
    rebuildClassesGrid();
    return true;
}


bool PANEL_SETUP_NET_CHAINS::Validate()
{
    if( !m_chainsGrid->CommitPendingChanges() )
        return false;

    if( !m_classesGrid->CommitPendingChanges() )
        return false;

    // Sync grid cell values back into the buffered rows so validation works
    // against what the user actually sees.
    for( int gr = 0; gr < static_cast<int>( m_gridToChainIdx.size() ); ++gr )
    {
        CHAIN_ROW& row = m_chainRows[m_gridToChainIdx[gr]];

        row.newName = m_chainsGrid->GetCellValue( gr, COL_NAME );
        row.newChainClass = m_chainsGrid->GetCellValue( gr, COL_CHAIN_CLASS );
        row.newNetClass = m_chainsGrid->GetCellValue( gr, COL_NET_CLASS );

        wxString colorStr = m_chainsGrid->GetCellValue( gr, COL_COLOUR );

        if( colorStr.IsEmpty() )
            row.newColor = KIGFX::COLOR4D::UNSPECIFIED;
        else
            row.newColor = KIGFX::COLOR4D( colorStr );
    }

    for( size_t i = 0; i < m_classRows.size(); ++i )
    {
        if( m_classRows[i].deletePending )
            continue;

        m_classRows[i].newName =
                m_classesGrid->GetCellValue( static_cast<int>( i ), CLASS_COL_NAME );
    }

    // Reject empty committed chain names and duplicate names within the grid.
    for( size_t i = 0; i < m_chainRows.size(); ++i )
    {
        const CHAIN_ROW& row = m_chainRows[i];

        if( row.deletePending )
            continue;

        if( row.newName.IsEmpty() )
        {
            wxMessageBox( wxString::Format( _( "Net chain on row %zu cannot have an empty name." ),
                                            i + 1 ),
                          _( "Net Chains" ), wxOK | wxICON_ERROR, this );
            return false;
        }

        if( !SCH_NETCHAIN::IsValidName( row.newName ) )
        {
            wxMessageBox( wxString::Format( _( "Net chain name '%s' contains invalid characters." ),
                                            row.newName ),
                          _( "Net Chains" ), wxOK | wxICON_ERROR, this );
            return false;
        }

        if( nameInChainGridAlready( row.newName, static_cast<int>( i ) ) )
        {
            wxMessageBox( wxString::Format( _( "Duplicate net chain name '%s' on row %zu." ),
                                            row.newName, i + 1 ),
                          _( "Net Chains" ), wxOK | wxICON_ERROR, this );
            return false;
        }
    }

    // Reject empty / duplicate class names.
    for( size_t i = 0; i < m_classRows.size(); ++i )
    {
        const CLASS_ROW& row = m_classRows[i];

        if( row.deletePending )
            continue;

        if( row.newName.IsEmpty() )
        {
            wxMessageBox( wxString::Format( _( "Class on row %zu cannot have an empty name." ),
                                            i + 1 ),
                          _( "Net Chain Classes" ), wxOK | wxICON_ERROR, this );
            return false;
        }

        if( nameInClassGridAlready( row.newName, static_cast<int>( i ) ) )
        {
            wxMessageBox( wxString::Format( _( "Duplicate class name '%s' on row %zu." ),
                                            row.newName, i + 1 ),
                          _( "Net Chain Classes" ), wxOK | wxICON_ERROR, this );
            return false;
        }
    }

    return true;
}


bool PANEL_SETUP_NET_CHAINS::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    return ApplyEdits();
}


bool PANEL_SETUP_NET_CHAINS::ApplyEdits()
{
    if( !m_frame )
        return false;

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return false;

    std::shared_ptr<NET_SETTINGS> ns = m_frame->Prj().GetProjectFile().NetSettings();

    // Apply renames on chains whose name changed.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( row.deletePending )
            continue;

        if( !row.origName.IsEmpty() && row.origName != row.newName )
        {
            if( graph->RenameCommittedNetChain( row.origName, row.newName ) )
            {
                if( ns && !row.origName.IsEmpty() )
                {
                    wxString oldClass = ns->GetNetChainClass( row.origName );

                    if( !oldClass.IsEmpty() )
                    {
                        ns->SetNetChainClass( row.origName, wxEmptyString );
                        ns->SetNetChainClass( row.newName, oldClass );
                    }
                }

                row.origName = row.newName;
            }
        }
    }

    // Apply colour and netclass override edits.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( row.deletePending || !row.livePtr )
            continue;

        row.livePtr->SetColor( row.newColor );
        row.livePtr->SetNetClass( row.newNetClass );
    }

    // Chain-class assignments into NET_SETTINGS.
    if( ns )
    {
        for( const CHAIN_ROW& row : m_chainRows )
        {
            if( row.deletePending )
                continue;

            ns->SetNetChainClass( row.newName, row.newChainClass );
        }
    }

    // Deletions — done last so renames above don't see ghost rows.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( !row.deletePending )
            continue;

        if( !row.origName.IsEmpty() && graph->DeleteCommittedNetChain( row.origName ) )
        {
            if( ns )
                ns->SetNetChainClass( row.origName, wxEmptyString );
        }
    }

    // Step 6 — chain-class master list.  Drop classes the user marked deleted
    // and clear them from every chain that referenced them.  Renames are
    // already reflected in row.newName because we round-tripped via grid earlier.
    if( ns )
    {
        for( const CLASS_ROW& cls : m_classRows )
        {
            if( !cls.deletePending )
                continue;

            // Collect first; SetNetChainClass with empty value erases from the map and would
            // invalidate the active iterator if mutated during the range-for.
            std::vector<wxString> toClear;

            for( const auto& [chainName, className] : ns->GetNetChainClasses() )
            {
                if( className == cls.origName )
                    toClear.push_back( chainName );
            }

            for( const wxString& chainName : toClear )
                ns->SetNetChainClass( chainName, wxEmptyString );
        }

        for( const CLASS_ROW& cls : m_classRows )
        {
            if( cls.deletePending )
                continue;

            if( !cls.origName.IsEmpty() && cls.origName != cls.newName )
            {
                // Two-pass for symmetry with the delete loop above and to avoid coupling to
                // std::map mutation semantics during iteration.
                std::vector<wxString> toRename;

                for( const auto& [chainName, className] : ns->GetNetChainClasses() )
                {
                    if( className == cls.origName )
                        toRename.push_back( chainName );
                }

                for( const wxString& chainName : toRename )
                    ns->SetNetChainClass( chainName, cls.newName );
            }
        }
    }

    m_frame->OnModify();

    return true;
}


int PANEL_SETUP_NET_CHAINS::selectedChainRow() const
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


int PANEL_SETUP_NET_CHAINS::selectedClassRow() const
{
    wxArrayInt rows = m_classesGrid->GetSelectedRows();

    if( !rows.empty() )
        return rows.front();

    int cursor = m_classesGrid->GetGridCursorRow();

    if( cursor < 0 || cursor >= m_classesGrid->GetNumberRows() )
        return -1;

    return cursor;
}




bool PANEL_SETUP_NET_CHAINS::nameInChainGridAlready( const wxString& aName, int aExceptRow ) const
{
    for( int i = 0; i < static_cast<int>( m_chainRows.size() ); ++i )
    {
        if( i == aExceptRow )
            continue;

        if( m_chainRows[i].deletePending )
            continue;

        if( m_chainRows[i].newName == aName && !aName.IsEmpty() )
            return true;
    }

    return false;
}


bool PANEL_SETUP_NET_CHAINS::nameInClassGridAlready( const wxString& aName, int aExceptRow ) const
{
    for( int i = 0; i < static_cast<int>( m_classRows.size() ); ++i )
    {
        if( i == aExceptRow )
            continue;

        if( m_classRows[i].deletePending )
            continue;

        if( m_classRows[i].newName == aName && !aName.IsEmpty() )
            return true;
    }

    return false;
}


void PANEL_SETUP_NET_CHAINS::OnDeleteChainClicked( wxCommandEvent& )
{
    int gridRow = selectedChainRow();

    if( gridRow < 0 || gridRow >= static_cast<int>( m_gridToChainIdx.size() ) )
        return;

    CHAIN_ROW& row = m_chainRows[m_gridToChainIdx[gridRow]];

    if( wxMessageBox( wxString::Format( _( "Delete net chain '%s'?" ), row.newName ),
                      _( "Delete Net Chain" ), wxYES_NO | wxICON_QUESTION, this ) != wxYES )
    {
        return;
    }

    row.deletePending = true;
    rebuildChainsGrid();
    rebuildClassesGrid();
}


void PANEL_SETUP_NET_CHAINS::OnChainGridSelectionChanged( wxGridEvent& aEvent )
{
    int gridRow = aEvent.GetRow();
    int dataIdx = ( gridRow >= 0 && gridRow < static_cast<int>( m_gridToChainIdx.size() ) )
                          ? m_gridToChainIdx[gridRow] : -1;
    updateMembersDetail( dataIdx );
    aEvent.Skip();
}


void PANEL_SETUP_NET_CHAINS::updateMembersDetail( int aRow )
{
    m_membersListBox->Clear();

    if( aRow < 0 || aRow >= static_cast<int>( m_chainRows.size() ) )
    {
        m_membersLabel->SetLabel( _( "Member Nets" ) );
        return;
    }

    const CHAIN_ROW& row = m_chainRows[aRow];

    wxString label = row.newName.IsEmpty()
                         ? wxString::Format( _( "Member Nets (%zu)" ), row.memberNets.size() )
                         : wxString::Format( _( "Member Nets \u2014 %s (%zu)" ), row.newName,
                                             row.memberNets.size() );

    m_membersLabel->SetLabel( label );

    wxArrayString sorted;
    sorted.reserve( row.memberNets.size() );

    for( const wxString& net : row.memberNets )
        sorted.Add( net );

    sorted.Sort();
    m_membersListBox->Append( sorted );
}


void PANEL_SETUP_NET_CHAINS::OnClassAddClicked( wxCommandEvent& )
{
    wxTextEntryDialog dlg( this, _( "New net chain class name:" ), _( "Add Class" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString name = dlg.GetValue();

    if( name.IsEmpty() || nameInClassGridAlready( name, -1 ) )
    {
        wxMessageBox( _( "That class name is empty or already in use." ),
                      _( "Add Class" ), wxOK | wxICON_ERROR, this );
        return;
    }

    CLASS_ROW row;
    row.origName = wxEmptyString;
    row.newName  = name;
    m_classRows.push_back( std::move( row ) );

    rebuildClassesGrid();
    refreshChainClassDropdownChoices();
}


void PANEL_SETUP_NET_CHAINS::OnClassRenameClicked( wxCommandEvent& )
{
    int r = selectedClassRow();

    if( r < 0 || r >= static_cast<int>( m_classRows.size() ) )
        return;

    CLASS_ROW& row = m_classRows[r];

    wxTextEntryDialog dlg( this, _( "Rename net chain class:" ), _( "Rename Class" ),
                           row.newName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString name = dlg.GetValue();

    if( name.IsEmpty() || nameInClassGridAlready( name, r ) )
    {
        wxMessageBox( _( "That class name is empty or already in use." ),
                      _( "Rename Class" ), wxOK | wxICON_ERROR, this );
        return;
    }

    wxString oldName = row.newName;
    row.newName = name;

    for( CHAIN_ROW& chainRow : m_chainRows )
    {
        if( chainRow.newChainClass == oldName )
            chainRow.newChainClass = name;
    }

    rebuildClassesGrid();
    rebuildChainsGrid();
    refreshChainClassDropdownChoices();
}


void PANEL_SETUP_NET_CHAINS::OnClassDeleteClicked( wxCommandEvent& )
{
    int r = selectedClassRow();

    if( r < 0 || r >= static_cast<int>( m_classRows.size() ) )
        return;

    CLASS_ROW& row = m_classRows[r];

    if( wxMessageBox( wxString::Format( _( "Delete net chain class '%s'?  Chains assigned to it "
                                           "will become unclassified." ),
                                        row.newName ),
                      _( "Delete Class" ), wxYES_NO | wxICON_QUESTION, this ) != wxYES )
    {
        return;
    }

    row.deletePending = true;

    // Strip the class label from any chain row that referenced it.
    for( CHAIN_ROW& chainRow : m_chainRows )
    {
        if( chainRow.newChainClass == row.newName )
            chainRow.newChainClass = wxEmptyString;
    }

    rebuildClassesGrid();
    rebuildChainsGrid();
    refreshChainClassDropdownChoices();
}
