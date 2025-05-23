/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

    m_stHint1->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    m_stHint2->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );

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


static int arcEditModeToComboIndex( ARC_EDIT_MODE aMode )
{
    switch( aMode )
    {
        case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
            return 0;
        case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
            return 1;
        case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
            return 2;
        // No default
    }
    wxFAIL_MSG( "Invalid ARC_EDIT_MODE" );
    return 0;
};


static ARC_EDIT_MODE arcEditModeToEnum( int aIndex )
{
    switch( aIndex )
    {
        case 0:
            return ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
        case 1:
            return ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
        case 2:
            return ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE;
        default:
            wxFAIL_MSG( wxString::Format( "Invalid index for ARC_EDIT_MODE: %d", aIndex ) );
            break;
    }

    return ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
};


void PANEL_EDIT_OPTIONS::loadPCBSettings( PCBNEW_SETTINGS* aCfg )
{
    m_cbConstrainHV45Mode->SetValue( aCfg->m_Use45DegreeLimit );
    m_rotationAngle.SetAngleValue( aCfg->m_RotationAngle );
    m_arcEditMode->SetSelection( arcEditModeToComboIndex( aCfg->m_ArcEditMode ) );
    m_trackMouseDragCtrl->SetSelection( (int) aCfg->m_TrackDragAction );

    if( aCfg->m_FlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        m_rbFlipLeftRight->SetValue( true );
    else
        m_rbFlipTopBottom->SetValue( true );

    m_allowFreePads->SetValue( aCfg->m_AllowFreePads );
    m_overrideLocks->SetValue( aCfg->m_LockingOptions.m_sessionSkipPrompts );
    m_autoRefillZones->SetValue( aCfg->m_AutoRefillZones );

    m_magneticPadChoice->SetSelection( static_cast<int>( aCfg->m_MagneticItems.pads ) );
    m_magneticTrackChoice->SetSelection( static_cast<int>( aCfg->m_MagneticItems.tracks ) );
    m_magneticGraphicsChoice->SetSelection( !aCfg->m_MagneticItems.graphics );

    /* Set display options */
    m_OptDisplayCurvedRatsnestLines->SetValue( aCfg->m_Display.m_DisplayRatsnestLinesCurved );
    m_showSelectedRatsnest->SetValue( aCfg->m_Display.m_ShowModuleRatsnest );
    m_ratsnestThickness->SetValue( aCfg->m_Display.m_RatsnestThickness );

#ifdef __WXOSX_MAC__
    m_rbCtrlClickActionMac->SetSelection( aCfg->m_CtrlClickHighlight );
#else
    m_rbCtrlClickAction->SetSelection( aCfg->m_CtrlClickHighlight );
#endif

    m_escClearsNetHighlight->SetValue( aCfg->m_ESCClearsNetHighlight );
    m_showPageLimits->SetValue( aCfg->m_ShowPageLimits );
    m_cbCourtyardCollisions->SetValue( aCfg->m_ShowCourtyardCollisions );
}


void PANEL_EDIT_OPTIONS::loadFPSettings( FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    m_rotationAngle.SetAngleValue( aCfg->m_RotationAngle );
    m_magneticPads->SetValue( aCfg->m_MagneticItems.pads == MAGNETIC_OPTIONS::CAPTURE_ALWAYS );
    m_magneticGraphics->SetValue( aCfg->m_MagneticItems.graphics );
    m_cbConstrainHV45Mode->SetValue( aCfg->m_Use45Limit );
    m_arcEditMode->SetSelection( arcEditModeToComboIndex( aCfg->m_ArcEditMode ) );
}


bool PANEL_EDIT_OPTIONS::TransferDataToWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( m_isFootprintEditor )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

        loadFPSettings( cfg );
    }
    else
    {
        PCBNEW_SETTINGS* cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        loadPCBSettings( cfg );
    }

   return true;
}


bool PANEL_EDIT_OPTIONS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    if( m_isFootprintEditor )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

        cfg->m_RotationAngle = m_rotationAngle.GetAngleValue();

        cfg->m_MagneticItems.pads = m_magneticPads->GetValue() ? MAGNETIC_OPTIONS::CAPTURE_ALWAYS
                                                               : MAGNETIC_OPTIONS::NO_EFFECT;
        cfg->m_MagneticItems.graphics = m_magneticGraphics->GetValue();

        cfg->m_Use45Limit = m_cbConstrainHV45Mode->GetValue();
        cfg->m_ArcEditMode = arcEditModeToEnum( m_arcEditMode->GetSelection() );
    }
    else
    {
        PCBNEW_SETTINGS* cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        cfg->m_Display.m_DisplayRatsnestLinesCurved = m_OptDisplayCurvedRatsnestLines->GetValue();
        cfg->m_Display.m_ShowModuleRatsnest = m_showSelectedRatsnest->GetValue();
        cfg->m_Display.m_RatsnestThickness = m_ratsnestThickness->GetValue();

        cfg->m_Use45DegreeLimit = m_cbConstrainHV45Mode->GetValue();
        cfg->m_RotationAngle = m_rotationAngle.GetAngleValue();
        cfg->m_ArcEditMode = arcEditModeToEnum( m_arcEditMode->GetSelection() );
        cfg->m_TrackDragAction = (TRACK_DRAG_ACTION) m_trackMouseDragCtrl->GetSelection();

        cfg->m_FlipDirection = m_rbFlipLeftRight->GetValue() ? FLIP_DIRECTION::LEFT_RIGHT
                                                             : FLIP_DIRECTION::TOP_BOTTOM;

        cfg->m_AllowFreePads = m_allowFreePads->GetValue();
        cfg->m_LockingOptions.m_sessionSkipPrompts = m_overrideLocks->GetValue();
        cfg->m_AutoRefillZones = m_autoRefillZones->GetValue();

        cfg->m_MagneticItems.pads = static_cast<MAGNETIC_OPTIONS>( m_magneticPadChoice->GetSelection() );
        cfg->m_MagneticItems.tracks = static_cast<MAGNETIC_OPTIONS>( m_magneticTrackChoice->GetSelection() );
        cfg->m_MagneticItems.graphics = !m_magneticGraphicsChoice->GetSelection();

        cfg->m_ESCClearsNetHighlight = m_escClearsNetHighlight->GetValue();
        cfg->m_ShowPageLimits = m_showPageLimits->GetValue();
        cfg->m_ShowCourtyardCollisions = m_cbCourtyardCollisions->GetValue();


#ifdef __WXOSX_MAC__
        cfg->m_CtrlClickHighlight = m_rbCtrlClickActionMac->GetSelection();
#else
        cfg->m_CtrlClickHighlight = m_rbCtrlClickAction->GetSelection();
#endif
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
