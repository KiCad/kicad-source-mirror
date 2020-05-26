/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Jon Evans <jon@craftyjon.com>
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

#include <config_params.h>
#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <settings/project_file.h>
#include <wildcards_and_files_ext.h>
#include <wx/config.h>
#include <wx/log.h>

extern const char* traceSettings;

///! Update the schema version whenever a migration is required
const int projectFileSchemaVersion = 1;


PROJECT_FILE::PROJECT_FILE( const std::string& aFullPath ) :
        JSON_SETTINGS( aFullPath, SETTINGS_LOC::NONE, projectFileSchemaVersion ),
        m_sheets(), m_boards()
{
    // Keep old files around
    m_deleteLegacyAfterMigration = false;

    m_params.emplace_back( new PARAM_LIST<FILE_INFO_PAIR>( "sheets", &m_sheets, {} ) );

    m_params.emplace_back( new PARAM_LIST<FILE_INFO_PAIR>( "boards", &m_boards, {} ) );

    m_params.emplace_back(
            new PARAM_LIST<wxString>( "libraries.pinned_symbol_libs", &m_PinnedSymbolLibs, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>(
            "libraries.pinned_footprint_libs", &m_PinnedFootprintLibs, {} ) );

    m_params.emplace_back(
            new PARAM_PATH_LIST( "cvpcb.equivalence_files", &m_EquivalenceFiles, {} ) );
}


bool PROJECT_FILE::MigrateFromLegacy( wxConfigBase* aLegacyFile )
{
    bool     ret = true;
    wxString str;
    long     index = 0;

    std::set<wxString> group_blacklist;

    // Legacy files don't store board info; they assume board matches project name
    // We will leave m_boards empty here so it can be populated with other code

    // First handle migration of data that will be stored locally in this object

    auto loadPinnedLibs =
            [&]( const std::string& aDest )
            {
                int      libIndex = 1;
                wxString libKey   = wxT( "PinnedItems" );
                libKey << libIndex;

                nlohmann::json libs = nlohmann::json::array();

                while( aLegacyFile->Read( libKey, &str ) )
                {
                    libs.push_back( str );

                    aLegacyFile->DeleteEntry( libKey, true );

                    libKey = wxT( "PinnedItems" );
                    libKey << ++libIndex;
                }

                ( *this )[PointerFromString( aDest )] = libs;
            };

    aLegacyFile->SetPath( wxT( "/LibeditFrame" ) );
    loadPinnedLibs( "libraries.pinned_symbol_libs" );

    aLegacyFile->SetPath( wxT( "/ModEditFrame" ) );
    loadPinnedLibs( "libraries.pinned_footprint_libs" );

    aLegacyFile->SetPath( wxT( "/cvpcb/equfiles" ) );

    {
        int      eqIdx = 1;
        wxString eqKey = wxT( "EquName" );
        eqKey << eqIdx;

        nlohmann::json eqs = nlohmann::json::array();

        while( aLegacyFile->Read( eqKey, &str ) )
        {
            eqs.push_back( str );

            eqKey = wxT( "EquName" );
            eqKey << ++eqIdx;
        }

        ( *this )[PointerFromString( "cvpcb.equivalence_files" )] = eqs;
    }

    // No other cvpcb params are currently used
    group_blacklist.insert( "/cvpcb" );

    // Next load sheet names and put all other legacy data in the legacy dict
    aLegacyFile->SetPath( "/" );

    auto loadSheetNames =
            [&]() -> bool
            {
                int            sheet = 1;
                wxString       entry;
                nlohmann::json arr   = nlohmann::json::array();

                wxLogTrace( traceSettings, "Migrating sheet names" );

                aLegacyFile->SetPath( wxT( "/sheetnames" ) );

                while( aLegacyFile->Read( wxString::Format( "%d", sheet++ ), &entry ) )
                {
                    wxArrayString tokens = wxSplit( entry, ':' );

                    if( tokens.size() == 2 )
                    {
                        wxLogTrace( traceSettings, "%d: %s = %s", sheet, tokens[0], tokens[1] );
                        arr.push_back( nlohmann::json::array( { tokens[0], tokens[1] } ) );
                    }
                }

                ( *this )[PointerFromString( "sheets" )] = arr;

                aLegacyFile->SetPath( "/" );

                // TODO: any reason we want to fail on this?
                return true;
            };

    std::vector<wxString> groups;

    groups.emplace_back( "" );

    auto loadLegacyPairs =
            [&]( const std::string& aGroup ) -> bool
            {
                wxLogTrace( traceSettings, "Migrating group %s", aGroup );
                bool     success = true;
                wxString keyStr;
                wxString val;

                index = 0;

                while( aLegacyFile->GetNextEntry( keyStr, index ) )
                {
                    if( !aLegacyFile->Read( keyStr, &val ) )
                        continue;

                    std::string key( keyStr.ToUTF8() );

                    wxLogTrace( traceSettings, "    %s = %s", key, val );

                    try
                    {
                        nlohmann::json::json_pointer ptr( "/legacy" + aGroup + "/" + key );
                        ( *this )[ptr] = val;
                    }
                    catch( ... )
                    {
                        success = false;
                    }
                }

                return success;
            };

    for( size_t i = 0; i < groups.size(); i++ )
    {
        aLegacyFile->SetPath( groups[i] );

        if( groups[i] == wxT( "/sheetnames" ) )
        {
            ret |= loadSheetNames();
            continue;
        }

        aLegacyFile->DeleteEntry( wxT( "last_client" ), true );
        aLegacyFile->DeleteEntry( wxT( "update" ), true );
        aLegacyFile->DeleteEntry( wxT( "version" ), true );

        ret &= loadLegacyPairs( groups[i].ToStdString() );

        index = 0;

        while( aLegacyFile->GetNextGroup( str, index ) )
        {
            wxString group = groups[i] + "/" + str;

            if( !group_blacklist.count( group ) )
                groups.emplace_back( group );
        }

        aLegacyFile->SetPath( "/" );
    }

    return ret;
}


wxString PROJECT_FILE::getFileExt() const
{
    return ProjectFileExtension;
}


wxString PROJECT_FILE::getLegacyFileExt() const
{
    return LegacyProjectFileExtension;
}


void to_json( nlohmann::json& aJson, const FILE_INFO_PAIR& aPair )
{
    aJson = nlohmann::json::array( { aPair.first.AsString().ToUTF8(), aPair.second.ToUTF8() } );
}


void from_json( const nlohmann::json& aJson, FILE_INFO_PAIR& aPair )
{
    wxASSERT( aJson.is_array() && aJson.size() == 2 );
    aPair.first  = KIID( wxString( aJson[0].get<std::string>().c_str(), wxConvUTF8 ) );
    aPair.second = wxString( aJson[1].get<std::string>().c_str(), wxConvUTF8 );
}