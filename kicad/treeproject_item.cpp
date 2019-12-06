/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file treeproject_item.cpp
 *
 * @brief Class TREEPROJECT_ITEM is a derived  class from wxTreeItemData and
 * store info about a file or directory shown in the KiCad tree project files
 */


#include <wx/regex.h>

#include <gestfich.h>
#include <kiway.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include "treeprojectfiles.h"
#include "pgm_kicad.h"
#include "tree_project_frame.h"
#include "treeproject_item.h"
#include "kicad_id.h"


TREEPROJECT_ITEM::TREEPROJECT_ITEM( enum TreeFileType type, const wxString& data,
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


void TREEPROJECT_ITEM::SetState( int state )
{
    wxImageList* imglist = m_parent->GetImageList();

    if( !imglist || state < 0 || state >= imglist->GetImageCount() / ( TREE_MAX - 2 ) )
        return;

    m_state = state;
    int imgid = m_Type - 1 + state * ( TREE_MAX - 1 );
    m_parent->SetItemImage( GetId(), imgid );
    m_parent->SetItemImage( GetId(), imgid, wxTreeItemIcon_Selected );
}


const wxString TREEPROJECT_ITEM::GetDir() const
{
    if( TREE_DIRECTORY == m_Type )
        return GetFileName();

    return wxFileName( GetFileName() ).GetPath();
}


bool TREEPROJECT_ITEM::Rename( const wxString& name, bool check )
{
    // this is broken & unsafe to use on linux.
    if( m_Type == TREE_DIRECTORY )
        return false;

    if( name.IsEmpty() )
        return false;

    const wxString  sep = wxFileName().GetPathSeparator();
    wxString        newFile;
    wxString        dirs = GetDir();

    if( !dirs.IsEmpty() && GetType() != TREE_DIRECTORY )
        newFile = dirs + sep + name;
    else
        newFile = name;

    if( newFile == GetFileName() )
        return false;

    wxString    ext = TREE_PROJECT_FRAME::GetFileExt( GetType() );

    wxRegEx     reg( wxT( "^.*\\" ) + ext + wxT( "$" ), wxRE_ICASE );

    if( check && !ext.IsEmpty() && !reg.Matches( newFile ) )
    {
        wxMessageDialog dialog( m_parent, _( "Changing file extension will change file type.\n"
                                             "Do you want to continue ?" ),
                                _( "Rename File" ), wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

    if( !wxRenameFile( GetFileName(), newFile, false ) )
    {
        wxMessageDialog( m_parent, _( "Unable to rename file ... " ), _( "Permission error?" ),
                         wxICON_ERROR | wxOK );
        return false;
    }

    return true;
}


void TREEPROJECT_ITEM::Delete()
{
    bool isDirectory = wxDirExists( GetFileName() );

    wxString msg = wxString::Format( _( "Are you sure you want to delete '%s'?" ), GetFileName() );
    wxMessageDialog dialog( m_parent, msg, isDirectory ?  _( "Delete Directory" ) : _( "Delete File" ),
                            wxYES_NO | wxICON_QUESTION );

    if( dialog.ShowModal() == wxID_YES )
    {
        bool success;

        if( !isDirectory )
            success = wxRemoveFile( GetFileName() );
        else
            success = DeleteDirectory( GetFileName() );

        if( success )
            m_parent->Delete( GetId() );
    }
}


void TREEPROJECT_ITEM::Print()
{
    PrintFile( GetFileName() );
}


void TREEPROJECT_ITEM::Activate( TREE_PROJECT_FRAME* aTreePrjFrame )
{
    wxString             sep = wxFileName::GetPathSeparator();
    wxString             fullFileName = GetFileName();
    wxTreeItemId         id = GetId();
    std::string          packet;

    KICAD_MANAGER_FRAME* frame = aTreePrjFrame->m_Parent;
    TOOL_MANAGER*        toolMgr = frame->GetToolManager();
    KIWAY&               kiway = frame->Kiway();

    switch( GetType() )
    {
    case TREE_PROJECT:
        // Select a new project if this is not the current project:
        if( id != aTreePrjFrame->m_TreeProject->GetRootItem() )
            frame->LoadProject( fullFileName );
        break;

    case TREE_DIRECTORY:
        m_parent->Toggle( id );
        break;

    case TREE_SCHEMA:
        if( fullFileName == frame->SchFileName() )
        {
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSchematic, true );
        }
        else
        {
            // schematics not part of the project are opened in a separate process.
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editOtherSch, true, &fullFileName );
        }
        break;

    case TREE_LEGACY_PCB:
    case TREE_SEXP_PCB:
        if( fullFileName == frame->PcbFileName() || fullFileName == frame->PcbLegacyFileName() )
        {
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editPCB, true );
        }
        else
        {
            // boards not part of the project are opened in a separate process.
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editOtherPCB, true, &fullFileName );
        }
        break;

    case TREE_GERBER:
    case TREE_DRILL:
    case TREE_DRILL_NC:
    case TREE_DRILL_XNC:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::viewGerbers, true, &fullFileName );
        break;

    case TREE_HTML:
        wxLaunchDefaultBrowser( fullFileName );
        break;

    case TREE_PDF:
        OpenPDF( fullFileName );
        break;

    case TREE_NET:
    case TREE_TXT:
    case TREE_REPORT:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::openTextEditor, true, &fullFileName );
        break;

    case TREE_PAGE_LAYOUT_DESCR:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editWorksheet, true, &fullFileName );
        break;

    case TREE_FOOTPRINT_FILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editFootprints, true );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_FP_EDIT, packet );
        break;

    case TREE_SCHEMATIC_LIBFILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSymbols, true );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_SCH_LIB_EDITOR, MAIL_LIB_EDIT, packet );
        break;

    default:
        OpenFile( fullFileName );
        break;
    }
}
