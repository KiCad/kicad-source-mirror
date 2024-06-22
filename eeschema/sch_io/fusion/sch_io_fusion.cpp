/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2024 Kicad Developers, see AUTHORS.txt for contributors.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sch_io/fusion/sch_io_fusion.h>

#include <memory>
#include <wx/filename.h>

#include <sch_sheet.h>
#include <sch_screen.h>
#include <schematic.h>

#include <wildcards_and_files_ext.h>
#include <progress_reporter.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include "project_sch.h"
#include "locale_io.h"

SCH_IO_FUSION::SCH_IO_FUSION() : SCH_IO( wxS( "Autodesk Fusion" ) ),
        m_rootSheet( nullptr ),
        m_schematic( nullptr )//,
        //m_module( nullptr ),
        //m_sheetIndex( 1 )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_FUSION::~SCH_IO_FUSION()
{
}

bool SCH_IO_FUSION::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    return true;// TODO: checkHeader( aFileName );
}

bool SCH_IO_FUSION::CanReadLibrary( const wxString& aFileName ) const {
    return false; // not implemented
}

int SCH_IO_FUSION::GetModifyHash() const
{
    return 0;
}

nlohmann::json ReadProjectFile( const wxString& aZipFileName )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aZipFileName );
    wxZipInputStream            zip( in );

    std::cerr << "ReadProjectFile: " << aZipFileName << std::endl;

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();
        std::cerr << "read: " << name << std::endl;

        try
        {
            if( name == wxS( "DesignDescription.json" ) )
            {
                wxMemoryOutputStream memos;
                memos << zip;
                wxStreamBuffer* buf = memos.GetOutputStreamBuffer();

                wxString str =
                        wxString::FromUTF8( (char*) buf->GetBufferStart(), buf->GetBufferSize() );

                std::cerr << "read: " << str << std::endl;

                return nlohmann::json::parse( str );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error reading '%s': %s" ), name, e.what() ) );
        }
        catch( std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error reading '%s': %s" ), name, e.what() ) );
        }
    }

    THROW_IO_ERROR( wxString::Format(
            _( "'%s' does not appear to be a valid Autodesk Fusion "
               "project or schematic file. Cannot find project.json or device.json." ),
            aZipFileName ) );
}

nlohmann::json ReadSchematicFile( const wxString& aZipFileName )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aZipFileName );
    wxZipInputStream            zip( in );

    std::cerr << "ReadProjectFile: " << aZipFileName << std::endl;

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();
        std::cerr << "read: " << name << std::endl;

        try
        {
            if( name == wxS( "Manifest.dat" ) )
            {
                wxMemoryOutputStream memos;
                memos << zip;
                wxStreamBuffer* buf = memos.GetOutputStreamBuffer();

                wxString str =
                        wxString::FromUTF8( (char*) buf->GetBufferStart(), buf->GetBufferSize() );

                std::cerr << "read: " << str << std::endl;
// TODO
            }
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error reading '%s': %s" ), name, e.what() ) );
        }
        catch( std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error reading '%s': %s" ), name, e.what() ) );
        }
    }

    THROW_IO_ERROR( wxString::Format(
            _( "'%s' does not appear to be a valid Autodesk Fusion "
               "project or schematic file. Cannot find project.json or device.json." ),
            aZipFileName ) );
}

SCH_SHEET* SCH_IO_FUSION::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                              SCH_SHEET*             aAppendToMe,
                              const STRING_UTF8_MAP* aProperties ) {
    wxASSERT( !aFileName || aSchematic != nullptr );
    LOCALE_IO toggle; // toggles on, then off, the C locale.

    m_filename = aFileName;
    m_schematic = aSchematic;

    std::cerr << "start loading!!" << aFileName << " - " << m_filename.GetExt() << std::endl;

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( ( "Open canceled by user." ) );
    }

    if( m_filename.GetExt() == "f3z" )
    {
        ReadProjectFile( m_filename.GetFullPath() );
    }
    if( m_filename.GetExt() == "fsch" )
    {
        ReadSchematicFile( m_filename.GetFullPath() );
    }

    // TODO: lead ZIP file: m_filename.GetFullPath()

    // Load the document
    /*wxXmlDocument xmlDocument = loadXmlDocument( m_filename.GetFullPath() );

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( static_cast<int>( GetNodeCount( currentNode ) ) );
*/
    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    std::unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet );

    wxFileName newFilename( m_filename );
    newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxT( "Can't append to a schematic with no root!" ) );

        m_rootSheet = &aSchematic->Root();
    }
    else
    {
        m_rootSheet = new SCH_SHEET( aSchematic );
        m_rootSheet->SetFileName( newFilename.GetFullPath() );
        aSchematic->SetRoot( m_rootSheet );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
        screen->SetFileName( newFilename.GetFullPath() );
        m_rootSheet->SetScreen( screen );

        // Virtual root sheet UUID must be the same as the schematic file UUID.
        const_cast<KIID&>( m_rootSheet->m_Uuid ) = screen->GetUuid();
    }

    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );

    wxCHECK_MSG( libTable, nullptr, wxT( "Could not load symbol lib table." ) );

    /*
    m_pi.reset( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    m_properties = std::make_unique<STRING_UTF8_MAP>();
    ( *m_properties )[SCH_IO_KICAD_LEGACY::PropBuffering] = "";

    /// @note No check is being done here to see if the existing symbol library exists so this
    ///       will overwrite the existing one.
    if( !libTable->HasLibrary( getLibName() ) )
    {
        // Create a new empty symbol library.
        m_pi->CreateLibrary( getLibFileName().GetFullPath() );
        wxString libTableUri = wxT( "${KIPRJMOD}/" ) + getLibFileName().GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( getLibName(), libTableUri,
                                                       wxT( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( m_schematic->Prj().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Reload the symbol library table.
        m_schematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, nullptr );
        PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );
    }

    m_eagleDoc = std::make_unique<EAGLE_DOC>( currentNode, this );

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = ( m_eagleDoc->version.IsEmpty() ) ? wxString( wxS( "0.0" ) ) : m_eagleDoc->version;

    // Load drawing
    loadDrawing( m_eagleDoc->drawing );

    m_pi->SaveLibrary( getLibFileName().GetFullPath() );

    SCH_SCREENS allSheets( m_rootSheet );
    allSheets.UpdateSymbolLinks(); // Update all symbol library links for all sheets.*/

    return m_rootSheet;
}