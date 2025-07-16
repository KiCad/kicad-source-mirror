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

#ifndef DESIGN_BLOCK_H
#define DESIGN_BLOCK_H

#include <kicommon.h>
#include <lib_id.h>
#include <lib_tree_item.h>
#include <json_common.h>


class KICOMMON_API DESIGN_BLOCK : public LIB_TREE_ITEM
{
public:
    // LIB_TREE_ITEM interface
    LIB_ID GetLIB_ID() const override { return GetLibId(); }
    wxString GetName() const override { return m_lib_id.GetLibItemName(); }
    wxString GetLibNickname() const override { return m_lib_id.GetLibNickname(); }
    wxString GetDesc() override { return GetLibDescription(); }
    std::vector<SEARCH_TERM> GetSearchTerms() override;

    void          SetLibId( const LIB_ID& aName ) { m_lib_id = aName; }
    const LIB_ID& GetLibId() const { return m_lib_id; }
    LIB_ID& GetLibId() { return m_lib_id; }

    const wxString& GetLibDescription() const { return m_libDescription; }
    void            SetLibDescription( const wxString& aDesc ) { m_libDescription = aDesc; }

    const wxString& GetKeywords() const { return m_keywords; }
    void            SetKeywords( const wxString& aKeywords ) { m_keywords = aKeywords; }

    const wxString& GetSchematicFile() const { return m_schematicFile; }
    void            SetSchematicFile( const wxString& aFile ) { m_schematicFile = aFile; }

    const wxString& GetBoardFile() const { return m_boardFile; }
    void            SetBoardFile( const wxString& aFile ) { m_boardFile = aFile; }

    const nlohmann::ordered_map<wxString, wxString>& GetFields() const { return m_fields; }
    nlohmann::ordered_map<wxString, wxString>& GetFields() { return m_fields; }

    DESIGN_BLOCK() = default;

    /// This is the only way to get m_fields to compile as a class member.
    DESIGN_BLOCK( DESIGN_BLOCK&& aOther ) = delete;

private:
    LIB_ID m_lib_id;
    wxString m_schematicFile;  ///< File name and path for schematic file.
    wxString m_boardFile;      ///< File name and path for board file
    wxString m_libDescription; ///< File name and path for documentation file.
    wxString m_keywords;       ///< Search keywords to find design block in library.

    nlohmann::ordered_map<wxString, wxString> m_fields;
};

#endif
