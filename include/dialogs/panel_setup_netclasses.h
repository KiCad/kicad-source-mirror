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

class NET_SETTINGS;
class NETCLASS;


class PANEL_SETUP_NETCLASSES : public PANEL_SETUP_NETCLASSES_BASE
{
public:
    PANEL_SETUP_NETCLASSES( wxWindow* aParentWindow, EDA_DRAW_FRAME* aFrame,
                            std::shared_ptr<NET_SETTINGS> aSettings,
                            const std::set<wxString>& aNetNames, bool isEEschema );
    ~PANEL_SETUP_NETCLASSES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    void ImportSettingsFrom( const std::shared_ptr<NET_SETTINGS>& aNetSettings );

    void UpdateDelayProfileNames( const std::vector<wxString>& aNames ) const;

private:
    void OnAddNetclassClick( wxCommandEvent& event ) override;
    void OnRemoveNetclassClick( wxCommandEvent& event ) override;
    void OnMoveNetclassUpClick( wxCommandEvent& event ) override;
    void OnMoveNetclassDownClick( wxCommandEvent& event ) override;
    void OnImportColorsClick( wxCommandEvent& event ) override;
    void OnSizeNetclassGrid( wxSizeEvent& event ) override;
	void OnSizeAssignmentGrid( wxSizeEvent& event ) override;
    void OnAddAssignmentClick( wxCommandEvent& event ) override;
    void OnRemoveAssignmentClick( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent &event ) override;
    void OnNetclassGridCellChanging( wxGridEvent& event );
    void OnNetclassGridMouseEvent( wxMouseEvent& event );
    void OnNetclassAssignmentSort( wxGridEvent& event );

    void onUnitsChanged( wxCommandEvent& aEvent );

    bool validateNetclassName( int aRow, const wxString& aName, bool focusFirst = true );
    bool validateNetclassClearance( int aRow );

    void rebuildNetclassDropdowns();

    void AdjustNetclassGridColumns( int aWidth );
    void AdjustAssignmentGridColumns( int aWidth );

    void loadNetclasses();
    void checkReload();

    void setNetclassRowNullableEditors( int aRowId, bool aIsDefault );

private:
    EDA_DRAW_FRAME*                 m_frame;
    bool                            m_isEEschema;
    std::shared_ptr<NET_SETTINGS>   m_netSettings;
    std::set<wxString>              m_netNames;

    std::unique_ptr<UNITS_PROVIDER> m_schUnitsProvider;
    std::unique_ptr<UNITS_PROVIDER> m_pcbUnitsProvider;

    std::map<wxString, std::shared_ptr<NETCLASS>> m_lastLoaded;
    int                                           m_lastCheckedTicker;

    std::map<int, int>    m_originalColWidths;  // Map col-number : orig-col-width
    bool                  m_netclassesDirty;    // The netclass drop-down menus need rebuilding
    int                   m_hoveredCol;         // Column being hovered over, for tooltips
    wxString              m_lastPattern;

    std::bitset<64>       m_shownColumns;
    int                   m_lastNetclassGridWidth;

    bool                  m_sortAsc;
    int                   m_sortCol;
};

#endif //PANEL_SETUP_NETCLASSES_H
