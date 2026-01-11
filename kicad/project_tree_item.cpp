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

/**
 * @brief Class PROJECT_TREE_ITEM is a derived class from wxTreeItemData and
 * store info about a file or directory shown in the KiCad tree project files
 */


#include <wx/regex.h>
#include <wx/textfile.h>
#include <wx/filename.h>

#include <set>

#include <confirm.h>
#include <mail_type.h>
#include <gestfich.h>
#include <kiplatform/environment.h>
#include <kiplatform/io.h>
#include <kiway.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
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
        wxMessageDialog dialog( m_parent, _( "Changing file extension will change file type.\n"
                                             "Do you want to continue ?" ),
                                _( "Rename File" ), wxYES_NO | wxICON_QUESTION );

        if( wxID_YES != dialog.ShowModal() )
            return false;
    }

    if( !wxRenameFile( GetFileName(), newFile, false ) )
    {
        wxMessageDialog( m_parent, _( "Unable to rename file ... " ), _( "Permission denied" ),
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


/**
 * Scan a schematic file hierarchy to collect all sheet file paths.
 *
 * This performs a lightweight text scan of schematic files to find sheet references
 * without fully loading the schematic. Used to determine if a file is part of the
 * project hierarchy before deciding how to open it.
 *
 * @param aRootSchematic Full path to the root schematic file
 * @param aSheetFiles Set to populate with all schematic file paths in the hierarchy
 */
static void ScanSchematicHierarchy( const wxString& aRootSchematic,
                                    std::set<wxString>& aSheetFiles )
{
    if( aSheetFiles.count( aRootSchematic ) )
        return;

    aSheetFiles.insert( aRootSchematic );

    wxTextFile file;

    if( !file.Open( aRootSchematic ) )
        return;

    wxFileName rootFn( aRootSchematic );
    wxString   rootDir = rootFn.GetPath();

    // Look for: (property "Sheetfile" "filename.kicad_sch"
    wxRegEx sheetfileRe( "\\(property\\s+\"Sheetfile\"\\s+\"([^\"]+)\"" );

    for( wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine() )
    {
        if( sheetfileRe.Matches( line ) )
        {
            wxString sheetFile = sheetfileRe.GetMatch( line, 1 );

            // Resolve relative path
            wxFileName sheetFn( sheetFile );

            if( !sheetFn.IsAbsolute() )
                sheetFn.MakeAbsolute( rootDir );

            wxString fullPath = sheetFn.GetFullPath();

            if( wxFileExists( fullPath ) )
                ScanSchematicHierarchy( fullPath, aSheetFiles );
        }
    }
}


void PROJECT_TREE_ITEM::Activate( PROJECT_TREE_PANE* aTreePrjFrame )
{
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

    case TREE_FILE_TYPE::JOBSET_FILE:
        frame->OpenJobsFile( fullFileName );

        break;

    case TREE_FILE_TYPE::DIRECTORY:
        m_parent->Toggle( id );
        break;

    case TREE_FILE_TYPE::LEGACY_SCHEMATIC:
    case TREE_FILE_TYPE::SEXPR_SCHEMATIC:
    {
        wxString rootSchematic = frame->SchFileName();

        if( rootSchematic.IsEmpty() )
            rootSchematic = frame->SchLegacyFileName();

        if( fullFileName == rootSchematic )
        {
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSchematic );
        }
        else
        {
            // Check if this file is part of the project hierarchy by scanning the schematic files
            std::set<wxString> hierarchyFiles;

            if( !rootSchematic.IsEmpty() && wxFileExists( rootSchematic ) )
                ScanSchematicHierarchy( rootSchematic, hierarchyFiles );

            bool isInHierarchy = hierarchyFiles.count( fullFileName ) > 0;

            if( isInHierarchy )
            {
                // Open root schematic and navigate to the target sheet
                KIWAY_PLAYER* schFrame = kiway.Player( FRAME_SCH, false );

                if( !schFrame )
                {
                    // Launch eeschema with the root schematic
                    toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSchematic );
                    schFrame = kiway.Player( FRAME_SCH, false );
                }

                if( schFrame )
                {
                    packet = fullFileName.ToStdString();
                    kiway.ExpressMail( FRAME_SCH, MAIL_SCH_NAVIGATE_TO_SHEET, packet );
                }
            }
            else
            {
                // Not in hierarchy, open as standalone schematic
                toolMgr->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::editOtherSch, &fullFileName );
            }
        }

        break;
    }

    case TREE_FILE_TYPE::LEGACY_PCB:
    case TREE_FILE_TYPE::SEXPR_PCB:
        // Boards not part of the project are opened in a separate process.
        if( fullFileName == frame->PcbFileName() || fullFileName == frame->PcbLegacyFileName() )
            toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editPCB );
        else
            toolMgr->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::editOtherPCB, &fullFileName );

        break;

    case TREE_FILE_TYPE::GERBER:
    case TREE_FILE_TYPE::GERBER_JOB_FILE:
    case TREE_FILE_TYPE::DRILL:
    case TREE_FILE_TYPE::DRILL_NC:
    case TREE_FILE_TYPE::DRILL_XNC:
        toolMgr->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::viewGerbers, &fullFileName );
        break;

    case TREE_FILE_TYPE::HTML:
        wxLaunchDefaultBrowser( fullFileName );
        break;

    case TREE_FILE_TYPE::PDF:
        OpenPDF( fullFileName );
        break;

    case TREE_FILE_TYPE::NET:
    case TREE_FILE_TYPE::TXT:
    case TREE_FILE_TYPE::MD:
    case TREE_FILE_TYPE::REPORT:
        toolMgr->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::openTextEditor, &fullFileName );
        break;

    case TREE_FILE_TYPE::DRAWING_SHEET:
        toolMgr->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::editDrawingSheet, &fullFileName );
        break;

    case TREE_FILE_TYPE::FOOTPRINT_FILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editFootprints );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_FP_EDIT, packet );
        break;

    case TREE_FILE_TYPE::SCHEMATIC_LIBFILE:
    case TREE_FILE_TYPE::SEXPR_SYMBOL_LIB_FILE:
        toolMgr->RunAction( KICAD_MANAGER_ACTIONS::editSymbols );
        packet = fullFileName.ToStdString();
        kiway.ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_LIB_EDIT, packet );
        break;

    default:
        wxLaunchDefaultApplication( fullFileName );
        break;
    }
}
