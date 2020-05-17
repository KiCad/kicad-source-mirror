/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_shim.h>
#include <dialogs/panel_mouse_settings.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <view/view_controls.h>
#include <wx/defs.h>

using KIGFX::MOUSE_DRAG_ACTION;


PANEL_MOUSE_SETTINGS::PANEL_MOUSE_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent ) :
        PANEL_MOUSE_SETTINGS_BASE( aParent ),
        m_dialog( aDialog )
{
    m_checkAutoZoomSpeed->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED,
            [&]( wxCommandEvent& aEvt )
            {
                m_zoomSpeed->Enable( !m_checkAutoZoomSpeed->GetValue() );
            } );
}


PANEL_MOUSE_SETTINGS::~PANEL_MOUSE_SETTINGS()
{
}


bool PANEL_MOUSE_SETTINGS::TransferDataToWindow()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    m_checkZoomCenter->SetValue( cfg->m_Input.center_on_zoom );
    m_checkAutoPan->SetValue( cfg->m_Input.auto_pan );
    m_checkZoomAcceleration->SetValue( cfg->m_Input.zoom_acceleration );
    m_zoomSpeed->SetValue( cfg->m_Input.zoom_speed );
    m_checkAutoZoomSpeed->SetValue( cfg->m_Input.zoom_speed_auto );
    m_checkEnablePanH->SetValue( cfg->m_Input.horizontal_pan );
    m_autoPanSpeed->SetValue( cfg->m_Input.auto_pan_acceleration );

    m_zoomSpeed->Enable( !cfg->m_Input.zoom_speed_auto );

    auto set_mouse_buttons =
            []( const MOUSE_DRAG_ACTION& aVal, wxChoice* aChoice )
            {
                switch( aVal )
                {
                case MOUSE_DRAG_ACTION::PAN:
                    aChoice->SetSelection( 0 );
                    break;

                case MOUSE_DRAG_ACTION::ZOOM:
                    aChoice->SetSelection( 1 );
                    break;

                case MOUSE_DRAG_ACTION::NONE:
                    aChoice->SetSelection( 2 );
                    break;

                case MOUSE_DRAG_ACTION::SELECT:
                default:
                    break;
                }
            };

    set_mouse_buttons(
            static_cast<MOUSE_DRAG_ACTION>( cfg->m_Input.drag_middle ), m_choiceMiddleButtonDrag );

    set_mouse_buttons(
            static_cast<MOUSE_DRAG_ACTION>( cfg->m_Input.drag_right ), m_choiceRightButtonDrag );

    m_currentScrollMod.zoom = cfg->m_Input.scroll_modifier_zoom;
    m_currentScrollMod.panh = cfg->m_Input.scroll_modifier_pan_h;
    m_currentScrollMod.panv = cfg->m_Input.scroll_modifier_pan_v;

    updateScrollModButtons();

    return true;
}


bool PANEL_MOUSE_SETTINGS::TransferDataFromWindow()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    m_currentScrollMod = getScrollModSet();

    if( !isScrollModSetValid( m_currentScrollMod ) )
        return false;

    int  drag_middle = static_cast<int>( MOUSE_DRAG_ACTION::NONE );
    int  drag_right  = static_cast<int>( MOUSE_DRAG_ACTION::NONE );

    switch( m_choiceMiddleButtonDrag->GetSelection() )
    {
    case 0:
        drag_middle = static_cast<int>( MOUSE_DRAG_ACTION::PAN );
        break;

    case 1:
        drag_middle = static_cast<int>( MOUSE_DRAG_ACTION::ZOOM );
        break;

    default:
    case 2:
        break;
    }

    switch( m_choiceRightButtonDrag->GetSelection() )
    {
    case 0:
        drag_right = static_cast<int>( MOUSE_DRAG_ACTION::PAN );
        break;

    case 1:
        drag_right = static_cast<int>( MOUSE_DRAG_ACTION::ZOOM );
        break;

    default:
    case 2:
        break;
    }

    cfg->m_Input.center_on_zoom        = m_checkZoomCenter->GetValue();
    cfg->m_Input.auto_pan              = m_checkAutoPan->GetValue();
    cfg->m_Input.auto_pan_acceleration = m_autoPanSpeed->GetValue();
    cfg->m_Input.zoom_acceleration     = m_checkZoomAcceleration->GetValue();
    cfg->m_Input.zoom_speed            = m_zoomSpeed->GetValue();
    cfg->m_Input.zoom_speed_auto       = m_checkAutoZoomSpeed->GetValue();
    cfg->m_Input.horizontal_pan        = m_checkEnablePanH->GetValue();

    cfg->m_Input.scroll_modifier_zoom  = m_currentScrollMod.zoom;
    cfg->m_Input.scroll_modifier_pan_h = m_currentScrollMod.panh;
    cfg->m_Input.scroll_modifier_pan_v = m_currentScrollMod.panv;

    cfg->m_Input.drag_middle           = drag_middle;
    cfg->m_Input.drag_right            = drag_right;

    return true;
}


