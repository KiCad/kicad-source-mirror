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

#ifndef PANEL_GRID_SETTINGS_H
#define PANEL_GRID_SETTINGS_H

#include <panel_grid_settings_base.h>
#include <widgets/unit_binder.h>
#include <frame_type.h>

class APP_SETTINGS_BASE;
struct GRID;


class PANEL_GRID_SETTINGS : public PANEL_GRID_SETTINGS_BASE
{
public:
    /// This has no dependencies on calling wxFrame derivative, such as PCB_BASE_FRAME.
    PANEL_GRID_SETTINGS( wxWindow* aParent, UNITS_PROVIDER* aUnitsProvider, wxWindow* aEventSource,
                         APP_SETTINGS_BASE* aCfg, FRAME_T aFrameType );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void ResetPanel() override;

private:
    void OnAddGrid( wxCommandEvent& event ) override;
    void OnEditGrid( wxCommandEvent& event ) override;
    void OnRemoveGrid( wxCommandEvent& event ) override;
    void OnMoveGridUp( wxCommandEvent& event ) override;
    void OnMoveGridDown( wxCommandEvent& event ) override;

    void OnUpdateEditGrid( wxUpdateUIEvent& event ) override;
    void OnUpdateMoveUp( wxUpdateUIEvent& event ) override;
    void OnUpdateMoveDown( wxUpdateUIEvent& event ) override;
    void OnUpdateRemove( wxUpdateUIEvent& event ) override;

    void RebuildGridSizes();

    void onEditGrid();

private:
    UNITS_PROVIDER*    m_unitsProvider;
    APP_SETTINGS_BASE* m_cfg;
    FRAME_T            m_frameType;
    wxWindow*          m_eventSource;

    std::vector<GRID> m_grids;
};

#endif // PANEL_GRID_SETTINGS_H
