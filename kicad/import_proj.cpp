/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "import_proj.h"
#include <wildcards_and_files_ext.h>
#include <macros.h>
#include <string_utils.h>
#include <richio.h>

#include <wx/fileconf.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/dir.h>
#include <wx/textfile.h>
#include <wx/wfstream.h>

#include <kiway.h>
#include <kiway_player.h>
#include <kicad_manager_frame.h>
#include <pcb_edit_frame.h>
#include <sch_edit_frame.h>

#include <sch_io/sch_io_mgr.h>
#include <pcb_io/pcb_io_mgr.h>
#include <project_sch.h>
#include <project_pcb.h>

#include <io/easyedapro/easyedapro_import_utils.h>
#include <io/easyedapro/easyedapro_parser.h>
#include <io/common/plugin_common_choose_project.h>
#include <io/pads/pads_common.h>
#include <dialogs/dialog_import_choose_project.h>

#include <wx/log.h>


IMPORT_PROJ_HELPER::IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME*         aFrame,
                                        const std::vector<wxString>& aSchFileExtensions,
                                        const std::vector<wxString>& aPcbFileExtensions ) :
        m_frame( aFrame ),
        m_schExtenstions( aSchFileExtensions ), m_pcbExtenstions( aPcbFileExtensions )
{
}


void IMPORT_PROJ_HELPER::FindEmptyTargetDir()
{
    // Append a new directory with the same name of the project file
    // Keep iterating until we find an empty directory
    wxString newDir = m_TargetProj.GetName();
    int      attempt = 0;

    m_TargetProj.AppendDir( newDir );

    while( m_TargetProj.DirExists() )
    {
        m_TargetProj.RemoveLastDir();
        wxString suffix = wxString::Format( "_%d", ++attempt );
        m_TargetProj.AppendDir( newDir + suffix );
    }
}


class SCOPED_FILE_REMOVER
{
    wxString m_file;

public:
    SCOPED_FILE_REMOVER( const wxString& aFile ) : m_file( aFile ) {}
    ~SCOPED_FILE_REMOVER() { wxRemoveFile( m_file ); }
};


void IMPORT_PROJ_HELPER::ImportIndividualFile( KICAD_T aFT, int aImportedFileType )
{
    FRAME_T               frame_type;
    wxString              appImportFile;
    std::vector<wxString> neededExts;

    switch( aFT )
    {
    case SCHEMATIC_T:
        neededExts = m_schExtenstions;
        frame_type = FRAME_SCH;
        break;

    case PCB_T:
        neededExts = m_pcbExtenstions;
        frame_type = FRAME_PCB_EDITOR;
        break;

    default: return;
    }

    std::vector<SCOPED_FILE_REMOVER> copiedFiles;

    for( wxString ext : neededExts )
    {
        if( ext == wxS( "INPUT" ) )
            ext = m_InputFile.GetExt();

        wxFileName candidate = m_InputFile;
        candidate.SetExt( ext );

        if( !candidate.FileExists() )
            continue;

        wxFileName targetFile( m_TargetProj.GetPath(), candidate.GetName(), candidate.GetExt() );

        if( !targetFile.FileExists() )
        {
            bool copied = wxCopyFile( candidate.GetFullPath(), targetFile.GetFullPath(), false );

            if( copied )
            {
                // Will be auto-removed
                copiedFiles.emplace_back( targetFile.GetFullPath() );
            }
        }

        // Pick the first file to pass to application
        if( appImportFile.empty() && targetFile.FileExists() )
            appImportFile = targetFile.GetFullPath();
    }

    if( appImportFile.empty() )
        return;

    doImport( appImportFile, frame_type, aImportedFileType );
}


void IMPORT_PROJ_HELPER::doImport( const wxString& aFile, FRAME_T aFrameType, int aImportedFileType )
{
    if( KIWAY_PLAYER* frame = m_frame->Kiway().Player( aFrameType, true ) )
    {
        std::stringstream ss;
        ss << aImportedFileType << '\n' << TO_UTF8( aFile );

        for( const auto& [key, value] : m_properties )
            ss << '\n' << key << '\n' << value.wx_str();

        std::string packet = ss.str();
        frame->Kiway().ExpressMail( aFrameType, MAIL_IMPORT_FILE, packet, m_frame );

        if( !frame->IsShownOnScreen() )
            frame->Show( true );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( frame->IsIconized() )
            frame->Iconize( false );

        frame->Raise();
    }
}


