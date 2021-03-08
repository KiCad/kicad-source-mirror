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

#include <bitmaps.h>
#include <dialogs/dialog_migrate_settings.h>
#include <settings/settings_manager.h>

#include <wx/dirdlg.h>


DIALOG_MIGRATE_SETTINGS::DIALOG_MIGRATE_SETTINGS( SETTINGS_MANAGER* aManager ) :
        DIALOG_MIGRATE_SETTINGS_BASE( nullptr ), m_manager( aManager )
{
    m_standardButtonsCancel->SetLabel( _( "Quit KiCad" ) );

    m_btnCustomPath->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_MIGRATE_SETTINGS::~DIALOG_MIGRATE_SETTINGS()
{
}


bool DIALOG_MIGRATE_SETTINGS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    wxString str;
    str.Printf( _( "Welcome to KiCad %s!" ), SETTINGS_MANAGER::GetSettingsVersion() );
    m_lblWelcome->SetLabelText( str );

    std::vector<wxString> paths;

    // SetValue does not fire the "OnRadioButton" event, so have to fabricate this
    wxCommandEvent dummy;

    if( !m_manager->GetPreviousVersionPaths( &paths ) )
    {
        m_btnPrevVer->SetLabelText( _( "Import settings from a previous version (none found)" ) );
        m_btnUseDefaults->SetValue( true );
        OnDefaultSelected( dummy );
    }
    else
    {
        m_cbPath->Clear();

        for( const auto& path : paths )
            m_cbPath->Append( path );

        m_cbPath->SetSelection( 0 );
        m_btnPrevVer->SetValue( true );
        OnPrevVerSelected( dummy );
    }

    Fit();

    return true;
}


bool DIALOG_MIGRATE_SETTINGS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_btnPrevVer->GetValue() )
        m_manager->SetMigrationSource( m_cbPath->GetValue() );
    else
        m_manager->SetMigrationSource( wxEmptyString );

    m_manager->SetMigrateLibraryTables( m_cbCopyLibraryTables->GetValue() );

    return true;
}


void DIALOG_MIGRATE_SETTINGS::OnPrevVerSelected( wxCommandEvent& event )
{
    m_standardButtons->GetAffirmativeButton()->Enable();
    m_cbPath->Enable();
    m_btnCustomPath->Enable();
    m_cbCopyLibraryTables->Enable();
    validatePath();
}


void DIALOG_MIGRATE_SETTINGS::OnPathChanged( wxCommandEvent& event )
{
    validatePath();
}


void DIALOG_MIGRATE_SETTINGS::OnPathDefocused( wxFocusEvent& event )
{
    validatePath();
}


void DIALOG_MIGRATE_SETTINGS::OnChoosePath( wxCommandEvent& event )
{
    wxDirDialog dlg( nullptr, _( "Select Settings Path" ), m_cbPath->GetValue(),
            wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_cbPath->SetValue( dlg.GetPath() );
        validatePath();
    }
}


void DIALOG_MIGRATE_SETTINGS::OnDefaultSelected( wxCommandEvent& event )
{
    m_standardButtons->GetAffirmativeButton()->Enable();
    m_cbPath->Disable();
    m_btnCustomPath->Disable();
    m_cbCopyLibraryTables->Disable();
    showPathError( false );
}


bool DIALOG_MIGRATE_SETTINGS::validatePath()
{
    wxString path = m_cbPath->GetValue();
    bool valid = m_manager->IsSettingsPathValid( path );

    showPathError( !valid );
    m_standardButtons->GetAffirmativeButton()->Enable( valid && !path.IsEmpty() );

    return valid;
}


void DIALOG_MIGRATE_SETTINGS::showPathError( bool aShow )
{
    m_lblPathError->Show( aShow );
    Layout();
    Fit();
}
