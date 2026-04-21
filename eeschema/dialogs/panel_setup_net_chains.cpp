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
    // Hook the colour-cell renderer/editor onto the chains grid colour column.
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( PAGED_DIALOG::GetDialog( this ) ) );
    attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( PAGED_DIALOG::GetDialog( this ),
                                                   m_chainsGrid ) );
    m_chainsGrid->SetColAttr( COL_COLOUR, attr );

    // Status and Members columns are read-only.
    wxGridCellAttr* roAttr = new wxGridCellAttr;
    roAttr->SetReadOnly( true );
    m_chainsGrid->SetColAttr( COL_STATUS, roAttr );

    wxGridCellAttr* roAttr2 = new wxGridCellAttr;
    roAttr2->SetReadOnly( true );
    m_chainsGrid->SetColAttr( COL_MEMBERS, roAttr2 );

    // Members column on the Classes grid is read-only.
    wxGridCellAttr* roAttr3 = new wxGridCellAttr;
    roAttr3->SetReadOnly( true );
    m_classesGrid->SetColAttr( CLASS_COL_MEMBERS, roAttr3 );

    m_promoteButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteChainButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_refreshButton->SetBitmap( KiBitmapBundle( BITMAPS::refresh ) );
    m_addClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_renameClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_deleteClassButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


PANEL_SETUP_NET_CHAINS::~PANEL_SETUP_NET_CHAINS()
{
}


