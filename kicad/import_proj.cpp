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
#include <wx/msgdlg.h>
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

    ImportIndividualFile( SCHEMATIC_T, aImportedSchFileType );
    ImportIndividualFile( PCB_T, aImportedPcbFileType );
}
