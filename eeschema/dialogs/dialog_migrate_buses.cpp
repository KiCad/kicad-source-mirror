/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_connection.h>
#include <schematic.h>
#include <connection_graph.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <dialog_migrate_buses.h>
#include <sch_label.h>
#include <view/view_controls.h>

/**
 * Migrates buses using legacy multi-label joining behavior.
 *
 * In KiCad versions before 6.0, you were allowed to place multiple labels
 * on a given bus subgraph, and that would have the effect of making those
 * bus descriptions equivalent according to the bus vector number.
 *
 * For example, if the labels PCA[0..15], ADR[0.7], and BUS[5..10] are all
 * attached to the same subgraph, the intention is that there is connectivity
 * between PCA0 and ADR0, between PCA10 and BUS10, and between PCA5, ADR5,
 * and BUS5 (basically connect all the prefix names where the vector numbers
 * line up).
 *
 * This is no longer allowed, because it doesn't map well onto the new
 * bus groups feature and because it is confusing (the netlist will take on
 * one of the possible names and it's impossible to control which one is
 * used).
 *
 * This dialog identifies all of the subgraphs that have this behavior,
 * and corrects them by determining a new name for the subgraph and removing
 * all but one label.  The name is determined by choosing a prefix and bus
 * vector notation that can capture all the attached buses.  In the above
 * example, the result would need to have the vector notation [0..15] to
 * capture all of the attached buses, and the name could be any of PCA, ADR,
 * or BUS.  We present a dialog to the user for them to select which name
 * they want to use.
 */


DIALOG_MIGRATE_BUSES::DIALOG_MIGRATE_BUSES( SCH_EDIT_FRAME* aParent )
        : DIALOG_MIGRATE_BUSES_BASE( aParent ), m_frame( aParent ), m_selected_index( 0 )
{
    OptOut( this );     // No control state save/restore

    m_migration_list->Bind( wxEVT_LIST_ITEM_SELECTED, &DIALOG_MIGRATE_BUSES::onItemSelected, this );
    m_btn_accept->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MIGRATE_BUSES::onAcceptClicked, this );

    loadGraphData();
    updateUi();

    aParent->GetToolManager()->RunAction( ACTIONS::zoomFitScreen );
}


DIALOG_MIGRATE_BUSES::~DIALOG_MIGRATE_BUSES()
{
    m_migration_list->Unbind( wxEVT_LIST_ITEM_SELECTED, &DIALOG_MIGRATE_BUSES::onItemSelected, this );
    m_btn_accept->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MIGRATE_BUSES::onAcceptClicked, this );
}


void DIALOG_MIGRATE_BUSES::loadGraphData()
{
    m_items.clear();
    auto subgraphs = m_frame->Schematic().ConnectionGraph()->GetBusesNeedingMigration();

    for( const CONNECTION_SUBGRAPH* subgraph : subgraphs )
    {
        BUS_MIGRATION_STATUS status;

        status.subgraph = subgraph;
        status.approved = false;

        std::vector<SCH_ITEM*> labels = subgraph->GetVectorBusLabels();
        wxASSERT( labels.size() > 1 );

        for( SCH_ITEM* label : labels )
            status.labels.push_back( static_cast<SCH_LABEL_BASE*>( label )->GetText() );

        status.possible_labels = getProposedLabels( status.labels );
        m_items.push_back( status );
    }
}


void DIALOG_MIGRATE_BUSES::updateUi()
{
    m_migration_list->DeleteAllItems();

    m_migration_list->InsertColumn( 0, _( "Sheet" ) );
    m_migration_list->InsertColumn( 1, _( "Conflicting Labels" ) );
    m_migration_list->InsertColumn( 2, _( "New Label" ) );
    m_migration_list->InsertColumn( 3, _( "Status" ) );

    for( auto& item : m_items )
    {
        wxString old = item.labels[0];

        for( unsigned j = 1; j < item.labels.size(); j++ )
            old << ", " << item.labels[j];

        auto i = m_migration_list->InsertItem( m_migration_list->GetItemCount(), wxEmptyString );

        m_migration_list->SetItem( i, 0, item.subgraph->GetSheet().PathHumanReadable() );
        m_migration_list->SetItem( i, 1, old );
        m_migration_list->SetItem( i, 2, item.possible_labels[0] );
        m_migration_list->SetItem( i, 3, "" );
    }

    m_migration_list->Select( 0 );
    m_migration_list->SetColumnWidth( 1, -1 );
}


std::vector<wxString> DIALOG_MIGRATE_BUSES::getProposedLabels( const std::vector<wxString>& aLabelList )
{
    int lowest_start = INT_MAX;
    int highest_end = -1;
    int widest_bus = -1;

    SCH_CONNECTION conn( m_frame->Schematic().ConnectionGraph() );

    for( const wxString& label : aLabelList )
    {
        conn.ConfigureFromLabel( label );

        int start = conn.VectorStart();
        int end = conn.VectorEnd();

        if( start < lowest_start )
            lowest_start = start;

        if( end > highest_end )
            highest_end = end;

        if( end - start + 1 > widest_bus )
            widest_bus = end - start + 1;
    }

    std::vector<wxString> proposals;

    for( const wxString& label : aLabelList )
    {
        conn.ConfigureFromLabel( label );
        wxString proposal = conn.VectorPrefix();
        proposal << "[" << highest_end << ".." << lowest_start << "]";
        proposals.push_back( proposal );
    }

    return proposals;
}


void DIALOG_MIGRATE_BUSES::onItemSelected( wxListEvent& aEvent )
{
    unsigned sel = aEvent.GetIndex();
    wxASSERT( sel < m_items.size() );

    m_selected_index = sel;

    const CONNECTION_SUBGRAPH* subgraph = m_items[sel].subgraph;
    const SCH_SHEET_PATH&      sheet = subgraph->GetSheet();
    const SCH_ITEM*            driver = subgraph->GetDriver();

    if( sheet != m_frame->GetCurrentSheet() )
    {
        sheet.UpdateAllScreenReferences();
        m_frame->Schematic().SetCurrentSheet( sheet );
        m_frame->TestDanglingEnds();
    }

    VECTOR2I pos = driver->GetPosition();

    m_frame->GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( pos, false );
    m_frame->RedrawScreen( pos, false );

    m_cb_new_name->Clear();

    for( const wxString& option : m_items[sel].possible_labels )
        m_cb_new_name->Append( option );

    m_cb_new_name->Select( 0 );
}


void DIALOG_MIGRATE_BUSES::onAcceptClicked( wxCommandEvent& aEvent )
{
    wxASSERT( m_selected_index < m_items.size() );

    unsigned sel = m_selected_index;

    m_items[sel].approved_label = m_cb_new_name->GetStringSelection();
    m_items[sel].approved = true;

    std::vector<SCH_ITEM*> labels =  m_items[sel].subgraph->GetVectorBusLabels();

    for( SCH_ITEM* label : labels )
        static_cast<SCH_LABEL_BASE*>( label )->SetText( m_items[sel].approved_label );

    m_migration_list->SetItem( sel, 2, m_items[sel].approved_label );
    m_migration_list->SetItem( sel, 3, _( "Updated" ) );

    if( sel < m_items.size() - 1 )
        m_migration_list->Select( sel + 1 );

    m_frame->GetCanvas()->Refresh();
}
