/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <view/view.h>
#include <gerbview_painter.h>
#include <gerbview_frame.h>
#include <tool/tool_manager.h>
#include <menus_helpers.h>
#include "gerbview_actions.h"
#include "gerbview_control.h"
#include "gerbview_selection_tool.h"


GERBVIEW_CONTROL::GERBVIEW_CONTROL() :
    TOOL_INTERACTIVE( "gerbview.Control" ), 
    m_frame( nullptr )
{
}


void GERBVIEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<GERBVIEW_FRAME>();
}


int GERBVIEW_CONTROL::HighlightControl( const TOOL_EVENT& aEvent )
{
    auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>( getView()->GetPainter() )->GetSettings();
    const auto& selection = m_toolMgr->GetTool<GERBVIEW_SELECTION_TOOL>()->GetSelection();
    GERBER_DRAW_ITEM* item = nullptr;

    if( selection.Size() == 1 )
    {
        item = static_cast<GERBER_DRAW_ITEM*>( selection[0] );
    }

    if( aEvent.IsAction( &GERBVIEW_ACTIONS::highlightClear ) )
    {
        m_frame->m_SelComponentBox->SetSelection( 0 );
        m_frame->m_SelNetnameBox->SetSelection( 0 );
        m_frame->m_SelAperAttributesBox->SetSelection( 0 );

        settings->m_netHighlightString = "";
        settings->m_componentHighlightString = "";
        settings->m_attributeHighlightString = "";
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightNet ) )
    {
        auto string = item->GetNetAttributes().m_Netname;
        settings->m_netHighlightString = string;
        m_frame->m_SelNetnameBox->SetStringSelection( UnescapeString( string ) );
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightComponent ) )
    {
        auto string = item->GetNetAttributes().m_Cmpref;
        settings->m_componentHighlightString = string;
        m_frame->m_SelComponentBox->SetStringSelection( string );
    }
    else if( item && aEvent.IsAction( &GERBVIEW_ACTIONS::highlightAttribute ) )
    {
        D_CODE* apertDescr = item->GetDcodeDescr();
        if( apertDescr )
        {
            auto string = apertDescr->m_AperFunction;
            settings->m_attributeHighlightString = string;
            m_frame->m_SelAperAttributesBox->SetStringSelection( string );
        }
    }

    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::DisplayControl( const TOOL_EVENT& aEvent )
{
    bool state;
    bool needs_refresh = false;
    auto options = m_frame->GetDisplayOptions();

    if( aEvent.IsAction( &GERBVIEW_ACTIONS::linesDisplayOutlines ) )
    {
        options.m_DisplayLinesFill = !options.m_DisplayLinesFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::flashedDisplayOutlines ) )
    {
        options.m_DisplayFlashedItemsFill = !options.m_DisplayFlashedItemsFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::polygonsDisplayOutlines ) )
    {
        options.m_DisplayPolygonsFill = !options.m_DisplayPolygonsFill;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::negativeObjectDisplay ) )
    {
        state = !m_frame->IsElementVisible( LAYER_NEGATIVE_OBJECTS );
        m_frame->SetElementVisibility( LAYER_NEGATIVE_OBJECTS, state );
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::dcodeDisplay ) )
    {
        state = !m_frame->IsElementVisible( LAYER_DCODES );
        m_frame->SetElementVisibility( LAYER_DCODES, state );
    }
    else if( aEvent.IsAction( &ACTIONS::highContrastMode ) )
    {
        options.m_HighContrastMode = !options.m_HighContrastMode;
        needs_refresh = true;
    }
    else if( aEvent.IsAction( &GERBVIEW_ACTIONS::toggleDiffMode ) )
    {
        options.m_DiffMode = !options.m_DiffMode;
        needs_refresh = true;
    }

    if( needs_refresh )
        m_frame->UpdateDisplayOptions( options );

    return 0;
}


int GERBVIEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer < GERBER_DRAWLAYERS_COUNT - 1 )
        m_frame->SetActiveLayer( layer + 1, true );

    return 0;
}


int GERBVIEW_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    int layer = m_frame->GetActiveLayer();

    if( layer > 0 )
        m_frame->SetActiveLayer( layer - 1, true );

    return 0;
}


int GERBVIEW_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    GERBVIEW_SELECTION_TOOL* selTool = m_toolMgr->GetTool<GERBVIEW_SELECTION_TOOL>();
    GERBVIEW_SELECTION&      selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->EraseMsgBox();
    }

    return 0;
}


void GERBVIEW_CONTROL::setTransitions()
{
    Go( &GERBVIEW_CONTROL::Print,              ACTIONS::print.MakeEvent() );

    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightClear.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightNet.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightComponent.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightAttribute.MakeEvent() );

    Go( &GERBVIEW_CONTROL::LayerNext,          GERBVIEW_ACTIONS::layerNext.MakeEvent() );
    Go( &GERBVIEW_CONTROL::LayerPrev,          GERBVIEW_ACTIONS::layerPrev.MakeEvent() );

    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::linesDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::flashedDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::polygonsDisplayOutlines.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::negativeObjectDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::dcodeDisplay.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     ACTIONS::highContrastMode.MakeEvent() );
    Go( &GERBVIEW_CONTROL::DisplayControl,     GERBVIEW_ACTIONS::toggleDiffMode.MakeEvent() );

    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::SelectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::UnselectedEvent );
    Go( &GERBVIEW_CONTROL::UpdateMessagePanel, EVENTS::ClearedEvent );
}
