/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dialogs/panel_setup_tuning_profiles.h>
#include <dialogs/panel_setup_tuning_profile_info.h>
#include <widgets/std_bitmap_button.h>
#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <grid_tricks.h>
#include <layer_ids.h>
#include <magic_enum.hpp>
#include <pgm_base.h>
#include <widgets/paged_dialog.h>

PANEL_SETUP_TUNING_PROFILES::PANEL_SETUP_TUNING_PROFILES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                                          BOARD* aBoard,
                                                          std::shared_ptr<TUNING_PROFILES> aTimeDomainParameters ) :
        PANEL_SETUP_TUNING_PROFILES_BASE( aParentWindow ),
        m_tuningProfileParameters( std::move( aTimeDomainParameters ) ),
        m_dlg( dynamic_cast<DIALOG_SHIM*>( aParentWindow ) ),
        m_frame( aFrame ),
        m_board( aFrame->GetBoard() ),
        m_unitsProvider( std::make_unique<UNITS_PROVIDER>( pcbIUScale, m_frame->GetUserUnits() ) )
{
    Freeze();

    m_addTuningProfileButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeTuningProfileButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    Thaw();
}


PANEL_SETUP_TUNING_PROFILES::~PANEL_SETUP_TUNING_PROFILES()
{
}


bool PANEL_SETUP_TUNING_PROFILES::TransferDataToWindow()
{
    m_tuningProfiles->DeleteAllPages();
    SyncCopperLayers( m_board->GetCopperLayerCount() );

    const std::vector<TUNING_PROFILE>& tuningProfiles = m_tuningProfileParameters->GetTuningProfiles();

    for( const TUNING_PROFILE& profile : tuningProfiles )
    {
        PANEL_SETUP_TUNING_PROFILE_INFO* panel = new PANEL_SETUP_TUNING_PROFILE_INFO( m_tuningProfiles, this );
        m_tuningProfiles->AddPage( panel, profile.m_ProfileName, true );
        panel->LoadProfile( profile );
    }

    return true;
}


bool PANEL_SETUP_TUNING_PROFILES::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    m_tuningProfileParameters->ClearTuningProfiles();

    for( size_t i = 0; i < m_tuningProfiles->GetPageCount(); ++i )
    {
        auto* panel = static_cast<PANEL_SETUP_TUNING_PROFILE_INFO*>( m_tuningProfiles->GetPage( i ) );
        TUNING_PROFILE profile = panel->GetProfile();
        m_tuningProfileParameters->AddTuningProfile( std::move( profile ) );
    }

    return true;
}


void PANEL_SETUP_TUNING_PROFILES::SyncCopperLayers( const int aNumCopperLayers )
{
    m_prevLayerNamesToIDs = m_layerNamesToIDs;
    m_copperLayerIdsToIndex.clear();
    m_copperIndexToLayerId.clear();
    m_layerNames.clear();
    m_layerNamesToIDs.clear();

    int layerIdx = 0;

    for( const auto& layer : LSET::AllCuMask( aNumCopperLayers ).CuStack() )
    {
        wxString layerName = m_board->GetLayerName( layer );
        m_layerNames.emplace_back( layerName );
        m_layerNamesToIDs[layerName] = layer;
        m_copperLayerIdsToIndex[layer] = layerIdx;
        m_copperIndexToLayerId[layerIdx] = layer;
        ++layerIdx;
    }

    if( m_prevLayerNamesToIDs.empty() )
        m_prevLayerNamesToIDs = m_layerNamesToIDs;

    for( size_t i = 0; i < m_tuningProfiles->GetPageCount(); ++i )
    {
        auto* panel = static_cast<PANEL_SETUP_TUNING_PROFILE_INFO*>( m_tuningProfiles->GetPage( i ) );
        panel->UpdateLayerNames();
    }
}


void PANEL_SETUP_TUNING_PROFILES::OnAddTuningProfileClick( wxCommandEvent& event )
{
    PANEL_SETUP_TUNING_PROFILE_INFO* panel = new PANEL_SETUP_TUNING_PROFILE_INFO( m_tuningProfiles, this );
    m_tuningProfiles->AddPage( panel, "Profile ", true );
}


void PANEL_SETUP_TUNING_PROFILES::OnRemoveTuningProfileClick( wxCommandEvent& event )
{
    if( const wxWindow* page = m_tuningProfiles->GetCurrentPage() )
    {
        const size_t pageIdx = m_tuningProfiles->FindPage( page );
        m_tuningProfiles->RemovePage( pageIdx );
    }
}


bool PANEL_SETUP_TUNING_PROFILES::Validate()
{
    for( size_t i = 0; i < m_tuningProfiles->GetPageCount(); ++i )
    {
        PANEL_SETUP_TUNING_PROFILE_INFO* panel =
                static_cast<PANEL_SETUP_TUNING_PROFILE_INFO*>( m_tuningProfiles->GetPage( i ) );

        if( !panel->ValidateProfile( i ) )
            return false;
    }

    return true;
}


void PANEL_SETUP_TUNING_PROFILES::UpdateProfileName( PANEL_SETUP_TUNING_PROFILE_INFO* panel, wxString newName ) const
{
    const int pageId = m_tuningProfiles->FindPage( panel );

    if( pageId == wxNOT_FOUND )
        return;

    m_tuningProfiles->SetPageText( pageId, newName );
}


void PANEL_SETUP_TUNING_PROFILES::ImportSettingsFrom( const std::shared_ptr<TUNING_PROFILES>& aOtherParameters )
{
    std::shared_ptr<TUNING_PROFILES> savedSettings = m_tuningProfileParameters;

    m_tuningProfileParameters = aOtherParameters;
    TransferDataToWindow();

    m_tuningProfileParameters = std::move( savedSettings );
}


std::vector<wxString> PANEL_SETUP_TUNING_PROFILES::GetDelayProfileNames() const
{
    std::vector<wxString> names;

    size_t profileCount = m_tuningProfiles->GetPageCount();

    for( size_t i = 0; i < profileCount; ++i )
    {
        const auto* profilePage = static_cast<PANEL_SETUP_TUNING_PROFILE_INFO*>( m_tuningProfiles->GetPage( i ) );

        if( const wxString& name = profilePage->GetProfileName(); name != wxEmptyString )
            names.push_back( name );
    }

    return names;
}
