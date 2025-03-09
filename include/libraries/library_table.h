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
class OUTPUTFORMATTER;


class KICOMMON_API LIBRARY_TABLE_ROW
{
public:
    friend class LIBRARY_TABLE;

    LIBRARY_TABLE_ROW() = default;

    bool operator==( const LIBRARY_TABLE_ROW& aOther ) const;

    void SetNickname( const wxString& aNickname ) { m_nickname = aNickname; }
    const wxString& Nickname() const { return m_nickname; }

    void SetURI( const wxString& aUri ) { m_uri = aUri; }
    const wxString& URI() const { return m_uri; }

    void SetType( const wxString& aType ) { m_type = aType; }
    const wxString& Type() const { return m_type; }

    void SetOptions( const wxString& aOptions ) { m_options = aOptions; }
    const wxString& Options() const { return m_options; }

    void SetDescription( const wxString& aDescription ) { m_description = aDescription; }
    const wxString& Description() const { return m_description; }

    void SetScope( LIBRARY_TABLE_SCOPE aScope ) { m_scope = aScope; }
    LIBRARY_TABLE_SCOPE Scope() const { return m_scope; }

    void SetDisabled( bool aDisabled = true ) { m_disabled = aDisabled; }
    bool Disabled() const { return m_disabled; }

    void SetHidden( bool aHidden = true ) { m_hidden = aHidden; }
    bool Hidden() const { return m_hidden; }

    std::map<std::string, UTF8> GetOptionsMap() const;

    void SetOk( bool aOk = true ) { m_ok = aOk; }
    bool IsOk() const { return m_ok; }

    void SetErrorDescription( const wxString& aDescription ) { m_errorDescription = aDescription; }
    const wxString& ErrorDescription() const { return m_errorDescription; }

private:
    wxString m_nickname;
    wxString m_uri;
    wxString m_type;
    wxString m_options;
    wxString m_description;
    bool m_disabled;
    bool m_hidden;

    bool m_ok = false;
    wxString m_errorDescription;
    LIBRARY_TABLE_SCOPE m_scope = LIBRARY_TABLE_SCOPE::UNINITIALIZED;
};


typedef std::vector<LIBRARY_TABLE_ROW>::iterator       LIBRARY_TABLE_ROWS_ITER;
typedef std::vector<LIBRARY_TABLE_ROW>::const_iterator LIBRARY_TABLE_ROWS_CITER;


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

    bool operator==( const LIBRARY_TABLE& aOther ) const;

    /// Builds a new row that is suitable for this table (does not insert it)
    LIBRARY_TABLE_ROW MakeRow() const;

    const wxString& Path() const { return m_path; }
    void SetPath( const wxString &aPath ) { m_path = aPath; }

    LIBRARY_TABLE_TYPE Type() const { return m_type; }
    void SetType( const LIBRARY_TABLE_TYPE aType ) { m_type = aType; }

    void SetScope( LIBRARY_TABLE_SCOPE aScope ) { m_scope = aScope; }
    LIBRARY_TABLE_SCOPE Scope() const { return m_scope; }

    std::optional<int> Version() const{ return m_version; }
    void SetVersion( const std::optional<int> &aVersion ) { m_version = aVersion; }

    bool IsOk() const { return m_ok; }
    const wxString& ErrorDescription() const { return m_errorDescription; }

    const std::vector<LIBRARY_TABLE_ROW>& Rows() const { return m_rows; }
    std::vector<LIBRARY_TABLE_ROW>& Rows() { return m_rows; }

    void Format( OUTPUTFORMATTER* aOutput ) const;

    bool HasRow( const wxString& aNickname ) const;

    std::optional<LIBRARY_TABLE_ROW*> Row( const wxString& aNickname );
    std::optional<const LIBRARY_TABLE_ROW*> Row( const wxString& aNickname ) const;

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
};

#endif //LIBRARY_TABLE_H
