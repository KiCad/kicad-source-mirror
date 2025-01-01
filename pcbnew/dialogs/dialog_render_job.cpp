/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

static std::map<JOB_PCB_RENDER::FORMAT, wxString> outputFormatMap = {
    { JOB_PCB_RENDER::FORMAT::JPEG, _HKI( "JPEG" ) },
    { JOB_PCB_RENDER::FORMAT::PNG, _HKI( "PNG" ) }
};

static std::map<JOB_PCB_RENDER::BG_STYLE, wxString> bgStyleMap = {
    { JOB_PCB_RENDER::BG_STYLE::BG_DEFAULT, _HKI( "Default" ) },
    { JOB_PCB_RENDER::BG_STYLE::BG_OPAQUE, _HKI( "Opaque" ) },
    { JOB_PCB_RENDER::BG_STYLE::BG_TRANSPARENT, _HKI( "Transparent" ) }
};

static std::map<JOB_PCB_RENDER::QUALITY, wxString> qualityMap = {
    { JOB_PCB_RENDER::QUALITY::BASIC, _HKI( "Basic" ) },
    { JOB_PCB_RENDER::QUALITY::HIGH, _HKI( "High" ) },
    { JOB_PCB_RENDER::QUALITY::USER, _HKI( "User" ) }
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
        DIALOG_RENDER_JOB_BASE( aParent ), m_job( aJob )
{
    SetTitle( aJob->GetOptionsDialogTitle() );

    for( const auto& [k, name] : outputFormatMap )
    {
        m_choiceFormat->Append( wxGetTranslation( name ) );
    }

    for( const auto& [k, name] : bgStyleMap )
    {
        m_choiceBgStyle->Append( wxGetTranslation( name ) );
    }

    for( const auto& [k, name] : qualityMap )
    {
        m_choiceQuality->Append( wxGetTranslation( name ) );
    }

    for( const auto& [k, name] : sideMap )
    {
        m_choiceSide->Append( wxGetTranslation( name ) );
    }

    SetupStandardButtons( { { wxID_OK,     _( "Save" ) },
                            { wxID_CANCEL, _( "Close" )  } } );
}


JOB_PCB_RENDER::FORMAT DIALOG_RENDER_JOB::getSelectedFormat()
{
    int  selIndx = m_choiceFormat->GetSelection();
    auto it = outputFormatMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RENDER_JOB::setSelectedFormat( JOB_PCB_RENDER::FORMAT aFormat )
{
    auto it = outputFormatMap.find( aFormat );
    if( it != outputFormatMap.end() )
    {
        int idx = std::distance( outputFormatMap.begin(), it );
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


JOB_PCB_RENDER::QUALITY DIALOG_RENDER_JOB::getSelectedQuality()
{
    int  selIndx = m_choiceQuality->GetSelection();
    auto it = qualityMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RENDER_JOB::setSelectedQuality( JOB_PCB_RENDER::QUALITY aQuality )
{
    auto it = qualityMap.find( aQuality );
    if( it != qualityMap.end() )
    {
        int idx = std::distance( qualityMap.begin(), it );
        m_choiceQuality->SetSelection( idx );
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
    m_job->SetOutputPath( m_textCtrlOutputFile->GetValue() );

    m_job->m_format = getSelectedFormat();
    m_job->m_quality = getSelectedQuality();
    m_job->m_bgStyle = getSelectedBgStyle();
    m_job->m_side = getSelectedSide();
    m_job->m_zoom = m_spinCtrlZoom->GetValue();
    m_job->m_floor = m_cbFloor->GetValue();

    m_job->m_width = m_spinCtrlWidth->GetValue();
    m_job->m_height = m_spinCtrlHeight->GetValue();

    m_radioProjection->GetSelection() == 0 ? m_job->m_perspective = true
                                           : m_job->m_perspective = false;

    return true;
}


bool DIALOG_RENDER_JOB::TransferDataToWindow()
{
    m_textCtrlOutputFile->SetValue( m_job->GetOutputPath() );

    setSelectedFormat( m_job->m_format );
    setSelectedBgStyle( m_job->m_bgStyle );
    setSelectedQuality( m_job->m_quality );
    setSelectedSide( m_job->m_side );
    m_spinCtrlZoom->SetValue( m_job->m_zoom );
    m_radioProjection->SetSelection( m_job->m_perspective ? 0 : 1 );
    m_cbFloor->SetValue( m_job->m_floor );

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

    return true;
}