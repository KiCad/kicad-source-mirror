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

#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>
#include <widgets/std_bitmap_button.h>
#include <wx/filedlg.h>

#include <dialog_import_settings.h>


wxString DIALOG_IMPORT_SETTINGS::m_filePath;     // remember for session


DIALOG_IMPORT_SETTINGS::DIALOG_IMPORT_SETTINGS( wxWindow* aParent, PCB_EDIT_FRAME* aFrame ) :
        DIALOG_IMPORT_SETTINGS_BASE( aParent ),
        m_frame( aFrame )
{
    wxSize sizeNeeded;

    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    // Make sure "Select All" button is big enough to hold "Deselect All"
    m_selectAllButton->SetLabel( _( "Deselect All" ) ); // Change the text temporarily
    sizeNeeded = m_selectAllButton->GetBestSize();      // Get control to tell us the width required
    m_selectAllButton->SetLabel( _( "Select All" ) );   // Restore "Select All" as default text
    sizeNeeded.y = m_selectAllButton->GetSize().y;      // Keep the height unchanged
    m_selectAllButton->SetMinSize( sizeNeeded );        // Set control to the required size

    SetupStandardButtons( { { wxID_OK, _( "Import Settings" ) } } );

    // Disable "Import Settings" button until user selects at least one import option
    m_sdbSizer1OK->Enable( false );

    m_buttonsSizer->Layout();

    m_showSelectAllOnBtn = true; // Store state to toggle message/usage of "Select All" button
}


void DIALOG_IMPORT_SETTINGS::OnCheckboxClicked( wxCommandEvent& event )
{
    bool importButtonEnabled = UpdateImportSettingsButton();

    // If clicking this checkbox clears the last of the import selection checkboxes,
    // then make sure the "Select All" button is actually going to select all.

    if( !importButtonEnabled )
    {
        m_showSelectAllOnBtn = true;
        UpdateSelectAllButton();
    }
}


bool DIALOG_IMPORT_SETTINGS::UpdateImportSettingsButton()
{
    // Enable "Import Settings" button if at least one import option is selected
    bool buttonEnableState =
            ( m_LayersOpt->IsChecked() || m_MaskAndPasteOpt->IsChecked() || m_ConstraintsOpt->IsChecked()
              || m_NetclassesOpt->IsChecked() || m_SeveritiesOpt->IsChecked() || m_TextAndGraphicsOpt->IsChecked()
              || m_FormattingOpt->IsChecked() || m_TracksAndViasOpt->IsChecked() || m_TuningPatternsOpt->IsChecked()
              || m_CustomRulesOpt->IsChecked() || m_ComponentClassesOpt->IsChecked() || m_TuningProfilesOpt->IsChecked()
              || m_TeardropsOpt->IsChecked() );

    m_sdbSizer1OK->Enable( buttonEnableState );

    return buttonEnableState;
}


void DIALOG_IMPORT_SETTINGS::UpdateSelectAllButton()
{
    // Update message on button
    if( m_showSelectAllOnBtn )
        m_selectAllButton->SetLabel( _( "Select All" ) );
    else
        m_selectAllButton->SetLabel( _( "Deselect All" ) );
}


bool DIALOG_IMPORT_SETTINGS::TransferDataToWindow()
{
    m_filePathCtrl->SetValue( m_filePath );
    return true;
}


void DIALOG_IMPORT_SETTINGS::OnBrowseClicked( wxCommandEvent& event )
{
    wxFileName fn = m_frame->GetBoard()->GetFileName();

    wxFileDialog dlg( this, _( "Import Settings From" ), fn.GetPath(), fn.GetFullName(),
                      FILEEXT::PcbFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK )
        m_filePathCtrl->SetValue( dlg.GetPath() );
}


bool DIALOG_IMPORT_SETTINGS::TransferDataFromWindow()
{
    if( !wxFileExists( m_filePathCtrl->GetValue() ) )
    {
        DisplayError( this, wxString::Format( _( "File not found." )  ) );
        m_filePathCtrl->SetFocus();
        return false;
    }

    m_filePath = m_filePathCtrl->GetValue();
    return true;
}


void DIALOG_IMPORT_SETTINGS::OnSelectAll( wxCommandEvent& event )
{
    // Select or deselect all options based on internal flag
    m_LayersOpt->SetValue( m_showSelectAllOnBtn );
    m_TextAndGraphicsOpt->SetValue( m_showSelectAllOnBtn );
    m_FormattingOpt->SetValue( m_showSelectAllOnBtn );
    m_ConstraintsOpt->SetValue( m_showSelectAllOnBtn );
    m_NetclassesOpt->SetValue( m_showSelectAllOnBtn );
    m_TracksAndViasOpt->SetValue( m_showSelectAllOnBtn );
    m_MaskAndPasteOpt->SetValue( m_showSelectAllOnBtn );
    m_SeveritiesOpt->SetValue( m_showSelectAllOnBtn );
    m_TeardropsOpt->SetValue( m_showSelectAllOnBtn );
    m_TuningPatternsOpt->SetValue( m_showSelectAllOnBtn );
    m_CustomRulesOpt->SetValue( m_showSelectAllOnBtn );
    m_TuningProfilesOpt->SetValue( m_showSelectAllOnBtn );
    m_ComponentClassesOpt->SetValue( m_showSelectAllOnBtn );

    // Ensure "Import Settings" button state is enabled as appropriate
    UpdateImportSettingsButton();

    // Toggle whether button selects or deselects all.
    m_showSelectAllOnBtn = !m_showSelectAllOnBtn;
    UpdateSelectAllButton();
}
