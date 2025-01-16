/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LIBRARY_TABLE_H
#define LIBRARY_TABLE_H

#include <map>
#include <optional>
#include <wx/filename.h>

#include <kicommon.h>
#include <core/utf8.h>


enum class LIBRARY_TABLE_TYPE
{
    SYMBOL,
    FOOTPRINT,
    DESIGN_BLOCK
};

enum class LIBRARY_TABLE_SCOPE
{
    UNINITIALIZED,
    GLOBAL,
    PROJECT,
    BOTH
};

struct LIBRARY_TABLE_IR;
struct LIBRARY_TABLE_ROW_IR;
struct LIBRARY_TABLE_INTERNALS;


class KICOMMON_API LIBRARY_TABLE_ROW
{
public:
    friend class LIBRARY_TABLE;

    LIBRARY_TABLE_ROW() = default;

    const wxString& Nickname() const { return m_nickname; }
    const wxString& URI() const { return m_uri; }
    const wxString& Type() const { return m_type; }
    const wxString& Options() const { return m_options; }
    const wxString& Description() const { return m_description; }
    LIBRARY_TABLE_SCOPE Scope() const { return m_scope; }

    std::unique_ptr<std::map<std::string, UTF8>> OptionsMap() const;

    bool IsOk() const { return m_ok; }
    const wxString& ErrorDescription() const { return m_errorDescription; }

private:
    wxString m_nickname;
    wxString m_uri;
    wxString m_type;
    wxString m_options;
    wxString m_description;

    bool m_ok = false;
    wxString m_errorDescription;
    LIBRARY_TABLE_SCOPE m_scope = LIBRARY_TABLE_SCOPE::UNINITIALIZED;
};


class KICOMMON_API LIBRARY_TABLE
{
public:
    /**
     * Creates a library table from a file on disk
     * @param aPath is the path to a library table file to parse
     * @param aScope is the scope of this table (is it global or part of a project)
     */
    LIBRARY_TABLE( const wxFileName &aPath, LIBRARY_TABLE_SCOPE aScope );

    /**
     * Creates a library table from parsed text
     * @param aBuffer is a string containing data to parse
     * @param aScope is the scope of this table (is it global or part of a project)
     */
    LIBRARY_TABLE( const wxString &aBuffer, LIBRARY_TABLE_SCOPE aScope );

    ~LIBRARY_TABLE() = default;

    // TODO move out of this class
    void LoadNestedTables();

    const wxString& Path() const { return m_path; }
    void SetPath( const wxString &aPath ) { m_path = aPath; }

    LIBRARY_TABLE_TYPE Type() const { return m_type; }
    void SetType( const LIBRARY_TABLE_TYPE aType ) { m_type = aType; }

    std::optional<int> Version() const{ return m_version; }
    void SetVersion( const std::optional<int> &aVersion ) { m_version = aVersion; }

    bool IsOk() const { return m_ok; }
    const wxString& ErrorDescription() const { return m_errorDescription; }

    const std::vector<LIBRARY_TABLE_ROW>& Rows() const { return m_rows; }
    std::vector<LIBRARY_TABLE_ROW>& Rows() { return m_rows; }

    const std::map<wxString, std::unique_ptr<LIBRARY_TABLE>>& Children() { return m_children; }

private:
    bool initFromIR( const LIBRARY_TABLE_IR& aIR );
    bool addRowFromIR( const LIBRARY_TABLE_ROW_IR& aIR );

    /// The full path to the file this table was parsed from, if any
    wxString m_path;

    LIBRARY_TABLE_SCOPE m_scope;

    /// What type of content this table contains (footprint, symbol, design block, etc)
    LIBRARY_TABLE_TYPE m_type;

    /// The format version, if present in the parsed file
    std::optional<int> m_version;

    bool m_ok;
    wxString m_errorDescription;

    std::vector<LIBRARY_TABLE_ROW> m_rows;

    std::map<wxString, std::unique_ptr<LIBRARY_TABLE>> m_children;
};

#endif //LIBRARY_TABLE_H
