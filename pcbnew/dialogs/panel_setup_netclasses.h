/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see change_log.txt for contributors.
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


#ifndef PANEL_SETUP_NETCLASSES_H
#define PANEL_SETUP_NETCLASSES_H

#include <class_board.h>
#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>

#include <panel_setup_netclasses_base.h>
#include "panel_setup_feature_constraints.h"

class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


class PANEL_SETUP_NETCLASSES : public PANEL_SETUP_NETCLASSES_BASE
{
private:
    PAGED_DIALOG*           m_Parent;
    PCB_EDIT_FRAME*         m_Frame;
    BOARD*                  m_Pcb;
    BOARD_DESIGN_SETTINGS*  m_BrdSettings;

    // We must validate against the current m_BrdSettings as they may have been
    // changed but not yet committed.  Fetch them from the constraints panel.
    PANEL_SETUP_FEATURE_CONSTRAINTS* m_ConstraintsPanel;

    int*                    m_originalColWidths;
    bool                    m_netclassesDirty;      // Indicates the netclass drop-down
                                                    // menus need rebuilding
    wxSize                  m_membershipSize;       // The size needed to show the membership
                                                    // properties
private:
    void OnAddNetclassClick( wxCommandEvent& event ) override;
    void OnRemoveNetclassClick( wxCommandEvent& event ) override;
    void OnSizeNetclassGrid( wxSizeEvent& event ) override;
    void OnSizeMembershipGrid( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent &event ) override;
    void OnNetclassGridCellChanging( wxGridEvent& event );
    void OnShowAll( wxCommandEvent& event ) override { doApplyFilters( true ); }
    void OnApplyFilters( wxCommandEvent& event ) override { doApplyFilters( false ); }
    void OnAssignAll( wxCommandEvent& event ) override { doAssignments( true ); }
    void OnAssignSelected( wxCommandEvent& event ) override { doAssignments( false ); }

    bool validateNetclassName( int aRow, wxString aName, bool focusFirst = true );
    bool validateData();

    void rebuildNetclassDropdowns();
    int getNetclassValue( int aRow, int aCol );

    void addNet( wxString netName, const wxString& netclass );
    void doApplyFilters( bool aShowAll );
    void doAssignments( bool aAssignAll );

    void AdjustNetclassGridColumns( int aWidth );
    void AdjustMembershipGridColumns( int aWidth );

public:
    PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                            PANEL_SETUP_FEATURE_CONSTRAINTS* aConstraintsPanel );
    ~PANEL_SETUP_NETCLASSES( ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( BOARD* aBoard );
};

#endif //PANEL_SETUP_NETCLASSES_H
