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

/**
 * @brief Class PROJECT_TREE_ITEM is a derived class from wxTreeItemData and
 * store info about a file or directory shown in the KiCad tree project files
 */


#include <wx/filename.h>

#include <confirm.h>
#include <gestfich.h>
#include <kiplatform/environment.h>
#include <kiplatform/io.h>
#include <kiway.h>
#include <project.h>
#include <wx/msgdlg.h>

#include "kicad_manager_frame.h"
#include "project_tree.h"
#include "pgm_kicad.h"
#include "project_tree_pane.h"
#include "project_tree_item.h"
#include "kicad_id.h"


PROJECT_TREE_ITEM::PROJECT_TREE_ITEM( TREE_FILE_TYPE type, const wxString& data,
                                      wxTreeCtrl* parent ) :
    wxTreeItemData()
{
    m_parent = parent;
    SetType( type );
    SetFileName( data );
    SetRootFile( false );    // true only for the root item of the tree (the project name)
    SetPopulated( false );
    m_state = 0;
}


void PROJECT_TREE_ITEM::SetState( int state )
{
    int treeEnumMax = static_cast<int>( TREE_FILE_TYPE::MAX );

    if( state < 0 || state >= m_parent->GetImageCount() / ( treeEnumMax - 2 ) )
        return;

    m_state   = state;
    int imgid = static_cast<int>( m_type ) - 1 + state * ( treeEnumMax - 1 );
    m_parent->SetItemImage( GetId(), imgid );
    m_parent->SetItemImage( GetId(), imgid, wxTreeItemIcon_Selected );
}


bool PROJECT_TREE_ITEM::CanDelete() const
{
    if( m_type == TREE_FILE_TYPE::DIRECTORY
        || m_type == TREE_FILE_TYPE::LEGACY_PROJECT
        || m_type == TREE_FILE_TYPE::JSON_PROJECT
        || m_type == TREE_FILE_TYPE::LEGACY_SCHEMATIC
        || m_type == TREE_FILE_TYPE::SEXPR_SCHEMATIC
        || m_type == TREE_FILE_TYPE::LEGACY_PCB
        || m_type == TREE_FILE_TYPE::SEXPR_PCB
        || m_type == TREE_FILE_TYPE::DRAWING_SHEET
        || m_type == TREE_FILE_TYPE::FOOTPRINT_FILE
        || m_type == TREE_FILE_TYPE::SCHEMATIC_LIBFILE
        || m_type == TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE
        || m_type == TREE_FILE_TYPE::DESIGN_RULES )
        return false;

    return true;
}


const wxString PROJECT_TREE_ITEM::GetDir() const
{
    if( TREE_FILE_TYPE::DIRECTORY == m_type )
        return GetFileName();

    return wxFileName( GetFileName() ).GetPath();
}


bool PROJECT_TREE_ITEM::Rename( const wxString& name, bool check )
{
    // this is broken & unsafe to use on linux.
    if( !CanRename() )
        return false;

    if( name.IsEmpty() )
        return false;

    const wxString  sep = wxFileName().GetPathSeparator();
    wxString        newFile;
    wxString        dirs = GetDir();

    if( !dirs.IsEmpty() && GetType() != TREE_FILE_TYPE::DIRECTORY )
        newFile = dirs + sep + name;
    else
        newFile = name;

    if( newFile == GetFileName() )
        return false;

    // If required, prompt the user if the filename extension has changed:
    wxString ext = PROJECT_TREE_PANE::GetFileExt( GetType() ).Lower();
    wxString full_ext = wxT( "." ) + ext;

    if( check && !ext.IsEmpty() && !newFile.Lower().EndsWith( full_ext ) )
    {
        KICAD_MESSAGE_DIALOG dialog( m_parent, _( "Changing file extension will change file type.\n"
                                                  "Do you want to continue ?" ),
                                     _( "Rename File" ), wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

    if( !wxRenameFile( GetFileName(), newFile, false ) )
    {
        KICAD_MESSAGE_DIALOG( m_parent, _( "Unable to rename file ... " ), _( "Permission denied" ),
                              wxICON_ERROR | wxOK );
        return false;
    }

    return true;
}


void PROJECT_TREE_ITEM::Delete()
{
    if( !CanDelete() )
        return;

    wxString errMsg;

    if( !KIPLATFORM::ENV::MoveToTrash( GetFileName(), errMsg ) )
    {
#ifdef __WINDOWS__
        wxString dialogMsg = wxString::Format( _( "Can not move '%s' to recycle bin."),
                                               GetFileName() );
#else
        wxString dialogMsg = wxString::Format( _( "Can not move '%s' to trash."),
                                               GetFileName() );
#endif

        DisplayErrorMessage( m_parent, dialogMsg, errMsg );
        return;
    }

    m_parent->Delete( GetId() );
}


void PROJECT_TREE_ITEM::Activate( PROJECT_TREE_PANE* aTreePrjFrame )
{
    wxString             fullFileName = GetFileName();
    wxTreeItemId         id = GetId();
    KICAD_MANAGER_FRAME* frame = aTreePrjFrame->m_Parent;

    switch( GetType() )
    {
    case TREE_FILE_TYPE::LEGACY_PROJECT:
    case TREE_FILE_TYPE::JSON_PROJECT:
        // Select a new project if this is not the current project:
        if( id != aTreePrjFrame->m_TreeProject->GetRootItem() )
            frame->LoadProject( fullFileName );

        break;

    case TREE_FILE_TYPE::DIRECTORY:
        m_parent->Toggle( id );
        break;

    default:
        frame->OpenProjectTreeFile( fullFileName, GetType() );
        break;
    }
}
