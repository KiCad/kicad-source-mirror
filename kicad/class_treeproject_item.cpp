/**
 * @file class_treeproject_item.cpp
 *
 * @brief Class TREEPROJECT_ITEM is a derived  class from wxTreeItemData and
 * store info about a file or directory shown in the KiCad tree project files
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <gestfich.h>
#include <macros.h>

#include <kicad.h>
#include <project.h>
#include <pgm_base.h>
#include <tree_project_frame.h>
#include <class_treeprojectfiles.h>
#include <class_treeproject_item.h>
#include <wx/imaglist.h>

#include <wx/regex.h>
#include <wx/dir.h>

TREEPROJECT_ITEM::TREEPROJECT_ITEM( enum TreeFileType type, const wxString& data,
                                    wxTreeCtrl* parent ) :
    wxTreeItemData()
{
    m_parent = parent;

    SetType( type );
    SetFileName( data );
    SetRootFile( false );    // true only for the root item of the tree (the project name)
    SetPopulated( false );
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
        wxMessageDialog dialog( m_parent,
                                _(
                                    "Changing file extension will change file \
type.\n Do you want to continue ?"                                                                                 ),
                                _( "Rename File" ),
                                wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

#if ( ( wxMAJOR_VERSION < 2 ) || ( ( wxMAJOR_VERSION == 2 ) \
    && ( wxMINOR_VERSION < 7 )  ) )

    if( !wxRenameFile( GetFileName(), newFile ) )
#else

    if( !wxRenameFile( GetFileName(), newFile, false ) )
#endif
    {
        wxMessageDialog( m_parent, _( "Unable to rename file ... " ),
                         _( "Permission error ?" ), wxICON_ERROR | wxOK );
        return false;
    }

#ifndef KICAD_USE_FILES_WATCHER
    SetFileName( newFile );
#endif

    return true;
}


bool TREEPROJECT_ITEM::Delete( bool check )
{
    wxString    msg = wxString::Format( _(
                    "Do you really want to delete '%s'" ),
                    GetChars( GetFileName() )
                    );

    wxMessageDialog dialog( m_parent, msg,
                            _( "Delete File" ), wxYES_NO | wxICON_QUESTION );

    if( !check || wxID_YES == dialog.ShowModal() )
    {
        bool success;

        if( !wxDirExists( GetFileName() ) )
            success = wxRemoveFile( GetFileName() );
        else
        {
            wxArrayString filelist;

            wxDir::GetAllFiles( GetFileName(), &filelist );

            for( unsigned int i = 0; i < filelist.Count(); i++ )
                wxRemoveFile( filelist[i] );

            success = wxRmdir( GetFileName() );
        }

        if( success )
            m_parent->Delete( GetId() );

        return success;
    }

    return false;
}


void TREEPROJECT_ITEM::Activate( TREE_PROJECT_FRAME* prjframe )
{
    wxString        sep = wxFileName().GetPathSeparator();
    wxString        fullFileName = GetFileName();
    wxTreeItemId    id = GetId();

    KICAD_MANAGER_FRAME* mainFrame = (KICAD_MANAGER_FRAME*) Pgm().App().GetTopWindow();

    switch( GetType() )
    {
    case TREE_PROJECT:
        break;

    case TREE_DIRECTORY:
        m_parent->Toggle( id );
        break;

    case TREE_SCHEMA:
        {
            wxFileName  ffn( fullFileName );
            wxFileName  pro( mainFrame->GetProjectFileName() );

            // compare all but the extension:
            if( pro.GetPath()==ffn.GetPath() && pro.GetName()==ffn.GetName() )
            {
                // the project's schematic is opened using the *.kiface as part of this process.
                mainFrame->RunEeschema( fullFileName );
            }
            else
            {
                // schematics not part of the project are opened in a separate process.
                mainFrame->Execute( m_parent, EESCHEMA_EXE, fullFileName );
            }
        }
        break;

    case TREE_LEGACY_PCB:
    case TREE_SEXP_PCB:
        {
            wxFileName  ffn( fullFileName );
            wxFileName  pro( mainFrame->GetProjectFileName() );

            // compare all but the extension:
            if( pro.GetPath()==ffn.GetPath() && pro.GetName()==ffn.GetName() )
            {
                // the project's BOARD is opened using the *.kiface as part of this process.
                mainFrame->RunPcbNew( fullFileName );
            }
            else
            {
                // boards not part of the project are opened in a separate process.
                mainFrame->Execute( m_parent, PCBNEW_EXE, fullFileName );
            }
        }
        break;

    case TREE_GERBER:
        mainFrame->Execute( m_parent, GERBVIEW_EXE, fullFileName );
        break;

    case TREE_PDF:
        AddDelimiterString( fullFileName );
        OpenPDF( fullFileName );
        break;

    case TREE_NET:
        mainFrame->Execute( m_parent, CVPCB_EXE, fullFileName );
        break;

    case TREE_TXT:
        {
            wxString editorname = Pgm().GetEditorName();

            if( !editorname.IsEmpty() )
                mainFrame->Execute( m_parent, editorname, fullFileName );
        }
        break;

    case TREE_PAGE_LAYOUT_DESCR:
        mainFrame->Execute( m_parent, PL_EDITOR_EXE, fullFileName );
        break;

    default:
        AddDelimiterString( fullFileName );
        OpenFile( fullFileName );
        break;
    }
}
