/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>
#include <panel_setup_netclasses_base.h>

class NETCLASSES;


class PANEL_SETUP_NETCLASSES : public PANEL_SETUP_NETCLASSES_BASE
{
public:
    PANEL_SETUP_NETCLASSES( PAGED_DIALOG* aParent, NETCLASSES* aNetclasses,
                            const std::vector<wxString>& aNetNames, bool isEEschema );
    ~PANEL_SETUP_NETCLASSES( ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    void ImportSettingsFrom( NETCLASSES* aBoard );

private:
    void OnAddNetclassClick( wxCommandEvent& event ) override;
    void OnRemoveNetclassClick( wxCommandEvent& event ) override;
    void OnSizeNetclassGrid( wxSizeEvent& event ) override;
	void OnSizeMembershipGrid( wxSizeEvent& event ) override;
    void onmembershipPanelSize( wxSizeEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent &event ) override;
    void OnNetclassGridCellChanging( wxGridEvent& event );
    void OnNetclassGridMouseEvent( wxMouseEvent& event );
    void OnShowAll( wxCommandEvent& event ) override { doApplyFilters( true ); }
    void OnApplyFilters( wxCommandEvent& event ) override { doApplyFilters( false ); }
    void OnAssignAll( wxCommandEvent& event ) override { doAssignments( true ); }
    void OnAssignSelected( wxCommandEvent& event ) override { doAssignments( false ); }

    bool validateNetclassName( int aRow, const wxString& aName, bool focusFirst = true );

    void rebuildNetclassDropdowns();

    void addNet( const wxString& netName, const wxString& netclass, bool aStale );
    void doApplyFilters( bool aShowAll );
    void doAssignments( bool aAssignAll );

    void AdjustNetclassGridColumns( int aWidth );
    void AdjustMembershipGridColumns( int aWidth );

    PAGED_DIALOG*         m_Parent;
    NETCLASSES*           m_netclasses;
    std::vector<wxString> m_netNames;

    int*                  m_originalColWidths;
    bool                  m_netclassesDirty;    // The netclass drop-down menus need rebuilding
    int                   m_hoveredCol;         // Column being hovered over, for tooltips
};

#endif //PANEL_SETUP_NETCLASSES_H
