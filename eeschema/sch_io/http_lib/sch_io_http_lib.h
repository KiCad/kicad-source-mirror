/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
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

#pragma once

#include "http_lib/http_lib_settings.h"
#include <http_lib/http_lib_connection.h>

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <wildcards_and_files_ext.h>

class LIBRARY_MANAGER_ADAPTER;

/**
 * A KiCad HTTP library provides both symbol and footprint metadata, so there are "shim" plugins
 * on both the symbol and footprint side of things that expose the database contents to the
 * schematic and board editors.  The architecture of these is slightly different from the other
 * plugins because the backing file is just a configuration file rather than something that
 * contains symbol or footprint data.
 */
class SCH_IO_HTTP_LIB : public SCH_IO
{
public:
    SCH_IO_HTTP_LIB();
    ~SCH_IO_HTTP_LIB() override = default;

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad HTTP library files" ), { FILEEXT::HTTPLibraryFileExtension } );
    }

    int GetModifyHash() const override { return 0; }

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool SupportsSubLibraries() const override { return true; }

    void GetSubLibraryNames( std::vector<wxString>& aNames ) override;

    wxString GetSubLibraryDescription( const wxString& aName ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;

    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    void SetLibraryManagerAdapter( SYMBOL_LIBRARY_ADAPTER* aAdapter ) override
    {
        m_adapter = aAdapter;
    }

    HTTP_LIB_SETTINGS* Settings() const { return m_settings.get(); }

    void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                     const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    const wxString& GetError() const override { return m_lastError; }

private:
    void ensureSettings( const wxString& aSettingsPath );

    void ensureConnection();

    void connect();

    void syncCache();

    void syncCache( const HTTP_LIB_CATEGORY& category );

    LIB_SYMBOL* loadSymbolFromPart( const wxString& aSymbolName, const HTTP_LIB_CATEGORY& aCategory,
                                    const HTTP_LIB_PART& aPart );

private:
    SYMBOL_LIBRARY_ADAPTER*              m_adapter;

    /// Generally will be null if no valid connection is established
    std::unique_ptr<HTTP_LIB_CONNECTION> m_conn;
    std::unique_ptr<HTTP_LIB_SETTINGS>   m_settings;
    std::set<wxString>                   m_customFields;
    std::set<wxString>                   m_defaultShownFields;
    wxString                             m_lastError;

    wxString symbol_field = "symbol";
    wxString footprint_field = "footprint";
    wxString description_field = "description";
    wxString keywords_field = "keywords";
    wxString value_field = "value";
    wxString datasheet_field = "datasheet";
    wxString reference_field = "reference";

    //     category.id       category
    std::map<std::string, HTTP_LIB_CATEGORY> m_cachedCategories;
};
