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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_ASSIGN_NETCLASS_H
#define DIALOG_ASSIGN_NETCLASS_H

#include <dialogs/dialog_assign_netclass_base.h>

#include <functional>
#include <set>


class EDA_BASE_FRAME;


class DIALOG_ASSIGN_NETCLASS : public DIALOG_ASSIGN_NETCLASS_BASE
{
public:
    DIALOG_ASSIGN_NETCLASS( EDA_BASE_FRAME* aParent, const std::set<wxString>& aNetNames,
                            const std::set<wxString> aCandidateNetNames,
                            const std::function<void( const std::vector<wxString>& )>& aPreviewer );
    ~DIALOG_ASSIGN_NETCLASS() override {}

private:
    void onPatternText( wxCommandEvent& aEvent ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    EDA_BASE_FRAME*    m_frame;

    const std::set<wxString> m_selectedNetNames;
    std::set<wxString>       m_netCandidates;

    std::function<void( const std::vector<wxString>& )> m_previewer;

    wxString           m_lastPattern;
};

#endif  //DIALOG_ASSIGN_NETCLASS_H