void PANEL_MOUSE_SETTINGS::OnScrollRadioButton( wxCommandEvent& event )
{
    wxRadioButton* btn = dynamic_cast<wxRadioButton*>( event.GetEventObject() );

    if( !btn )
        return;

    SCROLL_MOD_SET newSet = getScrollModSet();

    if( isScrollModSetValid( newSet ) )
        m_currentScrollMod = newSet;
    else
        updateScrollModButtons();
}


SCROLL_MOD_SET PANEL_MOUSE_SETTINGS::getScrollModSet()
{
    SCROLL_MOD_SET ret = {};

    if( m_rbZoomShift->GetValue() )
        ret.zoom = WXK_SHIFT;
    else if( m_rbZoomCtrl->GetValue() )
        ret.zoom = WXK_CONTROL;
    else if( m_rbZoomAlt->GetValue() )
        ret.zoom = WXK_ALT;

    if( m_rbPanHShift->GetValue() )
        ret.panh = WXK_SHIFT;
    else if( m_rbPanHCtrl->GetValue() )
        ret.panh = WXK_CONTROL;
    else if( m_rbPanHAlt->GetValue() )
        ret.panh = WXK_ALT;

    if( m_rbPanVShift->GetValue() )
        ret.panv = WXK_SHIFT;
    else if( m_rbPanVCtrl->GetValue() )
        ret.panv = WXK_CONTROL;
    else if( m_rbPanVAlt->GetValue() )
        ret.panv = WXK_ALT;

    return ret;
}


bool PANEL_MOUSE_SETTINGS::isScrollModSetValid( const SCROLL_MOD_SET& aSet )
{
    return ( aSet.zoom != aSet.panh && aSet.panh != aSet.panv && aSet.panv != aSet.zoom );
}


void PANEL_MOUSE_SETTINGS::updateScrollModButtons()
{
    auto set_wheel_buttons =
            []( int aModifier, wxRadioButton* aNoneBtn, wxRadioButton* aCtrlBtn,
                wxRadioButton* aShiftBtn, wxRadioButton* aAltBtn )
            {
                switch( aModifier )
                {
                case 0:
                    aNoneBtn->SetValue( true );
                    break;

                case WXK_CONTROL:
                    aCtrlBtn->SetValue( true );
                    break;

                case WXK_SHIFT:
                    aShiftBtn->SetValue( true );
                    break;

                case WXK_ALT:
                    aAltBtn->SetValue( true );
                    break;
                }
            };

    set_wheel_buttons(
            m_currentScrollMod.zoom, m_rbZoomNone, m_rbZoomCtrl, m_rbZoomShift, m_rbZoomAlt );

    set_wheel_buttons(
            m_currentScrollMod.panh, m_rbPanHNone, m_rbPanHCtrl, m_rbPanHShift, m_rbPanHAlt );

    set_wheel_buttons(
            m_currentScrollMod.panv, m_rbPanVNone, m_rbPanVCtrl, m_rbPanVShift, m_rbPanVAlt );
}
