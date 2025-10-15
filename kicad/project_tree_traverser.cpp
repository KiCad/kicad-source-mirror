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

#include "project_tree_traverser.h"
#include "kicad_manager_frame.h"
#include <kiway.h>
#include <kiway_express.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <gestfich.h>
#include <confirm.h>
#include <string_utils.h>
#include <richio.h>
#include <wildcards_and_files_ext.h>


PROJECT_TREE_TRAVERSER::PROJECT_TREE_TRAVERSER( KICAD_MANAGER_FRAME* aFrame,
                                                const wxString& aSrcProjectDirPath,
                                                const wxString& aSrcProjectName,
                                                const wxString& aNewProjectDirPath,
                                                const wxString& aNewProjectName ) :
        m_frame( aFrame ),
        m_projectDirPath( aSrcProjectDirPath ),
        m_projectName( aSrcProjectName ),
        m_newProjectDirPath( aNewProjectDirPath ),
        m_newProjectName( aNewProjectName )
{
}


wxDirTraverseResult PROJECT_TREE_TRAVERSER::OnFile( const wxString& aSrcFilePath )
{
    // Recursion guard for a Save As to a location inside the source project.
    if( aSrcFilePath.StartsWith( m_newProjectDirPath + wxFileName::GetPathSeparator() ) )
        return wxDIR_CONTINUE;

    wxFileName destFile( aSrcFilePath );
    wxString   ext = destFile.GetExt();
    bool       atRoot = destFile.GetPath() == m_projectDirPath;

    if( ext == FILEEXT::LegacyProjectFileExtension
      || ext == FILEEXT::ProjectFileExtension
      || ext == FILEEXT::ProjectLocalSettingsFileExtension )
    {
        wxString destPath = destFile.GetPath();

        if( destPath.StartsWith( m_projectDirPath ) )
        {
            destPath.Replace( m_projectDirPath, m_newProjectDirPath, false );
            destFile.SetPath( destPath );
        }

        if( destFile.GetName() == m_projectName )
        {
            destFile.SetName( m_newProjectName );

            if( atRoot && ext != FILEEXT::ProjectLocalSettingsFileExtension )
                m_newProjectFile = destFile;
        }

        if( ext == FILEEXT::LegacyProjectFileExtension )
        {
            // All paths in the settings file are relative so we can just do a straight copy
            KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
        }
        else if( ext == FILEEXT::ProjectFileExtension )
        {
            PROJECT_FILE projectFile( aSrcFilePath );
            projectFile.LoadFromFile();
            projectFile.SaveAs( destFile.GetPath(), destFile.GetName() );
        }
        else if( ext == FILEEXT::ProjectLocalSettingsFileExtension )
        {
            PROJECT_LOCAL_SETTINGS projectLocalSettings( nullptr, aSrcFilePath );
            projectLocalSettings.LoadFromFile();
            projectLocalSettings.SaveAs( destFile.GetPath(), destFile.GetName() );
        }
    }
    else if( m_frame && ( ext == FILEEXT::KiCadSchematicFileExtension
             || ext == FILEEXT::KiCadSchematicFileExtension + FILEEXT::BackupFileSuffix
             || ext == FILEEXT::LegacySchematicFileExtension
             || ext == FILEEXT::LegacySchematicFileExtension + FILEEXT::BackupFileSuffix
             || ext == FILEEXT::SchematicSymbolFileExtension
             || ext == FILEEXT::LegacySymbolLibFileExtension
             || ext == FILEEXT::LegacySymbolDocumentFileExtension
             || ext == FILEEXT::KiCadSymbolLibFileExtension
             || ext == FILEEXT::NetlistFileExtension
             || destFile.GetName() == FILEEXT::SymbolLibraryTableFileName ) )
    {
        KIFACE* eeschema = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH );
        eeschema->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                              m_newProjectName, aSrcFilePath, m_errors );
    }
    else if( m_frame && ( ext == FILEEXT::KiCadPcbFileExtension
             || ext == FILEEXT::KiCadPcbFileExtension + FILEEXT::BackupFileSuffix
             || ext == FILEEXT::LegacyPcbFileExtension
             || ext == FILEEXT::KiCadFootprintFileExtension
             || ext == FILEEXT::LegacyFootprintLibPathExtension
             || ext == FILEEXT::FootprintAssignmentFileExtension
             || destFile.GetName() == FILEEXT::FootprintLibraryTableFileName ) )
    {
        KIFACE* pcbnew = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB );
        pcbnew->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                            m_newProjectName, aSrcFilePath, m_errors );
    }
    else if( m_frame && ext == FILEEXT::DrawingSheetFileExtension )
    {
        KIFACE* pleditor = m_frame->Kiway().KiFACE( KIWAY::FACE_PL_EDITOR );
        pleditor->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                              m_newProjectName, aSrcFilePath, m_errors );
    }
    else if( m_frame && ( ext == FILEEXT::GerberJobFileExtension
           || ext == FILEEXT::DrillFileExtension
             || FILEEXT::IsGerberFileExtension( ext ) ) )
    {
        KIFACE* gerbview = m_frame->Kiway().KiFACE( KIWAY::FACE_GERBVIEW );
        gerbview->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                              m_newProjectName, aSrcFilePath, m_errors );
    }
    else if( destFile.GetName().StartsWith( FILEEXT::LockFilePrefix )
             && ext == FILEEXT::LockFileExtension )
    {
        // Ignore lock files
    }
    else
    {
        // Everything we don't recognize just gets a straight copy.
        wxString  destPath = destFile.GetPathWithSep();
        wxString  destName = destFile.GetName();
        wxUniChar pathSep = wxFileName::GetPathSeparator();

        wxString srcProjectFootprintLib = pathSep + m_projectName + ".pretty" + pathSep;
        wxString newProjectFootprintLib = pathSep + m_newProjectName + ".pretty" + pathSep;

        if( destPath.StartsWith( m_projectDirPath ) )
            destPath.Replace( m_projectDirPath, m_newProjectDirPath, false );

        destPath.Replace( srcProjectFootprintLib, newProjectFootprintLib, true );

        if( destName == m_projectName && ext != wxT( "zip" ) /* don't rename archives */ )
            destFile.SetName( m_newProjectName );

        destFile.SetPath( destPath );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
    }

    return wxDIR_CONTINUE;
}


