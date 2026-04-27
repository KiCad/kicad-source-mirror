/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef DIALOG_CREATE_NET_CHAIN_H
#define DIALOG_CREATE_NET_CHAIN_H

#include <dialogs/dialog_create_net_chain_base.h>

#include <set>
#include <vector>
#include <wx/string.h>
#include <kiid.h>

class SCH_EDIT_FRAME;
class SCH_NETCHAIN;
class SCH_SYMBOL;


class DIALOG_CREATE_NET_CHAIN : public DIALOG_CREATE_NET_CHAIN_BASE
{
public:
    DIALOG_CREATE_NET_CHAIN( SCH_EDIT_FRAME* aParent, const wxString& aFromRef = wxEmptyString,
                             const wxString& aToRef = wxEmptyString );

    ~DIALOG_CREATE_NET_CHAIN() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    void OnChainSelected( wxGridEvent& aEvent ) override;
    void OnFilterChanged( wxCommandEvent& aEvent ) override;
    void OnRefreshClicked( wxCommandEvent& aEvent ) override;
    void OnFindPathClicked( wxCommandEvent& aEvent ) override;

private:
    struct POTENTIAL_ROW
    {
        SCH_NETCHAIN*      livePtr = nullptr; ///< null for force-created manual rows
        wxString           suggestedName;
        std::set<wxString> memberNets;
        wxString           terminals; ///< e.g. "J1:1 -> J2:1"
        bool               isManual = false;

        /// For force-created rows without a livePtr
        KIID forceFromUuid;
        KIID forceToUuid;
    };

    void loadPotentials();
    void rebuildGrid();
    void updateMemberDetail( int aRow );
    void populateComponentCombos();
    void highlightChainNets( const std::set<wxString>& aNets );
    int  selectedRow() const;

    bool validateAndCreate();

    SCH_EDIT_FRAME*            m_frame;
    std::vector<POTENTIAL_ROW> m_rows;
    std::vector<int>           m_filteredIndices; ///< indices into m_rows shown in grid
    int                        m_createdCount = 0;
    bool                       m_rebuilding = false;
};

#endif // DIALOG_CREATE_NET_CHAIN_H
