/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include "remote_symbol_import_utils.h"

#include <common.h>
#include <eeschema_settings.h>
#include <io/io_mgr.h>
#include <lib_symbol.h>
#include <libraries/library_manager.h>
#include <pgm_base.h>
#include <remote_provider_settings.h>
#include <sch_edit_frame.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/intl.h>


wxString SanitizeRemoteFileComponent( const wxString& aValue, const wxString& aDefault, bool aLower )
{
    wxString result = aValue;
    result.Trim( true ).Trim( false );

    if( result.IsEmpty() )
        result = aDefault;

    for( size_t i = 0; i < result.length(); ++i )
    {
        const wxUniChar ch = result[i];

        if( !( wxIsalnum( ch ) || ch == '_' || ch == '-' || ch == '.' ) )
            result[i] = '_';
    }

    return aLower ? result.Lower() : result;
}


wxString RemoteLibraryPrefix()
{
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    wxString prefix = settings ? settings->m_RemoteSymbol.library_prefix : wxString();

    if( prefix.IsEmpty() )
        prefix = REMOTE_PROVIDER_SETTINGS::DefaultLibraryPrefix();

    return SanitizeRemoteFileComponent( prefix, wxS( "remote" ), true );
}


bool WriteRemoteBinaryFile( const wxFileName& aOutput, const std::vector<uint8_t>& aPayload,
                            wxString& aError )
{
    if( aPayload.empty() )
    {
        aError = _( "Payload was empty." );
        return false;
    }

    wxFileName targetDir = aOutput;
    targetDir.SetFullName( wxEmptyString );

    if( !targetDir.DirExists() && !targetDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), targetDir.GetFullPath() );
        return false;
    }

    wxFFile file( aOutput.GetFullPath(), wxS( "wb" ) );

    if( !file.IsOpened() )
    {
        aError = wxString::Format( _( "Unable to open '%s' for writing." ), aOutput.GetFullPath() );
        return false;
    }

    if( file.Write( aPayload.data(), aPayload.size() ) != aPayload.size() )
    {
        aError = wxString::Format( _( "Failed to write '%s'." ), aOutput.GetFullPath() );
        return false;
    }

    file.Close();
    return true;
}


bool EnsureRemoteDestinationRoot( wxFileName& aOutDir, wxString& aError )
{
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    wxString destination = settings->m_RemoteSymbol.destination_dir;

    if( destination.IsEmpty() )
        destination = REMOTE_PROVIDER_SETTINGS::DefaultDestinationDir();

    destination = ExpandEnvVarSubstitutions( destination, &Pgm().GetSettingsManager().Prj() );
    destination.Trim( true ).Trim( false );

    if( destination.IsEmpty() )
    {
        aError = _( "Destination directory is not configured." );
        return false;
    }

    wxFileName dir = wxFileName::DirName( destination );
    dir.Normalize( FN_NORMALIZE_FLAGS );

    if( !dir.DirExists() && !dir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create directory '%s'." ), dir.GetFullPath() );
        return false;
    }

    aOutDir = dir;
    return true;
}


bool EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE aTableType, const wxFileName& aLibraryPath,
                                const wxString& aNickname, bool aGlobalTable, bool aStrict,
                                wxString& aError )
{
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> tableOpt = manager.Table(
            aTableType,
            aGlobalTable ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT );

    if( !tableOpt )
    {
        if( aStrict )
            aError = _( "Unable to access the library table." );

        return !aStrict;
    }

    LIBRARY_TABLE* table = *tableOpt;
    const wxString fullPath = aLibraryPath.GetFullPath();

    if( table->HasRow( aNickname ) )
    {
        if( std::optional<LIBRARY_TABLE_ROW*> rowOpt = table->Row( aNickname ); rowOpt )
        {
            LIBRARY_TABLE_ROW* row = *rowOpt;

            if( row->URI() != fullPath )
            {
                row->SetURI( fullPath );

                if( !table->Save() )
                {
                    aError = _( "Failed to update the library table." );
                    return false;
                }
            }
        }

        return true;
    }

    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( aNickname );
    row.SetURI( fullPath );
    row.SetType( wxS( "KiCad" ) );
    row.SetOptions( wxString() );
    row.SetDescription( _( "Remote download" ) );
    row.SetOk( true );

    if( !table->Save() )
    {
        aError = _( "Failed to save the library table." );
        return false;
    }

    return true;
}


