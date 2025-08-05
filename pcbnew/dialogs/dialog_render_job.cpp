/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <dialogs/dialog_render_job.h>
#include <jobs/job_pcb_render.h>
#include <i18n_utility.h>
#include <wx/display.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <3d_viewer/eda_3d_viewer_settings.h>


static std::map<JOB_PCB_RENDER::BG_STYLE, wxString> bgStyleMap = {
    { JOB_PCB_RENDER::BG_STYLE::DEFAULT, _HKI( "Default" ) },
    { JOB_PCB_RENDER::BG_STYLE::OPAQUE, _HKI( "Opaque" ) },
    { JOB_PCB_RENDER::BG_STYLE::TRANSPARENT, _HKI( "Transparent" ) }
};

static std::map<JOB_PCB_RENDER::SIDE, wxString> sideMap = {
    { JOB_PCB_RENDER::SIDE::BACK, _HKI( "Back" ) },
    { JOB_PCB_RENDER::SIDE::BOTTOM, _HKI( "Bottom" ) },
    { JOB_PCB_RENDER::SIDE::FRONT, _HKI( "Front" ) },
    { JOB_PCB_RENDER::SIDE::LEFT, _HKI( "Left" ) },
    { JOB_PCB_RENDER::SIDE::RIGHT, _HKI( "Right" ) },
    { JOB_PCB_RENDER::SIDE::TOP, _HKI( "Top" ) }
};

DIALOG_RENDER_JOB::DIALOG_RENDER_JOB( wxWindow* aParent, JOB_PCB_RENDER* aJob  ) :
        DIALOG_RENDER_JOB_BASE( aParent ),
        m_job( aJob )
{
    SetTitle( aJob->GetSettingsDialogTitle() );

    for( const auto& [k, name] : JOB_PCB_RENDER::GetFormatNameMap() )
        m_choiceFormat->Append( wxGetTranslation( name ) );

    for( const auto& [k, name] : bgStyleMap )
        m_choiceBgStyle->Append( wxGetTranslation( name ) );

    for( const auto& [k, name] : sideMap )
        m_choiceSide->Append( wxGetTranslation( name ) );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        for( const LAYER_PRESET_3D& preset : cfg->m_LayerPresets )
            m_presetCtrl->Append( preset.name );
    }

    SetupStandardButtons();
}


