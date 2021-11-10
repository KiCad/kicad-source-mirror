/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _JSON_SETTINGS_H
#define _JSON_SETTINGS_H

#include <core/wx_stl_compat.h>

#include <utility>
#include <wx/string.h>

#include <core/optional.h>
#include <nlohmann/json_fwd.hpp>

class wxConfigBase;
class NESTED_SETTINGS;
class PARAM_BASE;
class SETTINGS_MANAGER;

/**
 * Flag to enable debug output of settings operations and management.
 *
 * Use "KICAD_SETTINGS" to enable.
 *
 * @ingroup trace_env_vars
 */
extern const wxChar* const traceSettings;

enum class SETTINGS_LOC {
    USER,       ///< The main config directory (e.g. ~/.config/kicad/)
    PROJECT,    ///< The settings directory inside a project folder
    COLORS,     ///< The color scheme directory (e.g. ~/.config/kicad/colors/)
    NESTED,     ///< Not stored in a file, but inside another JSON_SETTINGS
    NONE,       ///< No directory prepended, full path in filename (used for PROJECT_FILE)
};


/// pimpl to allow hiding json.hpp
class JSON_SETTINGS_INTERNALS;

class JSON_SETTINGS
{
public:
    friend class NESTED_SETTINGS;

    JSON_SETTINGS( const wxString& aFilename, SETTINGS_LOC aLocation, int aSchemaVersion ) :
            JSON_SETTINGS( aFilename, aLocation, aSchemaVersion, true, true, true )
    {}

    JSON_SETTINGS( const wxString& aFilename, SETTINGS_LOC aLocation, int aSchemaVersion,
                   bool aCreateIfMissing, bool aCreateIfDefault, bool aWriteFile );

    virtual ~JSON_SETTINGS();

    wxString GetFilename() const { return m_filename; }

    wxString GetFullFilename() const;

    void SetFilename( const wxString& aFilename ) { m_filename = aFilename; }

    void SetLocation( SETTINGS_LOC aLocation ) { m_location = aLocation; }
    SETTINGS_LOC GetLocation() const { return m_location; }

    void SetLegacyFilename( const wxString& aFilename ) { m_legacy_filename = aFilename; }

    bool IsReadOnly() const { return !m_writeFile; }
    void SetReadOnly( bool aReadOnly ) { m_writeFile = !aReadOnly; }

    /**
     * Wrappers for the underlying JSON API so that most consumers don't need json.hpp
     * All of these functions take a string that is passed to PointerFromString internally.
     */

    nlohmann::json& At( const std::string& aPath );
    bool Contains( const std::string& aPath ) const;
    size_t Count( const std::string& aPath ) const;

    JSON_SETTINGS_INTERNALS* Internals();

    /**
     * Updates the parameters of this object based on the current JSON document contents
     */
    virtual void Load();

    /**
     * Stores the current parameters into the JSON document represented by this object
     * Note: this doesn't do any writing to disk; that's handled by SETTINGS_MANAGER
     * @return true if any part of the JSON document was updated
     */
    virtual bool Store();

    /**
     * Loads the backing file from disk and then calls Load()
     * @param aDirectory is the path to the file
     * @return true if the file was found on disk and loaded or migrated
     */
    virtual bool LoadFromFile( const wxString& aDirectory = "" );

    /**
     * Calls Store() and then writes the contents of the JSON document to a file
     * @param aDirectory is the directory to save to, including trailing separator
c     * @return true if the file was saved
     */
    virtual bool SaveToFile( const wxString& aDirectory = "", bool aForce = false );

    /**
     * Resets all parameters to default values.  Does NOT write to file or update underlying JSON.
     */
    void ResetToDefaults();

    /**
     * Fetches a JSON object that is a subset of this JSON_SETTINGS object, using a path of the
     * form "key1.key2.key3" to refer to nested objects.
     * @param aPath is a string containing one or more keys separated by '.'
     * @return a JSON object from within this one
     */
    OPT<nlohmann::json> GetJson( const std::string& aPath ) const;

    /**
     * Fetches a value from within the JSON document.
     * Will return an empty optional if the value is not found or a mismatching type.
     * @tparam ValueType is the type to cast to
     * @param aPath is the path within the document to retrieve
     * @return a value from within this document
     */
    template<typename ValueType>
    OPT<ValueType> Get( const std::string& aPath ) const;

    /**
     * Stores a value into the JSON document
     * Will throw an exception if ValueType isn't something that the library can handle
     * @tparam ValueType is the type to store
     * @param aPath is a path to store in the form "key1.key2.key3"
     * @param aVal is the value to store
     */
    template<typename ValueType>
    void Set( const std::string& aPath, ValueType aVal );

    /**
     * Migrates the schema of this settings from the version in the file to the latest version
     *
     * Schema migration doesn't need to be used every time a setting is added!  This is intended
     * to be more of an "escape hatch" in the event that we decide to move settings around or make
     * other changes to a settings file format that can't simply be handled by loading a new default
     *
     * @return true if migration was successful
     */
    bool Migrate();

