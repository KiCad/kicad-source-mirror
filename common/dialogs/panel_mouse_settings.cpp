/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <bitmaps.h>
#include <dialogs/panel_mouse_settings.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <widgets/paged_dialog.h>
#include <wx/defs.h>


PANEL_MOUSE_SETTINGS::PANEL_MOUSE_SETTINGS( wxWindow* aParent ) :
        PANEL_MOUSE_SETTINGS_BASE( aParent ),
        m_currentScrollMod( {} )
{
#ifdef __WXOSX_MAC__
    for( wxSizerItem* child : m_zoomSizer->GetChildren() )
    {
        if( child->GetWindow() == m_zoomSpeed )
            child->SetBorder( 14 );
    }

    for( wxSizerItem* child : m_panSizer->GetChildren() )
    {
        if( child->GetWindow() == m_autoPanSpeed )
            child->SetBorder( 14 );
    }

    m_lblCtrl->SetLabel( _( "Cmd" ) );
    m_lblAlt->SetLabel( _( "Option" ) );
#endif

    m_scrollWarning->SetBitmap( KiBitmapBundle( BITMAPS::small_warning ) );
    m_scrollWarning->Hide();

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
    const COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    applySettingsToPanel( *cfg );

    return true;
}


bool PANEL_MOUSE_SETTINGS::TransferDataFromWindow()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    m_currentScrollMod = getScrollModSet();

    if( !isScrollModSetValid( m_currentScrollMod ) )
    {
        PAGED_DIALOG::GetDialog( this )->SetError( _( "Only one action can be assigned to each "
                                                      "vertical scroll setting" ),
                                                   this, nullptr );
        return false;
    }

    switch( m_choiceLeftButtonDrag->GetSelection() )
    {
    case 0: cfg->m_Input.drag_left = MOUSE_DRAG_ACTION::SELECT;        break;
    case 1: cfg->m_Input.drag_left = MOUSE_DRAG_ACTION::DRAG_SELECTED; break;
    case 2: cfg->m_Input.drag_left = MOUSE_DRAG_ACTION::DRAG_ANY;      break;
    default:                                                           break;
    }

    switch( m_choiceMiddleButtonDrag->GetSelection() )
    {
    case 0: cfg->m_Input.drag_middle = MOUSE_DRAG_ACTION::PAN;  break;
    case 1: cfg->m_Input.drag_middle = MOUSE_DRAG_ACTION::ZOOM; break;
    case 2: cfg->m_Input.drag_middle = MOUSE_DRAG_ACTION::NONE; break;
    default:                                                    break;
    }

    switch( m_choiceRightButtonDrag->GetSelection() )
    {
    case 0: cfg->m_Input.drag_right = MOUSE_DRAG_ACTION::PAN;  break;
    case 1: cfg->m_Input.drag_right = MOUSE_DRAG_ACTION::ZOOM; break;
    case 2: cfg->m_Input.drag_right = MOUSE_DRAG_ACTION::NONE; break;
    default:                                                   break;
    }

    switch( m_choicePanMoveKey->GetSelection() )
    {
    case 1: cfg->m_Input.motion_pan_modifier = WXK_ALT;     break;
    case 2: cfg->m_Input.motion_pan_modifier = WXK_CONTROL; break;
    case 3: cfg->m_Input.motion_pan_modifier = WXK_SHIFT;   break;
    default: cfg->m_Input.motion_pan_modifier = 0;          break;
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

    cfg->m_Input.reverse_scroll_zoom  = m_checkZoomReverse->GetValue();
    cfg->m_Input.reverse_scroll_pan_h = m_checkPanHReverse->GetValue();

    return true;
}


void PANEL_MOUSE_SETTINGS::ResetPanel()
{
    COMMON_SETTINGS defaultSettings;

    defaultSettings.ResetToDefaults();

    applySettingsToPanel( defaultSettings );
}