void IMPORT_PROJ_HELPER::EasyEDAProProjectHandler()
{
    wxFileName fname = m_InputFile;

    if( fname.GetExt() == wxS( "epro" ) || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( fname.GetFullPath() );

        std::map<wxString, EASYEDAPRO::PRJ_SCHEMATIC> prjSchematics = project.at( "schematics" );
        std::map<wxString, EASYEDAPRO::PRJ_BOARD>     prjBoards = project.at( "boards" );
        std::map<wxString, wxString>                  prjPcbNames = project.at( "pcbs" );

        std::vector<IMPORT_PROJECT_DESC> toImport =
                EASYEDAPRO::ProjectToSelectorDialog( project, false, false );

        if( toImport.size() > 1 )
            toImport = DIALOG_IMPORT_CHOOSE_PROJECT::RunModal( m_frame, toImport );

        if( toImport.size() == 1 )
        {
            const IMPORT_PROJECT_DESC& desc = toImport[0];

            m_properties["pcb_id"] = desc.PCBId;
            m_properties["sch_id"] = desc.SchematicId;
        }
        else
        {
            m_properties["pcb_id"] = "";
            m_properties["sch_id"] = "";
        }
    }
}


void IMPORT_PROJ_HELPER::addLocalLibraries( const std::set<wxString>& aNames, FRAME_T aFrameType )
{
    KIWAY_PLAYER* frame = m_frame->Kiway().Player( aFrameType, true );

    std::stringstream ss;

    // First line is the source project directory so the handler can distinguish
    // project-local libraries from external fixed-path references.
    ss << TO_UTF8( m_InputFile.GetPath() ) << '\n';

    for( const wxString& name : aNames )
    {
        wxFileName fname( name );
        fname.MakeAbsolute( m_InputFile.GetPath() );
        ss << TO_UTF8( fname.GetFullPath() ) << '\n';
    }

    std::string packet = ss.str();
    frame->Kiway().ExpressMail( aFrameType, MAIL_ADD_LOCAL_LIB, packet, m_frame );
}


void IMPORT_PROJ_HELPER::AltiumProjectHandler()
{
    wxFileConfig config( wxEmptyString, wxEmptyString, wxEmptyString, m_InputFile.GetFullPath(),
                         wxCONFIG_USE_NO_ESCAPE_CHARACTERS );

    wxString groupname;
    long groupid;

    std::set<wxString> sch_file;
    std::set<wxString> pcb_file;
    std::set<wxString> sch_libs;
    std::set<wxString> pcb_libs;

    for( bool more = config.GetFirstGroup( groupname, groupid ); more;
         more = config.GetNextGroup( groupname, groupid ) )
    {
        if( !groupname.StartsWith( wxS( "Document" ) ) )
            continue;

        wxString number = groupname.Mid( 8 );
        long docNumber;

        if( !number.ToLong( &docNumber ) )
            continue;

        wxString path = config.Read( groupname + wxS( "/DocumentPath" ), wxEmptyString );

        if( path.empty() )
            continue;

        wxFileName fname( path, wxPATH_WIN );

        if( !fname.IsAbsolute() )
            fname.MakeAbsolute( m_InputFile.GetPath() );

        if( !fname.GetExt().CmpNoCase( "PCBDOC" ) )
            pcb_file.insert( fname.GetFullPath() );

        if( !fname.GetExt().CmpNoCase( "SCHDOC" ) )
            sch_file.insert( fname.GetFullPath() );

        if( !fname.GetExt().CmpNoCase( "PCBLIB" ) )
            pcb_libs.insert( fname.GetFullPath() );

        if( !fname.GetExt().CmpNoCase( "SCHLIB" ) )
            sch_libs.insert( fname.GetFullPath() );
    }

    addLocalLibraries( sch_libs, FRAME_SCH );
    addLocalLibraries( pcb_libs, FRAME_PCB_EDITOR );

    m_properties["project_file"] = m_InputFile.GetFullPath();

    int ii = 0;

    for( auto& path : sch_file )
    {
        std::string key = "sch" + std::to_string( ii++ );
        m_properties[key] = path.ToStdString();
    }

    if( !sch_file.empty() )
        doImport( "", FRAME_SCH, SCH_IO_MGR::SCH_ALTIUM );

    if( !pcb_file.empty() )
        doImport( *pcb_file.begin(), FRAME_PCB_EDITOR, PCB_IO_MGR::ALTIUM_DESIGNER );
}


