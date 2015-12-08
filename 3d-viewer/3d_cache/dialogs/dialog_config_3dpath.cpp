/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <cstdlib>
#include <list>
#include <wx/filename.h>
#include <3d_cache.h>

#include "dialog_config_3dpath.h"

enum
{
    btnEditPath = wxID_HIGHEST + 1,
    btnAddPath,
    btnDeletePath,
    lbPathList
};


wxBEGIN_EVENT_TABLE( DLG_CFG_3DPATH, wxDialog )
    EVT_BUTTON( wxID_OK, DLG_CFG_3DPATH::OnOK )
    EVT_BUTTON( wxID_CANCEL, DLG_CFG_3DPATH::OnExit )
    EVT_BUTTON( btnEditPath, DLG_CFG_3DPATH::EditPath )
    EVT_BUTTON( btnAddPath, DLG_CFG_3DPATH::AddPath )
    EVT_BUTTON( btnDeletePath, DLG_CFG_3DPATH::DeletePath )
wxEND_EVENT_TABLE()


DLG_CFG_3DPATH::DLG_CFG_3DPATH( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver )
    : wxDialog( aParent, -1, _( "3D Model Path Configuration" ),
    wxDefaultPosition, wxDefaultSize,  wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU
    | wxRESIZE_BORDER | wxMINIMIZE_BOX )
{
    resolver = aResolver;

    Bind( wxEVT_LIST_ITEM_ACTIVATED, &DLG_CFG_3DPATH::EditPath, this, lbPathList );
    Bind( wxEVT_LIST_ITEM_SELECTED, &DLG_CFG_3DPATH::PathSelect, this, lbPathList );
    Bind( wxEVT_LIST_ITEM_DESELECTED, &DLG_CFG_3DPATH::PathSelect, this, lbPathList );

    wxBoxSizer *vboxMain = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *vboxSide = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* hboxTop = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hboxBot = new wxBoxSizer( wxHORIZONTAL );

    pathList = new wxListView( this, lbPathList, wxDefaultPosition, wxSize( 400, 200 ),
        wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL );

    pathList->AppendColumn( wxEmptyString );

    editButton = new wxButton( this, btnEditPath, _( "Edit" ),
        wxDefaultPosition, wxSize( 100, 30 ) );

    editButton->Enable( false );

    wxButton* addButton = new wxButton( this, btnAddPath, _( "Add" ),
        wxDefaultPosition, wxSize( 100, 30 ) );

    deleteButton = new wxButton( this, btnDeletePath, _( "Delete" ),
        wxDefaultPosition, wxSize( 100, 30 ) );

    deleteButton->Enable( false );

    wxButton* okButton = new wxButton( this, wxID_OK, _( "Ok" ),
        wxDefaultPosition, wxSize( 100, 30 ) );

    wxButton* cancelButton = new wxButton( this, wxID_CANCEL, _( "Cancel" ),
        wxDefaultPosition, wxSize( 100, 30 ) );

    vboxSide->Add( editButton, 0, wxALL, 10 );
    vboxSide->Add( addButton, 0, wxALL, 10 );
    vboxSide->Add( deleteButton, 0, wxALL, 10 );

    hboxTop->Add( pathList, 1, wxEXPAND | wxALL, 10 );
    hboxTop->Add( vboxSide, 0, wxEXPAND | wxALL, 10 );

    hboxBot->Add( okButton, 0, wxALL, 10 );
    hboxBot->Add( cancelButton, 0, wxALL, 10 );

    vboxMain->Add( hboxTop, 1, wxEXPAND | wxALL, 10 );
    vboxMain->Add( hboxBot, 0, wxEXPAND | wxALL, 10 );

    if( resolver )
    {
        const std::list< wxString >* pl = resolver->GetPaths();
        std::list< wxString >::const_iterator sL = pl->begin();
        std::list< wxString >::const_iterator eL = pl->end();

        // always skip the first entry which is the current project dir
        if( sL != eL )
            ++sL;

        long i = 0;

        while( sL != eL )
        {
            m_paths.push_back( *sL );
            pathList->InsertItem( i, *sL );
            ++i;
            ++sL;
        }

        pathList->SetColumnWidth(0, wxLIST_AUTOSIZE);
    }

    SetSizerAndFit( vboxMain );
    Centre();

    return;
}


void DLG_CFG_3DPATH::OnExit( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_EXIT );
    else
        Close( true );

    return;
}


void DLG_CFG_3DPATH::OnOK( wxCommandEvent& event )
{
    if( resolver )
        resolver->UpdatePathList( m_paths );

    if( IsModal() )
        EndModal( wxID_OK );
    else
        Close( true );

    return;
}


void DLG_CFG_3DPATH::EditPath( wxCommandEvent& event )
{
    long nItem = pathList->GetFirstSelected();
    wxString tmpname = m_paths[ nItem ];

    wxDirDialog* dd = new wxDirDialog( this, _( "Change 3D model directory" ),
        m_paths[ nItem ] );

    if( wxID_OK == dd->ShowModal() )
    {
        wxFileName path( wxFileName::DirName( dd->GetPath() ) );
        path.Normalize();
        wxString newname = path.GetPath();

        if( tmpname.Cmp( newname ) )
        {
            pathList->DeleteItem( nItem );
            pathList->InsertItem( nItem, newname );
            m_paths[ nItem ] = newname;
            pathList->Focus( nItem );
            editButton->Enable( false );
            deleteButton->Enable( false );
            pathList->SetColumnWidth(0, wxLIST_AUTOSIZE);
        }
    }

    delete dd;

    return;
}


void DLG_CFG_3DPATH::AddPath( wxCommandEvent& event )
{
    wxDirDialog* dd = new wxDirDialog( this, _( "Add a 3D model directory" ) );

    if( wxID_OK == dd->ShowModal() )
    {
        wxFileName path( wxFileName::DirName( dd->GetPath() ) );
        path.Normalize();
        wxString newname = path.GetPath();

        m_paths.push_back( newname );
        pathList->InsertItem( pathList->GetItemCount(), *m_paths.rbegin() );
        pathList->Focus( pathList->GetItemCount() -1 );
        editButton->Enable( false );
        deleteButton->Enable( false );
        pathList->SetColumnWidth(0, wxLIST_AUTOSIZE);
    }

    delete dd;

    return;
}


void DLG_CFG_3DPATH::DeletePath( wxCommandEvent& event )
{
    long nItem = pathList->GetFirstSelected();

    if( -1 == nItem )
        return;

    m_paths.erase( m_paths.begin() + nItem );
    pathList->DeleteItem( nItem );

    if( m_paths.size() > 0 )
    {
        if( nItem > 0 )
            --nItem;

        pathList->Select( nItem );
    }
    else
    {
        editButton->Enable( false );
        deleteButton->Enable( false );
        pathList->Select( -1 );
    }

    return;
}


void DLG_CFG_3DPATH::PathSelect( wxCommandEvent& event )
{
    long nItem = pathList->GetFirstSelected();

    if( -1 == nItem )
    {
        editButton->Enable( false );
        deleteButton->Enable( false );
        return;
    }

    editButton->Enable( true );
    deleteButton->Enable( true );

    return;
}
