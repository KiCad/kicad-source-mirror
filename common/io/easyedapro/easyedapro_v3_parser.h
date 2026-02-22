/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef EASYEDAPRO_V3_PARSER_H_
#define EASYEDAPRO_V3_PARSER_H_

#include <map>
#include <vector>

#include <wx/string.h>

#include <nlohmann/json.hpp>


namespace EASYEDAPRO
{

/**
 * One parsed row from an EasyEDA Pro v3 .epru document stream.
 */
struct V3_ROW
{
    wxString       type;
    wxString       id;
    int            ticket = 0;
    nlohmann::json outer;
    nlohmann::json inner;
};


/**
 * Raw parsed document from an EasyEDA Pro v3 .epru stream.
 */
struct V3_DOC_RAW
{
    wxString                   docType;
    wxString                   uuid;
    nlohmann::json             head;
    std::vector<V3_ROW>        rows;
    std::map<wxString, size_t> rowById;
};


// v3 JSON accessor utilities — type-safe extraction from nlohmann::json objects.

wxString V3JsonToString( const nlohmann::json& aValue,
                         const wxString& aDefault = wxEmptyString );

wxString V3GetString( const nlohmann::json& aObj, const char* aKey,
                      const wxString& aDefault = wxEmptyString );

wxString V3GetString( const nlohmann::json& aObj, const wxString& aKey,
                      const wxString& aDefault = wxEmptyString );

double V3GetDouble( const nlohmann::json& aObj, const char* aKey, double aDefault = 0.0 );

int V3GetInt( const nlohmann::json& aObj, const char* aKey, int aDefault = 0 );

bool V3GetBool( const nlohmann::json& aObj, const char* aKey, bool aDefault = false );

bool V3IsNullOrMissing( const nlohmann::json& aObj, const char* aKey );

nlohmann::json V3ParseIdArray( const wxString& aId );


/**
 * Parses EasyEDA Pro v3 .epro2 archives.
 *
 * Provides raw document access (GetRawDocs/FindRawDoc) for direct v3 parsing.
 */
class V3_DOC_PARSER
{
public:
    explicit V3_DOC_PARSER( const wxString& aFileName );

    /**
     * Return true if @a aFileName looks like an EasyEDA Pro v3 archive.
     *
     * Supported detection:
     * - .epro2 extension
     * - .zip containing project2.json and at least one .epru document file
     */
    static bool IsV3Archive( const wxString& aFileName );

    /**
     * Load all v3 docs from the archive.
     *
     * @throw IO_ERROR on malformed v3 archives or parse failures.
     */
    void Load();

    const nlohmann::json& GetProject2() const { return m_project2; }

    const std::map<wxString, V3_DOC_RAW>& GetRawDocs( const wxString& aDocType ) const;

    const V3_DOC_RAW* FindRawDoc( const wxString& aDocType, const wxString& aUuid ) const;

    int GetSkippedCount() const { return m_skippedCount; }

private:
    wxString m_fileName;
    nlohmann::json m_project2;
    std::map<wxString, std::map<wxString, V3_DOC_RAW>> m_rawDocs;
    int m_skippedCount = 0;
};

} // namespace EASYEDAPRO


#endif // EASYEDAPRO_V3_PARSER_H_
