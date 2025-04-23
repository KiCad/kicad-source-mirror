/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mike@mikebwilliams.com>
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
#include <kicommon.h>
#include <lib_id.h>
#include <json_common.h>


class KICOMMON_API DESIGN_BLOCK
{
public:
    void          SetLibId( const LIB_ID& aName ) { m_lib_id = aName; }
    const LIB_ID& GetLibId() const { return m_lib_id; }

    const wxString& GetLibDescription() const { return m_libDescription; }
    void            SetLibDescription( const wxString& aDesc ) { m_libDescription = aDesc; }

    const wxString& GetKeywords() const { return m_keywords; }
    void            SetKeywords( const wxString& aKeywords ) { m_keywords = aKeywords; }

    const wxString& GetSchematicFile() const { return m_schematicFile; }
    void            SetSchematicFile( const wxString& aFile ) { m_schematicFile = aFile; }

    void SetFields( nlohmann::ordered_map<wxString, wxString>& aFields )
    {
        m_fields = std::move( aFields );
    }

    const nlohmann::ordered_map<wxString, wxString>& GetFields() const { return m_fields; }

    DESIGN_BLOCK() = default;

    /// This is the only way to get m_fields to compile as a class member.
    DESIGN_BLOCK( DESIGN_BLOCK&& aOther ) = delete;

private:
    LIB_ID m_lib_id;
    wxString m_schematicFile;  ///< File name and path for schematic symbol.
    wxString m_libDescription; ///< File name and path for documentation file.
    wxString m_keywords;       ///< Search keywords to find footprint in library.

    nlohmann::ordered_map<wxString, wxString> m_fields;
};