void PANEL_MOUSE_SETTINGS::applySettingsToPanel( const COMMON_SETTINGS& aSettings )
{
    m_checkZoomCenter->SetValue( aSettings.m_Input.center_on_zoom );
    m_checkAutoPan->SetValue( aSettings.m_Input.auto_pan );
    m_checkZoomAcceleration->SetValue( aSettings.m_Input.zoom_acceleration );
    m_zoomSpeed->SetValue( aSettings.m_Input.zoom_speed );
    m_checkAutoZoomSpeed->SetValue( aSettings.m_Input.zoom_speed_auto );
    m_checkEnablePanH->SetValue( aSettings.m_Input.horizontal_pan );
    m_autoPanSpeed->SetValue( aSettings.m_Input.auto_pan_acceleration );

    m_zoomSpeed->Enable( !aSettings.m_Input.zoom_speed_auto );

    switch( aSettings.m_Input.drag_left )
    {
    case MOUSE_DRAG_ACTION::SELECT:        m_choiceLeftButtonDrag->SetSelection( 0 ); break;
    case MOUSE_DRAG_ACTION::DRAG_SELECTED: m_choiceLeftButtonDrag->SetSelection( 1 ); break;
    case MOUSE_DRAG_ACTION::DRAG_ANY:      m_choiceLeftButtonDrag->SetSelection( 2 ); break;
    default:                                                                          break;
    }

    switch( aSettings.m_Input.drag_middle )
    {
    case MOUSE_DRAG_ACTION::PAN:    m_choiceMiddleButtonDrag->SetSelection( 0 ); break;
    case MOUSE_DRAG_ACTION::ZOOM:   m_choiceMiddleButtonDrag->SetSelection( 1 ); break;
    case MOUSE_DRAG_ACTION::NONE:   m_choiceMiddleButtonDrag->SetSelection( 2 ); break;
    case MOUSE_DRAG_ACTION::SELECT:                                              break;
    default:                                                                     break;
    }

    switch( aSettings.m_Input.drag_right )
    {
    case MOUSE_DRAG_ACTION::PAN:    m_choiceRightButtonDrag->SetSelection( 0 ); break;
    case MOUSE_DRAG_ACTION::ZOOM:   m_choiceRightButtonDrag->SetSelection( 1 ); break;
    case MOUSE_DRAG_ACTION::NONE:   m_choiceRightButtonDrag->SetSelection( 2 ); break;
    case MOUSE_DRAG_ACTION::SELECT:                                             break;
    default:                                                                    break;
    }

    switch( aSettings.m_Input.motion_pan_modifier )
    {
    case WXK_ALT:     m_choicePanMoveKey->SetSelection( 1 ); break;
    case WXK_CONTROL: m_choicePanMoveKey->SetSelection( 2 ); break;
    case WXK_SHIFT:   m_choicePanMoveKey->SetSelection( 3 ); break;
    default:          m_choicePanMoveKey->SetSelection( 0 ); break;
    }

    m_currentScrollMod.zoom        = aSettings.m_Input.scroll_modifier_zoom;
    m_currentScrollMod.panh        = aSettings.m_Input.scroll_modifier_pan_h;
    m_currentScrollMod.panv        = aSettings.m_Input.scroll_modifier_pan_v;
    m_currentScrollMod.zoomReverse = aSettings.m_Input.reverse_scroll_zoom;
    m_currentScrollMod.panHReverse = aSettings.m_Input.reverse_scroll_pan_h;

    updateScrollModButtons();
}


