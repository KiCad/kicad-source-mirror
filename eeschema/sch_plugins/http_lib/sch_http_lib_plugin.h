/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
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


#ifndef KICAD_SCH_HTTP_LIB_PLUGIN_H
#define KICAD_SCH_HTTP_LIB_PLUGIN_H

#include "http_lib/http_lib_settings.h"
#include <http_lib/http_lib_connection.h>

#include <sch_io_mgr.h>
#include <wildcards_and_files_ext.h>

/**
 * A KiCad HTTP library provides both symbol and footprint metadata, so there are "shim" plugins
 * on both the symbol and footprint side of things that expose the database contents to the
 * schematic and board editors.  The architecture of these is slightly different from the other
 * plugins because the backing file is just a configuration file rather than something that
 * contains symbol or footprint data.
 */
class SCH_HTTP_LIB_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_HTTP_LIB_PLUGIN();
    virtual ~SCH_HTTP_LIB_PLUGIN();

    const wxString GetName() const override
    {
        return wxT( "HTTP library" );
    }

    const PLUGIN_FILE_DESC GetLibraryFileDesc() const override
    {
        return PLUGIN_FILE_DESC( _HKI( "KiCad HTTP library files" ),
            { HTTPLibraryFileExtension } );
    }

    int GetModifyHash() const override { return 0; }

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList,
                             const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString& aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool SupportsSubLibraries() const override { return true; }

    void GetSubLibraryNames( std::vector<wxString>& aNames ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;

    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override
    {
        // TODO: HTTP libraries are well capabale of supporting this.
        return false;
    }

    void SetLibTable( SYMBOL_LIB_TABLE* aTable ) override
    {
        m_libTable = aTable;
    }


    HTTP_LIB_SETTINGS* Settings() const { return m_settings.get(); }

    void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                     const STRING_UTF8_MAP* aProperties = nullptr ) override;

private:

    void ensureSettings( const wxString& aSettingsPath );

    void ensureConnection();

    void connect();

    LIB_SYMBOL* loadSymbolFromPart( const wxString& aSymbolName,
                                    const HTTP_LIB_CATEGORY& aCategory,
                                    const HTTP_LIB_PART& aPart );

    SYMBOL_LIB_TABLE* m_libTable;

    std::map<std::string, std::vector<HTTP_LIB_PART>> m_cachedParts;

     /// Generally will be null if no valid connection is established
    std::unique_ptr<HTTP_LIB_CONNECTION> m_conn;

    std::unique_ptr<HTTP_LIB_SETTINGS> m_settings;

    std::set<wxString> m_customFields;

    std::set<wxString> m_defaultShownFields;

    wxString m_lastError;

    std::string symbol_field = "symbol";
    std::string footprint_field = "footprint";
    std::string description_field = "description";
    std::string keywords_field = "keywords";
    std::string value_field = "value";
    std::string datasheet_field = "datasheet";
    std::string reference_field = "reference";

};

#endif //KICAD_SCH_HTTP_LIB_PLUGIN_H