void IMPORT_PROJ_HELPER::ImportPadsFiles()
{
    m_properties.clear();

    PADS_COMMON::RELATED_FILES related = PADS_COMMON::FindRelatedPadsFiles( m_InputFile.GetFullPath() );

    if( !related.HasPcb() && !related.HasSchematic() )
        return;

    std::vector<SCOPED_FILE_REMOVER> copiedFiles;

    auto copyAndImport = [&]( const wxString& sourceFile, FRAME_T frameType, int fileType )
    {
        if( sourceFile.IsEmpty() )
            return;

        wxFileName srcFn( sourceFile );
        wxFileName targetFile( m_TargetProj.GetPath(), srcFn.GetName(), srcFn.GetExt() );

        if( !targetFile.FileExists() )
        {
            bool copied = wxCopyFile( srcFn.GetFullPath(), targetFile.GetFullPath(), false );

            if( copied )
                copiedFiles.emplace_back( targetFile.GetFullPath() );
        }

        if( targetFile.FileExists() )
            doImport( targetFile.GetFullPath(), frameType, fileType );
    };

    copyAndImport( related.schematicFile, FRAME_SCH, SCH_IO_MGR::SCH_PADS );
    copyAndImport( related.pcbFile, FRAME_PCB_EDITOR, PCB_IO_MGR::PADS );
}


