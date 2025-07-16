/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
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

#ifndef SCH_IO_DATABASE_H_
#define SCH_IO_DATABASE_H_

#include <database/database_connection.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <wildcards_and_files_ext.h>
#include <optional>


class LIBRARY_MANAGER_ADAPTER;
class DATABASE_LIB_SETTINGS;
struct DATABASE_LIB_TABLE;


/**
 * A KiCad database library provides both symbol and footprint metadata, so there are "shim" plugins
 * on both the symbol and footprint side of things that expose the database contents to the
 * schematic and board editors.  The architecture of these is slightly different from the other
 * plugins because the backing file is just a configuration file rather than something that
 * contains symbol or footprint data.
 */
class SCH_IO_DATABASE : public SCH_IO
{
public:

    SCH_IO_DATABASE();
    virtual ~SCH_IO_DATABASE();

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad database library files" ),
                                      { FILEEXT::DatabaseLibraryFileExtension } );
    }

    int GetModifyHash() const override { return 0; }

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const std::map<std::string, UTF8>*         aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool SupportsSubLibraries() const override { return true; }

    void GetSubLibraryNames( std::vector<wxString>& aNames ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;

    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    // Database libraries can never be written using the symbol editing API
    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    void SetLibraryManagerAdapter( SYMBOL_LIBRARY_ADAPTER* aAdapter ) override
    {
        m_adapter = aAdapter;
    }

    bool SupportsConfigurationDialog() const override { return true; }

    DIALOG_SHIM* CreateConfigurationDialog( wxWindow* aParent ) override;

    DATABASE_LIB_SETTINGS* Settings() const { return m_settings.get(); }

    bool TestConnection( wxString* aErrorMsg = nullptr );

private:
    void cacheLib();

    void ensureSettings( const wxString& aSettingsPath );

    void ensureConnection();

    void connect();

    std::unique_ptr<LIB_SYMBOL> loadSymbolFromRow( const wxString& aSymbolName,
                                                   const DATABASE_LIB_TABLE& aTable,
                                                   const DATABASE_CONNECTION::ROW& aRow );

    static std::optional<bool> boolFromAny( const std::any& aVal );

    SYMBOL_LIBRARY_ADAPTER* m_adapter;

    std::unique_ptr<DATABASE_LIB_SETTINGS> m_settings;

    /// Generally will be null if no valid connection is established
    std::unique_ptr<DATABASE_CONNECTION> m_conn;

    std::set<wxString> m_customFields;

    std::set<wxString> m_defaultShownFields;

    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_nameToSymbolcache;
    std::map<wxString, std::pair<std::string, std::string>> m_sanitizedNameMap;

    long long m_cacheTimestamp;

    int m_cacheModifyHash;



    wxString m_lastError;
};

#endif //SCH_IO_DATABASE_H_
