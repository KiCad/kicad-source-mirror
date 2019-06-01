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
#include <ee_hotkeys.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/selection.h>
#include <tool/tool_manager.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <eda_doc.h>
#include <invoke_sch_dialog.h>

TOOL_ACTION EE_ACTIONS::showDatasheet( "eeschema.InspectionTool.showDatasheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SHOW_COMPONENT_DATASHEET ),
        _( "Show Datasheet" ), _( "Opens the datasheet in a browser" ),
        datasheet_xpm );

TOOL_ACTION EE_ACTIONS::runERC( "eeschame.InspectionTool.runERC",
        AS_GLOBAL, 0,
        _( "Electrical Rules &Checker" ), _( "Perform electrical rules check" ),
        erc_xpm );

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

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet, EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 400 );
    selToolMenu.AddItem( EE_ACTIONS::showMarkerInfo, singleMarkerCondition && EE_CONDITIONS::Idle, 400 );

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


int EE_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return 0;

    SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();
    wxString       datasheet = component->GetField( DATASHEET )->GetText();

    if( !datasheet.IsEmpty() )
        GetAssociatedDocument( m_frame, datasheet );

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
    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       EE_ACTIONS::showDatasheet.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ShowMarkerInfo,      EE_ACTIONS::showMarkerInfo.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
}


