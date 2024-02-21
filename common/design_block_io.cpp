/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <i18n_utility.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wildcards_and_files_ext.h>
#include <kiway_player.h>
#include <design_block_io.h>
#include <design_block.h>
#include <ki_exception.h>
#include <trace_helpers.h>
#include <fstream>

const wxString DESIGN_BLOCK_IO_MGR::ShowType( DESIGN_BLOCK_FILE_T aFileType )
{
    switch( aFileType )
    {
    case KICAD_SEXP: return _( "KiCad" );
    default: return wxString::Format( _( "UNKNOWN (%d)" ), aFileType );
    }
}


DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T
DESIGN_BLOCK_IO_MGR::EnumFromStr( const wxString& aFileType )
{
    if( aFileType == _( "KiCad" ) )
        return DESIGN_BLOCK_FILE_T( KICAD_SEXP );

    return DESIGN_BLOCK_FILE_T( DESIGN_BLOCK_FILE_UNKNOWN );
}


DESIGN_BLOCK_IO* DESIGN_BLOCK_IO_MGR::FindPlugin( DESIGN_BLOCK_FILE_T aFileType )
{
    switch( aFileType )
    {
    case KICAD_SEXP:          return new DESIGN_BLOCK_IO();
    default:                  return nullptr;
    }
}


DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T
DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl )
{
    if( IO_RELEASER<DESIGN_BLOCK_IO>( FindPlugin( KICAD_SEXP ) )->CanReadLibrary( aLibPath ) && aCtl != KICTL_NONKICAD_ONLY )
        return KICAD_SEXP;

    return DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE;
}


bool DESIGN_BLOCK_IO_MGR::ConvertLibrary( std::map<std::string, UTF8>* aOldFileProps,
                                          const wxString&              aOldFilePath,
                                          const wxString&              aNewFilePath )
{
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T oldFileType =
            DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( aOldFilePath );

    if( oldFileType == DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE )
        return false;


    IO_RELEASER<DESIGN_BLOCK_IO> oldFilePI( DESIGN_BLOCK_IO_MGR::FindPlugin( oldFileType ) );
    IO_RELEASER<DESIGN_BLOCK_IO> kicadPI(
            DESIGN_BLOCK_IO_MGR::FindPlugin( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) );
    wxArrayString dbNames;
    wxFileName    newFileName( aNewFilePath );

    if( newFileName.HasExt() )
    {
        wxString extraDir = newFileName.GetFullName();
        newFileName.ClearExt();
        newFileName.SetName( "" );
        newFileName.AppendDir( extraDir );
    }

    if( !newFileName.DirExists() && !wxFileName::Mkdir( aNewFilePath, wxS_DIR_DEFAULT ) )
        return false;

    try
    {
        bool bestEfforts = false; // throw on first error
        oldFilePI->DesignBlockEnumerate( dbNames, aOldFilePath, bestEfforts, aOldFileProps );

        for( const wxString& dbName : dbNames )
        {
            std::unique_ptr<const DESIGN_BLOCK> db(
                    oldFilePI->GetEnumeratedDesignBlock( aOldFilePath, dbName, aOldFileProps ) );
            kicadPI->DesignBlockSave( aNewFilePath, db.get() );
        }
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


const DESIGN_BLOCK_IO::IO_FILE_DESC DESIGN_BLOCK_IO::GetLibraryDesc() const
{
    return IO_BASE::IO_FILE_DESC( _HKI( "KiCad Design Block folders" ), {},
                                  { FILEEXT::KiCadDesignBlockLibPathExtension }, false );
}


long long DESIGN_BLOCK_IO::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return wxDateTime( 0.0 ).GetValue().GetValue();
}


void DESIGN_BLOCK_IO::CreateLibrary( const wxString&                    aLibraryPath,
                                     const std::map<std::string, UTF8>* aProperties )
{
    if( wxDir::Exists( aLibraryPath ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot overwrite library path '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    wxFileName dir;
    dir.SetPath( aLibraryPath );

    if( !dir.Mkdir() )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Library path '%s' could not be created.\n\n"
                                     "Make sure you have write permissions and try again." ),
                                  dir.GetPath() ) );
    }
}


