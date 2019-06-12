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
#include <hotkeys.h>

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

TOOL_ACTION GERBVIEW_ACTIONS::layerNext( "gerbview.Control.layerNext",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_NEXT ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::layerPrev( "gerbview.Control.layerPrev",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_PREVIOUS ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::linesDisplayOutlines( "gerbview.Control.linesDisplayOutlines",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_GBR_LINES_DISPLAY_MODE ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::flashedDisplayOutlines( "gerbview.Control.flashedDisplayOutlines",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_GBR_FLASHED_DISPLAY_MODE ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::polygonsDisplayOutlines( "gerbview.Control.polygonsDisplayOutlines",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_GBR_POLYGON_DISPLAY_MODE ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::negativeObjectDisplay( "gerbview.Control.negativeObjectDisplay",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_GBR_NEGATIVE_DISPLAY_ONOFF ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::dcodeDisplay( "gerbview.Control.dcodeDisplay",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_GBR_DCODE_DISPLAY_ONOFF ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::switchUnits( "gerbview.Control.switchUnits",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_UNITS ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::resetCoords( "gerbview.Control.resetCoords",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_RESET_LOCAL_COORD ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::showHotkeyList( "gerbview.Control.showHotkeyList",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_HELP ),
        "", "" );

TOOL_ACTION GERBVIEW_ACTIONS::showHelp( "gerbview.Control.showHelp",
        AS_GLOBAL, 0,
        "", "" );

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

    m_frame->GetGalCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int GERBVIEW_CONTROL::DisplayControl( const TOOL_EVENT& aEvent )
{
    bool state;
    bool needs_refresh = false;
    GBR_DISPLAY_OPTIONS options = m_frame->m_DisplayOptions;

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


int GERBVIEW_CONTROL::ResetCoords( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    m_frame->GetScreen()->m_O_Curseur = wxPoint( cursorPos.x, cursorPos.y );
    m_frame->UpdateStatusBar();

    return 0;
}


int GERBVIEW_CONTROL::SwitchUnits( const TOOL_EVENT& aEvent )
{
    // TODO: Refactor to share with pcbnew
    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );

    if( m_frame->GetUserUnits() == INCHES )
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_MM );
    else
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_INCH );

    m_frame->ProcessEvent( evt );

    return 0;
}


int GERBVIEW_CONTROL::ShowHotkeyList( const TOOL_EVENT& aEvent )
{
    DisplayHotkeyList( m_frame, m_frame->GetHotkeyConfig() );

    return 0;
}


int GERBVIEW_CONTROL::ShowHelp( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummyEvent;
    dummyEvent.SetId( wxID_HELP );
    m_frame->GetKicadHelp( dummyEvent );

    return 0;
}


void GERBVIEW_CONTROL::setTransitions()
{
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

    Go( &GERBVIEW_CONTROL::ResetCoords,        GERBVIEW_ACTIONS::resetCoords.MakeEvent() );
    Go( &GERBVIEW_CONTROL::SwitchUnits,        GERBVIEW_ACTIONS::switchUnits.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ShowHelp,           GERBVIEW_ACTIONS::showHelp.MakeEvent() );
    Go( &GERBVIEW_CONTROL::ShowHotkeyList,     GERBVIEW_ACTIONS::showHotkeyList.MakeEvent() );
}
