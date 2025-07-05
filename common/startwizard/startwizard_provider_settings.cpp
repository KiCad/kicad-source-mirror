/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 


#include <dialogs/panel_startwizard_settings_base.h>
#include "startwizard_provider_settings.h"

#include <bitmaps/bitmap_types.h>
#include <bitmaps/bitmaps_list.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/std_bitmap_button.h>
#include <wx/dirdlg.h>


class PANEL_STARTWIZARD_SETTINGS : public PANEL_STARTWIZARD_SETTINGS_BASE
{
public:
    PANEL_STARTWIZARD_SETTINGS( std::shared_ptr<STARTWIZARD_PROVIDER_SETTINGS_MODEL> aModel,
                                wxWindow* aParent ) :
            PANEL_STARTWIZARD_SETTINGS_BASE( aParent ),
            m_model( aModel )
    {
        m_btnCustomPath->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    };

    bool TransferDataFromWindow() override
    {
        if( m_btnUseDefaults->GetValue() )
        {
            m_model->mode = STARTWIZARD_SETTINGS_MODE::USE_DEFAULTS;
        }
        else
        {
            m_model->mode = STARTWIZARD_SETTINGS_MODE::IMPORT;
            // Round-trip through a wxFileName object to remove any trailing separators
            wxFileName path( m_cbPath->GetValue(), wxEmptyString );
            m_model->import_path = path.GetPath();
        }

        return true;
    }

    bool TransferDataToWindow() override
    {
        SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();
        std::vector<wxString> paths;

        if( m_model->mode == STARTWIZARD_SETTINGS_MODE::USE_DEFAULTS )
        {
            m_btnUseDefaults->SetValue( true );

            if( !manager.GetPreviousVersionPaths( &paths ) )
            {
                m_btnPrevVer->SetLabelText( _( "Import settings from a previous version (none found)" ) );
            }
            else
            {
                m_cbPath->Clear();

                for( const auto& path : paths )
                    m_cbPath->Append( path );

                m_cbPath->SetSelection( 0 );
            }

            // SetValue does not fire the "OnRadioButton" event, so have to fabricate this
            wxCommandEvent dummy;
            OnDefaultSelected( dummy );
        }
        else
        {
            m_btnPrevVer->SetValue( true );
            m_cbPath->SetStringSelection( m_model->import_path );

            wxCommandEvent dummy;
            OnPrevVerSelected( dummy );
        }

        return true;
    }

protected:
    void OnPrevVerSelected( wxCommandEvent& event ) override;

    void OnPathChanged( wxCommandEvent& event ) override;

    void OnPathDefocused( wxFocusEvent& event ) override;

    void OnChoosePath( wxCommandEvent& event ) override;

    void OnDefaultSelected( wxCommandEvent& event ) override;

private:
    bool validatePath();

    void showPathError( bool aShow = true );

    std::shared_ptr<STARTWIZARD_PROVIDER_SETTINGS_MODEL> m_model;
};


void PANEL_STARTWIZARD_SETTINGS::OnPrevVerSelected( wxCommandEvent& event )
{
    m_cbPath->Enable();
    m_btnCustomPath->Enable();
    validatePath();
}


void PANEL_STARTWIZARD_SETTINGS::OnPathChanged( wxCommandEvent& event )
{
    validatePath();
}


void PANEL_STARTWIZARD_SETTINGS::OnPathDefocused( wxFocusEvent& event )
{
    validatePath();
}


void PANEL_STARTWIZARD_SETTINGS::OnChoosePath( wxCommandEvent& event )
{
    wxDirDialog dlg( nullptr, _( "Select Settings Path" ), m_cbPath->GetValue(),
            wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_cbPath->SetValue( dlg.GetPath() );
        validatePath();
    }
}


void PANEL_STARTWIZARD_SETTINGS::OnDefaultSelected( wxCommandEvent& event )
{
    m_cbPath->Disable();
    m_btnCustomPath->Disable();
    showPathError( false );
}


bool PANEL_STARTWIZARD_SETTINGS::validatePath()
{
    SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();
    wxString path = m_cbPath->GetValue();
    bool valid = manager.IsSettingsPathValid( path );

    showPathError( !valid );

    return valid;
}


void PANEL_STARTWIZARD_SETTINGS::showPathError( bool aShow )
{
    m_lblPathError->Show( aShow );
    Layout();
    Fit();
}


STARTWIZARD_PROVIDER_SETTINGS::STARTWIZARD_PROVIDER_SETTINGS() :
        STARTWIZARD_PROVIDER( wxT( "Configuration" ) )
{
}


bool STARTWIZARD_PROVIDER_SETTINGS::NeedsUserInput() const
{
    return !Pgm().GetSettingsManager().SettingsDirectoryValid();
}


wxPanel* STARTWIZARD_PROVIDER_SETTINGS::GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard )
{
    m_model = std::make_shared<STARTWIZARD_PROVIDER_SETTINGS_MODEL>();
    return new PANEL_STARTWIZARD_SETTINGS( m_model, aParent );
}


void STARTWIZARD_PROVIDER_SETTINGS::Finish()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( m_model->mode == STARTWIZARD_SETTINGS_MODE::USE_DEFAULTS )
    {
        settings->SaveToFile( mgr.GetPathForSettingsFile( settings ) );
        return;
    }

    // Else, perform migration.  First copy the old files in, then reload the in-memory copies.
    mgr.MigrateFromPreviousVersion( m_model->import_path );
    mgr.Load();
}


void STARTWIZARD_PROVIDER_SETTINGS::ApplyDefaults()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // We will already have created default settings in memory; make sure they are persisted
    // so we don't get first-run wizard restarts
    settings->SaveToFile( mgr.GetPathForSettingsFile( settings ) );
}