bool DESIGN_BLOCK_IO::DeleteLibrary( const wxString&                    aLibraryPath,
                                     const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fn;
    fn.SetPath( aLibraryPath );

    // Return if there is no library path to delete.
    if( !fn.DirExists() )
        return false;

    if( !fn.IsDirWritable() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Insufficient permissions to delete folder '%s'." ),
                                          aLibraryPath.GetData() ) );
    }

    wxDir dir( aLibraryPath );

    // Design block folders should only contain sub-folders for each design block
    if( dir.HasFiles() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library folder '%s' has unexpected files." ),
                                          aLibraryPath.GetData() ) );
    }

    // Must delete all sub-directories before deleting the library directory
    if( dir.HasSubDirs() )
    {
        wxArrayString dirs;

        // Get all sub-directories in the library path
        dir.GetAllFiles( aLibraryPath, &dirs, wxEmptyString, wxDIR_DIRS );

        for( size_t i = 0; i < dirs.GetCount(); i++ )
        {
            wxFileName tmp = dirs[i];

            if( tmp.GetExt() != FILEEXT::KiCadDesignBlockLibPathExtension )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unexpected folder '%s' found in library "
                                                     "path '%s'." ),
                                                  dirs[i].GetData(), aLibraryPath.GetData() ) );
            }
        }

        for( size_t i = 0; i < dirs.GetCount(); i++ )
            wxRemoveFile( dirs[i] );
    }

    wxLogTrace( traceDesignBlocks, wxT( "Removing design block library '%s'." ),
                aLibraryPath.GetData() );

    // Some of the more elaborate wxRemoveFile() crap puts up its own wxLog dialog
    // we don't want that.  we want bare metal portability with no UI here.
    if( !wxFileName::Rmdir( aLibraryPath, wxPATH_RMDIR_RECURSIVE ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Design block library '%s' cannot be deleted." ),
                                          aLibraryPath.GetData() ) );
    }

    // For some reason removing a directory in Windows is not immediately updated.  This delay
    // prevents an error when attempting to immediately recreate the same directory when over
    // writing an existing library.
#ifdef __WINDOWS__
    wxMilliSleep( 250L );
#endif

    return true;
}


void DESIGN_BLOCK_IO::DesignBlockEnumerate( wxArrayString&  aDesignBlockNames,
                                            const wxString& aLibraryPath, bool aBestEfforts,
                                            const std::map<std::string, UTF8>* aProperties )
{
    // From the starting directory, look for all directories ending in the .block extension
    wxDir dir( aLibraryPath );

    if( !dir.IsOpened() )
        return;

    wxString dirname;
    wxString fileSpec = wxT( "*." ) + wxString( FILEEXT::KiCadDesignBlockPathExtension );
    bool     cont = dir.GetFirst( &dirname, fileSpec, wxDIR_DIRS );

    while( cont )
    {
        aDesignBlockNames.Add( dirname.Before( wxT( '.' ) ) );
        cont = dir.GetNext( &dirname );
    }
}


