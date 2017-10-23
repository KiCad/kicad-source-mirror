/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "selection_tool.h"

TOOL_ACTION GERBVIEW_ACTIONS::selectionTool( "gerbview.Control.selectionTool",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE );

TOOL_ACTION GERBVIEW_ACTIONS::layerChanged( "gerbview.Control.layerChanged",
        AS_GLOBAL, 0,
        "", "", NULL, AF_NOTIFY );

TOOL_ACTION GERBVIEW_ACTIONS::highlightClear( "gerbview.Control.highlightClear",
        AS_GLOBAL, 0,
        _( "Clear Highlight" ), "", highlight_remove_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::highlightNet( "gerbview.Control.highlightNet",
        AS_GLOBAL, 0,
        _( "Highlight Net" ), "", general_ratsnest_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::highlightComponent( "gerbview.Control.highlightComponent",
        AS_GLOBAL, 0,
        _( "Highlight Component" ), "", file_footprint_xpm );

TOOL_ACTION GERBVIEW_ACTIONS::highlightAttribute( "gerbview.Control.highlightAttribute",
        AS_GLOBAL, 0,
        _( "Highlight Attribute" ), "", flag_xpm );

GERBVIEW_CONTROL::GERBVIEW_CONTROL() :
    TOOL_INTERACTIVE( "gerbview.Control" ), m_frame( NULL )
{
}


GERBVIEW_CONTROL::~GERBVIEW_CONTROL()
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
    GERBER_DRAW_ITEM* item = NULL;

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
        m_frame->m_SelNetnameBox->SetStringSelection( string );
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

    m_frame->GetGalCanvas()->GetView()->RecacheAllItems();
    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


void GERBVIEW_CONTROL::setTransitions()
{
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightClear.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightNet.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightComponent.MakeEvent() );
    Go( &GERBVIEW_CONTROL::HighlightControl,   GERBVIEW_ACTIONS::highlightAttribute.MakeEvent() );
}
