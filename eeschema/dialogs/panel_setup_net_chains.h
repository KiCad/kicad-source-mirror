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

#ifndef PANEL_SETUP_NET_CHAINS_H
#define PANEL_SETUP_NET_CHAINS_H

#include <map>
#include <set>
#include <vector>
#include <wx/string.h>
#include <gal/color4d.h>

#include <dialogs/panel_setup_net_chains_base.h>

class SCH_EDIT_FRAME;
class SCH_NETCHAIN;


/**
 * Schematic-Setup tab for managing committed net chains.
 *
 * All edits are buffered in m_chainRows / m_classRows so that pressing Cancel
 * in the parent dialog leaves the live model untouched.  TransferDataFromWindow
 * applies them in dependency order:
 *
 *   1. Rename committed chains   (via RenameCommittedNetChain)
 *   2. Update colour / netclass override on each committed chain
 *   3. Apply chain-class assignments to NET_SETTINGS::m_netChainClasses
 *   4. Delete committed chains   (via DeleteCommittedNetChain)
 *   5. Reconcile the chain-class master list against tab 2's edits
 */
class PANEL_SETUP_NET_CHAINS : public PANEL_SETUP_NET_CHAINS_BASE
{
public:
    PANEL_SETUP_NET_CHAINS( wxWindow* aParent, SCH_EDIT_FRAME* aFrame );

    ~PANEL_SETUP_NET_CHAINS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    bool Validate() override;

    bool ApplyEdits();

protected:
    void OnDeleteChainClicked( wxCommandEvent& aEvent ) override;
    void OnChainGridSelectionChanged( wxGridEvent& aEvent ) override;
    void OnClassAddClicked( wxCommandEvent& aEvent ) override;
    void OnClassRenameClicked( wxCommandEvent& aEvent ) override;
    void OnClassDeleteClicked( wxCommandEvent& aEvent ) override;

private:
    enum CHAIN_COL
    {
        COL_NAME = 0,
        COL_MEMBERS,
        COL_CHAIN_CLASS,
        COL_NET_CLASS,
        COL_COLOUR,
    };

    enum CLASS_COL
    {
        CLASS_COL_NAME = 0,
        CLASS_COL_MEMBERS,
    };

    struct CHAIN_ROW
    {
        bool                 deletePending = false;
        wxString             origName;
        wxString             newName;
        wxString             newChainClass;
        wxString             newNetClass;
        KIGFX::COLOR4D       newColor = KIGFX::COLOR4D::UNSPECIFIED;
        SCH_NETCHAIN*        livePtr = nullptr;
        std::set<wxString>   memberNets;
    };

    struct CLASS_ROW
    {
        wxString origName;
        wxString newName;
        bool     deletePending = false;
    };

    void loadFromModel();
    void rebuildChainsGrid();
    void rebuildClassesGrid();
    void refreshChainClassDropdownChoices();
    void refreshNetClassDropdownChoices();

    int  selectedChainRow() const;
    int  selectedClassRow() const;

    bool nameInChainGridAlready( const wxString& aName, int aExceptRow ) const;
    bool nameInClassGridAlready( const wxString& aName, int aExceptRow ) const;
    void updateMembersDetail( int aRow );

    SCH_EDIT_FRAME* m_frame;

    std::vector<CHAIN_ROW> m_chainRows;
    std::vector<int>       m_gridToChainIdx;
    std::vector<CLASS_ROW> m_classRows;
};

#endif // PANEL_SETUP_NET_CHAINS_H
