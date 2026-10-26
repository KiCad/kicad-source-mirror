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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_DATABASE_LIB_SETTINGS_H
#define KICAD_DATABASE_LIB_SETTINGS_H

#include <json_common.h>
#include <functional>
#include <settings/json_settings.h>
#include <wx/string.h>

#include <map>
#include <memory>


enum class DATABASE_SOURCE_TYPE
{
    ODBC,
    SQLITE,
    ACCESS,
    INVALID
};


struct KICOMMON_API DATABASE_SOURCE
{
    DATABASE_SOURCE_TYPE type = DATABASE_SOURCE_TYPE::ODBC;
    std::string          dsn;
    std::string          username;
    std::string          password;
    int                  timeout = 0;
    std::string          connection_string;
    std::string          path;      ///< Database file of a SQLITE or ACCESS source
    std::string          type_name; ///< Raw source.type as written, kept for the error when the type is INVALID

    /// @return a copy with ${CWD} replaced by aBaseDir in the connection string and the path
    DATABASE_SOURCE Expanded( const wxString& aBaseDir ) const;

    /**
     * @return the database file of a SQLITE or ACCESS source, the DSN of an ODBC one, or for a
     *         connection string its driver and database file or name. Never the password, so it is
     *         safe for error messages and traces. Empty when there is nothing safe to show, in which
     *         case callers supply their own wording.
     */
    std::string Describe() const;
};


struct KICOMMON_API DATABASE_FIELD_MAPPING
{
    std::string column;             ///< Database column name
    std::string name;               ///< KiCad field name
    wxString    name_wx;            ///< KiCad field name (converted)
    bool        visible_on_add;     ///< Whether to show the field when placing the symbol
    bool        visible_in_chooser; ///< Whether the column is shown by default in the chooser
    bool        show_name;   ///< Whether or not to show the field name as well as its value
    bool        inherit_properties; ///< Whether or not to inherit properties from symbol field
    nlohmann::json source;          ///< Raw JSON as loaded; untouched keys are written back verbatim

    DATABASE_FIELD_MAPPING();

    explicit DATABASE_FIELD_MAPPING( const std::string& aColumn, const std::string& aName,
                                     bool aVisibleOnAdd, bool aVisibleInChooser, bool aShowName,
                                     bool aInheritProperties );

    void SetName( const std::string& aName );
};


struct KICOMMON_API MAPPABLE_SYMBOL_PROPERTIES
{
    std::string description;
    std::string footprint_filters;
    std::string keywords;
    std::string exclude_from_sim;
    std::string exclude_from_bom;
    std::string exclude_from_board;

    bool operator==( const MAPPABLE_SYMBOL_PROPERTIES& aOther ) const = default;
};


/**
 * A database library table will be mapped to a sub-library provided by the database library entry
 * in the KiCad symbol/footprint library table.  A single database library config file (managed by
 * this class) may contain more than one table mapping, and each will have its own nickname.
 *
 * The LIB_ID for parts placed from this library will be constructed from the nickname of the
 * database library itself, plus the nickname of the particular sub-library and the value of the
 * key column for the placed part.
 *
 * For example, if a database library is configured with the nickname "PartsDB" and it provides a
 * table called "Capacitors", with `key_col` set to "Part Number", the LIB_ID for a capacitor placed
 * from this table will look something like `PartsDB-Capacitors:CAP-001`
 */
struct KICOMMON_API DATABASE_LIB_TABLE
{
    std::string name;              ///< KiCad library nickname (will form part of the LIB_ID)
    std::string table;             ///< Database table to pull content from
    std::string key_col;           ///< Unique key column name (will form part of the LIB_ID)
    std::string symbols_col;       ///< Column name containing KiCad symbol refs
    std::string footprints_col;    ///< Column name containing KiCad footprint refs
    std::string pins_col;          ///< Column name containing JSON pin assignments (optional)

    MAPPABLE_SYMBOL_PROPERTIES properties;
    std::vector<DATABASE_FIELD_MAPPING> fields;

    nlohmann::json source;         ///< Raw JSON as loaded; untouched keys are written back verbatim
};


struct KICOMMON_API DATABASE_CACHE_SETTINGS
{
    int max_size;    ///< Maximum number of tables to cache, each holding the rows read from it
    int max_age;     ///< Max age of a cached table before it expires, in seconds
};


/**
 * Keyring lookup signature, matching KIPLATFORM::SECRETS::GetSecretNoPrompt.
 */
using DATABASE_SECRET_LOOKUP = std::function<bool( const wxString& aService, const wxString& aKey, wxString& aSecret )>;

class KICOMMON_API DATABASE_LIB_SETTINGS : public JSON_SETTINGS
{
public:
    DATABASE_LIB_SETTINGS( const std::string& aFilename );

    virtual ~DATABASE_LIB_SETTINGS() {}

    DATABASE_SOURCE m_Source{};

    std::vector<DATABASE_LIB_TABLE> m_Tables;

    DATABASE_CACHE_SETTINGS m_Cache{};

    bool m_GloballyUniqueKeys = false;

    std::string m_Name;
    std::string m_Description;

