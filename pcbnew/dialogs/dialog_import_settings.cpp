/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_board.h>
#include <wildcards_and_files_ext.h>
#include <confirm.h>

#include <dialog_import_settings.h>


wxString DIALOG_IMPORT_SETTINGS::m_filePath;     // remember for session


DIALOG_IMPORT_SETTINGS::DIALOG_IMPORT_SETTINGS( wxWindow* aParent, PCB_EDIT_FRAME* aFrame ) :
        DIALOG_IMPORT_SETTINGS_BASE( aParent ),
        m_frame( aFrame )
{
    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    m_sdbSizer1OK->SetLabel( _( "Import Settings" ) );
    m_buttonsSizer->Layout();

    m_sdbSizer1OK->SetDefault();
}


bool DIALOG_IMPORT_SETTINGS::TransferDataToWindow()
{
    m_filePathCtrl->SetValue( m_filePath );
    return true;
}


void DIALOG_IMPORT_SETTINGS::OnBrowseClicked( wxCommandEvent& event )
{
    wxFileName fn = m_frame->GetBoard()->GetFileName();
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Import Settings From" ), fn.GetPath(), fn.GetFullName(),
                      ProjectFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

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
    m_LayersOpt->SetValue( true );
    m_TextAndGraphicsOpt->SetValue( true );
    m_ConstraintsOpt->SetValue( true );
    m_NetclassesOpt->SetValue( true );
    m_TracksAndViasOpt->SetValue( true );
    m_MaskAndPasteOpt->SetValue( true );
}