void IMPORT_PROJ_HELPER::GedaProjectHandler()
{
    wxFileName prjFile = m_InputFile;

    if( prjFile.GetExt().CmpNoCase( "prj" ) != 0 )
        return;

    wxTextFile file;

    if( !file.Open( prjFile.GetFullPath() ) )
    {
        wxLogWarning( _( "Could not open gEDA / Lepton EDA project file '%s'." ), prjFile.GetFullPath() );
        return;
    }

    std::vector<wxFileName> schFiles;
    wxString                outputName;
    wxString                elementsDir;

    for( size_t i = 0; i < file.GetLineCount(); ++i )
    {
        wxString line = file.GetLine( i );

        line.Trim( true );
        line.Trim( false );

        if( line.IsEmpty() )
            continue;

        if( line.StartsWith( wxT( "#" ) ) || line.StartsWith( wxT( ";" ) ) )
            continue;

        int commentPos = line.Find( wxT( "#" ) );

        if( commentPos != wxNOT_FOUND )
        {
            line = line.Left( commentPos );
            line.Trim( true );
            line.Trim( false );
        }

        if( line.IsEmpty() )
            continue;

        wxStringTokenizer tok( line );

        if( !tok.HasMoreTokens() )
            continue;

        wxString keyword = tok.GetNextToken();

        if( keyword.CmpNoCase( "schematics" ) == 0 )
        {
            while( tok.HasMoreTokens() )
            {
                wxString  schToken = tok.GetNextToken();
                wxFileName schFile( schToken );

                if( !schFile.IsAbsolute() )
                    schFile.MakeAbsolute( prjFile.GetPath() );

                schFiles.push_back( schFile );
            }
        }
        else if( keyword.CmpNoCase( "output-name" ) == 0 )
        {
            wxString rest = line.Mid( keyword.length() );
            rest.Trim( true );
            rest.Trim( false );
            outputName = rest;
        }
        else if( keyword.CmpNoCase( "elements-dir" ) == 0 )
        {
            wxString rest = line.Mid( keyword.length() );
            rest.Trim( true );
            rest.Trim( false );
            elementsDir = rest;
        }
    }

    if( !elementsDir.IsEmpty() )
    {
        wxFileName elementsPath( elementsDir );

        if( !elementsPath.IsAbsolute() )
            elementsPath.MakeAbsolute( prjFile.GetPath() );

        m_properties["elements_dir"] = elementsPath.GetFullPath().ToStdString();
    }

    if( schFiles.empty() )
    {
        wxFileName candidate = prjFile;
        candidate.SetExt( wxS( "sch" ) );

        if( candidate.FileExists() )
            schFiles.push_back( candidate );
    }

    auto promptForMissingFile =
            [&]( const wxString& aTitle, const wxString& aWildcard, wxFileName& aFile ) -> bool
            {
                wxString defaultDir = aFile.GetPath();

                if( defaultDir.IsEmpty() )
                    defaultDir = prjFile.GetPath();

                wxFileDialog dlg( m_frame, aTitle, defaultDir, wxEmptyString, aWildcard,
                                  wxFD_OPEN | wxFD_FILE_MUST_EXIST );

                if( dlg.ShowModal() == wxID_OK )
                {
                    aFile.Assign( dlg.GetPath() );
                    return true;
                }

                aFile.Clear();
                return false;
            };

    std::vector<wxFileName> resolvedSchFiles;
    const wxString          schWildcard = _( "gEDA / Lepton EDA schematic files" ) + wxS( " (*.sch)|*.sch" );

    for( wxFileName schFile : schFiles )
    {
        if( !schFile.FileExists() )
        {
            if( !promptForMissingFile( _( "Locate gEDA / Lepton EDA Schematic" ), schWildcard, schFile ) )
                continue;
        }

        resolvedSchFiles.push_back( schFile );
    }

    if( schFiles.empty() && resolvedSchFiles.empty() )
    {
        wxFileName schFile;

        if( promptForMissingFile( _( "Locate gEDA / Lepton EDA Schematic" ), schWildcard, schFile ) )
            resolvedSchFiles.push_back( schFile );
    }

    if( resolvedSchFiles.size() > 1 )
    {
        // Pass additional schematic files as a semicolon-delimited property so the
        // importer can create sub-sheets for each additional page within a single
        // KiCad project hierarchy.
        wxString additionalFiles;

        for( size_t i = 1; i < resolvedSchFiles.size(); i++ )
        {
            if( !additionalFiles.IsEmpty() )
                additionalFiles += wxT( ";" );

            additionalFiles += resolvedSchFiles[i].GetFullPath();
        }

        m_properties["additional_schematics"] = additionalFiles.ToStdString();
    }

    // Auto-discover subdirectories containing .sym files and pass them as
    // additional search paths for the importer's symbol resolution.
    auto discoverSymDirs = [&]( const wxString& aBaseDir, wxString& aSymPaths )
    {
        wxDir dir( aBaseDir );

        if( !dir.IsOpened() )
            return;

        wxString subdir;
        bool     cont = dir.GetFirst( &subdir, wxEmptyString, wxDIR_DIRS );

        while( cont )
        {
            wxString subdirPath = aBaseDir + wxFileName::GetPathSeparator() + subdir;
            wxDir    childDir( subdirPath );

            if( childDir.IsOpened() && childDir.HasFiles( wxT( "*.sym" ) ) )
            {
                if( !aSymPaths.IsEmpty() )
                    aSymPaths += wxT( "\n" );

                aSymPaths += subdirPath;
            }

            cont = dir.GetNext( &subdir );
        }
    };

    wxString symPaths;
    discoverSymDirs( prjFile.GetPath(), symPaths );

    if( !resolvedSchFiles.empty() )
    {
        wxString schDir = resolvedSchFiles[0].GetPath();

        if( schDir != prjFile.GetPath() )
            discoverSymDirs( schDir, symPaths );
    }

    if( !symPaths.IsEmpty() )
        m_properties["sym_search_paths"] = symPaths.ToStdString();

    if( !resolvedSchFiles.empty() )
        doImport( resolvedSchFiles[0].GetFullPath(), FRAME_SCH, SCH_IO_MGR::SCH_GEDA );

    wxFileName layoutDir( prjFile.GetPath(), wxEmptyString );
    layoutDir.RemoveLastDir();
    layoutDir.AppendDir( wxS( "layout" ) );

    wxFileName pcbFile;

    if( !outputName.IsEmpty() )
    {
        wxFileName candidate( layoutDir.GetPath(), outputName, wxS( "pcb" ) );

        if( candidate.FileExists() )
            pcbFile = candidate;
    }

    if( !pcbFile.FileExists() )
    {
        wxFileName candidate( prjFile.GetPath(), prjFile.GetName(), wxS( "pcb" ) );

        if( candidate.FileExists() )
            pcbFile = candidate;
    }

    if( !pcbFile.FileExists() )
    {
        wxFileName candidate( layoutDir.GetPath(), prjFile.GetName(), wxS( "pcb" ) );

        if( candidate.FileExists() )
            pcbFile = candidate;
    }

    const wxString pcbWildcard = _( "gEDA / Lepton EDA PCB files" ) + wxS( " (*.pcb)|*.pcb" );

    if( !pcbFile.FileExists() )
        promptForMissingFile( _( "Locate gEDA / Lepton EDA PCB" ), pcbWildcard, pcbFile );

    if( pcbFile.FileExists() )
        doImport( pcbFile.GetFullPath(), FRAME_PCB_EDITOR, PCB_IO_MGR::GEDA_PCB );
}


