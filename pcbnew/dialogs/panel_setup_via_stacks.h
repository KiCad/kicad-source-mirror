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

#ifndef PANEL_SETUP_VIA_STACKS_H
#define PANEL_SETUP_VIA_STACKS_H

#include <vector>

#include <board_design_settings.h>
#include <dialogs/panel_setup_via_stacks_base.h>

class BOARD;
class PCB_EDIT_FRAME;


class PANEL_SETUP_VIA_STACKS : public PANEL_SETUP_VIA_STACKS_BASE
{
public:
    PANEL_SETUP_VIA_STACKS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ImportSettingsFrom( BOARD* aBoard );

private:
    void loadFrom( BOARD_DESIGN_SETTINGS& aBds );

    bool nameIsFree( const wxString& aName, int aIgnoreRow ) const;

    void onCellChanged( wxGridEvent& aEvent );
    void onAdd( wxCommandEvent& aEvent );
    void onEdit( wxCommandEvent& aEvent );
    void onRemove( wxCommandEvent& aEvent );

    void rebuildGrid();
    bool editPreset( VIA_STACK_PRESET& aPreset );

    PCB_EDIT_FRAME*               m_frame;
    std::vector<VIA_STACK_PRESET> m_presets;
    wxString                      m_activePreset;
};

#endif // PANEL_SETUP_VIA_STACKS_H
