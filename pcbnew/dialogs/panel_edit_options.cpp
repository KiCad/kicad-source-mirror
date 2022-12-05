/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <string_utils.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <panel_edit_options.h>


PANEL_EDIT_OPTIONS::PANEL_EDIT_OPTIONS( wxWindow* aParent, UNITS_PROVIDER* aUnitsProvider,
                                        wxWindow* aEventSource, bool isFootprintEditor ) :
        PANEL_EDIT_OPTIONS_BASE( aParent ),
        m_isFootprintEditor( isFootprintEditor ),
        m_rotationAngle( aUnitsProvider, aEventSource, m_rotationAngleLabel, m_rotationAngleCtrl,
                         m_rotationAngleUnits )
{
    m_magneticPads->Show( m_isFootprintEditor );
    m_magneticGraphics->Show( m_isFootprintEditor );
    m_sizerBoardEdit->Show( !m_isFootprintEditor );

    m_rotationAngle.SetUnits( EDA_UNITS::DEGREES );

#ifdef __WXOSX_MAC__
    m_mouseCmdsOSX->Show( true );
    m_mouseCmdsWinLin->Show( false );
    // Disable highlight net option for footprint editor
    m_rbCtrlClickActionMac->Enable( 1, !m_isFootprintEditor );
#else
    m_mouseCmdsWinLin->Show( true );
    m_mouseCmdsOSX->Show( false );
    // Disable highlight net option for footprint editor
    m_rbCtrlClickAction->Enable( 1, !m_isFootprintEditor );
#endif

    m_optionsBook->SetSelection( isFootprintEditor ? 0 : 1 );
}


void PANEL_EDIT_OPTIONS::loadPCBSettings( PCBNEW_SETTINGS* aCfg )
{
    m_rotationAngle.SetAngleValue( aCfg->m_RotationAngle );
    m_magneticPadChoice->SetSelection( static_cast<int>( aCfg->m_MagneticItems.pads ) );
    m_magneticTrackChoice->SetSelection( static_cast<int>( aCfg->m_MagneticItems.tracks ) );
    m_magneticGraphicsChoice->SetSelection( !aCfg->m_MagneticItems.graphics );
    m_flipLeftRight->SetValue( aCfg->m_FlipLeftRight );
    m_cbConstrainHV45Mode->SetValue( aCfg->m_Use45DegreeLimit );
    m_cbCourtyardCollisions->SetValue( aCfg->m_ShowCourtyardCollisions );
    m_arcEditMode->SetSelection(
            aCfg->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ? 0 : 1 );

    /* Set display options */
    m_OptDisplayCurvedRatsnestLines->SetValue( aCfg->m_Display.m_DisplayRatsnestLinesCurved );
    m_showSelectedRatsnest->SetValue( aCfg->m_Display.m_ShowModuleRatsnest );

    switch( aCfg->m_TrackDragAction )
    {
    case TRACK_DRAG_ACTION::MOVE:            m_rbTrackDragMove->SetValue( true ); break;
    case TRACK_DRAG_ACTION::DRAG:            m_rbTrackDrag45->SetValue( true );   break;
    case TRACK_DRAG_ACTION::DRAG_FREE_ANGLE: m_rbTrackDragFree->SetValue( true ); break;
    }

#ifdef __WXOSX_MAC__
    m_rbCtrlClickActionMac->SetSelection( aCfg->m_CtrlClickHighlight );
#else
    m_rbCtrlClickAction->SetSelection( aCfg->m_CtrlClickHighlight );
#endif

    m_showPageLimits->SetValue( aCfg->m_ShowPageLimits );
    m_autoRefillZones->SetValue( aCfg->m_AutoRefillZones );
    m_allowFreePads->SetValue( aCfg->m_AllowFreePads );

    m_escClearsNetHighlight->SetValue( aCfg->m_ESCClearsNetHighlight );
}