    /**
     * Migrates from wxConfig to JSON-based configuration.  Should be implemented by any subclasses
     * of JSON_SETTINGS that map to a legacy (wxConfig-based) config file.
     * @param aLegacyConfig is a wxConfigBase holding a loaded configuration to migrate
     * @return true if migration was successful
     */
    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig );

    /**
     * Transfers ownership of a given NESTED_SETTINGS to this object.
     * Can be used to construct a NESTED_SETTINGS without the parent object needing to know about
     * the implementation of the nested object;
     *
     * @param aSettings is the settings object to take ownership of
     * @param aTarget is a pointer to update to the passed in settings
     */
    void AddNestedSettings( NESTED_SETTINGS* aSettings );

    /**
     * Saves and frees a nested settings object, if it exists within this one
     * @param aSettings is a pointer to a NESTED_SETTINGS that has already been added to this one
     */
    void ReleaseNestedSettings( NESTED_SETTINGS* aSettings );

    void SetManager( SETTINGS_MANAGER* aManager )
    {
        m_manager = aManager;
    }

    /**
    * Sets the given string if the given key/path is present
    * @param aObj is the source object
    * @param aTarget is the storage destination
    * @return True if set, false if not
    */
    static bool SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                              wxString& aTarget );

    /**
    * Sets the given bool if the given key/path is present
    * @param aObj is the source object
    * @param aTarget is the storage destination
    * @return True if set, false if not
    */
    static bool SetIfPresent( const nlohmann::json& aObj, const std::string& aPath, bool& aTarget );

    /**
    * Sets the given int if the given key/path is present
    * @param aObj is the source object
    * @param aTarget is the storage destination
    * @return True if set, false if not
    */
    static bool SetIfPresent( const nlohmann::json& aObj, const std::string& aPath, int& aTarget );

    /**
    * Sets the given unsigned int if the given key/path is present
    * @param aObj is the source object
    * @param aTarget is the storage destination
    * @return True if set, false if not
    */
    static bool SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                              unsigned int& aTarget );
protected:

    /**
     * Registers a migration from one schema version to another.  If the schema version in the file
     * loaded from disk is less than the schema version of the JSON_SETTINGS class, migration
     * functions will be called one after the other until the data is up-to-date.
     * @param aOldSchemaVersion is the starting schema version for this migration
     * @param aNewSchemaVersion is the ending schema version for this migration
     * @param aMigrator is a function that performs the migration and returns true if successful
     */
    void registerMigration( int aOldSchemaVersion, int aNewSchemaVersion,
                            std::function<bool(void)> aMigrator );

    /**
    * Translates a legacy wxConfig value to a given JSON pointer value
    * @tparam ValueType is the basic type of the value
    * @param aConfig is the legacy config to read from
    * @param aKey is the key (within the current path) to read
    * @param aDest is a string that will form a JSON pointer (key1.key2.key3) to write to
    */
    template<typename ValueType>
    bool fromLegacy( wxConfigBase* aConfig, const std::string& aKey, const std::string& aDest );

    /**
     * Translates a legacy wxConfig string value to a given JSON pointer value
     * @param aConfig is the legacy config to read from
     * @param aKey is the key (within the current path) to read
     * @param aDest is a string that will form a JSON pointer (key1.key2.key3) to write to
     */
    bool fromLegacyString( wxConfigBase* aConfig, const std::string& aKey,
                           const std::string& aDest );

    /**
    * Translates a legacy COLOR4D stored in a wxConfig string to a given JSON pointer value
    * @param aConfig is the legacy config to read from
    * @param aKey is the key (within the current path) to read
    * @param aDest is a string that will form a JSON pointer (key1.key2.key3) to write to
    */
    bool fromLegacyColor( wxConfigBase* aConfig, const std::string& aKey,
                          const std::string& aDest );

    virtual wxString getFileExt() const
    {
        return wxT( "json" );
    }

    virtual wxString getLegacyFileExt() const
    {
        return wxEmptyString;
    }

    /// The filename (not including path) of this settings file (inicode)
    wxString m_filename;

    /// The filename of the wxConfig legacy file (if different from m_filename)
    wxString m_legacy_filename;

    /// The location of this settings file (@see SETTINGS_LOC)
    SETTINGS_LOC m_location;

    /// The list of parameters (owned by this object)
    std::vector<PARAM_BASE*> m_params;

    /// Nested settings files that live inside this one, if any
    std::vector<NESTED_SETTINGS*> m_nested_settings;

    /// Whether or not the backing store file should be created it if doesn't exist
    bool m_createIfMissing;

    /**
     * Whether or not the  backing store file should be created if all parameters are still
     * at their default values.  Ignored if m_createIfMissing is false or m_writeFile is false.
     */
    bool m_createIfDefault;

    /// Whether or not the backing store file should be written
    bool m_writeFile;

    /// Whether or not to delete legacy file after migration
    bool m_deleteLegacyAfterMigration;

    /// Whether or not to set parameters to their default value if missing from JSON on Load()
    bool m_resetParamsIfMissing;

    /// Version of this settings schema.
    int m_schemaVersion;

    /// A pointer to the settings manager managing this file (may be null)
    SETTINGS_MANAGER* m_manager;

    /// A map of starting schema version to a pair of <ending version, migrator function>
    std::map<int, std::pair<int, std::function<bool()>>> m_migrators;

    std::unique_ptr<JSON_SETTINGS_INTERNALS> m_internals;
};

// Specializations to allow conversion between wxString and std::string via JSON_SETTINGS API

template<> OPT<wxString> JSON_SETTINGS::Get( const std::string& aPath ) const;

template<> void JSON_SETTINGS::Set<wxString>( const std::string& aPath, wxString aVal );

// Specializations to allow directly reading/writing wxStrings from JSON

void to_json( nlohmann::json& aJson, const wxString& aString );

void from_json( const nlohmann::json& aJson, wxString& aString );

#endif
