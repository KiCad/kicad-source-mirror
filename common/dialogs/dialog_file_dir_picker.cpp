/*
 * Copyright (C) 2018 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

// inspired by David Hart's FileDirDlg widget (4Pane project)

#include "dialog_file_dir_picker.h"
#include <wx/filename.h>


DIALOG_FILE_DIR_PICKER::DIALOG_FILE_DIR_PICKER( wxWindow* parent, const wxString& title,
        const wxString& defaultPath, const wxString& wildcard, int style )
    : DIALOG_SHIM( parent, wxID_ANY, title, wxDefaultPosition,
                   wxSize( -1, 600 ), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    m_showHidden = nullptr;

    wxString path = defaultPath.IsEmpty() ? wxGetCwd() : defaultPath;
    m_filesOnly = style & FD_RETURN_FILESONLY;

    int m_GDC_style = wxDIRCTRL_3D_INTERNAL | wxDIRCTRL_EDIT_LABELS;

    if( !wildcard.IsEmpty() )
        m_GDC_style |= wxDIRCTRL_SHOW_FILTERS;

    if( style & FD_MULTIPLE )
        m_GDC_style |= wxDIRCTRL_MULTIPLE;


    SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* mainSizer;
    mainSizer = new wxBoxSizer( wxVERTICAL );

    m_GDC = new wxGenericDirCtrl( this, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxDefaultSize, m_GDC_style );

    m_GDC->ShowHidden( style & FD_SHOW_HIDDEN );

    // TODO filter is not applied until it is reselected in the drop-down list, why?
    if( !wildcard.IsEmpty() )
        m_GDC->SetFilter( wildcard );

    mainSizer->Add( m_GDC, 1, wxEXPAND | wxALL, 5 );

    // TODO commented out due to string freeze, uncomment in v6
    //m_showHidden = new wxCheckBox( this, wxID_ANY, _( "Show Hidden" ), wxDefaultPosition, wxDefaultSize, 0 );
    //m_showHidden->SetValue( style & FD_SHOW_HIDDEN );
    //mainSizer->Add( m_showHidden, 0, wxALL, 5 );

    auto sdbSizer = new wxStdDialogButtonSizer();
    sdbSizer->AddButton( new wxButton( this, wxID_OK ) );
    sdbSizer->AddButton( new wxButton( this, wxID_CANCEL ) );
    sdbSizer->Realize();
    mainSizer->Add( sdbSizer, 0, wxEXPAND | wxALL, 5 );

    SetSizer( mainSizer );
    Layout();
    Centre( wxBOTH );

    // Use Connect() here instead of an event table entry, as otherwise an
    // event is fired before the dialog is created and m_GDC and Text initialised
    Connect( wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &DIALOG_FILE_DIR_PICKER::onHidden );

    // Call SetDirectory() to make the path visible
    SetDirectory( path );
}


void DIALOG_FILE_DIR_PICKER::SetDirectory( const wxString& aDirectory ) const
{
    // Make the requested path visible. Without scrolling to the bottom, the requested path
    // is just below the window.
    wxArrayTreeItemIds selections;
    auto treeCtrl = m_GDC->GetTreeCtrl();

    treeCtrl->UnselectAll();
    m_GDC->SetPath( aDirectory );
    m_GDC->SetDefaultPath( aDirectory );
    treeCtrl->GetSelections( selections );

    if( !selections.IsEmpty() && selections[0].IsOk() )
    {
        auto lastChild = treeCtrl->GetLastChild( treeCtrl->GetRootItem() );

        if( lastChild.IsOk() )
            treeCtrl->ScrollTo( lastChild );

        treeCtrl->ScrollTo( selections[0] );
    }
}


wxString DIALOG_FILE_DIR_PICKER::GetDirectory() const
{
    wxFileName fileName( m_GDC->GetPath() );

    // Strip the file name, if it is included in the path
    return fileName.FileExists() ? fileName.GetPath() : fileName.GetFullPath();
}


size_t DIALOG_FILE_DIR_PICKER::GetFilenames( wxArrayString& aFilePaths )
{
    wxArrayTreeItemIds selectedIds;
    size_t count = m_GDC->GetTreeCtrl()->GetSelections( selectedIds );

    for( size_t c = 0; c < count; ++c )
    {
        wxDirItemData* data = (wxDirItemData*) m_GDC->GetTreeCtrl()->GetItemData( selectedIds[c] );

        if( m_filesOnly && wxDirExists( data->m_path ) )
            continue; // If we only want files, skip dirs

        aFilePaths.Add( data->m_path );
    }

    return aFilePaths.GetCount();
}


void DIALOG_FILE_DIR_PICKER::onHidden( wxCommandEvent& event )
{
    m_GDC->ShowHidden( event.IsChecked() );
}