wxDirTraverseResult PROJECT_TREE_TRAVERSER::OnDir( const wxString& aSrcDirPath )
{
    // Recursion guard for a Save As to a location inside the source project.
    if( aSrcDirPath.StartsWith( m_newProjectDirPath ) )
        return wxDIR_CONTINUE;

    wxFileName destDir( aSrcDirPath );
    wxString   destDirPath = destDir.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();

    if( destDirPath.StartsWith( m_projectDirPath + pathSep )
      || destDirPath.StartsWith( m_projectDirPath + PROJECT_BACKUPS_DIR_SUFFIX ) )
    {
        destDirPath.Replace( m_projectDirPath, m_newProjectDirPath, false );
        destDir.SetPath( destDirPath );
    }

    if( destDir.GetName() == m_projectName )
    {
        if( destDir.GetExt() == "pretty" )
            destDir.SetName( m_newProjectName );
#if 0
        // WAYNE STAMBAUGH TODO:
        // If we end up with a symbol equivalent to ".pretty" we'll want to handle it here....
        else if( destDir.GetExt() == "sym_lib_dir_extension" )
            destDir.SetName( m_newProjectName );
#endif
    }

    if( !wxMkdir( destDir.GetFullPath() ) )
    {
        wxString msg;

        if( !m_errors.empty() )
            m_errors += "\n";

        msg.Printf( _( "Cannot copy folder '%s'." ), destDir.GetFullPath() );
        m_errors += msg;
    }

    return wxDIR_CONTINUE;
}
