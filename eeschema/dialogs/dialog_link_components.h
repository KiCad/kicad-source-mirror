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
 * along with this program.  If not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <vector>
#include <set>
#include <dialog_link_components_base.h>

class SCH_EDIT_FRAME;
class SCH_SYMBOL;
class SCH_PIN;

struct KILINK_ROW
{
    SCH_SYMBOL* source;
    SCH_SYMBOL* target;
    wxString    name;
    int         connections;
    bool        selected;
    bool        existing;
};

class DIALOG_LINK_COMPONENTS : public DIALOG_LINK_COMPONENTS_BASE
{
public:
    DIALOG_LINK_COMPONENTS( SCH_EDIT_FRAME* aParent,
                            const std::vector<SCH_SYMBOL*>& aSources,
                            std::vector<SCH_SYMBOL*> aTargets );

    const std::vector<KILINK_ROW>& SelectedLinks() const { return m_linkRows; }

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void OnSrcFilter( wxCommandEvent& event ) override;
    void OnTargetFilter( wxCommandEvent& event ) override;
    void OnNetFilter( wxCommandEvent& event ) override;
    void OnSrcSelect( wxCommandEvent& event ) override;
    void OnTargetSelect( wxCommandEvent& event ) override;
    void OnNetSelect( wxCommandEvent& event ) override;
    void OnOK( wxCommandEvent& event ) override;

    void updateSourceList();
    void updateTargetList();
    void updateNetList();
    void updateGrid();

    std::set<wxString> getNets( SCH_SYMBOL* aSymbol ) const;
    bool connected( SCH_SYMBOL* aSrc, SCH_SYMBOL* aDst ) const;
    int connectionCount( SCH_SYMBOL* aSrc, SCH_SYMBOL* aDst ) const;

private:
    SCH_EDIT_FRAME*              m_frame;
    std::vector<SCH_SYMBOL*>     m_allSources;
    std::vector<SCH_SYMBOL*>     m_allTargets;
    std::vector<SCH_SYMBOL*>     m_filteredSources;
    std::vector<SCH_SYMBOL*>     m_filteredTargets;
    std::vector<wxString>        m_allNets;
    std::vector<wxString>        m_filteredNets;
    std::vector<SCH_SYMBOL*>     m_initialSources;
    std::vector<SCH_SYMBOL*>     m_initialTargets;
    std::vector<KILINK_ROW>      m_linkRows;
};