void PANEL_MOUSE_SETTINGS::OnScrollRadioButton( wxCommandEvent& event )
{
    wxRadioButton* btn = dynamic_cast<wxRadioButton*>( event.GetEventObject() );

    if( !btn )
        return;

    bool isZoom = ( btn == m_rbZoomCtrl || btn == m_rbZoomShift || btn == m_rbZoomAlt );
    bool isPanH = ( btn == m_rbPanHCtrl || btn == m_rbPanHShift || btn == m_rbPanHAlt );
    bool isPanV = ( btn == m_rbPanVCtrl || btn == m_rbPanVShift || btn == m_rbPanVAlt );

    int modifier = 0;

    if( btn == m_rbZoomCtrl || btn == m_rbPanHCtrl || btn == m_rbPanVCtrl )
        modifier = WXK_CONTROL;
    else if( btn == m_rbZoomShift || btn == m_rbPanHShift || btn == m_rbPanVShift )
        modifier = WXK_SHIFT;
    else if( btn == m_rbZoomAlt || btn == m_rbPanHAlt || btn == m_rbPanVAlt )
        modifier = WXK_ALT;

    auto column_taken =
            [this]( int mod )
            {
                switch( mod )
                {
                case 0: return m_rbZoomNone->GetValue() || m_rbPanHNone->GetValue() || m_rbPanVNone->GetValue();
                case WXK_CONTROL: return m_rbZoomCtrl->GetValue() || m_rbPanHCtrl->GetValue() || m_rbPanVCtrl->GetValue();
                case WXK_SHIFT: return m_rbZoomShift->GetValue() || m_rbPanHShift->GetValue() || m_rbPanVShift->GetValue();
                case WXK_ALT: return m_rbZoomAlt->GetValue() || m_rbPanHAlt->GetValue() || m_rbPanVAlt->GetValue();
                default: return false;
                }
            };

    auto assign_first_available =
            [&]( wxRadioButton* noneBtn, wxRadioButton* ctrlBtn, wxRadioButton* shiftBtn, wxRadioButton* altBtn )
            {
                int candidates[] = { 0, WXK_CONTROL, WXK_SHIFT, WXK_ALT };

                for( int candidate : candidates )
                {
                    if( candidate == modifier )
                        continue;

                    if( !column_taken( candidate ) )
                    {
                        switch( candidate )
                        {
                        case 0: noneBtn->SetValue( true ); break;
                        case WXK_CONTROL: ctrlBtn->SetValue( true ); break;
                        case WXK_SHIFT: shiftBtn->SetValue( true ); break;
                        case WXK_ALT: altBtn->SetValue( true ); break;
                        }

                        break;
                    }
                }
            };

    if( modifier == WXK_CONTROL )
    {
        if( !isZoom && m_rbZoomCtrl->GetValue() )
            assign_first_available( m_rbZoomNone, m_rbZoomCtrl, m_rbZoomShift, m_rbZoomAlt );
        if( !isPanH && m_rbPanHCtrl->GetValue() )
            assign_first_available( m_rbPanHNone, m_rbPanHCtrl, m_rbPanHShift, m_rbPanHAlt );
        if( !isPanV && m_rbPanVCtrl->GetValue() )
            assign_first_available( m_rbPanVNone, m_rbPanVCtrl, m_rbPanVShift, m_rbPanVAlt );
    }
    else if( modifier == WXK_SHIFT )
    {
        if( !isZoom && m_rbZoomShift->GetValue() )
            assign_first_available( m_rbZoomNone, m_rbZoomCtrl, m_rbZoomShift, m_rbZoomAlt );
        if( !isPanH && m_rbPanHShift->GetValue() )
            assign_first_available( m_rbPanHNone, m_rbPanHCtrl, m_rbPanHShift, m_rbPanHAlt );
        if( !isPanV && m_rbPanVShift->GetValue() )
            assign_first_available( m_rbPanVNone, m_rbPanVCtrl, m_rbPanVShift, m_rbPanVAlt );
    }
    else if( modifier == WXK_ALT )
    {
        if( !isZoom && m_rbZoomAlt->GetValue() )
            assign_first_available( m_rbZoomNone, m_rbZoomCtrl, m_rbZoomShift, m_rbZoomAlt );
        if( !isPanH && m_rbPanHAlt->GetValue() )
            assign_first_available( m_rbPanHNone, m_rbPanHCtrl, m_rbPanHShift, m_rbPanHAlt );
        if( !isPanV && m_rbPanVAlt->GetValue() )
            assign_first_available( m_rbPanVNone, m_rbPanVCtrl, m_rbPanVShift, m_rbPanVAlt );
    }

    SCROLL_MOD_SET newSet = getScrollModSet();

    m_scrollWarning->Show( !isScrollModSetValid( newSet ) );

    m_currentScrollMod = newSet;
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
                case 0:           aNoneBtn->SetValue( true );  break;
                case WXK_CONTROL: aCtrlBtn->SetValue( true );  break;
                case WXK_SHIFT:   aShiftBtn->SetValue( true ); break;
                case WXK_ALT:     aAltBtn->SetValue( true );   break;
                }
            };

    set_wheel_buttons( m_currentScrollMod.zoom, m_rbZoomNone, m_rbZoomCtrl, m_rbZoomShift,
                       m_rbZoomAlt );

    set_wheel_buttons( m_currentScrollMod.panh, m_rbPanHNone, m_rbPanHCtrl, m_rbPanHShift,
                       m_rbPanHAlt );

    set_wheel_buttons( m_currentScrollMod.panv, m_rbPanVNone, m_rbPanVCtrl, m_rbPanVShift,
                       m_rbPanVAlt );

    m_checkZoomReverse->SetValue( m_currentScrollMod.zoomReverse );
    m_checkPanHReverse->SetValue( m_currentScrollMod.panHReverse );
}


void PANEL_MOUSE_SETTINGS::onMouseDefaults( wxCommandEvent& event )
{
    m_currentScrollMod.zoom              = 0;
    m_currentScrollMod.panh              = WXK_CONTROL;
    m_currentScrollMod.panv              = WXK_SHIFT;
    m_currentScrollMod.zoomReverse       = false;
    m_currentScrollMod.panHReverse       = false;

    updateScrollModButtons();

    m_checkEnablePanH->SetValue( false );
}


void PANEL_MOUSE_SETTINGS::onTrackpadDefaults( wxCommandEvent& event )
{
    m_currentScrollMod.zoom              = WXK_CONTROL;
    m_currentScrollMod.panh              = WXK_SHIFT;
    m_currentScrollMod.panv              = 0;
    m_currentScrollMod.zoomReverse       = false;
    m_currentScrollMod.panHReverse       = false;

    updateScrollModButtons();

    m_checkEnablePanH->SetValue( true );
}
