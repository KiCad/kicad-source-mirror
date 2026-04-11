/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <../common/dialogs/panel_text_variables_base.h>
#include <wx/valtext.h>

#include <map>
#include <memory>

class PROJECT;


class PANEL_TEXT_VARIABLES: public PANEL_TEXT_VARIABLES_BASE
{
public:
    PANEL_TEXT_VARIABLES(  wxWindow* aParent, PROJECT* aProject  );
    ~PANEL_TEXT_VARIABLES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( const PROJECT* aOtherProject );

protected:
    // Various button callbacks
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnAddTextVar( wxCommandEvent& event ) override;
    void OnRemoveTextVar( wxCommandEvent& event ) override;

    void AppendTextVar( const wxString& aName, const wxString& aValue );

    void checkReload();

private:
    PROJECT*                     m_project;

    std::map<wxString, wxString> m_lastLoaded;
    int                          m_lastCheckedTicker;

    wxString                     m_errorMsg;
    int                          m_errorRow;
    int                          m_errorCol;

    wxTextValidator              m_nameValidator;
};
