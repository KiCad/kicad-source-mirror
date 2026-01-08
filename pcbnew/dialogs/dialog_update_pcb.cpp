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

#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <dialog_update_pcb.h>
#include <ratsnest/ratsnest_data.h>
#include "widgets/wx_html_report_panel.h"
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <view/view_controls.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>


DIALOG_UPDATE_PCB::DIALOG_UPDATE_PCB( PCB_EDIT_FRAME* aParent, NETLIST* aNetlist ) :
    DIALOG_UPDATE_PCB_BASE( aParent ),
    m_frame( aParent ),
    m_netlist( aNetlist ),
    m_initialized( false )
{
    m_messagePanel->SetLabel( _("Changes to Be Applied") );
    m_messagePanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    m_messagePanel->SetLazyUpdate( true );
    m_netlist->SortByReference();

    m_messagePanel->GetSizer()->SetSizeHints( this );
    m_messagePanel->Layout();

    SetupStandardButtons( { { wxID_OK,     _( "Update PCB" ) },
                            { wxID_CANCEL, _( "Close" )      } } );

    finishDialogSettings();

    m_initialized = true;
    PerformUpdate( true );
}


DIALOG_UPDATE_PCB::~DIALOG_UPDATE_PCB()
{
    if( m_runDragCommand )
    {
        PCB_SELECTION_TOOL* selTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
        PCB_SELECTION&      selection = selTool->GetSelection();

        // Set the reference point to (0,0) where the new footprints were spread. This ensures
        // the move tool knows where the items are located, preventing an offset when the "warp
        // cursor to origin of moved object" preference is disabled.
        if( selection.Size() > 0 )
            selection.SetReferencePoint( VECTOR2I( 0, 0 ) );

        KIGFX::VIEW_CONTROLS* controls = m_frame->GetCanvas()->GetViewControls();
        controls->SetCursorPosition( controls->GetMousePosition() );
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::move );
    }
}


void DIALOG_UPDATE_PCB::PerformUpdate( bool aDryRun )
{
    m_messagePanel->Clear();

    REPORTER& reporter = m_messagePanel->Reporter();

    m_runDragCommand = false;

    m_netlist->SetFindByTimeStamp( !m_cbRelinkFootprints->GetValue() );
    m_netlist->SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );

    if( !aDryRun )
    {
        m_frame->GetToolManager()->DeactivateTool();
        m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );
    }

    BOARD_NETLIST_UPDATER updater( m_frame, m_frame->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun );
    updater.SetLookupByTimestamp( !m_cbRelinkFootprints->GetValue() );
    updater.SetDeleteUnusedFootprints( m_cbDeleteExtraFootprints->GetValue());
    updater.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );
    updater.SetTransferGroups( m_cbTransferGroups->GetValue() );
    updater.SetOverrideLocks( m_cbOverrideLocks->GetValue() );
    updater.SetUpdateFields( m_cbUpdateFields->GetValue() );
    updater.SetRemoveExtraFields( m_cbRemoveExtraFields->GetValue() );
    updater.UpdateNetlist( *m_netlist );

    m_messagePanel->Flush( true );

    if( aDryRun )
        return;

    m_frame->OnNetlistChanged( updater, &m_runDragCommand );
}


void DIALOG_UPDATE_PCB::OnOptionChanged( wxCommandEvent& event )
{
    if( m_initialized )
    {
        PerformUpdate( true );
        m_sdbSizer1OK->Enable( true );
        m_sdbSizer1OK->SetDefault();
    }
}


void DIALOG_UPDATE_PCB::OnUpdateClick( wxCommandEvent& event )
{
    m_messagePanel->SetLabel( _( "Changes Applied to PCB" ) );
    PerformUpdate( false );

    m_sdbSizer1Cancel->SetDefault();
    // wxWidgets has a tendency to keep both buttons highlighted without the following:
    m_sdbSizer1OK->Enable( false );
}