void PANEL_EDIT_OPTIONS::loadFPSettings( FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    m_rotationAngle.SetAngleValue( aCfg->m_RotationAngle );
    m_magneticPads->SetValue( aCfg->m_MagneticItems.pads == MAGNETIC_OPTIONS::CAPTURE_ALWAYS );
    m_magneticGraphics->SetValue( aCfg->m_MagneticItems.graphics );
    m_cbConstrainHV45Mode->SetValue( aCfg->m_Use45Limit );
    m_arcEditMode->SetSelection(
            aCfg->m_ArcEditMode == ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ? 0 : 1 );
}


bool PANEL_EDIT_OPTIONS::TransferDataToWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( m_isFootprintEditor )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

        loadFPSettings( cfg );
    }
    else
    {
        PCBNEW_SETTINGS* cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

        loadPCBSettings( cfg );
    }

   return true;
}


bool PANEL_EDIT_OPTIONS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( m_isFootprintEditor )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

        cfg->m_RotationAngle = m_rotationAngle.GetAngleValue();

        cfg->m_MagneticItems.pads = m_magneticPads->GetValue() ? MAGNETIC_OPTIONS::CAPTURE_ALWAYS
                                                               : MAGNETIC_OPTIONS::NO_EFFECT;
        cfg->m_MagneticItems.graphics = m_magneticGraphics->GetValue();

        cfg->m_Use45Limit = m_cbConstrainHV45Mode->GetValue();

        cfg->m_ArcEditMode = m_arcEditMode->GetSelection() == 0
                                     ? ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS
                                     : ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    }
    else
    {
        PCBNEW_SETTINGS* cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

        cfg->m_Display.m_DisplayRatsnestLinesCurved = m_OptDisplayCurvedRatsnestLines->GetValue();
        cfg->m_Display.m_ShowModuleRatsnest = m_showSelectedRatsnest->GetValue();

        cfg->m_RotationAngle = m_rotationAngle.GetAngleValue();

        cfg->m_MagneticItems.pads = static_cast<MAGNETIC_OPTIONS>( m_magneticPadChoice->GetSelection() );
        cfg->m_MagneticItems.tracks = static_cast<MAGNETIC_OPTIONS>( m_magneticTrackChoice->GetSelection() );
        cfg->m_MagneticItems.graphics = !m_magneticGraphicsChoice->GetSelection();

        cfg->m_FlipLeftRight = m_flipLeftRight->GetValue();
        cfg->m_ESCClearsNetHighlight = m_escClearsNetHighlight->GetValue();
        cfg->m_AutoRefillZones = m_autoRefillZones->GetValue();
        cfg->m_AllowFreePads = m_allowFreePads->GetValue();
        cfg->m_ShowPageLimits = m_showPageLimits->GetValue();

        if( m_rbTrackDragMove->GetValue() )
            cfg->m_TrackDragAction = TRACK_DRAG_ACTION::MOVE;
        else if( m_rbTrackDrag45->GetValue() )
            cfg->m_TrackDragAction = TRACK_DRAG_ACTION::DRAG;
        else if( m_rbTrackDragFree->GetValue() )
            cfg->m_TrackDragAction = TRACK_DRAG_ACTION::DRAG_FREE_ANGLE;

#ifdef __WXOSX_MAC__
        cfg->m_CtrlClickHighlight = m_rbCtrlClickActionMac->GetSelection();
#else
        cfg->m_CtrlClickHighlight = m_rbCtrlClickAction->GetSelection();
#endif

        cfg->m_Use45DegreeLimit = m_cbConstrainHV45Mode->GetValue();
        cfg->m_ShowCourtyardCollisions = m_cbCourtyardCollisions->GetValue();

        cfg->m_ArcEditMode = m_arcEditMode->GetSelection() == 0
                                     ? ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS
                                     : ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    }

    return true;
}


void PANEL_EDIT_OPTIONS::ResetPanel()
{
    if( m_isFootprintEditor )
    {
        FOOTPRINT_EDITOR_SETTINGS cfg;
        cfg.Load();                     // Loading without a file will init to defaults

        loadFPSettings( &cfg );
    }
    else
    {
        PCBNEW_SETTINGS cfg;
        cfg.Load();           // Loading without a file will init to defaults

        loadPCBSettings( &cfg );
    }
}