void IMPORT_PROJ_HELPER::ImportFiles( int aImportedSchFileType, int aImportedPcbFileType )
{
    m_properties.clear();

    if( aImportedSchFileType == SCH_IO_MGR::SCH_EASYEDAPRO
        || aImportedPcbFileType == PCB_IO_MGR::EASYEDAPRO )
    {
        EasyEDAProProjectHandler();
    }
    else if( aImportedSchFileType == SCH_IO_MGR::SCH_ALTIUM
             || aImportedPcbFileType == PCB_IO_MGR::ALTIUM_DESIGNER )
    {
        AltiumProjectHandler();
        return;
    }
    else if( aImportedSchFileType == SCH_IO_MGR::SCH_GEDA )
    {
        if( m_InputFile.GetExt().CmpNoCase( "prj" ) == 0 )
        {
            GedaProjectHandler();
        }
        else if( m_InputFile.GetExt().CmpNoCase( "pcb" ) == 0 )
        {
            ImportIndividualFile( PCB_T, aImportedPcbFileType );
        }
        else
        {
            // When importing a bare .sch file, pass the original source directory
            // so the importer can find symbols in subdirectories relative to the
            // gEDA / Lepton EDA schematic, even though the file may be copied to a new location
            // for the KiCad project.
            wxString sourceDir = m_InputFile.GetPath();
            wxString symPaths = sourceDir;

            auto discoverSymDirs = [&]( const wxString& aBaseDir )
            {
                wxDir dir( aBaseDir );

                if( !dir.IsOpened() )
                    return;

                wxString subdir;
                bool     cont = dir.GetFirst( &subdir, wxEmptyString, wxDIR_DIRS );

                while( cont )
                {
                    wxString subdirPath =
                            aBaseDir + wxFileName::GetPathSeparator() + subdir;
                    wxDir childDir( subdirPath );

                    if( childDir.IsOpened() && childDir.HasFiles( wxT( "*.sym" ) ) )
                        symPaths += wxT( "\n" ) + subdirPath;

                    cont = dir.GetNext( &subdir );
                }
            };

            discoverSymDirs( sourceDir );
            m_properties["sym_search_paths"] = symPaths.ToStdString();

            doImport( m_InputFile.GetFullPath(), FRAME_SCH, SCH_IO_MGR::SCH_GEDA );
            ImportIndividualFile( PCB_T, aImportedPcbFileType );
        }

        return;
    }

    ImportIndividualFile( SCHEMATIC_T, aImportedSchFileType );
    ImportIndividualFile( PCB_T, aImportedPcbFileType );
}