void PANEL_SETUP_NET_CHAINS::loadFromModel()
{
    m_chainRows.clear();
    m_classRows.clear();
    m_potentialAutoNames.clear();

    if( !m_frame )
        return;

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return;

    // Pull the per-project chain-class map first so the chain rows can
    // pre-populate their chain-class cell.
    std::shared_ptr<NET_SETTINGS> ns = m_frame->Prj().GetProjectFile().NetSettings();
    std::map<wxString, wxString>  chainToClass;

    if( ns )
        chainToClass = ns->GetNetChainClasses();

    // Committed chains.
    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetCommittedNetChains() )
    {
        if( !chain )
            continue;

        CHAIN_ROW row;
        row.isPotential = false;
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

    // Potential chains.
    int potentialIdx = 0;

    for( const std::unique_ptr<SCH_NETCHAIN>& chain : graph->GetPotentialNetChains() )
    {
        if( !chain )
            continue;

        CHAIN_ROW row;
        row.isPotential = true;
        row.origName    = wxEmptyString;     // never committed
        row.newName     = wxEmptyString;
        row.livePtr     = chain.get();
        row.memberNets  = chain->GetNets();

        // Suggest a name from the longest member-net name; the user can edit
        // the cell or just accept it before promoting.
        wxString suggestion;

        for( const wxString& net : row.memberNets )
        {
            if( net.length() > suggestion.length() )
                suggestion = net;
        }

        m_potentialAutoNames[(int) m_chainRows.size()] = suggestion;
        m_chainRows.push_back( std::move( row ) );
        ++potentialIdx;
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

    int committedCount = 0;
    int potentialCount = 0;

    for( const CHAIN_ROW& row : m_chainRows )
    {
        if( row.deletePending )
            continue;

        if( row.isPotential && !row.promotePending )
            ++potentialCount;
        else
            ++committedCount;
    }

    m_chainsHeader->SetLabel( wxString::Format( _( "%d net chain(s) — %d committed, %d potential" ),
                                                committedCount + potentialCount,
                                                committedCount, potentialCount ) );

    m_chainsGrid->AppendRows( static_cast<int>( m_chainRows.size() ) );

    for( size_t i = 0; i < m_chainRows.size(); ++i )
    {
        const CHAIN_ROW& row = m_chainRows[i];
        int              r   = static_cast<int>( i );

        m_chainsGrid->SetCellValue( r, COL_STATUS,
                                    ( row.isPotential && !row.promotePending )
                                            ? c_statusPotential : c_statusCommitted );

        m_chainsGrid->SetCellValue( r, COL_NAME, row.newName );

        // Members display: "<n> nets: a, b, c…" with full list in tooltip
        wxString memberSummary = wxString::Format( _( "%zu nets" ), row.memberNets.size() );
        wxString memberTooltip;
        bool     first = true;

        for( const wxString& n : row.memberNets )
        {
            if( !first )
                memberTooltip += wxT( ", " );

            memberTooltip += n;
            first = false;
        }

        m_chainsGrid->SetCellValue( r, COL_MEMBERS, memberSummary );
        // Per-cell tooltips aren't supported across our wxGrid version, so the
        // summary string carries the count; the full member list lives in the
        // model and can be surfaced via a future right-click "Show members"
        // action.  (memberTooltip is intentionally unused for now.)
        (void) memberTooltip;

        m_chainsGrid->SetCellValue( r, COL_CHAIN_CLASS, row.newChainClass );
        m_chainsGrid->SetCellValue( r, COL_NET_CLASS, row.newNetClass );

        if( row.newColor != KIGFX::COLOR4D::UNSPECIFIED )
            m_chainsGrid->SetCellValue( r, COL_COLOUR, row.newColor.ToCSSString() );

        // Potential rows: name column is editable (so the user can supply a
        // promote name); other rows: name editable too; deletePending: grey out.
        if( row.deletePending )
        {
            for( int c = 0; c < m_chainsGrid->GetNumberCols(); ++c )
                m_chainsGrid->SetReadOnly( r, c, true );
        }
    }
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
    for( size_t i = 0; i < m_chainRows.size(); ++i )
    {
        m_chainRows[i].newName = m_chainsGrid->GetCellValue( static_cast<int>( i ), COL_NAME );
        m_chainRows[i].newChainClass =
                m_chainsGrid->GetCellValue( static_cast<int>( i ), COL_CHAIN_CLASS );
        m_chainRows[i].newNetClass =
                m_chainsGrid->GetCellValue( static_cast<int>( i ), COL_NET_CLASS );

        wxString colorStr = m_chainsGrid->GetCellValue( static_cast<int>( i ), COL_COLOUR );

        if( colorStr.IsEmpty() )
            m_chainRows[i].newColor = KIGFX::COLOR4D::UNSPECIFIED;
        else
            m_chainRows[i].newColor = KIGFX::COLOR4D( colorStr );

        // Promoted potentials need a non-empty name.
        if( m_chainRows[i].isPotential && !m_chainRows[i].newName.IsEmpty() )
            m_chainRows[i].promotePending = true;
    }

    for( size_t i = 0; i < m_classRows.size(); ++i )
    {
        m_classRows[i].newName =
                m_classesGrid->GetCellValue( static_cast<int>( i ), CLASS_COL_NAME );
    }

    // Reject empty committed chain names and duplicate names within the grid.
    for( size_t i = 0; i < m_chainRows.size(); ++i )
    {
        const CHAIN_ROW& row = m_chainRows[i];

        if( row.deletePending )
            continue;

        if( !row.isPotential && row.newName.IsEmpty() )
        {
            wxMessageBox( wxString::Format( _( "Net chain on row %zu cannot have an empty name." ),
                                            i + 1 ),
                          _( "Net Chains" ), wxOK | wxICON_ERROR, this );
            return false;
        }

        if( !row.newName.IsEmpty() && nameInChainGridAlready( row.newName, static_cast<int>( i ) ) )
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

    // Step 1 — promote potentials so the rest of the steps can find their
    // newly-committed live pointer.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( !row.isPotential || row.deletePending || !row.promotePending )
            continue;

        SCH_NETCHAIN* committed =
                graph->CreateNetChainFromPotential( row.livePtr, row.newName );

        if( committed )
        {
            row.livePtr     = committed;
            row.isPotential = false;
            row.origName    = row.newName;
        }
    }

    // Step 2 — apply renames on committed chains whose name changed.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( row.isPotential || row.deletePending )
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

    // Step 3 — colour and netclass override edits on committed chains.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( row.isPotential || row.deletePending || !row.livePtr )
            continue;

        row.livePtr->SetColor( row.newColor );
        row.livePtr->SetNetClass( row.newNetClass );
    }

    // Step 4 — chain-class assignments into NET_SETTINGS.
    if( ns )
    {
        for( const CHAIN_ROW& row : m_chainRows )
        {
            if( row.isPotential || row.deletePending )
                continue;

            ns->SetNetChainClass( row.newName, row.newChainClass );
        }
    }

    // Step 5 — deletions of committed chains.  Done last so renames above
    // don't see ghost rows.
    for( CHAIN_ROW& row : m_chainRows )
    {
        if( !row.deletePending || row.isPotential )
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

            for( const auto& [chainName, className] : ns->GetNetChainClasses() )
            {
                if( className == cls.origName )
                    ns->SetNetChainClass( chainName, wxEmptyString );
            }
        }

        for( const CLASS_ROW& cls : m_classRows )
        {
            if( cls.deletePending )
                continue;

            if( !cls.origName.IsEmpty() && cls.origName != cls.newName )
            {
                // Re-assign every chain that referenced the old class label.
                for( const auto& [chainName, className] : ns->GetNetChainClasses() )
                {
                    if( className == cls.origName )
                        ns->SetNetChainClass( chainName, cls.newName );
                }
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

    return m_chainsGrid->GetGridCursorRow();
}


int PANEL_SETUP_NET_CHAINS::selectedClassRow() const
{
    wxArrayInt rows = m_classesGrid->GetSelectedRows();

    if( !rows.empty() )
        return rows.front();

    return m_classesGrid->GetGridCursorRow();
}


bool PANEL_SETUP_NET_CHAINS::isReservedChainName( const wxString& aName ) const
{
    if( aName.IsEmpty() )
        return false;

    // Reject characters that the file format / DRC scope syntax can't carry.
    for( wxUniChar c : aName )
    {
        if( c == '"' || c == '\'' || c == '(' || c == ')' || c == ' ' )
            return true;
    }

    return false;
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


void PANEL_SETUP_NET_CHAINS::OnPromoteClicked( wxCommandEvent& )
{
    int r = selectedChainRow();

    if( r < 0 || r >= static_cast<int>( m_chainRows.size() ) )
        return;

    CHAIN_ROW& row = m_chainRows[r];

    if( !row.isPotential || row.promotePending )
        return;

    wxString suggested = row.newName;

    if( suggested.IsEmpty() )
    {
        auto it = m_potentialAutoNames.find( r );

        if( it != m_potentialAutoNames.end() )
            suggested = it->second;
    }

    wxTextEntryDialog dlg( this, _( "Net chain name:" ), _( "Promote Net Chain" ),
                           suggested );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString name = dlg.GetValue();

    if( name.IsEmpty() || isReservedChainName( name ) || nameInChainGridAlready( name, r ) )
    {
        wxMessageBox( _( "That name is reserved or already in use." ),
                      _( "Promote Net Chain" ), wxOK | wxICON_ERROR, this );
        return;
    }

    row.newName        = name;
    row.promotePending = true;

    rebuildChainsGrid();
}


void PANEL_SETUP_NET_CHAINS::OnDeleteChainClicked( wxCommandEvent& )
{
    int r = selectedChainRow();

    if( r < 0 || r >= static_cast<int>( m_chainRows.size() ) )
        return;

    CHAIN_ROW& row = m_chainRows[r];

    if( row.isPotential )
        return; // potentials can't be deleted, only ignored

    if( wxMessageBox( wxString::Format( _( "Delete net chain '%s'?" ), row.newName ),
                      _( "Delete Net Chain" ), wxYES_NO | wxICON_QUESTION, this ) != wxYES )
    {
        return;
    }

    row.deletePending = true;
    rebuildChainsGrid();
    rebuildClassesGrid();
}


void PANEL_SETUP_NET_CHAINS::OnRefreshClicked( wxCommandEvent& )
{
    if( !m_frame )
        return;

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();

    if( !graph )
        return;

    graph->Recalculate( m_frame->Schematic().BuildSheetListSortedByPageNumbers(), true );
    loadFromModel();
    rebuildChainsGrid();
    rebuildClassesGrid();
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

    row.newName = name;
    rebuildClassesGrid();
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
        if( chainRow.newChainClass == row.origName )
            chainRow.newChainClass = wxEmptyString;
    }

    rebuildClassesGrid();
    rebuildChainsGrid();
    refreshChainClassDropdownChoices();
}