DESIGN_BLOCK* DESIGN_BLOCK_IO::DesignBlockLoad( const wxString& aLibraryPath,
                                                const wxString& aDesignBlockName, bool aKeepUUID,
                                                const std::map<std::string, UTF8>* aProperties )
{
    DESIGN_BLOCK* newDB = new DESIGN_BLOCK();
    wxString      dbPath = aLibraryPath + wxFileName::GetPathSeparator() +
                           aDesignBlockName + wxT( "." ) + FILEEXT::KiCadDesignBlockPathExtension + wxFileName::GetPathSeparator();
    wxString      dbSchPath = dbPath + aDesignBlockName + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension;
    wxString      dbMetadataPath = dbPath + aDesignBlockName + wxT( "." ) + FILEEXT::JsonFileExtension;



    // Library name needs to be empty for when we fill it in with the correct library nickname
    // one layer above
    newDB->SetLibId( LIB_ID( wxEmptyString, aDesignBlockName ) );
    newDB->SetSchematicFile(
            // Library path
            aLibraryPath + wxFileName::GetPathSeparator() +
            // Design block name (project folder)
            aDesignBlockName + +wxT( "." ) + FILEEXT::KiCadDesignBlockPathExtension + wxT( "/" ) +
            // Schematic file
            aDesignBlockName + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension );

    // Parse the JSON file if it exists
    if( wxFileExists( dbMetadataPath ) )
    {
        try
        {
            nlohmann::json dbMetadata;
            std::ifstream  dbMetadataFile( dbMetadataPath.fn_str() );
            dbMetadataFile >> dbMetadata;

            if( dbMetadata.contains( "description" ) )
                newDB->SetLibDescription( dbMetadata["description"] );

            if( dbMetadata.contains( "keywords" ) )
                newDB->SetKeywords( dbMetadata["keywords"] );

            if( dbMetadata.contains( "documentation_url" ) )
                newDB->SetDocumentationUrl( dbMetadata["documentation_url"] );
        }
        catch( ... )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Design block metadata file '%s' could not be read." ), dbMetadataPath ) );
        }
    }


    return newDB;
}


void DESIGN_BLOCK_IO::DesignBlockSave( const wxString&                    aLibraryPath,
                                       const DESIGN_BLOCK*                aDesignBlock,
                                       const std::map<std::string, UTF8>* aProperties )
{
    // Make sure we have a valid LIB_ID or we can't save the design block
    if( !aDesignBlock->GetLibId().IsValid() )
    {
        THROW_IO_ERROR( _( "Design block does not have a valid library ID." ) );
    }

    if( !wxFileExists( aDesignBlock->GetSchematicFile() ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Schematic source file '%s' does not exist." ),
                                          aDesignBlock->GetSchematicFile() ) );
    }

    // Create the design block folder
    wxFileName dbFolder( aLibraryPath + wxFileName::GetPathSeparator()
                         + aDesignBlock->GetLibId().GetLibItemName() + wxT( "." )
                         + FILEEXT::KiCadDesignBlockPathExtension
                         + wxFileName::GetPathSeparator() );

    if( !dbFolder.DirExists() )
    {
        if( !dbFolder.Mkdir() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Design block folder '%s' could not be created." ),
                                              dbFolder.GetFullPath().GetData() ) );
        }
    }

    // The new schematic file name is based on the design block name, not the source sheet name
    wxString dbSchematicFile = dbFolder.GetFullPath() + aDesignBlock->GetLibId().GetLibItemName()
                               + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension;

    // Copy the source sheet file to the design block folder, under the design block name
    if( !wxCopyFile( aDesignBlock->GetSchematicFile(), dbSchematicFile ) )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Schematic file '%s' could not be saved as design block at '%s'." ),
                aDesignBlock->GetSchematicFile().GetData(), dbSchematicFile ) );
    }
}


void DESIGN_BLOCK_IO::DesignBlockDelete( const wxString& aLibPath, const wxString& aDesignBlockName,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxFileName dbDir = wxFileName( aLibPath + wxFileName::GetPathSeparator() + aDesignBlockName
                                   + wxT( "." ) + FILEEXT::KiCadDesignBlockPathExtension );


    if( !dbDir.DirExists() )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Design block '%s' does not exist." ), dbDir.GetFullName() ) );
    }

    // Delete the whole design block folder
    if( !wxFileName::Rmdir( dbDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Design block folder '%s' could not be deleted." ),
                                          dbDir.GetFullPath().GetData() ) );
    }
}


bool DESIGN_BLOCK_IO::IsLibraryWritable( const wxString& aLibraryPath )
{
    wxFileName path( aLibraryPath );
    return path.IsOk() && path.IsDirWritable();
}
