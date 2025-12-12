/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <set>
#include <vector>

#include <embedded_files.h>
#include "panel_embedded_files_base.h"

#include "grid_tricks.h"

#define NO_MARGINS 0x0001


class EMBEDDED_FILES_GRID_TRICKS : public GRID_TRICKS
{
    enum
    {
        EMBEDDED_FILES_GRID_TRICKS_COPY_FILENAME = GRIDTRICKS_FIRST_CLIENT_ID
    };

public:
    explicit EMBEDDED_FILES_GRID_TRICKS( WX_GRID* aGrid );

    ~EMBEDDED_FILES_GRID_TRICKS() override = default;

    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    //virtual void optionsEditor( int aRow ) = 0;

    virtual bool supportsVisibilityColumn() { return false; }

    int m_curRow;
};


class PANEL_EMBEDDED_FILES : public PANEL_EMBEDDED_FILES_BASE
{
public:
    PANEL_EMBEDDED_FILES( wxWindow* aParent, EMBEDDED_FILES* aFiles, int aFlags = 0,
                          std::vector<const EMBEDDED_FILES*> aInheritedFiles = {} );
    ~PANEL_EMBEDDED_FILES() override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
    bool GetEmbedFonts() const { return m_cbEmbedFonts->GetValue(); }

    EMBEDDED_FILES* GetLocalFiles() { return m_localFiles; }
    EMBEDDED_FILES::EMBEDDED_FILE* AddEmbeddedFile( const wxString& aFileName );
    bool RemoveEmbeddedFile( const wxString& aFileName );

protected:
    void onFontEmbedClick( wxCommandEvent& event ) override;
    void onAddEmbeddedFiles( wxCommandEvent& event ) override;
    void onDeleteEmbeddedFile( wxCommandEvent& event ) override;
    void onExportFiles( wxCommandEvent& event ) override;

private:
    EMBEDDED_FILES* m_files;
    EMBEDDED_FILES* m_localFiles;
    std::vector<const EMBEDDED_FILES*> m_inheritedFiles;
    std::set<wxString>                 m_inheritedFileNames;
};
