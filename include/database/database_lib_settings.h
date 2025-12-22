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

#ifndef KICAD_DATABASE_LIB_SETTINGS_H
#define KICAD_DATABASE_LIB_SETTINGS_H

#include <settings/json_settings.h>
#include <wx/string.h>


enum class DATABASE_SOURCE_TYPE
{
    ODBC,
    INVALID
};


struct KICOMMON_API DATABASE_SOURCE
{
    DATABASE_SOURCE_TYPE type;
    std::string          dsn;
    std::string          username;
    std::string          password;
    int                  timeout;
    std::string          connection_string;
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

    explicit DATABASE_FIELD_MAPPING( const std::string& aColumn, const std::string& aName,
                                     bool aVisibleOnAdd, bool aVisibleInChooser, bool aShowName,
                                     bool aInheritProperties );
};


struct KICOMMON_API MAPPABLE_SYMBOL_PROPERTIES
{
    std::string description;
    std::string footprint_filters;
    std::string keywords;
    std::string exclude_from_sim;
    std::string exclude_from_bom;
    std::string exclude_from_board;
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

    MAPPABLE_SYMBOL_PROPERTIES properties;
    std::vector<DATABASE_FIELD_MAPPING> fields;
};


struct KICOMMON_API DATABASE_CACHE_SETTINGS
{
    int max_size;    ///< Maximum number of single-row results to cache
    int max_age;     ///< Max age of cached rows before they expire, in seconds
};


class KICOMMON_API DATABASE_LIB_SETTINGS : public JSON_SETTINGS
{
public:
    DATABASE_LIB_SETTINGS( const std::string& aFilename );

    virtual ~DATABASE_LIB_SETTINGS() {}

    DATABASE_SOURCE m_Source;

    std::vector<DATABASE_LIB_TABLE> m_Tables;

    DATABASE_CACHE_SETTINGS m_Cache;

    bool m_GloballyUniqueKeys;

protected:
    wxString getFileExt() const override;
};

#endif //KICAD_DATABASE_LIB_SETTINGS_H