    /// Records the scalars as loaded; runs before a schema migration writes the file back
    void Load() override;

    /**
     * Stores parameters, then restores the raw file value of every scalar the caller left unchanged
     * so that hand-edited or newer-format values survive a save.
     */
    bool Store() override;

    /**
     * Fills in the source password from the keyring when the file has none. The password is
     * treated as loaded, so Store() never writes it back to the file.
     */
    void ResolvePassword( const DATABASE_SECRET_LOOKUP& aLookup = nullptr );

    /**
     * @param aBaseDir replaces the settings file's directory for ${CWD}; used while the file has no path yet
     * @return a copy of the source with ${CWD} expanded, exactly as the driver receives it
     */
    DATABASE_SOURCE ExpandedSource( const wxString& aBaseDir = wxEmptyString ) const;

    /// @return the file LoadFromFile() reads, which is the settings name with the library extension forced
    wxString GetLoadPath() const;

protected:
    wxString getFileExt() const override;

private:
    static nlohmann::json tableToJson( const DATABASE_LIB_TABLE& aTable );
    /// Missing or mistyped keys load as defaults so one incomplete entry cannot stop the others
    static DATABASE_LIB_TABLE tableFromJson( const nlohmann::json& aEntry );
    static nlohmann::json fieldToJson( const DATABASE_FIELD_MAPPING& aField );
    static DATABASE_FIELD_MAPPING fieldFromJson( const nlohmann::json& aEntry );

    /// Scalar settings as parsed at load, so Store() can tell which ones the caller changed
    struct LOADED_SCALARS
    {
        std::string             name;
        std::string             description;
        DATABASE_SOURCE         source{};
        DATABASE_CACHE_SETTINGS cache{};
        bool                    globallyUniqueKeys = false;
    } m_loaded;
};


/**
 * Loads a database library settings file without allowing the load itself (e.g. a schema
 * migration) to write back to disk.  Callers must SetReadOnly( false ) around an explicit save.
 *
 * @param aLoaded is set to false when the file exists but could not be read; saving such
 *                settings would replace the file with defaults
 */
KICOMMON_API std::unique_ptr<DATABASE_LIB_SETTINGS> LoadDatabaseLibSettingsReadOnly( const wxString& aPath,
                                                                                     bool* aLoaded = nullptr );

/**
 * @return a file name without extension for a new settings file, derived from the library name the
 *         user typed, or a generic name when that leaves nothing usable
 */
KICOMMON_API wxString DefaultDatabaseLibFileName( const wxString& aLibName );

/**
 * @return aPath as ${CWD}/<relative> when it lies in aBaseDir or below it, so a library moves with its
 *         database file; any other path is returned unchanged
 */
KICOMMON_API std::string DatabasePathForStorage( const wxString& aPath, const wxString& aBaseDir );

/**
 * @return the path a user typed or pasted as an absolute path, with ${CWD} expanded and a relative path
 *         taken from aBaseDir; never touches the file system, so a path on an unreachable share costs nothing
 */
KICOMMON_API wxString DatabasePathFromInput( const wxString& aInput, const wxString& aBaseDir );

/// The source timeout a library file gets when it sets none
inline constexpr int DEFAULT_DATABASE_TIMEOUT_SECONDS = 2;

/**
 * @return the seconds a caller waits on a database source with the given timeout before giving up, which is
 *         the timeout plus 10 s for the worker, or 30 s when the timeout is 0
 */
inline int DatabaseFileWaitSeconds( int aTimeoutSeconds )
{
    return aTimeoutSeconds > 0 ? aTimeoutSeconds + 10 : 30;
}

/**
 * The lookup used when none is injected, which must never prompt to unlock the keyring.
 */
KICOMMON_API DATABASE_SECRET_LOOKUP DefaultDatabaseSecretLookup();

/**
 * @return true when aLookup is the system keyring lookup that never prompts to unlock it.
 */
KICOMMON_API bool IsNoPromptSecretLookup( const DATABASE_SECRET_LOOKUP& aLookup );

/**
 * DSN passwords that are not saved in the (often shared) library file live in the system keyring,
 * keyed by DSN and user name so every library on the same login shares one entry.
 *
 * @param aLookup replaces the system keyring when set, for tests.
 * @return false when the source has no DSN login or the keyring holds no password for it
 */
KICOMMON_API bool GetDatabasePassword( const DATABASE_SOURCE& aSource, std::string& aPassword,
                                       const DATABASE_SECRET_LOOKUP& aLookup = nullptr );

/**
 * Fills in the source password from the keyring, but only when the library file did not supply one.
 */
KICOMMON_API void ResolveDatabasePassword( DATABASE_SOURCE& aSource,
                                           const DATABASE_SECRET_LOOKUP& aLookup = nullptr );

/**
 * Stores the password for the source's DSN login in the system keyring, or removes the entry when
 * the password is empty.
 */
KICOMMON_API bool StoreDatabasePassword( const DATABASE_SOURCE& aSource, const std::string& aPassword );

#endif //KICAD_DATABASE_LIB_SETTINGS_H
