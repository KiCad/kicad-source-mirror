/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Class PROJECT_TREE_ITEM is a derived class from wxTreeItemData and
 * store info about a file or directory shown in the KiCad tree project files
 */


#include <wx/regex.h>

#include <confirm.h>
#include <gestfich.h>
#include <kiplatform/environment.h>
#include <kiway.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>

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
    wxImageList* imglist     = m_parent->GetImageList();
    int          treeEnumMax = static_cast<int>( TREE_FILE_TYPE::MAX );

    if( !imglist || state < 0 || state >= imglist->GetImageCount() / ( treeEnumMax - 2 ) )
        return;

    m_state   = state;
    int imgid = static_cast<int>( m_type ) - 1 + state * ( treeEnumMax - 1 );
    m_parent->SetItemImage( GetId(), imgid );
    m_parent->SetItemImage( GetId(), imgid, wxTreeItemIcon_Selected );
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
    if( m_type == TREE_FILE_TYPE::DIRECTORY )
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

    wxString ext = PROJECT_TREE_PANE::GetFileExt( GetType() );
    wxRegEx  reg( wxT( "^.*\\" ) + ext + wxT( "$" ), wxRE_ICASE );

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


void PROJECT_TREE_ITEM::Delete()
{
    wxString errMsg;

    if( !KIPLATFORM::ENV::MoveToTrash( GetFileName(), errMsg ) )
    {
        wxString dialogMsg = wxString::Format( _( "Failed to delete '%s'"), GetFileName() );
        DisplayErrorMessage( m_parent, dialogMsg, errMsg );
        return;
    }

    m_parent->Delete( GetId() );
}


void PROJECT_TREE_ITEM::Print()
{
    PrintFile( GetFileName() );
}


void PROJECT_TREE_ITEM::Activate( PROJECT_TREE_PANE* aTreePrjFrame )
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
    case TREE_FILE_TYPE::LEGACY_PROJECT:
    case TREE_FILE_TYPE::JSON_PROJECT:
        // Select a new project if this is not the current project:
        if( id != aTreePrjFrame->m_TreeProject->GetRootItem() )
            frame->LoadProject( fullFileName );

        break;

    case TREE_FILE_TYPE::DIRECTORY:
        m_parent->Toggle( id );
        break;

    case TREE_FILE_TYPE::LEGACY_SCHEMATIC:
    case TREE_FILE_TYPE::SEXPR_SCHEMATIC:
        // Schematics not part of the project are opened in a separate process.
        if( fullFileName == frame->SchFileName() || fullFileName == frame->SchLegacyFileName() )
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSchematic, true );
        else
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editOtherSch, true, &fullFileName );

        break;

    case TREE_FILE_TYPE::LEGACY_PCB:
    case TREE_FILE_TYPE::SEXPR_PCB:
        // Boards not part of the project are opened in a separate process.
        if( fullFileName == frame->PcbFileName() || fullFileName == frame->PcbLegacyFileName() )
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editPCB, true );
        else
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editOtherPCB, true, &fullFileName );

        break;

    case TREE_FILE_TYPE::GERBER:
    case TREE_FILE_TYPE::GERBER_JOB_FILE:
    case TREE_FILE_TYPE::DRILL:
    case TREE_FILE_TYPE::DRILL_NC:
    case TREE_FILE_TYPE::DRILL_XNC:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::viewGerbers, true, &fullFileName );
        break;

    case TREE_FILE_TYPE::HTML:
        wxLaunchDefaultBrowser( fullFileName );
        break;

    case TREE_FILE_TYPE::PDF:
        OpenPDF( fullFileName );
        break;

    case TREE_FILE_TYPE::NET:
    case TREE_FILE_TYPE::TXT:
    case TREE_FILE_TYPE::REPORT:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::openTextEditor, true, &fullFileName );
        break;

    case TREE_FILE_TYPE::PAGE_LAYOUT_DESCR:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editDrawingSheet, true, &fullFileName );
        break;

    case TREE_FILE_TYPE::FOOTPRINT_FILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editFootprints, true );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_FP_EDIT, packet );
        break;

    case TREE_FILE_TYPE::SCHEMATIC_LIBFILE:
    case TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSymbols, true );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_LIB_EDIT, packet );
        break;

    default:
        OpenFile( fullFileName );
        break;
    }
}
