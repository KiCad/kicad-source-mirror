/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SCH_DATABASE_PLUGIN_H
#define KICAD_SCH_DATABASE_PLUGIN_H

#include <database/database_connection.h>
#include <sch_io_mgr.h>
#include <wildcards_and_files_ext.h>


class DATABASE_LIB_SETTINGS;
struct DATABASE_LIB_TABLE;


/**
 * A KiCad database library provides both symbol and footprint metadata, so there are "shim" plugins
 * on both the symbol and footprint side of things that expose the database contents to the
 * schematic and board editors.  The architecture of these is slightly different from the other
 * plugins because the backing file is just a configuration file rather than something that
 * contains symbol or footprint data.
 */
class SCH_DATABASE_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_DATABASE_PLUGIN();
    virtual ~SCH_DATABASE_PLUGIN();

    const wxString GetName() const override
    {
        return wxT( "Database library" );
    }

    const wxString GetLibraryFileExtension() const override
    {
        return DatabaseLibraryFileExtension;
    }

    const wxString GetFileExtension() const override
    {
        wxFAIL_MSG( "Database libraries are not schematic files!  Fix call site." );
        return DatabaseLibraryFileExtension;
    }

    int GetModifyHash() const override { return 0; }

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const STRING_UTF8_MAP*         aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool SupportsSubLibraries() const override { return true; }

    void GetSubLibraryNames( std::vector<wxString>& aNames ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;

    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    bool CheckHeader( const wxString& aFileName ) override;

    // Database libraries can never be written using the symbol editing API
    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override
    {
        return false;
    }

    void SetLibTable( SYMBOL_LIB_TABLE* aTable ) override
    {
        m_libTable = aTable;
    }

    DATABASE_LIB_SETTINGS* Settings() const { return m_settings.get(); }

    bool TestConnection( wxString* aErrorMsg = nullptr );

private:
    void ensureSettings( const wxString& aSettingsPath );

    void ensureConnection();

    void connect();

    LIB_SYMBOL* loadSymbolFromRow( const wxString& aSymbolName,
                                   const DATABASE_LIB_TABLE& aTable,
                                   const DATABASE_CONNECTION::ROW& aRow );

    static std::optional<bool> boolFromAny( const std::any& aVal );

    SYMBOL_LIB_TABLE* m_libTable;

    std::unique_ptr<DATABASE_LIB_SETTINGS> m_settings;

    /// Generally will be null if no valid connection is established
    std::unique_ptr<DATABASE_CONNECTION> m_conn;

    std::set<wxString> m_customFields;

    std::set<wxString> m_defaultShownFields;

    wxString m_lastError;
};

#endif //KICAD_SCH_DATABASE_PLUGIN_H
