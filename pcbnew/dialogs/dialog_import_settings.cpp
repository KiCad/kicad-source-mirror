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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>

#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>
#include <widgets/std_bitmap_button.h>
#include <wx/filedlg.h>
#include <kiplatform/ui.h>

#include <dialog_import_settings.h>


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

    m_filePathCtrl->Bind( wxEVT_TEXT, &DIALOG_IMPORT_SETTINGS::onFilePathChanged, this );

    m_options = { m_LayersOpt,         m_MaskAndPasteOpt,   m_ZoneHatchingOffsetsOpt, m_TextAndGraphicsOpt,
                  m_FormattingOpt,     m_ConstraintsOpt,    m_TracksAndViasOpt,       m_ZonesOpt,
                  m_TeardropsOpt,      m_TuningPatternsOpt, m_NetclassesOpt,          m_ComponentClassesOpt,
                  m_TuningProfilesOpt, m_CustomRulesOpt,    m_SeveritiesOpt };

    m_buttonsSizer->Layout();

    updateImportSettingsButton();
}


bool DIALOG_IMPORT_SETTINGS::anyOptionSelected() const
{
    return std::any_of( m_options.begin(), m_options.end(),
                        []( const wxCheckBox* aOpt )
                        {
                            return aOpt->IsChecked();
                        } );
}


void DIALOG_IMPORT_SETTINGS::OnCheckboxClicked( wxCommandEvent& event )
{
    updateImportSettingsButton();
    updateSelectAllButton();
}


void DIALOG_IMPORT_SETTINGS::updateImportSettingsButton()
{
    // Importing needs both a source board and something to take from it
    m_sdbSizer1OK->Enable( anyOptionSelected() && !m_filePathCtrl->GetValue().IsEmpty() );
}


void DIALOG_IMPORT_SETTINGS::onFilePathChanged( wxCommandEvent& aEvent )
{
    updateImportSettingsButton();

    aEvent.Skip();
}


void DIALOG_IMPORT_SETTINGS::updateSelectAllButton()
{
    if( anyOptionSelected() )
        m_selectAllButton->SetLabel( _( "Deselect All" ) );
    else
        m_selectAllButton->SetLabel( _( "Select All" ) );
}


bool DIALOG_IMPORT_SETTINGS::TransferDataToWindow()
{
    // DIALOG_SHIM has restored the checkboxes by now and wxCheckBox::SetValue() emits no
    // event, so both buttons are stale until synced here
    updateImportSettingsButton();
    updateSelectAllButton();

    return true;
}


void DIALOG_IMPORT_SETTINGS::OnBrowseClicked( wxCommandEvent& event )
{
    // Honor a path the user has already entered; fall back to the board's location
    // when the field is empty or its directory no longer exists.
    wxString   currentPath = m_filePathCtrl->GetValue();
    wxFileName fn( currentPath.IsEmpty() ? m_frame->GetBoard()->GetFileName() : currentPath );

    if( !fn.DirExists() )
        fn = m_frame->GetBoard()->GetFileName();

    wxFileDialog dlg( this, _( "Import Settings From" ), fn.GetPath(), fn.GetFullName(),
                      FILEEXT::PcbFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

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

    return true;
}


void DIALOG_IMPORT_SETTINGS::OnSelectAll( wxCommandEvent& event )
{
    bool select = !anyOptionSelected();

    for( wxCheckBox* opt : m_options )
        opt->SetValue( select );

    updateImportSettingsButton();
    updateSelectAllButton();
}
