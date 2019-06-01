/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_selection_tool.h>
#include <view/view_controls.h>
#include <sch_component.h>
#include <sch_marker.h>
#include <id.h>
#include <kiway.h>
#include <ee_hotkeys.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/selection.h>
#include <tool/tool_manager.h>
#include <search_stack.h>
#include <sim/sim_plot_frame.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <eda_doc.h>
#include <invoke_sch_dialog.h>
#include <project.h>

TOOL_ACTION EE_ACTIONS::runERC( "eeschame.InspectionTool.runERC",
        AS_GLOBAL, 0,
        _( "Electrical Rules &Checker" ), _( "Perform electrical rules check" ),
        erc_xpm );

TOOL_ACTION EE_ACTIONS::runSimulation( "eeschema.EditorControl.runSimulation",
        AS_GLOBAL, 0,
        _( "Simulator..." ), _( "Simulate circuit in SPICE" ),
        simulator_xpm );

TOOL_ACTION EE_ACTIONS::showDatasheet( "eeschema.InspectionTool.showDatasheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SHOW_DATASHEET ),
        _( "Show Datasheet" ), _( "Opens the datasheet in a browser" ),
        datasheet_xpm );

TOOL_ACTION EE_ACTIONS::showMarkerInfo( "eeschema.InspectionTool.showMarkerInfo",
        AS_GLOBAL, 0,
        _( "Show Marker Info" ), _( "Display the marker's info in a dialog" ),
        info_xpm );


EE_INSPECTION_TOOL::EE_INSPECTION_TOOL()
    : EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" )
{
}


bool EE_INSPECTION_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto singleMarkerCondition = SELECTION_CONDITIONS::OnlyType( SCH_MARKER_T )
                              && SELECTION_CONDITIONS::Count( 1 );

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet, EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );
    selToolMenu.AddItem( EE_ACTIONS::showMarkerInfo, singleMarkerCondition && EE_CONDITIONS::Idle, 220 );

    return true;
}


int EE_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
    wxWindow*       erc = wxWindow::FindWindowById( ID_DIALOG_ERC, m_frame );

    if( erc )
        // Bring it to the top if already open.  Dual monitor users need this.
        erc->Raise();
    else if( editFrame )
        InvokeDialogERC( editFrame );

    return 0;
}


int EE_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
#ifdef KICAD_SPICE
    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );
    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();
#endif /* KICAD_SPICE */
    return 0;
}


int EE_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    wxString datasheet;

    LIB_EDIT_FRAME* libEditFrame = dynamic_cast<LIB_EDIT_FRAME*>( m_frame );
    LIB_VIEW_FRAME* libViewFrame = dynamic_cast<LIB_VIEW_FRAME*>( m_frame );

    if( libEditFrame )
    {
        LIB_PART* part = libEditFrame->GetCurPart();

        if( !part )
            return 0;

        if( part->GetAliasCount() > 1 )
        {
            ACTION_MENU  popup;
            wxString     msg;
            int          id = 0;

            for( LIB_ALIAS* alias : part->GetAliases() )
            {
                msg.Printf( wxT( "%s (%s)" ), alias->GetName(), alias->GetDocFileName() );
                popup.Append( id++, msg );
            }

            m_frame->PopupMenu( &popup );

            if( popup.GetSelected() >= 0 )
                datasheet = part->GetAlias( (unsigned) popup.GetSelected() )->GetDocFileName();
        }
        else
            datasheet = part->GetAlias( 0 )->GetDocFileName();
    }
    else if( libViewFrame )
    {
        LIB_ALIAS* entry = libViewFrame->GetSelectedAlias();

        if( !entry )
            return 0;

        datasheet = entry->GetDocFileName();
    }
    else
    {
        SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

        if( selection.Empty() )
            return 0;

        SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

        datasheet = component->GetField( DATASHEET )->GetText();
    }

    if( !datasheet.IsEmpty() && datasheet != wxT( "~" ) )
    {
        SEARCH_STACK* lib_search = m_frame->Prj().SchSearchS();

        GetAssociatedDocument( m_frame, datasheet, lib_search );
    }

    return 0;
}


int EE_INSPECTION_TOOL::ShowMarkerInfo( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return 0;

    SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( selection.Front() );

    if( marker )
        marker->DisplayMarkerInfo( m_frame );

    return 0;
}


int EE_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    SELECTION&         selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->ClearMsgPanel();
    }

    return 0;
}


void EE_INSPECTION_TOOL::setTransitions()
{
    Go( &EE_INSPECTION_TOOL::RunERC,              EE_ACTIONS::runERC.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::RunSimulation,       EE_ACTIONS::runSimulation.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       EE_ACTIONS::showDatasheet.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ShowMarkerInfo,      EE_ACTIONS::showMarkerInfo.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
}