bool PlaceRemoteDownloadedSymbol( SCH_EDIT_FRAME* aFrame, const wxString& aNickname,
                                  const wxString& aLibItemName, wxString& aError )
{
    if( !aFrame )
    {
        aError = _( "No schematic editor is available for placement." );
        return false;
    }

    LIB_ID libId;
    libId.SetLibNickname( aNickname );
    libId.SetLibItemName( aLibItemName );

    LIB_SYMBOL* libSymbol = aFrame->GetLibSymbol( libId );

    if( !libSymbol )
    {
        aError = _( "Unable to load the downloaded symbol for placement." );
        return false;
    }

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &aFrame->GetCurrentSheet(), 1 );
    symbol->SetParent( aFrame->GetScreen() );

    if( EESCHEMA_SETTINGS* cfg = aFrame->eeconfig(); cfg && cfg->m_AutoplaceFields.enable )
        symbol->AutoplaceFields( nullptr, AUTOPLACE_AUTO );

    TOOL_MANAGER* toolMgr = aFrame->GetToolManager();

    if( !toolMgr )
    {
        delete symbol;
        aError = _( "Unable to access the schematic placement tools." );
        return false;
    }

    aFrame->Raise();
    toolMgr->PostAction( SCH_ACTIONS::placeSymbol,
                         SCH_ACTIONS::PLACE_SYMBOL_PARAMS{ symbol, true } );
    return true;
}


std::unique_ptr<LIB_SYMBOL> LoadRemoteSymbolFromPayload( const std::vector<uint8_t>& aPayload,
                                                        const wxString& aLibItemName,
                                                        wxString& aError )
{
    if( aPayload.empty() )
    {
        aError = _( "Symbol payload was empty." );
        return nullptr;
    }

    wxString tempPath = wxFileName::CreateTempFileName( wxS( "remote_symbol" ) );

    if( tempPath.IsEmpty() )
    {
        aError = _( "Unable to create a temporary file for the symbol payload." );
        return nullptr;
    }

    wxFileName tempFile( tempPath );
    wxFFile    file( tempFile.GetFullPath(), wxS( "wb" ) );

    if( !file.IsOpened() )
    {
        aError = _( "Unable to create a temporary file for the symbol payload." );
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    if( file.Write( aPayload.data(), aPayload.size() ) != aPayload.size() )
    {
        aError = _( "Failed to write the temporary symbol payload." );
        file.Close();
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    file.Close();

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !plugin )
    {
        aError = _( "Unable to access the KiCad symbol plugin." );
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    std::unique_ptr<LIB_SYMBOL> symbol;

    try
    {
        LIB_SYMBOL* loaded = plugin->LoadSymbol( tempFile.GetFullPath(), aLibItemName );

        if( loaded )
            symbol = std::make_unique<LIB_SYMBOL>( *loaded );
        else
            aError = _( "Symbol payload did not include the expected symbol." );
    }
    catch( const IO_ERROR& e )
    {
        aError = wxString::Format( _( "Unable to decode the symbol payload: %s" ), e.What() );
    }

    wxRemoveFile( tempFile.GetFullPath() );
    return symbol;
}


LIB_ID BuildRemoteLibId( const wxString& aResolvedLibrary, const wxString& aResolvedItemName )
{
    const wxString nickname =
            RemoteLibraryPrefix() + wxS( "_" )
            + SanitizeRemoteFileComponent( aResolvedLibrary, wxS( "footprints" ), true );

    const wxString itemName = SanitizeRemoteFileComponent( aResolvedItemName, wxS( "footprint" ) );

    LIB_ID id;
    id.SetLibNickname( nickname );
    id.SetLibItemName( itemName );
    return id;
}


void ApplyFootprintLinks( LIB_SYMBOL& aSymbol, const std::vector<LIB_ID>& aLinks )
{
    if( aLinks.empty() )
        return;

    aSymbol.SetFootprintProp( aLinks.front().GetUniStringLibId() );

    if( aLinks.size() == 1 )
        return;

    wxArrayString filters = aSymbol.GetFPFilters();

    for( size_t i = 1; i < aLinks.size(); ++i )
        filters.Add( aLinks[i].GetUniStringLibItemName() );

    aSymbol.SetFPFilters( filters );
}
