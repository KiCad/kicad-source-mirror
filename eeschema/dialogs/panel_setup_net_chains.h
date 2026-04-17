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
 * Schematic-Setup tab listing every committed and potential net chain in the
 * project.  Lets the user rename, recolour, assign a netclass override or a
 * chain-class label, promote auto-detected potentials into committed chains,
 * and delete committed ones.
 *
 * All edits are buffered in m_chainRows / m_classRows so that pressing Cancel
 * in the parent dialog leaves the live model untouched.  TransferDataFromWindow
 * applies them in dependency order:
 *
 *   1. Promote potentials -> committed (via CreateNetChainFromPotential)
 *   2. Rename committed chains   (via RenameCommittedNetChain)
 *   3. Update colour / netclass override on each committed chain
 *   4. Apply chain-class assignments to NET_SETTINGS::m_netChainClasses
 *   5. Delete committed chains   (via DeleteCommittedNetChain)
 *   6. Reconcile the chain-class master list against tab 2's edits
 */
class PANEL_SETUP_NET_CHAINS : public PANEL_SETUP_NET_CHAINS_BASE
{
public:
    PANEL_SETUP_NET_CHAINS( wxWindow* aParent, SCH_EDIT_FRAME* aFrame );

    ~PANEL_SETUP_NET_CHAINS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    bool Validate() override;

    /**
     * Apply a buffered edit snapshot to the live model.  Public so the
     * headless QA test can exercise the same code path the UI uses.
     */
    bool ApplyEdits();

protected:
    void OnPromoteClicked( wxCommandEvent& aEvent ) override;
    void OnDeleteChainClicked( wxCommandEvent& aEvent ) override;
    void OnRefreshClicked( wxCommandEvent& aEvent ) override;
    void OnClassAddClicked( wxCommandEvent& aEvent ) override;
    void OnClassRenameClicked( wxCommandEvent& aEvent ) override;
    void OnClassDeleteClicked( wxCommandEvent& aEvent ) override;

private:
    enum CHAIN_COL
    {
        COL_STATUS = 0,
        COL_NAME,
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

    /// Per-row buffered state for the Chains grid.
    struct CHAIN_ROW
    {
        bool                 isPotential = false;
        bool                 deletePending = false;
        bool                 promotePending = false;     ///< set when isPotential and user named it
        wxString             origName;                   ///< empty for potential rows
        wxString             newName;                    ///< user-edited name (== origName if unchanged)
        wxString             newChainClass;              ///< chain-class assignment
        wxString             newNetClass;                ///< per-chain netclass override
        KIGFX::COLOR4D       newColor = KIGFX::COLOR4D::UNSPECIFIED;
        SCH_NETCHAIN*        livePtr = nullptr;          ///< the underlying chain (committed or potential)
        std::set<wxString>   memberNets;                 ///< snapshot for display & tooltip
    };

    /// Per-row buffered state for the Net Chain Classes grid.
    struct CLASS_ROW
    {
        wxString origName;       ///< empty if newly added
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

    bool isReservedChainName( const wxString& aName ) const;
    bool nameInChainGridAlready( const wxString& aName, int aExceptRow ) const;
    bool nameInClassGridAlready( const wxString& aName, int aExceptRow ) const;

    SCH_EDIT_FRAME* m_frame;

    std::vector<CHAIN_ROW> m_chainRows;
    std::vector<CLASS_ROW> m_classRows;

    /// Cache of the auto-suggested name for each potential row; used as the
    /// initial value if the user types nothing before promoting.
    std::map<int, wxString> m_potentialAutoNames;
};

#endif // PANEL_SETUP_NET_CHAINS_H
