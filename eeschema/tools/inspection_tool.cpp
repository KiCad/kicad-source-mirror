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

#include <tools/sch_actions.h>
#include <tools/inspection_tool.h>
#include <tools/sch_selection_tool.h>
#include <view/view_controls.h>
#include <sch_component.h>
#include <sch_marker.h>
#include <hotkeys.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/selection.h>
#include <tool/tool_manager.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <eda_doc.h>


TOOL_ACTION SCH_ACTIONS::showDatasheet( "eeschema.InspectionTool.showDatasheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SHOW_COMPONENT_DATASHEET ),
        _( "Show Datasheet" ), _( "Opens the datasheet in a browser" ),
        datasheet_xpm );

TOOL_ACTION SCH_ACTIONS::showMarkerInfo( "eeschema.InspectionTool.showMarkerInfo",
        AS_GLOBAL, 0,
        _( "Show Marker Info" ), _( "Display the marker's info in a dialog" ),
        info_xpm );


INSPECTION_TOOL::INSPECTION_TOOL()
    : TOOL_INTERACTIVE( "eeschema.InspectionTool" ),
      m_selectionTool( nullptr ),
      m_view( nullptr ),
      m_controls( nullptr ),
      m_frame( nullptr )
{
}


bool INSPECTION_TOOL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );

    auto singleMarkerCondition = SELECTION_CONDITIONS::OnlyType( SCH_MARKER_T )
                              && SELECTION_CONDITIONS::Count( 1 );

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( SCH_ACTIONS::showDatasheet, SCH_CONDITIONS::SingleSymbol && SCH_CONDITIONS::Idle, 400 );
    selToolMenu.AddItem( SCH_ACTIONS::showMarkerInfo, singleMarkerCondition && SCH_CONDITIONS::Idle, 400 );

    return true;
}


void INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<SCH_BASE_FRAME>();
}


int INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::ComponentsOnly );

    if( selection.Empty() )
        return 0;

    SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();
    wxString       datasheet = component->GetField( DATASHEET )->GetText();

    if( !datasheet.IsEmpty() )
        GetAssociatedDocument( m_frame, datasheet );

    return 0;
}


int INSPECTION_TOOL::ShowMarkerInfo( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return 0;

    SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( selection.Front() );

    if( marker )
        marker->DisplayMarkerInfo( m_frame );

    return 0;
}


int INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION&          selection = selTool->GetSelection();

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


void INSPECTION_TOOL::setTransitions()
{
    Go( &INSPECTION_TOOL::ShowDatasheet,       SCH_ACTIONS::showDatasheet.MakeEvent() );
    Go( &INSPECTION_TOOL::ShowMarkerInfo,      SCH_ACTIONS::showMarkerInfo.MakeEvent() );

    Go( &INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
}