JOB_PCB_RENDER::FORMAT DIALOG_RENDER_JOB::getSelectedFormat()
{
    int  selIndx = m_choiceFormat->GetSelection();
    auto it = JOB_PCB_RENDER::GetFormatNameMap().begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RENDER_JOB::setSelectedFormat( JOB_PCB_RENDER::FORMAT aFormat )
{
    auto it = JOB_PCB_RENDER::GetFormatNameMap().find( aFormat );

    if( it != JOB_PCB_RENDER::GetFormatNameMap().end() )
    {
        int idx = std::distance( JOB_PCB_RENDER::GetFormatNameMap().begin(), it );
        m_choiceFormat->SetSelection( idx );
    }
}


JOB_PCB_RENDER::SIDE DIALOG_RENDER_JOB::getSelectedSide()
{
    int  selIndx = m_choiceSide->GetSelection();
    auto it = sideMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RENDER_JOB::setSelectedSide( JOB_PCB_RENDER::SIDE aSide )
{
    auto it = sideMap.find( aSide );

    if( it != sideMap.end() )
    {
        int idx = std::distance( sideMap.begin(), it );
        m_choiceSide->SetSelection( idx );
    }
}


JOB_PCB_RENDER::BG_STYLE DIALOG_RENDER_JOB::getSelectedBgStyle()
{
    int  selIndx = m_choiceBgStyle->GetSelection();
    auto it = bgStyleMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RENDER_JOB::setSelectedBgStyle( JOB_PCB_RENDER::BG_STYLE aBgStyle )
{
    auto it = bgStyleMap.find( aBgStyle );

    if( it != bgStyleMap.end() )
    {
        int idx = std::distance( bgStyleMap.begin(), it );
        m_choiceBgStyle->SetSelection( idx );
    }
}


bool DIALOG_RENDER_JOB::TransferDataFromWindow()
{
    m_job->SetConfiguredOutputPath( m_textCtrlOutputFile->GetValue() );

    m_job->m_format = getSelectedFormat();
    m_job->m_quality = JOB_PCB_RENDER::QUALITY::JOB_SETTINGS;

    if( m_presetCtrl->GetSelection() == 0 )
        m_job->m_appearancePreset = "";
    else if( m_presetCtrl->GetSelection() == 1 )
        m_job->m_appearancePreset = wxString( FOLLOW_PCB ).ToStdString();
    else if( m_presetCtrl->GetSelection() == 2 )
        m_job->m_appearancePreset = wxString( FOLLOW_PLOT_SETTINGS ).ToStdString();
    else
        m_job->m_appearancePreset = m_presetCtrl->GetStringSelection();

    m_job->m_useBoardStackupColors = m_cbUseBoardStackupColors->GetValue();

    m_job->m_bgStyle = getSelectedBgStyle();
    m_job->m_side = getSelectedSide();
    m_job->m_zoom = m_spinCtrlZoom->GetValue();
    m_job->m_proceduralTextures = m_cbRaytracing_proceduralTextures->GetValue();
    m_job->m_floor = m_cbRaytracing_addFloor->GetValue();
    m_job->m_antiAlias = m_cbRaytracing_antiAliasing->GetValue();
    m_job->m_postProcess = m_cbRaytracing_postProcessing->GetValue();

    m_job->m_width = m_spinCtrlWidth->GetValue();
    m_job->m_height = m_spinCtrlHeight->GetValue();

    m_job->m_pivot.x = m_spinCtrlPivotX->GetValue();
    m_job->m_pivot.y = m_spinCtrlPivotY->GetValue();
    m_job->m_pivot.z = m_spinCtrlPivotZ->GetValue();

    m_job->m_pan.x = m_spinCtrlPanX->GetValue();
    m_job->m_pan.y = m_spinCtrlPanY->GetValue();
    m_job->m_pan.z = m_spinCtrlPanZ->GetValue();

    m_job->m_rotation.x = m_spinCtrlRotX->GetValue();
    m_job->m_rotation.y = m_spinCtrlRotY->GetValue();
    m_job->m_rotation.z = m_spinCtrlRotZ->GetValue();

    m_job->m_lightTopIntensity.SetAll( m_spinCtrlLightsTop->GetValue() );
    m_job->m_lightBottomIntensity.SetAll( m_spinCtrlLightsBottom->GetValue() );
    m_job->m_lightSideIntensity.SetAll( m_spinCtrlLightsSides->GetValue() );
    m_job->m_lightCameraIntensity.SetAll( m_spinCtrlLightsCamera->GetValue() );

    m_job->m_lightSideElevation = m_spinCtrlLightsSideElevation->GetValue();

    m_radioProjection->GetSelection() == 0 ? m_job->m_perspective = true
                                           : m_job->m_perspective = false;

    return true;
}


bool DIALOG_RENDER_JOB::TransferDataToWindow()
{
    m_textCtrlOutputFile->SetValue( m_job->GetConfiguredOutputPath() );

    setSelectedFormat( m_job->m_format );

    if( m_job->m_appearancePreset == wxString( FOLLOW_PCB ).ToStdString() )
        m_presetCtrl->SetSelection( 1 );
    else if( m_job->m_appearancePreset == wxString( FOLLOW_PLOT_SETTINGS ).ToStdString() )
        m_presetCtrl->SetSelection( 2 );
    else if( !m_presetCtrl->SetStringSelection( m_job->m_appearancePreset ) )
        m_presetCtrl->SetSelection( 0 );

    m_cbUseBoardStackupColors->SetValue( m_job->m_useBoardStackupColors );

    setSelectedBgStyle( m_job->m_bgStyle );
    setSelectedSide( m_job->m_side );
    m_spinCtrlZoom->SetValue( m_job->m_zoom );
    m_radioProjection->SetSelection( m_job->m_perspective ? 0 : 1 );
    m_cbRaytracing_proceduralTextures->SetValue( m_job->m_proceduralTextures );
    m_cbRaytracing_addFloor->SetValue( m_job->m_floor );
    m_cbRaytracing_antiAliasing->SetValue( m_job->m_antiAlias );
    m_cbRaytracing_postProcessing->SetValue( m_job->m_postProcess );

    int width = m_job->m_width;
    int height = m_job->m_height;

    // if the values are the job constructor default, use the screen size
    // as a reasonable default
    if (width == 0 || height == 0)
    {
        int disp = wxDisplay::GetFromWindow( this );
        wxRect rect = wxDisplay( disp ).GetGeometry();

        if( width == 0 )
            width = rect.GetWidth();

        if( height == 0 )
            height = rect.GetHeight();
    }

    m_spinCtrlWidth->SetValue( width );
    m_spinCtrlHeight->SetValue( height );

    m_spinCtrlPivotX->SetValue( m_job->m_pivot.x );
    m_spinCtrlPivotY->SetValue( m_job->m_pivot.y );
    m_spinCtrlPivotZ->SetValue( m_job->m_pivot.z );

    m_spinCtrlPanX->SetValue( m_job->m_pan.x );
    m_spinCtrlPanY->SetValue( m_job->m_pan.y );
    m_spinCtrlPanZ->SetValue( m_job->m_pan.z );

    m_spinCtrlRotX->SetValue( m_job->m_rotation.x );
    m_spinCtrlRotY->SetValue( m_job->m_rotation.y );
    m_spinCtrlRotZ->SetValue( m_job->m_rotation.z );

    m_spinCtrlLightsTop->SetValue( m_job->m_lightTopIntensity.x );
    m_spinCtrlLightsBottom->SetValue( m_job->m_lightBottomIntensity.x );
    m_spinCtrlLightsSides->SetValue( m_job->m_lightSideIntensity.x );
    m_spinCtrlLightsCamera->SetValue( m_job->m_lightCameraIntensity.x );

    m_spinCtrlLightsSideElevation->SetValue( m_job->m_lightSideElevation );

    return true;
}